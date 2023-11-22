#pragma once


#include "execution_engine/chunked_list.h"
#include "utils/custom_hash.h"
#include "libduckdb-src/duckdb.hpp"
#include "execution_engine/operators/physical_operator.h"
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_for_each.h>
#include <tbb/global_control.h>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/scalable_allocator.h>
#include <tbb/task_arena.h>

namespace free_join {

    using Range = tbb::blocked_range<uint64_t>;
    using LogicalType = duckdb::LogicalTypeId;

    static auto num_threads = std::thread::hardware_concurrency();
//    static auto num_threads = 1u;

    class ChunkedListBuilder {
    public:
        static constexpr uint64_t power = 5ull;
        static constexpr uint64_t num_ptrs = 1ull << power;
    public:
        using MyAllocator = tbb::scalable_allocator<size_t>;
        static std::vector<ChunkedList<size_t, max_chunk_size_idx>> build(const TupleVec &, std::vector<size_t, MyAllocator>);
        static std::vector<ChunkedList<Tuple, max_chunk_size_tuple>> build_cl(const ChunkedList<Tuple, max_chunk_size_tuple> &, std::vector<size_t, MyAllocator>);
    };
}