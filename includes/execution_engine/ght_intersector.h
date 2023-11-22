#pragma once

#include "query_optimizer/free_join_plan.h"
#include "execution_engine/chunked_list_builder.h"
#include "ght_builder.h"

namespace free_join {
    class GHTIntersector {
    public:
//        static std::vector<TupleVec *> intersect(int16_t leftmost_src_idx,
//                                                 std::unordered_map<int16_t, TupleVec> &,
//                                                 const std::vector<size_t> &,
//                                                 const std::map<int16_t, void *> &,
//                                                 const FreeJoinPlan &,
//                                                 size_t,
//                                                 std::unordered_map<int16_t, std::vector<size_t >> &);

        static std::vector<TupleVec *> intersect(int16_t leftmost_src_idx,
                                                 std::unordered_map<int16_t, TupleVec> &,
                                                 const std::vector<size_t, MyAllocator> &,
                                                 const std::map<int16_t, std::vector<void *>> &,
                                                 const FreeJoinPlan &,
                                                 size_t,
                                                 std::unordered_map<int16_t, std::vector<size_t >> &);

        static std::vector<TupleVec *> intersect_direct(int16_t leftmost_src_idx,
                                                        std::unordered_map<int16_t, TupleVec> &,
                                                        const std::vector<size_t, MyAllocator> &,
                                                        std::map<int16_t, GHTWrapper> &,
                                                        const FreeJoinPlan &,
                                                        size_t,
                                                        std::unordered_map<int16_t, std::vector<size_t >> &);

        static ChunkedList<Tuple, max_chunk_size_tuple> intersect_direct_cl(int16_t leftmost_src_idx,
                                                                            std::unordered_map<int16_t, ChunkedList<Tuple, max_chunk_size_tuple> *> &,
                                                                            const std::vector<size_t, MyAllocator> &,
                                                                            std::map<int16_t, GHTWrapper> &,
                                                                            const FreeJoinPlan &,
                                                                            size_t,
                                                                            std::unordered_map<int16_t, std::vector<size_t >> &);

        static std::vector<TupleVec *> intersect_direct_vectorized(int16_t leftmost_src_idx,
                                                                   std::unordered_map<int16_t, TupleVec> &,
                                                                   const std::vector<size_t, MyAllocator> &,
                                                                   std::map<int16_t, GHTWrapper> &,
                                                                   const FreeJoinPlan &,
                                                                   size_t,
                                                                   std::unordered_map<int16_t, std::vector<size_t >> &);
    };
};
