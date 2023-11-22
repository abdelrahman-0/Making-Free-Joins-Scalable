#pragma once

#include "logical_operator.h"

namespace free_join {
    struct FilterPredicate {
        std::string binding;
        std::string condition;
    };

    class LogicalFilter : public LogicalOperator {

    private:
        OperatorType type = LOGICAL_FILTER;
    public:
        std::vector<FilterPredicate> filter_predicates;
    public:

        LogicalFilter() = default;

        [[nodiscard]] OperatorType get_type() const override { return type; };
    };
}