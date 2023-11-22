#pragma once

#include <vector>
#include <cstdint>
#include "physical_operator.h"
#include "execution_engine/chunked_list_builder.h"
#include "execution_engine/ght_builder.h"
#include "execution_engine/ght_intersector.h"
#include "utils/PerfEvent.hpp"

namespace free_join {

    class PhysicalFreeJoinSmart : public PhysicalNaryOperator {

    private:
        std::unordered_map<int16_t, TupleVec> materialized_input;
        std::vector<TupleVec *> materialized_output;
        std::map<int16_t, GHTWrapper> trie_map;
        std::map<int16_t, std::vector<ChunkedList<size_t, max_chunk_size_idx>>> chunked_list_map;
        Tuple tuple_register;
        uint64_t rows_read;
        uint64_t total_rows;
        uint64_t current_output_idx;
        uint16_t current_output;
        LogicalOperatorTree *tree;
        bool first_call;

    public:
        FreeJoinPlan plan;

    public:

        explicit PhysicalFreeJoinSmart(int16_t src, LogicalOperatorTree *tree) :
                tree(tree),
                rows_read(0),
                total_rows(0),
                current_output(0),
                current_output_idx(0),
                first_call(true) {
            src_idx = src;
        };

        void open() override {
            for (auto &child: children) {
                child->open();
            }
            tuple_register.resize(out_attributes.size());
        };

        bool next() override {
            if (first_call) {
                first_call = false;
                for (auto i = 0u; i < children.size(); ++i) {
                    // Materialize input
                    auto child_src_idx = children[i]->src_idx;
                    auto &input_reg = children[i]->get_output();

                    while (children[i]->next()) {
                        materialized_input[child_src_idx].push_back(input_reg);
                    }
                    if (materialized_input[child_src_idx].empty()) {
                        return false;
                    }
                }

                int16_t leftmost_src_idx = children[0]->src_idx;
//                size_t max_size_input = 0;
//                for (const auto &[k, v]: materialized_input) {
//                    if (v.size() > max_size_input) {
//                        max_size_input = v.size();
//                        leftmost_src_idx = k;
//                    }
//                }
//                std::vector<std::pair<size_t, int16_t>> sizes;
//                sizes.reserve(children.size());
//                for (const auto &[k, v]: materialized_input) {
//                    sizes.emplace_back(v.size(), k);
//                }
//                std::sort(sizes.begin(), sizes.end());
//                leftmost_src_idx = sizes[sizes.size() / 2].second;

//                auto actual_pos_leftmost = 0u;
//                for (auto i = 0u; i < children.size(); ++i) {
//                    if (children[i]->src_idx == leftmost_src_idx) {
//                        actual_pos_leftmost = i;
//                        break;
//                    }
//                }
//
//                // puts largest child on the left
//                children[0].swap(children[actual_pos_leftmost]);
//                tree->children[0].swap(tree->children[actual_pos_leftmost]);
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
                for (const auto & child : children) {
                    for (const auto &attr: child->out_attributes) {
                        if (attr.fj_attribute_idx == most_common_fj_idx) {
                            possible_leftmost_srcs.push_back(child->src_idx);
                        }
                    }
                }


                size_t max_size_input = 0;
                for (auto i: possible_leftmost_srcs) {
                    if (materialized_input[i].size() > max_size_input) {
                        max_size_input = materialized_input[i].size();
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
                for (const auto & child : children) {
                    // Materialize input
                    auto child_src_idx = child->src_idx;
                    // Get schema from Free Join Plan
                    const auto &trie_schema = plan.schema_map.find(child_src_idx)->second;
                    const auto &input_schema = child->out_attributes;
                    // Find input idxs from schema
                    assert(child != 0 || (child == 0 && trie_schema.size() == 1));
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

                    std::cout << "Num Tuples: " << materialized_input[child_src_idx].size() << std::endl;
                    if (leftmost_src_idx == child_src_idx) {
                        continue;
                    }

                    std::cout << "Trie Height: " << plan.schema_map[child_src_idx].size() << std::endl;
                    BenchmarkParameters params;
                    params.setParam("name", "chunked list building");
                    {
                        PerfEventBlock perf(1, params, print_header);
                        chunked_list_map[child_src_idx] = ChunkedListBuilder::build(materialized_input[child_src_idx],
                                                                                    physical_schema[0]);
                    }
                    print_header = false;
                    BenchmarkParameters params3;
                    params3.setParam("name", "trie building");
                    {
                        PerfEventBlock perf(1, params3, print_header);
                        // Build Trie (Maybe StoreHash = True?)
                        auto trie_ptrs = GHTBuilder::build_direct(materialized_input[child_src_idx],
                                                                  chunked_list_map[child_src_idx],
                                                                  physical_schema,
                                                                  trie_schema);
                        trie_map[child_src_idx] = trie_ptrs;
                    }
                    print_header = false;
                }

                // Intersect tries and store result
                BenchmarkParameters params2;
                params2.setParam("name", "trie intersection");
                {
                    PerfEventBlock perf(1, params2, false);
                    materialized_output = GHTIntersector::intersect_direct(leftmost_src_idx,
                                                                           materialized_input,
                                                                           leftmost_physical_schema[0],
                                                                           trie_map,
                                                                           plan,
                                                                           out_attributes.size(),
                                                                           out_physical_schema);
                }
                for (auto *ptr: materialized_output) {
                    total_rows += ptr->size();
                }

            }
            if (rows_read >= total_rows) {
                return false;
            }
            // move next element to tuple register
            while (materialized_output[current_output]->empty()) {
                current_output++;
            }
            tuple_register = (*materialized_output[current_output])[current_output_idx++];
            if (current_output_idx == materialized_output[current_output]->size()) {
                current_output_idx = 0;
                current_output++;
            }
            rows_read++;
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

            // free materialized output
            for (auto *ptr: materialized_output) {
                delete ptr;
            }

            // close children
            for (auto &child: children) {
                child->close();
            }
        };

        Tuple &get_output() override {
            return tuple_register;
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