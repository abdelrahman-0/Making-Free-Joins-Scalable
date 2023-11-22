#pragma once

#include "libduckdb-src/duckdb.hpp"
#include "nlohmann/json.hpp"
#include "memory"

using json = nlohmann::json;
using DuckDB = duckdb::DuckDB;
using Connection = duckdb::Connection;
using MaterializedQueryResult = duckdb::MaterializedQueryResult;

class DuckDBManager {
    static constexpr std::string_view parquet_ext = ".parquet";
private:
    std::string json_schema_path;
    std::string sql_schema_path;
    std::vector<std::unique_ptr<MaterializedQueryResult>> data_ptrs;
public:
    DuckDB db;
private:
    [[nodiscard]] std::unique_ptr<MaterializedQueryResult> query_table(Connection &con, std::string &path);
public:
    DuckDBManager() = default;

    ~DuckDBManager() = default;

    void start_duck_db();

    bool setup_db();

    void clear_data_map() {
        data_ptrs.clear();
        data_ptrs.shrink_to_fit();
    };


    [[nodiscard]] std::unordered_map<std::string, MaterializedQueryResult *>
    get_tables(const std::string &parquet_folder_path);
};
