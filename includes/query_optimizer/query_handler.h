#pragma once

#include "data_loader/duckdb_manager.h"
#include "libduckdb-src/duckdb.hpp"

class QueryHandler {
    struct FilterBaseTableSQL {
        uint64_t binding;
        std::string sql_stmt;
    };

private:
    DuckDBManager &db;
    std::string raw_query;
    std::string raw_query_path;
    std::unique_ptr<duckdb::LogicalOperator> plan;
    std::unordered_map<std::string, std::string> bindings_map;
public:
private:
    [[nodiscard]] std::vector<FilterBaseTableSQL> get_filter_queries() const;

    std::vector<FilterBaseTableSQL> get_filter_queries_recursive(duckdb::LogicalOperator *) const;

    [[nodiscard]] FilterBaseTableSQL parse_logical_get_info(duckdb::LogicalOperator *, std::string) const;

    [[nodiscard]] FilterBaseTableSQL parse_logical_filter_info(duckdb::LogicalOperator *, std::string ) const;

public:
    QueryHandler(DuckDBManager &, const std::string &);

    void extract_plan();

    void load_filtered_data() const;
};