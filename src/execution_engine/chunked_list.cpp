#include "execution_engine/chunked_list.h"

using namespace free_join;

template<typename T, size_t max_size>
void ChunkedList<T, max_size>::insert(const T &val) {
    if (!current || current->is_full()) {
        auto *new_chunk = new Chunk<T, max_size>();
        if (!current) {
            head = new_chunk;
        } else {
            current->next = new_chunk;
        }
        current = new_chunk;
    }
    current->data[current->size++] = val;
    size++;
}

template<typename T, size_t max_size>
void ChunkedList<T, max_size>::connect(const ChunkedList<T, max_size> &other) {
    if (!other.head) {
        return;
    }
    if (!head) {
        *this = other;
    } else {
        size += other.size;
        current->next = other.head;
        current = other.current;
    }
}
