#pragma once

#include <vector>
#include <cstdint>
#include "physical_operator.h"

namespace free_join {
    class PhysicalScan : public PhysicalOperator {
    private:
        uint64_t rows_read;
        MaterializedQueryResult *data;
        std::vector<uint16_t> physical_idxs;
        Tuple tuple_register;
    public:
        explicit PhysicalScan(int16_t src, const std::vector<BindingAttribute> &attributes,
                              MaterializedQueryResult *data)
                : data(data), rows_read(0) {
            src_idx = src;
            out_attributes = attributes;
            tuple_register.resize(out_attributes.size());
            is_scan = true;
        };

        void open() override {
            for (const auto &attr: out_attributes) {
                physical_idxs.push_back(attr.table_attribute_idx);
            }
        };

        bool next() override {
            if (rows_read >= data->RowCount()) {
                return false;
            }
            for (auto i = 0u; i < physical_idxs.size(); ++i) {
                tuple_register[i] = data->GetValue(physical_idxs[i], rows_read);
            }
            rows_read++;
            return true;
        };

        void close() override {

        };

        Tuple &get_output() override {
            return tuple_register;
        }

        void print() const override {
            std::cout << "SCAN";
        };
    };
}