cmake_minimum_required(VERSION 2.8.11)

cmake_policy(SET CMP0042 NEW)
cmake_policy(SET CMP0028 NEW)
cmake_policy(SET CMP0048 OLD)
cmake_policy(SET CMP0057 NEW)
cmake_policy(SET CMP0079 OLD)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake"
     "${CMAKE_CURRENT_LIST_DIR}/cmake/modules/share/modules/cmake")

include(CMConfig)
include(CMSetupVersion)

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

cm_project(${CMAKE_WORKSPACE_NAME} engine)

list(APPEND ${CURRENT_PROJECT_NAME}_PUBLIC_HEADERS
     include/nil/engine/advanced_options.hpp
     include/nil/engine/cache.hpp
     include/nil/engine/cleanable.hpp
     include/nil/engine/compaction_filter.hpp
     include/nil/engine/compaction_job_stats.hpp
     include/nil/engine/comparator.hpp
     include/nil/engine/concurrent_task_limiter.hpp
     include/nil/engine/convenience.hpp
     include/nil/engine/database.hpp
     include/nil/engine/env_encryption.hpp
     include/nil/engine/environment.hpp
     include/nil/engine/experimental.hpp
     include/nil/engine/filter_policy.hpp
     include/nil/engine/flush_block_policy.hpp
     include/nil/engine/iostats_context.hpp
     include/nil/engine/iterator.hpp
     include/nil/engine/listener.hpp
     include/nil/engine/memory_allocator.hpp
     include/nil/engine/memtablerep.hpp
     include/nil/engine/merge_operator.hpp
     include/nil/engine/metadata.hpp
     include/nil/engine/options.hpp
     include/nil/engine/perf_context.hpp
     include/nil/engine/perf_level.hpp
     include/nil/engine/persistent_cache.hpp
     include/nil/engine/rate_limiter.hpp
     include/nil/engine/slice.hpp
     include/nil/engine/slice_transform.hpp
     include/nil/engine/snapshot.hpp
     include/nil/engine/sst_file_manager.hpp
     include/nil/engine/sst_file_reader.hpp
     include/nil/engine/sst_file_writer.hpp
     include/nil/engine/statistics.hpp
     include/nil/engine/stats_history.hpp
     include/nil/engine/status.hpp
     include/nil/engine/table.hpp
     include/nil/engine/table_properties.hpp
     include/nil/engine/thread_status.hpp
     include/nil/engine/threadpool.hpp
     include/nil/engine/trace_reader_writer.hpp
     include/nil/engine/transaction_log.hpp
     include/nil/engine/types.hpp
     include/nil/engine/universal_compaction.hpp
     include/nil/engine/wal_filter.hpp
     include/nil/engine/write_batch.hpp
     include/nil/engine/write_batch_base.hpp
     include/nil/engine/write_buffer_manager.hpp
     )

list(APPEND ${CURRENT_PROJECT_NAME}_UNGROUPED_SOURCES
     src/environment.cpp
     )

list(APPEND ${CURRENT_PROJECT_NAME}_HEADERS
     ${${CURRENT_PROJECT_NAME}_PUBLIC_HEADERS}
     ${${CURRENT_PROJECT_NAME}_UNGROUPED_SOURCES}
     )

list(APPEND ${CURRENT_PROJECT_NAME}_SOURCES
     ${${CURRENT_PROJECT_NAME}_UNGROUPED_SOURCES}
     )

cm_setup_version(VERSION 0.1.0 PREFIX ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME})

add_library(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME}
            ${${CURRENT_PROJECT_NAME}_HEADERS}
            ${${CURRENT_PROJECT_NAME}_SOURCES})

set_target_properties(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PROPERTIES
                      LINKER_LANGUAGE CXX
                      EXPORT_NAME ${CURRENT_PROJECT_NAME}
                      CXX_STANDARD 14)

target_link_libraries(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PRIVATE

                      ${Boost_LIBRARIES})

target_include_directories(${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} PRIVATE
                           "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                           "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>"

                           ${Boost_INCLUDE_DIRS})

cm_deploy(TARGETS ${CMAKE_WORKSPACE_NAME}_${CURRENT_PROJECT_NAME} INCLUDE include NAMESPACE ${CMAKE_WORKSPACE_NAME}::)

add_subdirectory(test)