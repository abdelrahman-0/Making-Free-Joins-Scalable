#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "physical_operator.h"
#include "query_optimizer/operators/logical_operator.h"

namespace free_join {
    class PhysicalProjection : public PhysicalUnaryOperator {
    public:
        explicit PhysicalProjection(int16_t src) {
            src_idx = src;
        };

        void open() override {
            child->open();
        };

        bool next() override {
            return child->next();
        };

        void close() override {
            child->close();
        };

        Tuple &get_output() override {
            return child->get_output();
        }

        void print() const override {
            std::cout << "PROJ( ";
            child->print();
            std::cout << ")";
        };
    };
}