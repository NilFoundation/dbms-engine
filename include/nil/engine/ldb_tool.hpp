#pragma once
#ifndef ROCKSDB_LITE

#include <string>
#include <vector>

#include <nil/engine/db.hpp>
#include <nil/engine/options.hpp>

namespace nil {
    namespace dcdb {

// An interface for converting a slice to a readable string
        class SliceFormatter {
        public:
            virtual ~SliceFormatter() {
            }

            virtual std::string Format(const slice &s) const = 0;
        };

// Options for customizing ldb tool (beyond the DB Options)
        struct LDBOptions {
            // Create LDBOptions with default values for all fields
            LDBOptions();

            // Key formatter that converts a slice to a readable string.
            // Default: slice::ToString()
            std::shared_ptr<SliceFormatter> key_formatter;

            std::string print_help_header = "ldb - RocksDB Tool";
        };

        class LDBTool {
        public:
            void Run(int argc, char **argv, Options db_options = Options(),
                     const LDBOptions &ldb_options = LDBOptions(),
                     const std::vector<ColumnFamilyDescriptor> *column_families = nullptr);
        };

    }
} // namespace nil

#endif  // ROCKSDB_LITE
