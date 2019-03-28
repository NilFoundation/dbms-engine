cmake_minimum_required(VERSION 2.8.11)

cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0079 OLD)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/share/modules/cmake")

if(UNIX)
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)
endif()

include(CMConfig)
include(CMSetupVersion)

include(CheckCSourceCompiles)
include(CheckCSourceRuns)

cm_workspace(nil)

macro(cm_find_package NAME)
    if(NOT "${NAME}" MATCHES "^${CMAKE_WORKSPACE_NAME}_.*$" AND NOT "${NAME}" STREQUAL CM)
        find_package(${ARGV})
    else()
        set(${ARGV0}_FOUND ON CACHE BOOL "")
    endif()
endmacro()

cm_find_package(CM)
include(CMDeploy)
include(CMSetupVersion)

cm_project(${CMAKE_WORKSPACE_NAME} storage)

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_HEADERS
     include/nil/storage/advanced_options.hpp
     include/nil/storage/cache.hpp
     include/nil/storage/cleanable.hpp
     include/nil/storage/compaction_filter.hpp
     include/nil/storage/compaction_job_stats.hpp
     include/nil/storage/comparator.hpp
     include/nil/storage/concurrent_task_limiter.hpp
     include/nil/storage/convenience.hpp
     include/nil/storage/db.hpp
     include/nil/storage/db_bench_tool.hpp
     include/nil/storage/db_dump_tool.hpp
     include/nil/storage/env.hpp
     include/nil/storage/env_encryption.hpp
     include/nil/storage/experimental.hpp
     include/nil/storage/filter_policy.hpp
     include/nil/storage/flush_block_policy.hpp
     include/nil/storage/iostats_context.hpp
     include/nil/storage/iterator.hpp
     include/nil/storage/ldb_tool.hpp
     include/nil/storage/listener.hpp
     include/nil/storage/memory_allocator.hpp
     include/nil/storage/memtablerep.hpp
     include/nil/storage/merge_operator.hpp
     include/nil/storage/metadata.hpp
     include/nil/storage/options.hpp
     include/nil/storage/perf_context.hpp
     include/nil/storage/perf_level.hpp
     include/nil/storage/persistent_cache.hpp
     include/nil/storage/rate_limiter.hpp
     include/nil/storage/slice.hpp
     include/nil/storage/slice_transform.hpp
     include/nil/storage/snapshot.hpp
     include/nil/storage/sst_dump_tool.hpp
     include/nil/storage/sst_file_manager.hpp
     include/nil/storage/sst_file_reader.hpp
     include/nil/storage/sst_file_writer.hpp
     include/nil/storage/statistics.hpp
     include/nil/storage/stats_history.hpp
     include/nil/storage/status.hpp
     include/nil/storage/table.hpp
     include/nil/storage/table_properties.hpp
     include/nil/storage/thread_status.hpp
     include/nil/storage/threadpool.hpp
     include/nil/storage/trace_reader_writer.hpp
     include/nil/storage/transaction_log.hpp
     include/nil/storage/types.hpp
     include/nil/storage/universal_compaction.hpp
     include/nil/storage/wal_filter.hpp
     include/nil/storage/write_batch.hpp
     include/nil/storage/write_batch_base.hpp
     include/nil/storage/write_buffer_manager.hpp
     )

list(APPEND ${CURRENT_PROJECT_NAME}_UNGROUPED_SOURCES

     )

list(APPEND ${CURRENT_PROJECT_NAME}_HEADERS
     ${${CURRENT_PROJECT_NAME}_PUBLIC_HEADERS}
     ${${CURRENT_PROJECT_NAME}_PRIVATE_HEADERS}
     )

list(APPEND ${CURRENT_PROJECT_NAME}_SOURCES
     ${${CURRENT_PROJECT_NAME}_UNGROUPED_SOURCES}
     )

cm_setup_version(VERSION 0.1.0 PREFIX ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME})

add_library(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} ${${CURRENT_PROJECT_NAME}_HEADERS} ${${CURRENT_PROJECT_NAME}_SOURCES})

set_target_properties(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PROPERTIES
                      LINKER_LANGUAGE CXX
                      EXPORT_NAME ${CURRENT_PROJECT_NAME}
                      CXX_STANDARD 14)

#target_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PRIVATE mtl::core)

target_include_directories(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PRIVATE
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/public>"
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/private>"
                           "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>")

cm_deploy(TARGETS ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} INCLUDE include/public include/private NAMESPACE ${CMAKE_WORKSPACE_NAME}::)

if(APPLE OR NOT ${CMAKE_TARGET_ARCHITECTURE} STREQUAL ${CMAKE_HOST_SYSTEM_PROCESSOR})
    set_target_properties(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PROPERTIES
                          XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${APPLE_CODE_SIGN_IDENTITY}"
                          XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM}")
endif()

add_subdirectory(test)