#pragma once

#include <nil/engine/status.hpp>
#include <nil/engine/types.hpp>
#include <nil/engine/write_batch.hpp>

#include <memory>
#include <vector>

namespace nil {
    namespace dcdb {

        class LogFile;

        typedef std::vector<std::unique_ptr<LogFile>> VectorLogPtr;

        enum WalFileType {
            /* Indicates that WAL file is in archive directory. WAL files are moved from
             * the main db directory to archive directory once they are not live and stay
             * there until cleaned up. Files are cleaned depending on archive size
             * (Options::WAL_size_limit_MB) and time since last cleaning
             * (Options::WAL_ttl_seconds).
             */
                    kArchivedLogFile = 0,

            /* Indicates that WAL file is live and resides in the main db directory */
                    kAliveLogFile = 1
        };

        class LogFile {
        public:
            LogFile() {
            }

            virtual ~LogFile() {
            }

            // Returns log file's pathname relative to the main db dir
            // Eg. For a live-log-file = /000003.log
            //     For an archived-log-file = /archive/000003.log
            virtual std::string PathName() const = 0;


            // Primary identifier for log file.
            // This is directly proportional to creation time of the log file
            virtual uint64_t LogNumber() const = 0;

            // Log file can be either alive or archived
            virtual WalFileType Type() const = 0;

            // Starting sequence number of writebatch written in this log file
            virtual sequence_number StartSequence() const = 0;

            // Size of log file on disk in Bytes
            virtual uint64_t SizeFileBytes() const = 0;
        };

        struct BatchResult {
            sequence_number sequence = 0;
            std::unique_ptr<write_batch> writeBatchPtr;

            // add empty __ctor and __dtor for the rule of five
            // However, preserve the original semantics and prohibit copying
            // as the std::unique_ptr member does not copy.
            BatchResult() {
            }

            ~BatchResult() {
            }

            BatchResult(const BatchResult &) = delete;

            BatchResult &operator=(const BatchResult &) = delete;

            BatchResult(BatchResult &&bResult) : sequence(std::move(bResult.sequence)),
                    writeBatchPtr(std::move(bResult.writeBatchPtr)) {
            }

            BatchResult &operator=(BatchResult &&bResult) {
                sequence = std::move(bResult.sequence);
                writeBatchPtr = std::move(bResult.writeBatchPtr);
                return *this;
            }
        };

// A transaction_log_iterator is used to iterate over the transactions in a db.
// One run of the iterator is continuous, i.e. the iterator will stop at the
// beginning of any gap in sequences
        class transaction_log_iterator {
        public:
            transaction_log_iterator() {
            }

            virtual ~transaction_log_iterator() {
            }

            // An iterator is either positioned at a write_batch or not valid.
            // This method returns true if the iterator is valid.
            // Can read data from a valid iterator.
            virtual bool Valid() = 0;

            // Moves the iterator to the next write_batch.
            // REQUIRES: Valid() to be true.
            virtual void Next() = 0;

            // Returns ok if the iterator is valid.
            // Returns the Error when something has gone wrong.
            virtual status_type status() = 0;

            // If valid return's the current write_batch and the sequence number of the
            // earliest transaction contained in the batch.
            // ONLY use if Valid() is true and status() is OK.
            virtual BatchResult GetBatch() = 0;

            // The read options for transaction_log_iterator.
            struct read_options {
                // If true, all data read from underlying engine will be
                // verified against corresponding checksums.
                // default_environment: true
                bool verify_checksums_;

                read_options() : verify_checksums_(true) {
                }

                explicit read_options(bool verify_checksums) : verify_checksums_(verify_checksums) {
                }
            };
        };
    }
} //  namespace nil
