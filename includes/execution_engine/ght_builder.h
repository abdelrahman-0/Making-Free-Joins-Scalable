#pragma once

#include <vector>
#include "execution_engine/chunked_list_builder.h"
#include "query_optimizer/operators/logical_operator.h"
#include <tsl/hopscotch_map.h>
#include "tsl/bhopscotch_map.h"
#include <tsl/robin_map.h>
#include "ska/flat_hash_map.hpp"
#include "ska/bytell_hash_map.hpp"
#include "utils/Allocator.h"


namespace free_join {

    using FastTuple = std::vector<int32_t, tbb::scalable_allocator<int32_t >>;
    using MyAllocator = tbb::scalable_allocator<size_t>;
    using MyAllocator2 = tbb::scalable_allocator<uint16_t>;
    using Key = std::vector<int32_t, tbb::scalable_allocator<int32_t >>;
    using Value = void *;
//    using PhysicalHashTable = tsl::hopscotch_map<
//            Key,
//            Value,
//            CustomHash,
//            std::equal_to<>,
//            tbb::scalable_allocator<std::pair<Key, Value>>,
//            62,
//            false
//    >;
//    using PhysicalHashTable = std::unordered_map<Key, Value, CustomHash, std::equal_to<>, tbb::scalable_allocator<std::pair<Key, Value>>>;
    using PhysicalHashTable = tsl::robin_map<
            Key,
            Value,
            CustomHash,
            std::equal_to<>,
            tbb::scalable_allocator<std::pair<Key, Value>>,
            true
    >;
//    using PhysicalHashTable = ska::flat_hash_map<Key, Value, CustomHash, std::equal_to<>, tbb::scalable_allocator<std::pair<Key, Value>>>;
//    using PhysicalHashTable = ska::bytell_hash_map<Key, Value, CustomHash, std::equal_to<>, tbb::scalable_allocator<std::pair<Key, Value>>>;

    class GHT {
    public:
        PhysicalHashTable node;
        std::vector<BindingAttribute, tbb::scalable_allocator<BindingAttribute>> schema;
        // TODO optimistic lock coupling
    public:
        GHT() = default;

        explicit GHT(size_t n) : node(n) {};

        ~GHT() {
            for (const auto &[_, ptr]: node) {
                if (uintptr_t(ptr) & (1ul << 63)) {
                    auto *vec_ptr = reinterpret_cast<ChunkedList<Tuple, max_chunk_size_tuple> *>(uintptr_t(ptr) & (-1ul >> 1));
                    delete vec_ptr;
                } else {
                    auto *trie_ptr = reinterpret_cast<GHT *>(ptr);
                    delete trie_ptr; // calls destructor recursively
                }
            }
        }

        void print() const {
            print_trie(0);
        }

    private:
        void print_trie(unsigned level) const {
            std::cout << "Level " << level << ": " << std::endl;
            for (const auto &[vec, tagged_ptr]: node) {

                // print key
                for (const auto &val: vec) {
                    std::cout << val << " ";
                }
                std::cout << std::endl;
                auto untagged_ptr = uintptr_t(tagged_ptr) & (-1ul >> 1);
                if (uintptr_t(tagged_ptr) & (1ul << 63)) {
                    std::cout << "indices: ";
                    for (auto idx: *reinterpret_cast<std::vector<size_t> *>(untagged_ptr)) {
                        std::cout << idx << " ";
                    }
                    std::cout << std::endl;
                } else {
                    reinterpret_cast<GHT *>(untagged_ptr)->print_trie(level + 1);
                }
            }
        }
    };

    class GHTWrapper {

    public:
        GHTWrapper() = default;

        explicit GHTWrapper(size_t size) : mapping(size) {};
    public:
        std::vector<void *, tbb::scalable_allocator<void *>> ghts;
        std::vector<uint16_t, MyAllocator2> mapping;
    };

    class GHTBuilder {
    public:
        static std::vector<void *>
        build(const TupleVec &,
              const std::vector<ChunkedList<size_t, max_chunk_size_idx>> &,
              const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &,
              const TrieSchema &,
              bool = false);

        static GHTWrapper
        build_direct(const TupleVec &,
                     const std::vector<ChunkedList<size_t, max_chunk_size_idx>> &,
                     const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &,
                     const TrieSchema &);

        static GHTWrapper
        build_direct_cl(const std::vector<ChunkedList<Tuple, max_chunk_size_tuple>> &,
                     const std::vector<std::vector<size_t, MyAllocator>, tbb::scalable_allocator<std::vector<size_t>>> &,
                     const TrieSchema &);
    };
}
