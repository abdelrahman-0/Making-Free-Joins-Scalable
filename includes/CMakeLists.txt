set(INCLUDES_DATA_LOADER
        includes/data_loader/database.h
        includes/data_loader/duckdb_manager.h
        includes/data_loader/relation.h
)

set(INCLUDES_EXECUTION_ENGINE
        includes/execution_engine/operators/physical_free_join.h
        includes/execution_engine/operators/physical_free_join_direct.h
        includes/execution_engine/operators/physical_free_join_full.h
        includes/execution_engine/operators/physical_free_join_fullmat.h
        includes/execution_engine/operators/physical_free_join_smart.h
        includes/execution_engine/operators/physical_operator.h
        includes/execution_engine/operators/physical_projection.h
        includes/execution_engine/operators/physical_scan.h
        includes/execution_engine/operators/physical_scan_full.h
        includes/execution_engine/chunked_list.h
        includes/execution_engine/chunked_list_builder.h
        includes/execution_engine/engine.h
        includes/execution_engine/ght_builder.h
        includes/execution_engine/ght_intersector.h
)

set(INCLUDES_EXTRA
        includes/nlohmann/json.hpp
        includes/utils/custom_hash.h
        includes/utils/PerfEvent.hpp
        includes/utils/utils.h
        includes/utils/wyhash.h
        includes/tsl/robin_map.h
)

set(INCLUDES_QUERY_OPTIMIZER
        includes/query_optimizer/operators/logical_filter.h
        includes/query_optimizer/operators/logical_get.h
        includes/query_optimizer/operators/logical_join.h
        includes/query_optimizer/operators/logical_operator.h
        includes/query_optimizer/operators/logical_operator_tree.h
        includes/query_optimizer/operators/logical_projection.h
        includes/query_optimizer/free_join_plan.h
        includes/query_optimizer/hybrid_plan.h
        includes/query_optimizer/optimizer.h
        includes/query_optimizer/query.h
        includes/query_optimizer/query_handler.h
)


set(DUCKDB_SRC includes/libduckdb-src/duckdb.cpp)

add_library(duckdb_src SHARED ${DUCKDB_SRC})