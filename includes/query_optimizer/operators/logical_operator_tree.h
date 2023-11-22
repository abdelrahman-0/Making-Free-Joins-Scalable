#pragma once

#include <memory>
#include <vector>
#include <string>
#include "query_optimizer/operators/logical_operator.h"
#include "query_optimizer/operators/logical_join.h"
#include "query_optimizer/operators/logical_get.h"
#include "query_optimizer/operators/logical_filter.h"
#include "query_optimizer/operators/logical_projection.h"

namespace free_join {
    enum TreeType {
        SINGLETON,
        LEFTDEEP,
        BUSHY
    };

    static int16_t global_src_idx;

    class LogicalOperatorTree {
    private:
        void _print(bool print_join_conds = false) const;

    public:
        std::unique_ptr<LogicalOperator> op;
        uint64_t num_relations = 0;
        TreeType tree_type = SINGLETON;
        int16_t src_idx = -1;
        std::vector<std::unique_ptr<LogicalOperatorTree>> children;
        std::vector<std::string> available_bindings;
        bool is_free_join = false;

        LogicalOperatorTree() = default;

        [[nodiscard]] std::vector<std::string> get_table_bindings() const;

        void print() const;
    };
}