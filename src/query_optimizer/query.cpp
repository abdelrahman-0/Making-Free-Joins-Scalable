#include <fstream>
#include <iostream>
#include <utility>
#include "query_optimizer/query.h"

using namespace free_join;

Query::Query(json plan)
        : json_plan(std::move(plan)) {
    tree = json_to_operator_tree();
};

std::unique_ptr<LogicalOperatorTree> Query::json_to_operator_tree() const {
    return build_tree(json_plan);
}

std::unique_ptr<LogicalOperatorTree> Query::build_tree(const json &param) const {
    if (param["name"] == "projection") {
        auto ptr = std::make_unique<LogicalOperatorTree>();
        auto op = std::make_unique<LogicalProjection>();
        for (auto &cond: param["attributes"]) {
            BindingAttribute proj_attr;
            proj_attr.attribute = cond["attribute"];
            proj_attr.binding = cond["binding"];
            proj_attr.table_attribute_idx = cond["table_attribute_idx"];
            op->projection_attributes.push_back(proj_attr);
        }
        ptr->op = std::move(op);
        ptr->children.resize(1);
        ptr->children[0] = build_tree(param["children"][0]);
        ptr->tree_type = (ptr->children[0]->tree_type == SINGLETON) ? LEFTDEEP : ptr->children[0]->tree_type;
        return ptr;
    } else if (param["name"] == "hashjoin") {
        auto left_child = build_tree(param["children"][0]);
        auto right_child = build_tree(param["children"][1]);
        auto ptr = std::make_unique<LogicalOperatorTree>();
        auto op = std::make_unique<LogicalBinaryJoin>();
        // loop through equality conditions and store them in a vector
        std::vector<EqualityCondition> equality_conditions;
        for (auto &cond: param["conditions"]) {
            BindingAttribute left;
            {
                left.binding = cond["left"]["binding"];
                left.attribute = cond["left"]["attribute"];
                left.fj_attribute_idx = cond["left"]["fj_attribute_idx"];
                left.table_attribute_idx = cond["left"]["table_attribute_idx"];
            }
            BindingAttribute right;
            {
                right.binding = cond["right"]["binding"];
                right.attribute = cond["right"]["attribute"];
                right.fj_attribute_idx = cond["right"]["fj_attribute_idx"];
                right.table_attribute_idx = cond["right"]["table_attribute_idx"];
            }
            equality_conditions.push_back(EqualityCondition{left, right});
        }
        op->equality_conditions = equality_conditions;
        ptr->op = std::move(op);
        ptr->num_relations = left_child->num_relations + right_child->num_relations;
        ptr->children.resize(2);
        // more relations on the left
        ptr->children[0] = std::move(left_child);
        ptr->children[1] = std::move(right_child);
//        if (left_child->num_relations < right_child->num_relations) {
//            ptr->children[0] = std::move(right_child);
//            ptr->children[1] = std::move(left_child);
//        } else {
//            ptr->children[0] = std::move(left_child);
//            ptr->children[1] = std::move(right_child);
//        }
        if (ptr->children[1]->num_relations > 1 || ptr->children[0]->tree_type == BUSHY) {
            ptr->tree_type = BUSHY;
        } else {
            ptr->tree_type = LEFTDEEP;
        }
        return ptr;
    } else if (param["name"] == "scan") {
        auto ptr = std::make_unique<LogicalOperatorTree>();
        auto op = std::make_unique<LogicalGet>();
        op->table_binding.binding = param["binding"];
        ptr->op = std::move(op);
        ptr->tree_type = SINGLETON;
        ptr->num_relations = 1;
        return ptr;
    }
    return nullptr;
}

std::unique_ptr<PhysicalOperator>
Query::optimize(const std::unordered_map<std::string, MaterializedQueryResult *> &data_map) {
    tree->print();
    Optimizer optimizer(data_map);
    hybrid_plan = optimizer.convert_to_hybrid_plan(tree.get());
    hybrid_plan->print();
    auto physical_plan = optimizer.convert_to_physical_plan(hybrid_plan.get());
    return physical_plan;
}
