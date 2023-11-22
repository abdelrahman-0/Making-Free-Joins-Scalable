#pragma once

#include "wyhash.h"
#include "komihash.h"
#include <tbb/scalable_allocator.h>

namespace free_join {

    // Generated using:
    //
    // uint64_t secret[4];
    // make_secret(time(nullptr), secret);
    //
    static constexpr uint64_t secret[4] = {7621046655731324189ull, 5676256460979103945ull, 15292402801784477097ull,
                                           3852895885969497237ull};


    struct CustomHash {
        // Effectively implements Boost's hash_combine using WYHash as hash function:
        // https://www.boost.org/doc/libs/1_36_0/boost/functional/hash/hash.hpp
        uint64_t operator()(const std::vector<int32_t, tbb::scalable_allocator<int32_t>> &v) const noexcept {
            uint64_t seed = 0;
            for (auto x: v) {
                seed ^= wyhash(&x, 4, 0, secret) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }
            return seed;
        }

//        uint64_t operator()(const std::vector<int32_t, tbb::scalable_allocator<int32_t>> &v) const noexcept {
//            return komihash(&v[0], v.size() << 2, 0);
//        }


    };
}
