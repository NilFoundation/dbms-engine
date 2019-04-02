#pragma once

#ifndef ROCKSDB_LITE

#include <memory>
#include <string>

#include <nil/engine/env.hpp>
#include <nil/engine/options.hpp>
#include <nil/engine/table_properties.hpp>
#include <nil/engine/types.hpp>

#if defined(__GNUC__) || defined(__clang__)
#define ROCKSDB_DEPRECATED_FUNC __attribute__((__deprecated__))
#elif _WIN32
#define ROCKSDB_DEPRECATED_FUNC __declspec(deprecated)
#endif

namespace nil {
    namespace dcdb {

        class comparator;

// external_sst_file_info include information about sst files created
// using SstFileWriter.
        struct external_sst_file_info {
            external_sst_file_info() : file_path(""), smallest_key(""), largest_key(""), smallest_range_del_key(""),
                    largest_range_del_key(""), sequence_number(0), file_size(0), num_entries(0),
                    num_range_del_entries(0), version(0) {
            }

            external_sst_file_info(const std::string &_file_path, const std::string &_smallest_key,
                                const std::string &_largest_key, sequence_number _sequence_number, uint64_t _file_size,
                                int32_t _num_entries, int32_t _version) : file_path(_file_path),
                    smallest_key(_smallest_key), largest_key(_largest_key), smallest_range_del_key(""),
                    largest_range_del_key(""), sequence_number(_sequence_number), file_size(_file_size),
                    num_entries(_num_entries), num_range_del_entries(0), version(_version) {
            }

            std::string file_path;     // external sst file path
            std::string smallest_key;  // smallest user key in file
            std::string largest_key;   // largest user key in file
            std::string smallest_range_del_key;  // smallest range deletion user key in file
            std::string largest_range_del_key;  // largest range deletion user key in file
            sequence_number sequence_number;     // sequence number of all keys in file
            uint64_t file_size;                 // file size in bytes
            uint64_t num_entries;               // number of entries in file
            uint64_t num_range_del_entries;  // number of range deletion entries in file
            int32_t version;                 // file version
        };

// SstFileWriter is used to create sst files that can be added to database later
// All keys in files generated by SstFileWriter will have sequence number = 0.
        class SstFileWriter {
        public:
            // User can pass `column_family` to specify that the generated file will
            // be ingested into this column_family, note that passing nullptr means that
            // the column_family is unknown.
            // If invalidate_page_cache is set to true, SstFileWriter will give the OS a
            // hint that this file pages is not needed every time we write 1MB to the file.
            // To use the rate limiter an io_priority smaller than IO_TOTAL can be passed.
            SstFileWriter(const environment_options &env_options, const Options &options,
                          column_family_handle *column_family = nullptr, bool invalidate_page_cache = true,
                          environment_type::io_priority io_priority = environment_type::io_priority::IO_TOTAL,
                          bool skip_filters = false) : SstFileWriter(env_options, options, options.comparator,
                    column_family, invalidate_page_cache, io_priority, skip_filters) {
            }

            // Deprecated API
            SstFileWriter(const environment_options &env_options, const Options &options,
                          const comparator *user_comparator, column_family_handle *column_family = nullptr,
                          bool invalidate_page_cache = true,
                          environment_type::io_priority io_priority = environment_type::io_priority::IO_TOTAL,
                          bool skip_filters = false);

            ~SstFileWriter();

            // Prepare SstFileWriter to write into file located at "file_path".
            status_type Open(const std::string &file_path);

            // add a insert key with value to currently opened file (deprecated)
            // REQUIRES: key is after any previously added key according to comparator.
            ROCKSDB_DEPRECATED_FUNC status_type Add(const slice &user_key, const slice &value);

            // add a insert key with value to currently opened file
            // REQUIRES: key is after any previously added key according to comparator.
            status_type Put(const slice &user_key, const slice &value);

            // add a merge key with value to currently opened file
            // REQUIRES: key is after any previously added key according to comparator.
            status_type Merge(const slice &user_key, const slice &value);

            // add a deletion key to currently opened file
            // REQUIRES: key is after any previously added key according to comparator.
            status_type Delete(const slice &user_key);

            // add a range deletion tombstone to currently opened file
            status_type DeleteRange(const slice &begin_key, const slice &end_key);

            // Finalize writing to sst file and close file.
            //
            // An optional external_sst_file_info pointer can be passed to the function
            // which will be populated with information about the created sst file.
            status_type Finish(external_sst_file_info *file_info = nullptr);

            // Return the current file size.
            uint64_t FileSize();

        private:
            void InvalidatePageCache(bool closing);

            struct Rep;
            std::unique_ptr<Rep> rep_;
        };
    }
} // namespace nil

#endif  // !ROCKSDB_LITE
