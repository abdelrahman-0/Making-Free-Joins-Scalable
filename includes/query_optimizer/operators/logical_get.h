#pragma once

#include "logical_operator.h"

namespace free_join {
    class LogicalGet : public LogicalOperator {

    private:
        OperatorType type = LOGICAL_GET;
    public:
        TableBinding table_binding;
    public:

        LogicalGet() = default;

        LogicalGet(const LogicalGet &other) : LogicalOperator(other) {
            table_binding = other.table_binding;
        }

        [[nodiscard]] OperatorType get_type() const override { return type; };

    };
}