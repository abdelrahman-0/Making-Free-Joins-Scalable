#pragma once

#include "query_optimizer/operators/logical_operator_tree.h"
#include "hybrid_plan.h"
#include "data_loader/duckdb_manager.h"
#include <vector>
#include <unordered_set>
#include "execution_engine/operators/physical_operator.h"

namespace free_join {

    class Optimizer {
    private:
        std::vector<LogicalOperatorTree *> dependency_stack;
        std::unordered_map<std::string, int16_t> bindings_map;
        std::unordered_map<int16_t, std::map<uint16_t, uint16_t>> physical_info;
        const std::unordered_map<std::string, MaterializedQueryResult *> &data_map;

        void to_hybrid_plan(LogicalOperatorTree *, LogicalOperatorTree *);

        void to_physical_plan(LogicalOperatorTree *logical_tree, std::unique_ptr<PhysicalOperator> &physical_tree,
                              std::vector<BindingAttribute> requested_attr);

    public:
        std::unique_ptr<LogicalOperatorTree> convert_to_hybrid_plan(LogicalOperatorTree *);

        std::unique_ptr<PhysicalOperator> convert_to_physical_plan(LogicalOperatorTree *);

//        void fill_dependency_stack(LogicalOperatorTree *);

//        void recognize_dependencies(LogicalOperatorTree *);


//        void init_physical_info(LogicalOperatorTree *);

//        void print_dependency_stack() const;

        Optimizer() = delete;

        explicit Optimizer(const std::unordered_map<std::string, MaterializedQueryResult *> &);
    };
}