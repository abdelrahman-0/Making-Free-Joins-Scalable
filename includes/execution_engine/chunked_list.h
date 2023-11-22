#pragma once

#include <iostream>
#include <vector>
#include <array>
#include <list>
#include "utils/custom_hash.h"
#include "libduckdb-src/duckdb.hpp"


namespace free_join {

    static constexpr size_t max_chunk_size_idx = 64;
    static constexpr size_t max_chunk_size_tuple = 10;

    using DataValue = duckdb::Value;
    using Tuple = std::vector<DataValue, tbb::scalable_allocator<DataValue>>;
    using TupleVec = std::vector<Tuple, tbb::scalable_allocator<Tuple>>;


    template<typename S, size_t max_size>
    struct Chunk {
        std::array<S, max_size> data; // stores indexes in base tables
        Chunk<S, max_size> *next = nullptr;
        uint64_t size = 0;

        Chunk() = default;

        ~Chunk() = default;

        [[nodiscard]] bool is_full() const { return size == max_size; }
    };


    template<typename T, size_t max_size>
    class ChunkedList {
    public:
        Chunk<T, max_size> *head = nullptr;
        Chunk<T, max_size> *current = nullptr;
        size_t size = 0;

        inline ChunkedList &operator=(const ChunkedList &o) = default;

        ChunkedList() = default;

        void free_chunked_list() {
            auto ptr = head;
            auto tmp = ptr;
            while (ptr) {
                tmp = ptr->next;
                delete ptr;
                ptr = tmp;
            }
        }

        void insert(const T &val);

        void connect(const ChunkedList<T, max_size> &);

        [[nodiscard]] bool empty() const {
            return size == 0;
        }

        // copied from:
        // https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp
        struct Iterator {
            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Chunk<T, max_size>;
            using pointer = Chunk<T, max_size> *;
            using reference = Chunk<T, max_size> &;

            explicit Iterator(pointer ptr) : m_ptr(ptr) {}

            reference operator*() const { return *m_ptr; }

            pointer operator->() { return m_ptr; }

            Iterator &operator++() {
                m_ptr = m_ptr->next;
                return *this;
            }

            Iterator operator++(int) {
                Iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            friend bool operator==(const Iterator &a, const Iterator &b) { return a.m_ptr == b.m_ptr; };

            friend bool operator!=(const Iterator &a, const Iterator &b) { return a.m_ptr != b.m_ptr; };

        private:
            pointer m_ptr;
        };

        Iterator begin() { return Iterator(head); }

        Iterator end() { return Iterator(nullptr); }

        [[nodiscard]] Iterator cbegin() const { return Iterator(head); }

        [[nodiscard]] Iterator cend() const { return Iterator(nullptr); }
    };

    template
    class ChunkedList<size_t, max_chunk_size_idx>;

    template
    class ChunkedList<Tuple, max_chunk_size_tuple>;

    template
    class Chunk<size_t, max_chunk_size_idx>;

    template
    class Chunk<Tuple, max_chunk_size_tuple>;
}