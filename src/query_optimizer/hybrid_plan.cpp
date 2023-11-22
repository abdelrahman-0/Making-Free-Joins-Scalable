#include "query_optimizer/hybrid_plan.h"

using namespace free_join;

HybridPlan::HybridPlan(std::vector<LogicalOperatorTree*> dependency_stack){
    for (auto* i : dependency_stack ) {
        hybrid_plan.push_back(std::make_unique<FreeJoinPlan>(i));
    }
}

void HybridPlan::optimize(){

}