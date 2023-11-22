#include "query_optimizer/free_join_plan.h"
#include "iostream"
#include "algorithm"

using namespace free_join;

FreeJoinPlan::FreeJoinPlan(LogicalOperatorTree *tree) : tree(tree) {
    generate_fully_active_plan();
    print();
    extract_trie_schemas();
}


void FreeJoinPlan::generate_active_plan() {
    generate_canonical_plan();
    auto n = plan.size();
    std::vector<bool> is_active(plan.size(), false);
    is_active[0] = true; // first node is always active
    std::set<uint16_t> stale_attributes;

    for (auto i = 1u; i < n; ++i) {
        for (const auto &attr: plan[i].atoms[0].attributes) {
            if (std::find(stale_attributes.begin(), stale_attributes.end(), attr.fj_attribute_idx) ==
                stale_attributes.end()) {
                stale_attributes.insert(attr.fj_attribute_idx);
                is_active[i] = true;
            }
        }
    }

    for (auto i = n - 1; i > 0; --i) {
        if (!is_active[i]) {
            plan[i - 1].atoms.insert(plan[i - 1].atoms.end(), plan[i].atoms.begin(), plan[i].atoms.end());
        }
    }

    auto new_plan = std::vector<FreeJoinNode>();
    for (auto i = 0; i < n; ++i) {
        if (is_active[i]) {
            new_plan.push_back(plan[i]);
        }
    }

    plan = new_plan;
}


void FreeJoinPlan::generate_canonical_plan() {
    auto fj_op = reinterpret_cast<LogicalNaryJoin *>(tree->op.get());
    std::unordered_map<std::string, std::set<BindingAttribute>> table_attributes;
    for (const auto &cond: fj_op->equality_conditions) {
        table_attributes[cond.left.binding].insert(cond.left);
        table_attributes[cond.right.binding].insert(cond.right);
    }

    auto node = FreeJoinNode{};
    std::set<uint16_t> stale_attributes;
    for (auto child = tree->children.begin(); child != tree->children.end(); ++child) {
        if (node.empty()) {
            std::vector<BindingAttribute, tbb::scalable_allocator<BindingAttribute>> attributes;
            for (const auto &binding: (*child)->available_bindings) {
                attributes.insert(attributes.end(), table_attributes[binding].begin(), table_attributes[binding].end());
            }
            node.atoms.push_back(FreeJoinAtom{(*child)->src_idx, attributes, true});
        } else {
            for (auto attr: node.atoms[0].attributes) {
                stale_attributes.insert(attr.fj_attribute_idx);
            }
            FreeJoinAtom atom{(*child)->src_idx};
            std::vector<BindingAttribute, tbb::scalable_allocator<BindingAttribute>> remaining_attributes;
            for (const auto &binding: (*child)->available_bindings) {
                for (const auto &attr: table_attributes[binding]) {
                    if (std::find(stale_attributes.begin(), stale_attributes.end(), attr.fj_attribute_idx) !=
                        stale_attributes.end()) {
                        // found
                        atom.attributes.push_back(attr);
                    } else {
                        // not found
                        remaining_attributes.push_back(attr);
                    }
                }
            }
            node.atoms.push_back(atom);
            plan.push_back(node);
            node.atoms.clear();
            if (!remaining_attributes.empty()) {
                node.atoms.push_back(FreeJoinAtom{(*child)->src_idx, remaining_attributes, true});
            }
        }
    }
    if (!node.empty()) {
        plan.push_back(node);
    }
}


// converts left deep tree to fully-active free join plan
void FreeJoinPlan::generate_fully_active_plan() {
    auto fj_op = reinterpret_cast<LogicalNaryJoin *>(tree->op.get());
    std::unordered_map<std::string, std::set<BindingAttribute>> table_attributes;
    for (const auto &cond: fj_op->equality_conditions) {
        table_attributes[cond.left.binding].insert(cond.left);
        table_attributes[cond.right.binding].insert(cond.right);
    }
    std::vector<uint16_t> current_fj_idxs;
    std::vector<uint16_t> explored_fj_idxs;
    uint16_t cover_num_attributes;
    for (auto child = tree->children.begin();
         child != tree->children.end() - 1 /* last child cannot be singleton */; ++child) {
        current_fj_idxs.clear();
        cover_num_attributes = 0;
        FreeJoinNode node;
        FreeJoinAtom atom{(*child)->src_idx, true};
        // get bindings under subtree
        for (const auto &binding: (*child)->available_bindings) {
            for (const auto &binding_attr: table_attributes[binding]) {
                if (std::find(explored_fj_idxs.begin(), explored_fj_idxs.end(), binding_attr.fj_attribute_idx) ==
                    explored_fj_idxs.end()) {
                    // if one of the attributes of the binding has not been explored, add it to node
                    explored_fj_idxs.push_back(binding_attr.fj_attribute_idx);
                    current_fj_idxs.push_back(binding_attr.fj_attribute_idx);
                    cover_num_attributes++;
                    atom.attributes.push_back(binding_attr);
                }
            }
        }
        if (atom.empty()) {
            continue;
        }
        node.atoms.push_back(atom);
        for (auto next_child = child + 1; next_child != tree->children.end(); ++next_child) {
            FreeJoinAtom next_atom{(*next_child)->src_idx, false};
            uint16_t num_attributes = 0;
            for (const auto &binding: (*next_child)->available_bindings) {
                // loop through available bindings
                for (const auto &binding_attr: table_attributes[binding]) {
                    // loop through attributes
                    if (std::find(current_fj_idxs.begin(), current_fj_idxs.end(), binding_attr.fj_attribute_idx) !=
                        current_fj_idxs.end()) {
                        // if fj_idx in current_fj_idxs, then add attribute to current atom
                        next_atom.attributes.push_back(binding_attr);
                        num_attributes++;
                    }
                }
            }
            if (next_atom.empty()) {
                continue;
            }
            // check if cover
            if (num_attributes == cover_num_attributes) {
                next_atom.is_cover = true;
            }
            // add to current node
            node.atoms.push_back(next_atom);
        }
        plan.push_back(node);
    }
}

void FreeJoinPlan::extract_trie_schemas() {
    for (const auto &node: plan) {
        for (const auto &atom: node.atoms) {
            schema_map[atom.src_index].push_back(atom.attributes);
        }
    }
}

void FreeJoinPlan::print() const {
    std::cout << "Free Join Plan:\t";
    auto num_nodes = plan.size();
    /////////////////////////////
    // Extra Plan Stats
    std::vector<uint16_t> atoms_per_node; // number of atoms for each node
    std::vector<uint16_t> attrs_per_node; // number of attributes considered per node (check cover)
    /////////////////////////////
    // Print Plan
    for (const auto &node: plan) {
        std::cout << "[ ";
        for (const auto &atom: node.atoms) {
//            if (atom.is_cover) {
//                std::cout << "COVER";
//            }
            std::cout << "( ";
            for (const auto &attr: atom.attributes) {
                std::cout << attr.binding << "." << attr.attribute << " ";
            }
            std::cout << ") ";
        }
        std::cout << "]";
        atoms_per_node.push_back(node.atoms.size());
        attrs_per_node.push_back(node.atoms[0].attributes.size());
    }
    std::cout << std::endl;
    /////////////////////////////
    // Print Plan Stats
    std::cout << "Num Nodes:\t" << num_nodes << std::endl;
    std::cout << "Num Atoms:\t";
    for (auto n: atoms_per_node) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    std::cout << "Num Attributes:\t";
    for (auto n: attrs_per_node) {
        std::cout << n << " ";
    }
    std::cout << std::endl;
    /////////////////////////////
    // Print Trie Stats
//    int16_t leftmost_src = plan[0].atoms[0].src_index;
//    std::cout << "Trie Height:" << std::endl;
    for (const auto &[src_idx, schema]: schema_map) {
//        if (src_idx == leftmost_src) {
//            continue;
//        }
        std::cout << "Trie Height: " << schema.size() << std::endl;
    }
    std::cout << std::endl;
}


// converts left deep tree to free join (OLD)
//void FreeJoinPlan::generate_free_join_plan_old() {
//    plan = initialize_fj_plan(tree);
//    combine_adjacent_nodes();
//    remove_empty_nodes();
//    std::cout << "Canonical Plan:\t";
//    print();
//    optimize();
//    std::cout << "Optimized Plan:\t";
//    print();
//    std::cout << "----" << std::endl;
//}
//
//void FreeJoinPlan::combine_adjacent_nodes() {
////     loop through nodes starting index 1
//    for (auto i = std::next(plan.begin()); i != plan.end(); ++i) {
//        uint16_t count = 0;
//        // loop through content of first (and only) atom
//        std::vector<BindingAttribute> tmp_content;
//        for (const auto &bind_attr: i->atoms[0].content) {
//            auto &conditions = conditions_index[bind_attr];
//            // loop through atoms of previous node
//            for (auto j = plan.begin(); j < i; ++j) {
//                for (const auto &other_atom: j->atoms) {
//                    for (const auto &other_bind_attr: other_atom.content) {
//                        if (std::find(conditions.begin(), conditions.end(), other_bind_attr) != conditions.end()) {
//                            // move current atom to previous node
//                            tmp_content.push_back(bind_attr);
//                            goto continue_next;
//                        }
//                    }
//                }
//            }
//            i->atoms[0].content[count++] = bind_attr;
//            continue_next:;
//        }
//        i->atoms[0].content.resize(count);
//        if (!tmp_content.empty()) {
//            (i - 1)->atoms.push_back(FreeJoinAtom{i->atoms[0].src_index, tmp_content});
//        }
//    }
//}
//
//// iterate through relations left deep + insert schema into map during iteration while reading join conditions
//std::vector<FreeJoinNode> FreeJoinPlan::initialize_fj_plan(LogicalOperatorTree *ptr) {
//    if (ptr->tree_type == SINGLETON) {
//        // scan node
//        auto *scan_ptr = reinterpret_cast<LogicalGet *>(ptr->op.get());
//        auto binding = scan_ptr->table_binding.binding;
//        std::vector<BindingAttribute> content;
//        // create singleton atoms
//        for (const auto &attr: schema_index[binding]) {
//            content.push_back(BindingAttribute{binding, attr});
//        }
//        return {FreeJoinNode{{FreeJoinAtom{-1, content}}}};
//    } else {
//        // determine necessary attribute from join conditions
//        for (const auto &cond: reinterpret_cast<LogicalBinaryJoin *>(ptr->op.get())->equality_conditions) {
//            conditions_index[cond.left].push_back(cond.right);
//            conditions_index[cond.right].push_back(cond.left);
//            schema_index[cond.left.binding].insert(cond.left.attribute);
//            schema_index[cond.right.binding].insert(cond.right.attribute);
//        }
//        // explore left child
//        auto plan = initialize_fj_plan(ptr->children[0].get());
//
//        // explore right child (might be intermediate result)
//        if (ptr->children[1]->tree_type == SINGLETON) {
//            // scan
//            plan.push_back(initialize_fj_plan(ptr->children[1].get())[0]);
//        } else {
//            // intermediate result
//            auto right_child_src_idx = ptr->children[1]->src_idx;
//            std::vector<BindingAttribute> content;
//            for (const auto &binding: ptr->children[1]->get_table_bindings()) {
//                for (const auto &attr: schema_index[binding]) {
//                    content.push_back({binding, attr});
//                }
//            }
//            plan.push_back(FreeJoinNode{{FreeJoinAtom{right_child_src_idx, content}}});
//        }
//        return plan;
//    }
//}
//
//
//void FreeJoinPlan::optimize() {
//    // add other optimizations?
//    factor();
//}
//
//void FreeJoinPlan::factor() {
//    std::vector<FreeJoinNode> optimized_plan;
//    // loop through nodes backwards and try to move them to previous node and break.
//    outer:
//    for (auto i = plan.rbegin(); i != plan.rend() - 1; ++i) {
//        // loop through current node's atoms
//        bool stop_moving_atoms = false;
//        FreeJoinNode optimized_node;
//        for (auto &atom: i->atoms) {
//            if (stop_moving_atoms) {
//                optimized_node.atoms.push_back(atom);
//            } else {
//                if ((i + 1)->contains_binding(atom.src_index, atom.content[0].binding)) {
//                    // previous node contains either same intermediate result or same base table
//                    stop_moving_atoms = true;
//                    optimized_node.atoms.push_back(atom);
//                } else {
//                    // loop through all previous nodes' atoms and check for matches
//                    if (can_move_atom((plan.rend() - i) - 1, atom)) {
//                        (i + 1)->atoms.push_back(atom);
//                    } else {
//                        stop_moving_atoms = true;
//                        optimized_node.atoms.push_back(atom);
//                    }
//                }
//            }
//        }
//        if (!optimized_node.is_empty()) {
//            optimized_plan.push_back(optimized_node);
//        }
//    }
//    optimized_plan.push_back(plan[0]);
//    std::reverse(optimized_plan.begin(), optimized_plan.end());
//    plan = optimized_plan;
//}
//
//
//void FreeJoinPlan::print() const {
//    for (const auto &node: plan) {
//        std::cout << "[ ";
//        for (const auto &atom: node.atoms) {
//            if (atom.src_index != -1) {
//                std::cout << "INTMD_RES" << atom.src_index;
//            }
//            std::cout << "(";
//            for (const auto &binding: atom.content) {
//                std::cout << " " << binding.binding << "." << binding.attribute;
//            }
//            std::cout << " ) ";
//        }
//        std::cout << "] ";
//    }
//    std::cout << std::endl;
//}
