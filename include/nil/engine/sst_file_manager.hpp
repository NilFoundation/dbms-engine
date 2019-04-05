#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nil/engine/status.hpp>

namespace nil {
    namespace dcdb {

        class environment_type;

        class Logger;

// sst_file_manager is used to track SST files in the database and control their
// deletion rate.
// All sst_file_manager public functions are thread-safe.
// sst_file_manager is not extensible.
        class sst_file_manager {
        public:
            virtual ~sst_file_manager() {
            }

            // update the maximum allowed space that should be used by RocksDB, if
            // the total size of the SST files exceeds max_allowed_space, writes to
            // RocksDB will fail.
            //
            // Setting max_allowed_space to 0 will disable this feature; maximum allowed
            // space will be infinite (default_environment value).
            //
            // thread-safe.
            virtual void set_max_allowed_space_usage(uint64_t max_allowed_space) = 0;

            // Set the amount of buffer room each compaction should be able to leave.
            // In other words, at its maximum disk space consumption, the compaction
            // should still leave compaction_buffer_size available on the disk so that
            // other background functions may continue, such as logging and flushing.
            virtual void set_compaction_buffer_size(uint64_t compaction_buffer_size) = 0;

            // Return true if the total size of SST files exceeded the maximum allowed
            // space usage.
            //
            // thread-safe.
            virtual bool is_max_allowed_space_reached() = 0;

            // Returns true if the total size of SST files as well as estimated size
            // of ongoing compactions exceeds the maximums allowed space usage.
            virtual bool is_max_allowed_space_reached_including_compactions() = 0;

            // Return the total size of all tracked files.
            // thread-safe
            virtual uint64_t get_total_size() = 0;

            // Return a map containing all tracked files and their corresponding sizes.
            // thread-safe
            virtual std::unordered_map<std::string, uint64_t> get_tracked_files() = 0;

            // Return delete rate limit in bytes per second.
            // thread-safe
            virtual int64_t get_delete_rate_bytes_per_second() = 0;

            // update the delete rate limit in bytes per second.
            // zero means disable delete rate limiting and delete files immediately
            // thread-safe
            virtual void set_delete_rate_bytes_per_second(int64_t delete_rate) = 0;

            // Return trash/database size ratio where new files will be deleted immediately
            // thread-safe
            virtual double get_max_trash_db_ratio() = 0;

            // update trash/database size ratio where new files will be deleted immediately
            // thread-safe
            virtual void set_max_trash_db_ratio(double ratio) = 0;

            // Return the total size of trash files
            // thread-safe
            virtual uint64_t get_total_trash_size() = 0;
        };

// Create a new sst_file_manager that can be shared among multiple RocksDB
// instances to track SST file and control there deletion rate.
//
// @param env: Pointer to environment_type object, please see "rocksdb/env.h".
// @param info_log: If not nullptr, info_log will be used to log errors.
//
// == Deletion rate limiting specific arguments ==
// @param trash_dir: Deprecated, this argument have no effect
// @param rate_bytes_per_sec: How many bytes should be deleted per second, If
//    this value is set to 1024 (1 Kb / sec) and we deleted a file of size 4 Kb
//    in 1 second, we will wait for another 3 seconds before we delete other
//    files, Set to 0 to disable deletion rate limiting.
// @param delete_existing_trash: Deprecated, this argument have no effect, but
//    if user provide trash_dir we will schedule deletes for files in the dir
// @param status: If not nullptr, status will contain any errors that happened
//    during creating the missing trash_dir or deleting existing files in trash.
// @param max_trash_db_ratio: If the trash size constitutes for more than this
//    fraction of the total database size we will start deleting new files passed to
//    DeleteScheduler immediately
// @param bytes_max_delete_chunk: if a file to delete is larger than delete
//    chunk, ftruncate the file by this size each time, rather than dropping the
//    whole file. 0 means to always delete the whole file. If the file has more
//    than one linked names, the file will be deleted as a whole. Either way,
//    `rate_bytes_per_sec` will be appreciated. NOTE that with this option,
//    files already renamed as a trash may be partial, so users should not
//    directly recover them without checking.
        extern sst_file_manager *new_sst_file_manager(environment_type *env, std::shared_ptr<Logger> info_log = nullptr,
                                                      string trash_dir = "", int64_t rate_bytes_per_sec = 0,
                                                      bool delete_existing_trash = true, status_type *status = nullptr,
                                                      double max_trash_db_ratio = 0.25,
                                                      uint64_t bytes_max_delete_chunk = 64 * 1024 * 1024);

    }
} // namespace nil