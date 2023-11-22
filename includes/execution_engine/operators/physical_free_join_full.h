#pragma once

#include <vector>
#include <cstdint>
#include "physical_operator.h"
#include "execution_engine/chunked_list_builder.h"
#include "execution_engine/ght_builder.h"
#include "execution_engine/ght_intersector.h"
#include "utils/PerfEvent.hpp"

namespace free_join {

    class PhysicalFreeJoinFull : public PhysicalNaryOperator {

    private:
        std::unordered_map<int16_t, ChunkedList<Tuple, max_chunk_size_tuple> *> input_ptrs;
        std::unordered_map<int16_t, size_t> input_sizes;
        ChunkedList<Tuple, max_chunk_size_tuple> output_cl;
        std::map<int16_t, GHTWrapper> trie_map;
        std::map<int16_t, std::vector<ChunkedList<Tuple, max_chunk_size_tuple>>> chunked_list_map;
        uint64_t total_rows;
        LogicalOperatorTree *tree;
        Tuple tuple_register;

    public:
        FreeJoinPlan plan;

    public:

        explicit PhysicalFreeJoinFull(int16_t src, LogicalOperatorTree *tree) :
                tree(tree),
                total_rows(0) {
            src_idx = src;
            is_scan = false;
        };

        void open() override {
            for (auto &child: children) {
                child->open();
            }
        };

        bool next() override {
            BenchmarkParameters params;
            for (const auto &child: children) {
                // Materialize input
                auto child_src_idx = child->src_idx;
                if (child->next()) {
                    input_ptrs[child_src_idx] = child->get_tuples();
                    input_sizes[child_src_idx] = reinterpret_cast<ChunkedList<Tuple, max_chunk_size_tuple> *>(child->get_tuples())->size;
//                        if (child->is_scan) {
//                            // update input sizes
//                            input_sizes[child_src_idx] = reinterpret_cast<TupleVec *>(child->get_tuples())->size();
//                        } else {
//                            input_sizes[child_src_idx] = reinterpret_cast<ChunkedList<ChunkTuple, Tuple> *>(child->get_tuples())->size;
//                        }
                } else {
                    return false;
                }
            }

            int16_t leftmost_src_idx = children[0]->src_idx;
            std::unordered_map<uint16_t, size_t> tallys;
            for (const auto &cond: reinterpret_cast<LogicalNaryJoin *>(tree->op.get())->equality_conditions) {
                tallys[cond.left.fj_attribute_idx]++;
                tallys[cond.right.fj_attribute_idx]++;
            }
            size_t max_tally = 0;
            uint16_t most_common_fj_idx = 0;
            for (const auto &[k, v]: tallys) {
                if (v > max_tally) {
                    most_common_fj_idx = k;
                    max_tally = v;
                }
            }
            std::vector<int16_t> possible_leftmost_srcs;
            for (const auto &child: children) {
                for (const auto &attr: child->out_attributes) {
                    if (attr.fj_attribute_idx == most_common_fj_idx) {
                        possible_leftmost_srcs.push_back(child->src_idx);
                    }
                }
            }


            size_t max_size_input = 0;
            for (auto i: possible_leftmost_srcs) {
                if (input_sizes[i] > max_size_input) {
                    max_size_input = input_sizes[i];
                    leftmost_src_idx = i;
                }
            }

            auto actual_pos_leftmost = 0u;
            for (auto i = 0u; i < children.size(); ++i) {
                if (children[i]->src_idx == leftmost_src_idx) {
                    actual_pos_leftmost = i;
                    break;
                }
            }


            // puts child on the left
            children[0].swap(children[actual_pos_leftmost]);
            tree->children[0].swap(tree->children[actual_pos_leftmost]);

            plan = FreeJoinPlan{tree};

            std::unordered_map<int16_t, std::vector<size_t>> out_physical_schema;
            std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> leftmost_physical_schema(
                    1);
            for (auto i = 0u; i < children.size(); ++i) {
                // Materialize input
                auto child_src_idx = children[i]->src_idx;
                // Get schema from Free Join Plan
                const auto &trie_schema = plan.schema_map.find(child_src_idx)->second;
                const auto &input_schema = children[i]->out_attributes;
                // Find input idxs from schema
                assert(i != 0 || (i == 0 && trie_schema.size() == 1));
                std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> physical_schema(
                        trie_schema.size());
                auto &current_physical_schema = (child_src_idx == leftmost_src_idx) ? leftmost_physical_schema
                                                                                    : physical_schema;
                for (auto trie_level = 0u; trie_level < trie_schema.size(); ++trie_level) {
                    for (const auto &attr: trie_schema[trie_level]) {
                        current_physical_schema[trie_level].push_back(
                                std::find(input_schema.begin(), input_schema.end(), attr) -
                                input_schema.begin());
                    }
                }

                for (const auto &out_attr: out_attributes) {
                    auto it = std::find(input_schema.begin(), input_schema.end(), out_attr);
                    if (it != input_schema.end()) {
                        out_physical_schema[child_src_idx].push_back(it - input_schema.begin());
                    }
                }

                std::cout << "Num Tuples: " << input_sizes[child_src_idx] << std::endl;
                if (leftmost_src_idx == child_src_idx) {
                    continue;
                }

                std::cout << "Trie Height: " << plan.schema_map[child_src_idx].size() << std::endl;
                params.setParam("name", "chunked list building");
                {
                    PerfEventBlock perf(1, params, print_header);
                    chunked_list_map[child_src_idx] = ChunkedListBuilder::build_cl(*input_ptrs[child_src_idx],
                                                                                   physical_schema[0]);
                }
                print_header = false;
                params.setParam("name", "trie building");
                {
                    PerfEventBlock perf(1, params, print_header);
                    auto trie_ptrs = GHTBuilder::build_direct_cl(chunked_list_map[child_src_idx],
                                                                 physical_schema,
                                                                 trie_schema);
                    trie_map[child_src_idx] = trie_ptrs;
                }
                print_header = false;
            }

            // Intersect tries and store result
            params.setParam("name", "trie intersection");
            {
                PerfEventBlock perf(1, params, false);
                // TODO (if scan -> parallel_for else -> CAS) + connect output tuples
                output_cl = GHTIntersector::intersect_direct_cl(leftmost_src_idx,
                                                                input_ptrs,
                                                                leftmost_physical_schema[0],
                                                                trie_map,
                                                                plan,
                                                                out_attributes.size(),
                                                                out_physical_schema);
            }

            if (output_cl.empty()) {
                return false;
            }
            return true;
        };

        void close() override {
            for (auto &[_, cl_vec]: chunked_list_map) {
                for (auto &cl: cl_vec) {
                    cl.free_chunked_list();
                }
            }
            // free tries
            for (auto &[_, tries_ptr]: trie_map) {
                for (auto ptr: tries_ptr.ghts) {
                    delete reinterpret_cast<GHT *>(ptr);
                }
            }

//            // free materialized output
//            for (auto *ptr: materialized_output) {
//                delete ptr;
//            }

            // close children
            for (auto &child: children) {
                child->close();
            }
        };

        Tuple &get_output() override {
            return tuple_register;
        };

        ChunkedList<Tuple, max_chunk_size_tuple> *get_tuples() override {
            return &output_cl;
        };

        void print() const override {
            std::cout << "FREEJOIN( ";
            for (const auto &child: children) {
                child->print();
                std::cout << " ";
            }
            std::cout << ")";
        };
    };
}