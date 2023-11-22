#include "query_optimizer/optimizer.h"
#include "execution_engine/operators/physical_scan.h"
#include "execution_engine/operators/physical_scan_full.h"
#include "execution_engine/operators/physical_projection.h"
#include "execution_engine/operators/physical_free_join.h"
#include "execution_engine/operators/physical_free_join_fullmat.h"
#include "execution_engine/operators/physical_free_join_direct.h"
#include "execution_engine/operators/physical_free_join_full.h"
#include "execution_engine/operators/physical_free_join_smart.h"
#include <iostream>
#include <queue>

using namespace free_join;

Optimizer::Optimizer(const std::unordered_map<std::string, MaterializedQueryResult *> &data_map) : data_map(data_map) {}

std::unique_ptr<LogicalOperatorTree> Optimizer::convert_to_hybrid_plan(LogicalOperatorTree *root) {
    // generate logical hybrid free join plan
    global_src_idx = 0;
    auto hybrid_plan = std::make_unique<LogicalOperatorTree>(); // logical parent of root
    to_hybrid_plan(root, hybrid_plan.get());
    hybrid_plan = std::move(hybrid_plan->children[0]); // discard logical parent of root // TODO remove this
    return hybrid_plan;
};

void Optimizer::to_hybrid_plan(LogicalOperatorTree *old_tree, LogicalOperatorTree *new_tree /* new parent */) {
    auto bushy_node = old_tree->op->get_type() == LOGICAL_BINARY_JOIN &&
                      old_tree->children[1]->op->get_type() == LOGICAL_BINARY_JOIN;
    if (bushy_node) {
        old_tree->children[1]->is_free_join = true;
    }
    auto unary_child_is_join =
            old_tree->op->get_type() != LOGICAL_BINARY_JOIN && old_tree->op->get_type() != LOGICAL_GET &&
            old_tree->children[0]->op->get_type() == LOGICAL_BINARY_JOIN;
    if (unary_child_is_join) {
        old_tree->children[0]->is_free_join = true;
    }
    if (old_tree->op->get_type() == LOGICAL_GET) {
        auto current_tree = std::make_unique<LogicalOperatorTree>();
        auto scan_op = reinterpret_cast<const LogicalGet *>(old_tree->op.get());
        current_tree->src_idx = global_src_idx++;
        current_tree->op = std::make_unique<LogicalGet>(*scan_op);
        bindings_map[scan_op->table_binding.binding] = current_tree->src_idx;
        new_tree->available_bindings.push_back(scan_op->table_binding.binding);
        current_tree->available_bindings.push_back(scan_op->table_binding.binding);
        new_tree->children.push_back(std::move(current_tree));
    } else if (old_tree->op->get_type() == LOGICAL_PROJECTION) {
        auto current_tree = std::make_unique<LogicalOperatorTree>();
        current_tree->op = std::make_unique<LogicalProjection>(
                *reinterpret_cast<LogicalProjection *>(old_tree->op.get()));
        to_hybrid_plan(old_tree->children[0].get(), current_tree.get());
        for (const auto &child_binding: current_tree->available_bindings) {
            new_tree->available_bindings.push_back(child_binding);
        }
        current_tree->src_idx = global_src_idx++;
        new_tree->children.push_back(std::move(current_tree));
    } else if (old_tree->op->get_type() == LOGICAL_BINARY_JOIN) {
        auto bin_join_op = reinterpret_cast<const LogicalBinaryJoin *>(old_tree->op.get());
        if (old_tree->is_free_join) {
            auto current_tree = std::make_unique<LogicalOperatorTree>();
            auto free_join_op = std::make_unique<LogicalNaryJoin>();
            std::copy(bin_join_op->equality_conditions.begin(), bin_join_op->equality_conditions.end(),
                      back_inserter(free_join_op->equality_conditions));
            current_tree->op = std::move(free_join_op);
            to_hybrid_plan(old_tree->children[0].get(), current_tree.get());
            to_hybrid_plan(old_tree->children[1].get(), current_tree.get());
            current_tree->src_idx = global_src_idx++;
            for (const auto &child_binding: current_tree->available_bindings) {
                new_tree->available_bindings.push_back(child_binding);
            }
            new_tree->children.push_back(std::move(current_tree));
        } else {
            auto free_join_op = reinterpret_cast<LogicalNaryJoin *>(new_tree->op.get());
            std::copy(bin_join_op->equality_conditions.begin(), bin_join_op->equality_conditions.end(),
                      back_inserter(free_join_op->equality_conditions));
            to_hybrid_plan(old_tree->children[0].get(), new_tree);
            to_hybrid_plan(old_tree->children[1].get(), new_tree);
        }
    }
}

std::unique_ptr<PhysicalOperator> Optimizer::convert_to_physical_plan(LogicalOperatorTree *root) {
    // build physical plan from logical plan
    std::unique_ptr<PhysicalOperator> physical_plan;
    to_physical_plan(root, physical_plan, {});
    return physical_plan;
};

void Optimizer::to_physical_plan(LogicalOperatorTree *logical_tree, std::unique_ptr<PhysicalOperator> &physical_tree,
                                 std::vector<BindingAttribute> requested_attr) {
    if (logical_tree->op->get_type() == LOGICAL_GET) {
        // create physical scan
        std::vector<BindingAttribute> requested_attr_vec(requested_attr.begin(), requested_attr.end());
        // sort by physical idx for better cache locality (especially if row store)
//        std::sort(requested_attr_vec.begin(), requested_attr_vec.end(),
//                  [&](const BindingAttribute &lhs, const BindingAttribute &rhs) {
//                      return lhs.table_attribute_idx < rhs.table_attribute_idx;
//                  });
        auto scan_binding = reinterpret_cast<LogicalGet *>(logical_tree->op.get())->table_binding.binding;
        std::vector<BindingAttribute> attributes;
        for (const auto &attr: requested_attr_vec) {
            if (attr.binding == scan_binding) {
                attributes.push_back(attr);
            }
        }
        physical_tree = std::make_unique<PhysicalScan>(logical_tree->src_idx, attributes,
                                                       data_map.find(scan_binding)->second);
    } else if (logical_tree->op->get_type() == LOGICAL_PROJECTION) {
        // create physical projection
        auto logical_proj_op = reinterpret_cast<LogicalProjection *>(logical_tree->op.get());
        auto more_requested_attr = requested_attr;
        for (const auto &attr: logical_proj_op->projection_attributes) {
            if (std::find(more_requested_attr.begin(), more_requested_attr.end(), attr) == more_requested_attr.end()) {
                more_requested_attr.push_back(attr);
            }
        }
        auto physical_proj = std::make_unique<PhysicalProjection>(logical_tree->src_idx);
        to_physical_plan(logical_tree->children[0].get(), physical_proj->child, more_requested_attr);
        for (const auto &binding_attr: physical_proj->child->out_attributes) {
            if (std::find(requested_attr.begin(), requested_attr.end(), binding_attr) !=
                requested_attr.end() /* use set::contains() with C++20 */) {
                physical_proj->out_attributes.push_back(binding_attr);
            }
        }
        physical_tree = std::move(physical_proj);
    } else if (logical_tree->op->get_type() == LOGICAL_NARY_JOIN) {
        // create physical free join
        auto logical_join_op = reinterpret_cast<LogicalNaryJoin *>(logical_tree->op.get());
        auto more_requested_attr = requested_attr;
        for (const auto &cond: logical_join_op->equality_conditions) {
            if (std::find(more_requested_attr.begin(), more_requested_attr.end(), cond.left) ==
                more_requested_attr.end()) {
                more_requested_attr.push_back(cond.left);
            }
            if (std::find(more_requested_attr.begin(), more_requested_attr.end(), cond.right) ==
                more_requested_attr.end()) {
                more_requested_attr.push_back(cond.right);
            }
        }
        auto physical_free_join = std::make_unique<PhysicalFreeJoinSmart>(logical_tree->src_idx, logical_tree);
        physical_free_join->children.resize(logical_tree->children.size());
        for (auto i = 0u; i < physical_free_join->children.size(); ++i) {
            to_physical_plan(logical_tree->children[i].get(), physical_free_join->children[i], more_requested_attr);
            for (const auto &binding_attr: physical_free_join->children[i]->out_attributes) {
                if (std::find(requested_attr.begin(), requested_attr.end(), binding_attr) !=
                    requested_attr.end() /* use set::contains() with C++20 */) {
                    physical_free_join->out_attributes.push_back(binding_attr);
                }
            }
        }
        physical_tree = std::move(physical_free_join);
    }
    /*
     * could add more physical operators
     */
}
//void Optimizer::fill_dependency_stack(LogicalOperatorTree *root) {
//    recognize_dependencies(root);
//    root->src_idx = global_src_idx;
//    root->is_free_join = true;
//    dependency_stack.push_back(root);
//    global_src_idx = 0; // reset for next query
//};
//
//void Optimizer::recognize_dependencies(LogicalOperatorTree *root) {
//    if (root->tree_type != SINGLETON) {
//        recognize_dependencies(root->children[0].get());
//        if (root->children.size() > 1) {
//            recognize_dependencies(root->children[1].get());
//            if (root->children[1]->tree_type != SINGLETON) {
//                root->children[1]->src_idx = global_src_idx++;
//                root->children[1]->is_free_join = true;
//                dependency_stack.push_back(root->children[1].get());
//            }
//        }
//    } else {
//        auto op = reinterpret_cast<const LogicalGet *>(root->op.get());
//        root->src_idx = global_src_idx++;
//        bindings_map[op->table_binding.binding] = root->src_idx;
//    }
//};

//void Optimizer::init_physical_info(LogicalOperatorTree *root) {
//    if (root->op->get_type() == LOGICAL_PROJECTION) {
//        for (auto &proj_attr: reinterpret_cast<const LogicalProjection *>(root->op.get())->projection_attributes) {
//            auto src = bindings_map[proj_attr.binding];
//            physical_info[src][proj_attr.table_attribute_idx]++;
//        }
//        init_physical_info(root->children[0].get());
//    } else if (root->op->get_type() == LOGICAL_BINARY_JOIN) {
//        for (auto &cond: reinterpret_cast<const LogicalBinaryJoin *>(root->op.get())->equality_conditions) {
//            auto src = bindings_map[cond.left.binding];
//            physical_info[src][cond.left.table_attribute_idx]++;
//            src = bindings_map[cond.right.binding];
//            physical_info[src][cond.right.table_attribute_idx]++;
//        }
//        init_physical_info(root->children[0].get());
//        init_physical_info(root->children[1].get());
//    }
//}

//void Optimizer::print_dependency_stack() const {
//    auto level = 0u;
//    std::cout << "Dependency Stack:" << std::endl;
//    for (auto ldt: dependency_stack) {
//        std::cout << "Level: " << level++ << std::endl;
//        ldt->pretty_print();
//    }
//};

