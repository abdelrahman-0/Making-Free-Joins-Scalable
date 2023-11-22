#pragma once

#include "query_optimizer/operators/logical_operator_tree.h"
#include "query_optimizer/free_join_plan.h"

namespace free_join {
    class HybridPlan {
    private:
        std::vector<std::unique_ptr<FreeJoinPlan>> hybrid_plan;
    public:
        HybridPlan() = default;

        ~HybridPlan() = default;

        explicit HybridPlan(std::vector<LogicalOperatorTree *> dependency_stack);

        void optimize();
    };
}