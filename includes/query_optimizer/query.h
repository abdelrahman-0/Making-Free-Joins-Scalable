#pragma once

#include "nlohmann/json.hpp"
#include "query_optimizer/operators/logical_operator_tree.h"
#include "operators/logical_operator.h"
#include "libduckdb-src/duckdb.hpp"
#include "optimizer.h"

namespace free_join {
    class Query {
        using json = nlohmann::json;
        using MaterializedQueryResult = duckdb::MaterializedQueryResult;

    private:
        json json_plan;
        std::unique_ptr<LogicalOperatorTree> hybrid_plan;

    public:
        std::unique_ptr<LogicalOperatorTree> tree;

    private:
        [[nodiscard]] std::unique_ptr<LogicalOperatorTree> build_tree(const json &param) const;

        [[nodiscard]] std::unique_ptr<LogicalOperatorTree> json_to_operator_tree() const;

    public:
        explicit Query(json plan);

        [[nodiscard]] std::unique_ptr<PhysicalOperator>
        optimize(const std::unordered_map<std::string, MaterializedQueryResult *> &);
    };
}