#pragma once
#ifndef ROCKSDB_LITE

#include <string>

#include <nil/engine/db.hpp>

namespace nil {
    namespace dcdb {

        struct DumpOptions {
            // Database that will be dumped
            std::string db_path;
            // File location that will contain dump output
            std::string dump_location;
            // Don't include db information header in the dump
            bool anonymous = false;
        };

        class DbDumpTool {
        public:
            bool Run(const DumpOptions &dump_options, nil::dcdb::Options options = nil::dcdb::Options());
        };

        struct UndumpOptions {
            // Database that we will load the dumped file into
            std::string db_path;
            // File location of the dumped file that will be loaded
            std::string dump_location;
            // Compact the db after loading the dumped file
            bool compact_db = false;
        };

        class DbUndumpTool {
        public:
            bool Run(const UndumpOptions &undump_options, nil::dcdb::Options options = nil::dcdb::Options());
        };
    }
} // namespace nil
#endif  // ROCKSDB_LITE
