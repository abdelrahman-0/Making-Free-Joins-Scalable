#pragma once

#include <chrono>
#include <iostream>

namespace free_join {
    class CustomClock {
    private:
        std::string header;
        std::chrono::steady_clock::time_point begin;
    public:

        explicit CustomClock(std::string header = "Time taken") : header(std::move(header)) {
            begin = std::chrono::steady_clock::now();
        }

        ~CustomClock() {
            auto end = std::chrono::steady_clock::now();
            std::cout << header
                      << ":\t"
                      << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count()
                      << "[ms]" << std::endl;
        }
    };
}