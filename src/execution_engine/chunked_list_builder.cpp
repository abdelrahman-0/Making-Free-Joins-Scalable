#include "execution_engine/chunked_list_builder.h"

using namespace free_join;

std::vector<ChunkedList<size_t, max_chunk_size_idx>>
ChunkedListBuilder::build(const TupleVec &data, std::vector<size_t, MyAllocator> idxs) {
    // parallel for
    auto range = Range(0, data.size());
    tbb::enumerable_thread_specific<std::vector<ChunkedList<size_t, max_chunk_size_idx>>> tls{
            std::vector<ChunkedList<size_t, max_chunk_size_idx>>(num_ptrs)};
    // insert only
    tbb::parallel_for(range,
                      [&](const Range &r) {
                          std::vector<int32_t, tbb::scalable_allocator<int32_t>> tuple;
                          tuple.resize(idxs.size());
                          for (size_t tuple_idx = r.begin(); tuple_idx != r.end(); ++tuple_idx) {
                              const auto &full_tuple = data[tuple_idx];
                              for (auto i = 0u; i < idxs.size(); ++i) {
                                  tuple[i] = full_tuple[idxs[i]].value_.integer;
                              }
                              auto hash = CustomHash{}(tuple);
//                              tls.local()[hash & (num_ptrs - 1)].insert(tuple_idx);
                              tls.local()[hash >> (64ull - power)].insert(tuple_idx);
                          }
                      });
    // combine chunked lists (could be faster to combine them in a tree since they're a power of two)
    auto cl = *tls.begin();
    for (auto it = tls.begin() + 1; it != tls.end(); ++it) {
        for (auto i = 0u; i < num_ptrs; ++i) {
            cl[i].connect((*it)[i]);
        }
    }
    return cl;
}

std::vector<ChunkedList<Tuple, max_chunk_size_tuple>>
ChunkedListBuilder::build_cl(const ChunkedList<Tuple, max_chunk_size_tuple> &data,
                             std::vector<size_t, MyAllocator> idxs) {

    tbb::enumerable_thread_specific<std::vector<ChunkedList<Tuple, max_chunk_size_tuple>>> tls{
            std::vector<ChunkedList<Tuple, max_chunk_size_tuple>>(num_ptrs)};

//    tbb::parallel_for_each(data.cbegin(), data.cend(), [&](const Chunk<Tuple, max_chunk_size_tuple> &chunk) {
//        std::vector<int32_t, tbb::scalable_allocator<int32_t>> tuple(idxs.size());
//        for (auto tuple_idx = 0u; tuple_idx < chunk.size; ++tuple_idx) {
//            const auto &full_tuple = chunk.data[tuple_idx];
//            for (auto i = 0u; i < idxs.size(); ++i) {
//                tuple[i] = full_tuple[idxs[i]].value_.integer;
//            }
//            auto hash = CustomHash{}(tuple);
//            tls.local()[hash >> (64ull - power)].insert(full_tuple);
//        }
//    });

    tbb::task_arena arena;
    std::atomic<Chunk<Tuple, max_chunk_size_tuple> *> control(data.head);
    arena.execute([&]() {
        Chunk<Tuple, max_chunk_size_tuple> *cur = data.head;
        while (true) {
            while (cur && !control.compare_exchange_strong(cur, cur->next));
            if (cur == nullptr) {
                break;
            }
            std::vector<int32_t, tbb::scalable_allocator<int32_t>> tuple(idxs.size());
            for (auto tuple_idx = 0u; tuple_idx < cur->size; ++tuple_idx) {
                const auto &full_tuple = cur->data[tuple_idx];
                for (auto i = 0u; i < idxs.size(); ++i) {
                    tuple[i] = full_tuple[idxs[i]].value_.integer;
                }
                auto hash = CustomHash{}(tuple);
                tls.local()[hash >> (64ull - power)].insert(full_tuple);

            }
        }
    });
    // combine chunked lists (could be faster to combine them in a tree since they're a power of two)
    auto cl = *tls.begin();
    for (auto it = tls.begin() + 1; it != tls.end(); ++it) {
        for (auto i = 0u; i < num_ptrs; ++i) {
            cl[i].connect((*it)[i]);
        }
    }
    return cl;
}

//    std::cout << "PARALLELISM:" << tbb::global_control::active_value(tbb::global_control::max_allowed_parallelism)
//              << std::endl;
