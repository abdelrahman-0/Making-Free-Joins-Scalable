#include "query_optimizer/operators/logical_operator_tree.h"
#include <iostream>
#include <vector>

using namespace free_join;

void LogicalOperatorTree::print() const {
    std::cout << "Logical Operator Tree:\t\t";
    _print(false);
    std::cout << std::endl;
}

void LogicalOperatorTree::_print(bool print_join_conditions) const {
    auto op_type = op->get_type();
    if (op_type == LOGICAL_PROJECTION) {
        std::cout << "π( ";
        children[0]->_print(print_join_conditions);
        std::cout << " )";
    } else if (op_type == LOGICAL_BINARY_JOIN) {
        std::cout << "( ";
        children[0]->_print(print_join_conditions);
        std::cout << " ⨝ ";
        if (print_join_conditions) {
            std::cout << " {";
            for (auto &cond: reinterpret_cast<const LogicalBinaryJoin *>(op.get())->equality_conditions) {
                std::cout << cond.left.binding << "." << cond.left.attribute;
                std::cout << "=";
                std::cout << cond.right.binding << "." << cond.right.attribute << " ";
            }
            std::cout << "} ";
        }
        children[1]->_print(print_join_conditions);
        std::cout << " )";
    } else if (op_type == LOGICAL_NARY_JOIN) {
        std::cout << "FJ( ";
        for(auto& child: children){
            child->_print(print_join_conditions);
            std::cout << " ";
        }
        std::cout << ")";
    } else if (op_type == LOGICAL_GET) {
        auto op_scan = reinterpret_cast<const LogicalGet *>(op.get());
        std::cout << op_scan->table_binding.binding;
    }
}

std::vector<std::string> LogicalOperatorTree::get_table_bindings() const {
    if (tree_type != SINGLETON) {
        auto left_bindings = children[0]->get_table_bindings();
        auto right_bindings = children[1]->get_table_bindings();
        left_bindings.insert(left_bindings.end(), right_bindings.begin(), right_bindings.end());
        return left_bindings;
    } else {
        auto op_scan = reinterpret_cast<LogicalGet *>(op.get());
        return {op_scan->table_binding.binding};
    }
}
