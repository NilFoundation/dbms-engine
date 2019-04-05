#pragma once

#include <cstdint>

#include <limits>
#include <string>
#include <vector>

#include <nil/engine/types.hpp>

namespace nil {
    namespace dcdb {
        struct column_family_meta_data;
        struct level_meta_data;
        struct sst_file_meta_data;

// The metadata that describes a column family.
        struct column_family_meta_data {
            column_family_meta_data() : size(0), file_count(0), name("") {
            }

            column_family_meta_data(const std::string &_name, uint64_t _size,
                                    const std::vector<level_meta_data> &&_levels) : size(_size), name(_name),
                    levels(_levels) {
            }

            // The size of this column family in bytes, which is equal to the sum of
            // the file size of its "levels".
            uint64_t size;
            // The number of files in this column family.
            size_t file_count;
            // The name of the column family.
            std::string name;
            // The metadata of all levels in this column family.
            std::vector<level_meta_data> levels;
        };

// The metadata that describes a level.
        struct level_meta_data {
            level_meta_data(int _level, uint64_t _size, const std::vector<sst_file_meta_data> &&_files) : level(_level),
                    size(_size), files(_files) {
            }

            // The level which this meta data describes.
            const int level;
            // The size of this level in bytes, which is equal to the sum of
            // the file size of its "files".
            const uint64_t size;
            // The metadata of all sst files in this level.
            const std::vector<sst_file_meta_data> files;
        };

// The metadata that describes a SST file.
        struct sst_file_meta_data {
            sst_file_meta_data() : size(0), name(""), db_path(""), smallest_seqno(0), largest_seqno(0), smallestkey(""),
                    largestkey(""), num_reads_sampled(0), being_compacted(false), num_entries(0), num_deletions(0) {
            }

            sst_file_meta_data(const std::string &_file_name, const std::string &_path, size_t _size,
                            sequence_number _smallest_seqno, sequence_number _largest_seqno,
                            const std::string &_smallestkey, const std::string &_largestkey,
                            uint64_t _num_reads_sampled, bool _being_compacted) : size(_size), name(_file_name),
                    db_path(_path), smallest_seqno(_smallest_seqno), largest_seqno(_largest_seqno),
                    smallestkey(_smallestkey), largestkey(_largestkey), num_reads_sampled(_num_reads_sampled),
                    being_compacted(_being_compacted), num_entries(0), num_deletions(0) {
            }

            // File size in bytes.
            size_t size;
            // The name of the file.
            std::string name;
            // The full path where the file locates.
            std::string db_path;

            sequence_number smallest_seqno;  // Smallest sequence number in file.
            sequence_number largest_seqno;   // Largest sequence number in file.
            std::string smallestkey;     // Smallest user defined key in the file.
            std::string largestkey;      // Largest user defined key in the file.
            uint64_t num_reads_sampled;  // How many times the file is read.
            bool being_compacted;  // true if the file is currently being compacted.

            uint64_t num_entries;
            uint64_t num_deletions;
        };

// The full set of metadata associated with each SST file.
        struct live_file_meta_data : sst_file_meta_data {
            std::string column_family_name;  // name of the column family
            int level;               // Level at which this file resides.
            live_file_meta_data() : column_family_name(), level(0) {
            }
        };
    }
} // namespace nil
