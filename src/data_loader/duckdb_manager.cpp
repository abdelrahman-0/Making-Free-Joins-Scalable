#include <fstream>
#include <iostream>
#include "data_loader/duckdb_manager.h"
#include "utils/utils.h"

using json = nlohmann::json;
using DuckDB = duckdb::DuckDB;
using Connection = duckdb::Connection;

void DuckDBManager::start_duck_db() {
    // creates an in-memory database instance
    db = DuckDB(nullptr);
}


bool DuckDBManager::setup_db() {
    Connection con(db);
    auto query_result = con.Query("INSTALL parquet");
    if (query_result->HasError()) {
        std::cerr << query_result->GetError();
        return false;
    }
    return true;
}


std::unique_ptr<MaterializedQueryResult> DuckDBManager::query_table(Connection& con, std::string& path){
    std::string sql_query;
    if (!std::equal(parquet_ext.rbegin(), parquet_ext.rend(), path.rbegin())) {
        std::ifstream ifs(path);
        path.assign((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        path.erase(std::remove(path.begin(), path.end(), '\n'), path.end());
        sql_query = "SELECT * FROM read_csv_auto('" + path + "', ESCAPE='\\')";
    } else {
        sql_query = "SELECT * FROM '" + path + "'";
    }
    return con.Query(sql_query);
}


std::unordered_map<std::string, MaterializedQueryResult *>
DuckDBManager::get_tables(const std::string &parquet_folder_path) {
    std::unordered_map<std::string, MaterializedQueryResult *> data_map;
    Connection con{db};
    con.Query("LOAD parquet");
    for (auto path: utils::get_directory_contents(parquet_folder_path)) {
        auto binding = utils::get_base_name(path);
        binding = binding.substr(0, binding.find('.'));
        auto query_result = query_table(con, path);
        if (query_result->HasError()) {
            std::cerr << "Encounter the following error in DuckDB :"
                      << std::endl
                      << query_result->GetError()
                      << std::endl;
        }
        data_ptrs.push_back(std::move(query_result));
        data_map[binding] = data_ptrs.back().get();
    }
    return data_map;
}