#pragma once

#include "nlohmann/json.hpp"
#include "database.h"
#include <iostream>
#include <string>

constexpr std::string_view integer_data_type = "integer";
constexpr int32_t null_value = -1;
class Relation {
    using json = nlohmann::json;

private:
    std::string name;
    std::string csv_path;
    json raw_schema;
    uint64_t raw_num_cols = 0;
    std::vector<std::string> schema;
    uint64_t schema_mask = 0;
    uint64_t num_cols = 0;
    std::vector<std::vector<int32_t>> data;

public:
    Relation(std::string &name, std::string &csv_path, json &raw_schema);

    ~Relation() = default;

    void load_csv_data();

    void filter_schema();

    [[nodiscard]] std::vector<std::string> get_schema() const;
};