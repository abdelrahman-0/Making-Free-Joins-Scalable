#include "data_loader/database.h"
#include "data_loader/relation.h"
#include <fstream>
#include <iostream>

Database::Database(std::string &data_path, std::string &schema_path) : data_path(data_path),
                                                                       schema_path(schema_path) {
    load_schema();
};

void Database::load_schema() {
    std::ifstream ifs(schema_path);
    raw_schema = json::parse(ifs);
    assert(raw_schema["file_ending"] == "csv");
    assert(raw_schema["format"] == "csv");
    assert(!raw_schema["tables"].empty());
}

void Database::load_relations() {
    for (auto &table: raw_schema["tables"]) {
        std::string relation_name = table["name"];
        std::string csv_path = data_path + relation_name + ".csv";
        auto relation_ptr = std::make_unique<Relation>(relation_name, csv_path, table["columns"]);
        relation_ptr->load_csv_data();
        relations.insert(std::make_pair(relation_name, std::move(relation_ptr)));
        break; // TODO remove
    }
}

const Relation *Database::get_relation(std::string_view relation_name) const {
    if (auto search = relations.find(relation_name); search != relations.end()) {
        return search->second.get();
    }
    return nullptr;
}
