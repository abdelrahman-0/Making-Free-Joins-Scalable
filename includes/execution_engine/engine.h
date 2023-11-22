#pragma once
#include "execution_engine/operators/physical_operator.h"

namespace free_join {
    class Engine {
    private:
        PhysicalOperator *root_op;
    public:
        Engine() = delete;

        Engine(PhysicalOperator *);

        ~Engine() = default;

        void execute();
    };
}