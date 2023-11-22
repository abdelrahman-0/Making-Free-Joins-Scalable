#pragma once

#include "execution_engine/operators/physical_operator.h"
#include <set>
#include <algorithm>
#include "unordered_map"
#include "map"
#include "query_optimizer/operators/logical_operator_tree.h"

//struct FJQuark {
//    std::string name;
//    std::set<std::string> schema;
//};

namespace free_join {

    struct FreeJoinAtom {
        int16_t src_index = -1;
        std::vector<BindingAttribute, tbb::scalable_allocator<BindingAttribute>> attributes;
        bool is_cover = false;

        FreeJoinAtom() = default;

        explicit FreeJoinAtom(int16_t src_index, bool is_cover = false) : src_index(src_index), is_cover(is_cover) {};

        explicit FreeJoinAtom(int16_t src_index,
                              std::vector<BindingAttribute, tbb::scalable_allocator<BindingAttribute>> attrs,
                              bool is_cover = false) : src_index(src_index), attributes(attrs), is_cover(is_cover) {};

        [[nodiscard]] bool empty() const {
            return attributes.empty();
        }
    };

    struct FreeJoinNode {
    public:
        std::vector<FreeJoinAtom> atoms;

    public:
        FreeJoinNode() = default;

        [[nodiscard]] bool empty() const {
            return atoms.empty();
        }

        bool contains_binding(const int &src_idx, const std::string &binding) {
            if (src_idx == -1) {
                // if base table, simply compare bindings
                for (const auto &atom: atoms) {
//                    if (atom.src_index == -1 && atom.content[0].binding == binding) {
//                        return true;
//                    }
                }
            } else {
                //
                for (const auto &atom: atoms) {
                    if (src_idx == atom.src_index) {
                        return true;
                    }
                }
            }
            return false;
        }
    };

    class FreeJoinPlan {

    private:
        LogicalOperatorTree *tree = nullptr;
    public:
        std::vector<FreeJoinNode> plan;
        TrieSchemaMap schema_map;
//        std::unordered_map<std::string, std::set<std::string>> schema_index;
//        std::map<BindingAttribute, std::vector<BindingAttribute>, BindingAttributeCompare> conditions_index;
    public:
        FreeJoinPlan() = default;

        explicit FreeJoinPlan(LogicalOperatorTree *tree);

        void generate_canonical_plan();

        void generate_active_plan();

        void generate_fully_active_plan();

        void extract_trie_schemas();

        void print() const;

//        void generate_free_join_plan_old();
//
//        [[nodiscard]] std::vector<FreeJoinNode> initialize_fj_plan(LogicalOperatorTree *ptr);
//
//        void optimize();
//
//        void combine_adjacent_nodes();
//
//        void factor();
//
//        struct AtomIsEmpty {
//            bool operator()(const FreeJoinAtom &atom) {
//                return atom.content.empty();
//                return false;
//            }
//        };
//
//        struct NodeIsEmpty {
//            bool operator()(const FreeJoinNode &node) {
//                return node.empty();
//            }
//        };
//
//        void remove_empty_nodes() {
//            for (auto &node: plan) {
//                node.atoms.erase(std::remove_if(node.atoms.begin(), node.atoms.end(), AtomIsEmpty()), node.atoms.end());
//            }
//            plan.erase(std::remove_if(plan.begin(), plan.end(), NodeIsEmpty()), plan.end());
//        }

//        bool can_move_atom(uint16_t node_idx, const FreeJoinAtom &atom) {
//            for (const auto &bind_attr: atom.content) {
//                auto matches = conditions_index[bind_attr];
//                for (auto i = 0u; i < node_idx; ++i) {
//                    for (const auto &other_atom: plan[i].atoms) {
//                        for (const auto &other_bind_attr: other_atom.content) {
//                            if (std::find(matches.begin(), matches.end(), other_bind_attr) != matches.end()) {
//                                goto continue_next;
//                            }
//                        }
//                    }
//                }
//                return false;
//                continue_next:;
//            }
//            return true;
//        }
//
//        void print_plan() const;
//
//        void to_json() const;

    private:
    };
}