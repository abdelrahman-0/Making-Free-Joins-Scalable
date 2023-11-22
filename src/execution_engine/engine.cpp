#include "execution_engine/engine.h"
#include "utils/PerfEvent.hpp"
//#include "utils/custom_clock.h"

using namespace free_join;

Engine::Engine(PhysicalOperator *root_op) : root_op(root_op) {}

void Engine::execute() {
//    BenchmarkParameters params;
//    params.setParam("name", "total");
//    PerfEvent e;
    auto &output_regs = root_op->get_output();
    root_op->open();
    auto tuple_idx = 1u;
    {
//        CustomClock _("Execution time");
//        PerfEventBlock perf(e, 1, params, false);
        while (root_op->next()) {
#ifndef NDEBUG
            std::cout << tuple_idx++ << ": ";
            for (const auto &val: output_regs) {
                std::cout << val << " ";
            }
            std::cout << std::endl;
#endif
        }
    }
    root_op->close();
}
