#pragma once

#include <string>
#include "relation.h"
#include "database.h"
#include "nlohmann/json.hpp"

class Relation;

class Database {
    using json = nlohmann::json;
private:
    std::string data_path;
    std::string schema_path;
    json raw_schema;
    std::unordered_map<std::string_view, std::unique_ptr<Relation>> relations;
private:
    void load_schema();

public:
    Database(std::string &data_path, std::string &schema_path);

    ~Database() = default;

    void load_relations();
    const Relation* get_relation(std::string_view relation_name) const;

};