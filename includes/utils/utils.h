#pragma once

#include <string>
#include <filesystem>
#include <fstream>
#include <vector>
#include "nlohmann/json.hpp"

namespace utils {

using json = nlohmann::json;

inline json read_json_from_file(const std::string& file_path) {
    std::ifstream ifs(file_path);
    auto result = json::parse(ifs);
    return result;
}

inline std::string get_current_path() {
    return static_cast<std::string>(std::filesystem::current_path());
}

inline std::vector<std::string> get_directory_contents(const std::string &dir_path) {
    std::vector<std::string> contents;
    for (const auto &entry: std::filesystem::directory_iterator(dir_path)) {
        contents.push_back(entry.path());
    }
    return contents;
}

inline std::string get_base_name(const std::string &path) {
    return path.substr(path.find_last_of('/') + 1);
}

inline std::string remove_extension(const std::string &file) {
    return file.substr(0, file.find_last_of('.'));
}

inline std::string redirect_base_file(const std::string &dir, const std::string &file_path) {
    return dir + get_base_name(file_path);
}

} // namespace utils
