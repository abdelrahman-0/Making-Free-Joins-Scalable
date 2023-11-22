#pragma once

#include <vector>
#include <cstdint>
#include "execution_engine/chunked_list.h"
#include "physical_operator.h"

namespace free_join {
    class PhysicalScanFull : public PhysicalOperator {
    private:
        MaterializedQueryResult *data;
        std::vector<uint16_t> physical_idxs;
//        TupleVec tuples;
        ChunkedList<Tuple, max_chunk_size_tuple> tuples;
        Tuple tuple_register;
    public:
        explicit PhysicalScanFull(int16_t src, const std::vector<BindingAttribute> &attributes,
                                  MaterializedQueryResult *data)
                : data(data) {
            src_idx = src;
            out_attributes = attributes;
            is_scan = true;
        };

        void open() override {
            for (const auto &attr: out_attributes) {
                physical_idxs.push_back(attr.table_attribute_idx);
            }
//            tuples.resize(data->RowCount());
            Tuple t;
            for (auto row_idx = 0u; row_idx < data->RowCount(); ++row_idx) {
                for (auto i = 0u; i < physical_idxs.size(); ++i) {
                    t[i] = data->GetValue(physical_idxs[i], row_idx);
                }
                tuples.insert(t);
            }
        };

        bool next() override {
            if (tuples.empty()) {
                return false;
            }
            return true;
        };

        void close() override {

        };

        Tuple &get_output() override {
            return tuple_register;
        }

        ChunkedList <Tuple, max_chunk_size_tuple> *get_tuples() override {
            return &tuples;
        }

        void print() const override {
            std::cout << "SCAN";
        };
    };
}