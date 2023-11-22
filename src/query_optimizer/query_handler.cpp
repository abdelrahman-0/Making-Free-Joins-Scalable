#include <fstream>
#include <iostream>
#include "query_optimizer/query_handler.h"

//QueryHandler::QueryHandler(DuckDBManager &db, const std::string &raw_query_path) : db(db),
//                                                                                   raw_query_path(raw_query_path) {
//    extract_plan();
//}

//void QueryHandler::extract_plan() {
//    std::ifstream ifs(raw_query_path);
//    std::stringstream buffer;
//    buffer << ifs.rdbuf();
//    raw_query = buffer.str();
//    plan = db.extract_plan(raw_query);
//    std::cout << plan->ToString() << std::endl;
//}
//
//void print_types(std::vector<duckdb::LogicalType> &v) {
//    for (auto &x: v) {
//        std::cout << x.ToString();
//    }
//    std::cout << std::endl;
//}
//
//QueryHandler::FilterBaseTableSQL
//QueryHandler::parse_logical_get_info(duckdb::LogicalOperator *ptr, std::string info) const {
//    auto table_binding_idx = ptr->GetTableIndex()[0];
//    auto sep = info.find('\n');
//    auto table_name = info.substr(0, sep);
//    auto filter_predicates = info.substr(sep + 1);
//    std::string sql_filter_stmt =
//            "SELECT * FROM " + table_name + (filter_predicates.empty() ? "" : " WHERE (" + filter_predicates + ")");
//    return FilterBaseTableSQL{table_binding_idx, sql_filter_stmt};
//}
//
//QueryHandler::FilterBaseTableSQL
//QueryHandler::parse_logical_filter_info(duckdb::LogicalOperator *ptr, std::string info) const {
//    auto logical_get_ptr = ptr->children[0].get();
//    auto table_binding_idx = logical_get_ptr->GetTableIndex()[0];
//    std::string logical_get_info = logical_get_ptr->ParamsToString();
//    auto sep = logical_get_info.find('\n');
//    auto table_name = logical_get_info.substr(0, sep);
//    auto logical_get_filter_predicates = logical_get_info.substr(sep + 1);
//    std::string filter_predicates;
//    if (!logical_get_filter_predicates.empty()) {
//        filter_predicates += "(" + logical_get_filter_predicates + ")";
//        filter_predicates += " AND ";
//    }
//    filter_predicates += "(" + info + ")"; // duckdb always has info for LOGICAL_FILTER nodes
//    std::string sql_filter_stmt =
//            "SELECT * FROM " + table_name + (filter_predicates.empty() ? "" : " WHERE (" + filter_predicates + ")");
//    return FilterBaseTableSQL{table_binding_idx, sql_filter_stmt};
//}
//
//std::vector<QueryHandler::FilterBaseTableSQL>
//QueryHandler::get_filter_queries_recursive(duckdb::LogicalOperator *tree) const {
//    if (tree->type == duckdb::LogicalOperatorType::LOGICAL_GET) {
//        auto logical_get_info = tree->ParamsToString();
//        return {parse_logical_get_info(tree, logical_get_info)};
//    } else if (tree->type == duckdb::LogicalOperatorType::LOGICAL_FILTER) {
//        auto logical_filter_info = tree->ParamsToString();
//        return {parse_logical_filter_info(tree, logical_filter_info)};
//    } else if (tree->type == duckdb::LogicalOperatorType::LOGICAL_COMPARISON_JOIN) {
//        auto &t = tree->Cast<duckdb::LogicalComparisonJoin>();
//        for (auto &cond: t.conditions) {
//            std::cout << cond.left->Cast<duckdb::BoundColumnRefExpression>().binding.table_index << " ("
//                      << cond.left->Cast<duckdb::BoundColumnRefExpression>().alias << " "
//                      << cond.left->Cast<duckdb::BoundColumnRefExpression>().binding.column_index << ")";
//            std::cout << " = ";
//            std::cout << cond.right->Cast<duckdb::BoundColumnRefExpression>().binding.table_index << " ("
//                      << cond.right->Cast<duckdb::BoundColumnRefExpression>().alias << " "
//                      << cond.right->Cast<duckdb::BoundColumnRefExpression>().binding.column_index << "), ";
//        }
//        std::cout << std::endl;
//    }
//    std::vector<FilterBaseTableSQL> filter_queries;
//    for (auto &i: tree->children) {
//        auto child_filter_queries = get_filter_queries_recursive(i.get());
//        filter_queries.insert(filter_queries.end(), child_filter_queries.begin(), child_filter_queries.end());
//    }
//    return filter_queries;
//}
//
//std::vector<QueryHandler::FilterBaseTableSQL> QueryHandler::get_filter_queries() const {
//    return get_filter_queries_recursive(plan.get());
//}
//
//void QueryHandler::load_filtered_data() const {
//    auto filter_queries = get_filter_queries();
//    for (auto q: filter_queries) {
////        auto result = db.execute_query(q);
//        std::cout << q.binding << " : " << q.sql_stmt << std::endl;
//    }
//}