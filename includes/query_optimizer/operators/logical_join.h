#pragma once

#include "logical_operator.h"

namespace free_join {

    class LogicalBinaryJoin : public LogicalOperator {
    private:
        OperatorType type = LOGICAL_BINARY_JOIN;
    public:
        std::vector<EqualityCondition> equality_conditions;
    public:
        LogicalBinaryJoin() = default;

        [[nodiscard]] OperatorType get_type() const override { return type; };
    };

    class LogicalNaryJoin : public LogicalOperator {
    private:
        OperatorType type = LOGICAL_NARY_JOIN;
    public:
        std::vector<EqualityCondition> equality_conditions;
    public:
        LogicalNaryJoin() = default;

        [[nodiscard]] OperatorType get_type() const override { return type; };
    };

}
