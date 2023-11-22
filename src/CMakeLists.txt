set(SOURCES
        src/query_optimizer/optimizer.cpp
        src/execution_engine/chunked_list.cpp
        src/execution_engine/chunked_list_builder.cpp
        src/execution_engine/ght_builder.cpp
        src/execution_engine/ght_intersector.cpp
        src/query_optimizer/logical_operator_tree.cpp
        src/query_optimizer/query.cpp
        src/query_optimizer/hybrid_plan.cpp
        src/query_optimizer/free_join_plan.cpp
        src/data_loader/relation.cpp
        src/data_loader/database.cpp
        src/data_loader/duckdb_manager.cpp
        src/query_optimizer/query_handler.cpp
        src/execution_engine/engine.cpp
)

add_library(src_lib SHARED ${SOURCES} ${INCLUDES_DATA_LOADER} ${INCLUDES_EXECUTION_ENGINE} ${INCLUDES_EXTRA} ${INCLUDES_QUERY_OPTIMIZER})
target_include_directories(src_lib PUBLIC includes)