#pragma once

#ifndef ROCKSDB_LITE

#include <nil/engine/iterator.hpp>
#include <nil/engine/options.hpp>
#include <nil/engine/slice.hpp>
#include <nil/engine/table_properties.hpp>

namespace nil {
    namespace dcdb {

// SstFileReader is used to read sst files that are generated by database or
// SstFileWriter.
        class SstFileReader {
        public:
            SstFileReader(const Options &options);

            ~SstFileReader();

            // Prepares to read from the file located at "file_path".
            status_type Open(const std::string &file_path);

            // Returns a new iterator over the table contents.
            // Most read options provide the same control as we read from database.
            // If "snapshot" is nullptr, the iterator returns only the latest keys.
            Iterator *NewIterator(const ReadOptions &options);

            std::shared_ptr<const table_properties> GetTableProperties() const;

            // Verifies whether there is corruption in this table.
            status_type VerifyChecksum();

        private:
            struct Rep;
            std::unique_ptr<Rep> rep_;
        };

    }
} // namespace nil

#endif  // !ROCKSDB_LITE
