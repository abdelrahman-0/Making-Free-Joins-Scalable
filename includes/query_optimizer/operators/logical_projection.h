#pragma once

#include "logical_operator.h"
#include "vector"

namespace free_join {
    class LogicalProjection : public LogicalOperator {

    private:
        OperatorType type = LOGICAL_PROJECTION;
    public:
        std::vector<BindingAttribute> projection_attributes;
    public:
        LogicalProjection() = default;

        LogicalProjection(const LogicalProjection& other) : LogicalOperator(other) {
            projection_attributes = other.projection_attributes;
        };

        [[nodiscard]] OperatorType get_type() const override { return type; };

    };

}
