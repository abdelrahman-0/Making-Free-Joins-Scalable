#pragma once

#include "iostream"
#include "vector"
#include "query_optimizer/operators/logical_operator.h"
#include "memory"
#include "libduckdb-src/duckdb.hpp"
#include "execution_engine/chunked_list.h"
#include <tbb/scalable_allocator.h>

namespace free_join {

    using TrieSchema = std::vector<std::vector<BindingAttribute,
            tbb::scalable_allocator<BindingAttribute>>, tbb::scalable_allocator<std::vector<BindingAttribute>>>;
    using TrieSchemaMap = std::unordered_map<int16_t, TrieSchema>;

    class PhysicalOperator {
    public:
        std::vector<BindingAttribute> out_attributes;
        int16_t src_idx = -1;
        bool is_scan = false;
    public:
        PhysicalOperator() = default;

        virtual ~PhysicalOperator() = default;

        PhysicalOperator(const PhysicalOperator &) = delete;

        PhysicalOperator(PhysicalOperator &&) = delete;

        PhysicalOperator &operator=(const PhysicalOperator &) = delete;

        PhysicalOperator &operator=(PhysicalOperator &&) = delete;

        virtual void open() = 0;

        virtual bool next() = 0;

        virtual void close() = 0;

        virtual Tuple &get_output() = 0;

        virtual ChunkedList<Tuple, max_chunk_size_tuple> *get_tuples() {
            return nullptr;
        };

        virtual void print() const = 0;
    };

    class PhysicalUnaryOperator : public PhysicalOperator {
    public:
        std::unique_ptr<PhysicalOperator> child;
    };

    class PhysicalBinaryOperator : public PhysicalOperator {
    public:
        std::unique_ptr<PhysicalOperator> left_child;
        std::unique_ptr<PhysicalOperator> right_child;
    };

    class PhysicalNaryOperator : public PhysicalOperator {
    public:
        std::vector<std::unique_ptr<PhysicalOperator>> children;
    };
}
