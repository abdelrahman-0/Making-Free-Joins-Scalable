#include "execution_engine/ght_builder.h"
#include "utils/PerfEvent.hpp"
#include "utils/Allocator.h"

using namespace free_join;

void insert_into_trie_cl(const Tuple &tuple,
                         const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &physical_schema,
                         const TrieSchema &schema,
                         void *trie,
                         size_t trie_level) {
    auto key = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(physical_schema[trie_level].size());
    auto j = 0u;
    for (const auto &physical_idx: physical_schema[trie_level]) {
        key[j++] = tuple[physical_idx].value_.integer;
    }
    auto &trie_ptr = reinterpret_cast<GHT *>(trie)->node;
    if (trie_level == physical_schema.size() - 1) {
        auto it = trie_ptr.find(key);
        ChunkedList<Tuple, max_chunk_size_tuple> *cl;
        if (it == trie_ptr.end()) {
            cl = new ChunkedList<Tuple, max_chunk_size_tuple>();
            trie_ptr[key] = reinterpret_cast<void *>(uintptr_t(cl) | (1ul << 63));
        } else {
            cl = reinterpret_cast<ChunkedList<Tuple, max_chunk_size_tuple> *> (uintptr_t(it->second) & (-1ul >> 1));
        }
        cl->insert(tuple);
    } else {
        auto *subtrie = reinterpret_cast<GHT *>(trie_ptr[key]);
        if (!subtrie) {
            subtrie = new GHT();
            subtrie->schema = schema[trie_level + 1];
            trie_ptr[key] = reinterpret_cast<void *>(uintptr_t(subtrie) & (-1ul >> 1));
        }
        // tail-call optimized
        insert_into_trie_cl(tuple, physical_schema, schema, subtrie, trie_level + 1);
    }
}

void insert_into_trie(const Tuple &tuple,
                      const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &physical_schema,
                      const TrieSchema &schema,
                      void *trie,
                      size_t materialized_vec_idx,
                      size_t trie_level) {
    auto key = std::vector<int32_t, tbb::scalable_allocator<int32_t >>(physical_schema[trie_level].size());
    auto j = 0u;
    for (const auto &physical_idx: physical_schema[trie_level]) {
        key[j++] = tuple[physical_idx].value_.integer;
    }
    auto &trie_ptr = reinterpret_cast<GHT *>(trie)->node;
    if (trie_level == physical_schema.size() - 1) {
        auto it = trie_ptr.find(key);
        std::vector<size_t, MyAllocator> *vec;
        if (it == trie_ptr.end()) {
            vec = new std::vector<size_t, MyAllocator>();
            trie_ptr[key] = reinterpret_cast<void *>(uintptr_t(vec) | (1ul << 63));
        } else {
            vec = reinterpret_cast<std::vector<size_t, MyAllocator> *> (uintptr_t(it->second) & (-1ul >> 1));
        }
        vec->push_back(materialized_vec_idx);
    } else {
        auto *subtrie = reinterpret_cast<GHT *>(trie_ptr[key]);
        if (!subtrie) {
            subtrie = new GHT();
            subtrie->schema = schema[trie_level + 1];
            trie_ptr[key] = reinterpret_cast<void *>(uintptr_t(subtrie) & (-1ul >> 1));
        }
        // tail-call optimized
        insert_into_trie(tuple, physical_schema, schema, subtrie, materialized_vec_idx, trie_level + 1);
    }
}

std::vector<void *> GHTBuilder::build(const TupleVec &data,
                                      const std::vector<ChunkedList<size_t, max_chunk_size_idx>> &cl_vec,
                                      const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &physical_schema,
                                      const TrieSchema &schema,
                                      bool combine_top_level) {
    auto range = Range(0, cl_vec.size());
    // Set number of threads (comment out next line to use all available cores)
    // Might want to control affinity (to prevent hyper-threading)
//    oneapi::tbb::global_control global_limit(oneapi::tbb::global_control::max_allowed_parallelism, num_threads);
    tbb::enumerable_thread_specific<GHT *> tls;

    tbb::parallel_for(range,
                      [&](const Range &r) {
                          if (!tls.local()) {
                              tls.local() = new GHT();
                              tls.local()->schema = schema[0];
                          }
                          for (size_t idx = r.begin(); idx != r.end(); ++idx) {
                              const auto &cl = cl_vec[idx];
                              auto *tmp = cl.head;
                              auto chunk_local_idx = 0u;
                              while (tmp) {
                                  const auto &tuple_idx = tmp->data[chunk_local_idx++];

                                  insert_into_trie(data[tuple_idx],
                                                   physical_schema,
                                                   schema,
                                                   tls.local(),
                                                   tuple_idx,
                                                   0u);

                                  if (chunk_local_idx == tmp->size) {
                                      chunk_local_idx = 0u;
                                      tmp = tmp->next;
                                  }
                              }
                          }
                      });
    if (combine_top_level) {
        size_t size = 0;
        for (const auto &it: tls) {
            size += it->node.size();
        }
        auto *combined_trie = new GHT(static_cast<size_t>(static_cast<double >(size) * 1.5));
        {
            for (const auto &it: tls) {
                for (const auto &[k, v]: it->node) {
                    (combined_trie->node)[k] = v;
                }
            }
            combined_trie->schema = schema[0];
        }
        return {combined_trie};
    } else {
        return {tls.begin(), tls.end()};
    }
}

void insert_into_trie_direct(FastTuple &mem,
                             const Tuple &tuple,
                             const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &physical_schema,
                             const TrieSchema &schema,
                             void *trie,
                             size_t materialized_vec_idx,
                             size_t trie_level) {
    auto &key = mem;
    for (const auto &physical_idx: physical_schema[trie_level]) {
        key.push_back(tuple[physical_idx].value_.integer);
    }
    auto &trie_ptr = reinterpret_cast<GHT *>(trie)->node;
    if (trie_level == physical_schema.size() - 1) {
        auto it = trie_ptr.find(key);
        std::vector<size_t, MyAllocator> *vec;
        if (it == trie_ptr.end()) {
            vec = new std::vector<size_t, MyAllocator>();
            trie_ptr[key] = reinterpret_cast<void *>(uintptr_t(vec) | (1ul << 63));
        } else {
            vec = reinterpret_cast<std::vector<size_t, MyAllocator> *> (uintptr_t(it->second) & (-1ul >> 1));
        }
        vec->push_back(materialized_vec_idx);
        mem.clear();
    } else {
        auto *subtrie = reinterpret_cast<GHT *>(trie_ptr[key]);
        if (!subtrie) {
            subtrie = new GHT();
            subtrie->schema = schema[trie_level + 1];
            trie_ptr[key] = reinterpret_cast<void *>(uintptr_t(subtrie) & (-1ul >> 1));
        }
        // tail-call optimized
        mem.clear();
        insert_into_trie_direct(mem, tuple, physical_schema, schema, subtrie, materialized_vec_idx, trie_level + 1);
    }
}

GHTWrapper GHTBuilder::build_direct(const TupleVec &data,
                                    const std::vector<ChunkedList<size_t, max_chunk_size_idx>> &cl_vec,
                                    const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &physical_schema,
                                    const TrieSchema &schema) {
    auto range = Range(0, cl_vec.size());
    // Set number of threads (comment out next line to use all available cores)
    // Might want to control affinity (to prevent hyper-threading)
    using enum_tls = tbb::enumerable_thread_specific<std::tuple<GHT *,
            std::vector<uint16_t, MyAllocator2>,
            FastTuple
    >>;
    enum_tls tls;
    std::vector<size_t> mapping(cl_vec.size());

    tbb::parallel_for(range,
                      [&](const Range &r) {
                          if (!std::get<0>(tls.local())) {
                              std::get<0>(tls.local()) = new GHT();
                              std::get<0>(tls.local())->schema = schema[0];
                              size_t max = 0;
                              for (const auto &sch: physical_schema) {
                                  max = std::max(max, sch.size());
                              }
                              std::get<2>(tls.local()).reserve(max);
                          }
                          for (size_t idx = r.begin(); idx != r.end(); ++idx) {
                              // keep track of hash values in local partial-GHT
                              std::get<1>(tls.local()).push_back(idx);
                              const auto &cl = cl_vec[idx];
                              auto *tmp = cl.head;
                              auto chunk_local_idx = 0u;
                              while (tmp) {
                                  const auto &tuple_idx = tmp->data[chunk_local_idx++];

                                  insert_into_trie(
//                                          std::get<2>(tls.local()),
                                          data[tuple_idx],
                                          physical_schema,
                                          schema,
                                          std::get<0>(tls.local()),
                                          tuple_idx,
                                          0u);

                                  if (chunk_local_idx == tmp->size) {
                                      chunk_local_idx = 0u;
                                      tmp = tmp->next;
                                  }
                              }
                          }
                      });
    GHTWrapper result(cl_vec.size());
    for (enum_tls::iterator it = tls.begin();
         it != tls.end(); ++it) {
        for (auto i: std::get<1>(*it)) {
            result.mapping[i] = it - tls.begin();
        }
        result.ghts.push_back(std::get<0>(*it));
    }
    return result;
}

// TODO add to header and implement it using parallel_for_each
GHTWrapper GHTBuilder::build_direct_cl(const std::vector<ChunkedList<Tuple, max_chunk_size_tuple>> &cl_vec,
                                       const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &physical_schema,
                                       const TrieSchema &schema) {
    auto range = Range(0, cl_vec.size());
    // Set number of threads (comment out next line to use all available cores)
    // Might want to control affinity (to prevent hyper-threading)
    using enum_tls = tbb::enumerable_thread_specific<std::tuple<GHT *,
            std::vector<uint16_t, MyAllocator2>
    >>;
    enum_tls tls;
    std::vector<size_t> mapping(cl_vec.size());

    tbb::parallel_for(range,
                      [&](const Range &r) {
                          if (!std::get<0>(tls.local())) {
                              std::get<0>(tls.local()) = new GHT();
                              std::get<0>(tls.local())->schema = schema[0];
                          }
                          for (size_t idx = r.begin(); idx != r.end(); ++idx) {
                              // keep track of hash values in local partial-GHT
                              std::get<1>(tls.local()).push_back(idx);
                              const auto &cl = cl_vec[idx];
                              auto *tmp = cl.head;
                              auto chunk_local_idx = 0u;
                              while (tmp) {
                                  insert_into_trie_cl(
                                          tmp->data[chunk_local_idx++],
                                          physical_schema,
                                          schema,
                                          std::get<0>(tls.local()),
                                          0u);

                                  if (chunk_local_idx == tmp->size) {
                                      chunk_local_idx = 0u;
                                      tmp = tmp->next;
                                  }
                              }
                          }
                      });
    GHTWrapper result(cl_vec.size());
    for (enum_tls::iterator it = tls.begin();
         it != tls.end(); ++it) {
        for (auto i: std::get<1>(*it)) {
            result.mapping[i] = it - tls.begin();
        }
        result.ghts.push_back(std::get<0>(*it));
    }
    return result;
}