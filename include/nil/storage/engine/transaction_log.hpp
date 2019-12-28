//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <nil/storage/engine/write_batch.hpp>
#include <nil/storage/engine/status.hpp>
#include <nil/storage/engine/types.hpp>

#include <memory>
#include <vector>

namespace nil {
    namespace engine {

        class log_file;

        typedef std::vector<std::unique_ptr<log_file>> log_ptr_vector;

        enum wal_file_type {
            /* Indicates that WAL file is in archive directory. WAL files are moved from
             * the main db directory to archive directory once they are not live and stay
             * there until cleaned up. Files are cleaned depending on archive size
             * (opts::WAL_size_limit_MB) and time since last cleaning
             * (opts::WAL_ttl_seconds).
             */
            kArchivedLogFile = 0,

            /* Indicates that WAL file is live and resides in the main db directory */
            kAliveLogFile = 1
        };

        class log_file {
        public:
            log_file() {
            }

            virtual ~log_file() {
            }

            // Returns log file's pathname relative to the main db dir
            // Eg. For a live-log-file = /000003.log
            //     For an archived-log-file = /archive/000003.log
            virtual std::string path_name() const = 0;

            // Primary identifier for log file.
            // This is directly proportional to creation time of the log file
            virtual uint64_t log_number() const = 0;

            // Log file can be either alive or archived
            virtual wal_file_type type() const = 0;

            // Starting sequence number of writebatch written in this log file
            virtual engine::sequence_number start_sequence() const = 0;

            // Size of log file on disk in Bytes
            virtual uint64_t size_file_bytes() const = 0;
        };

        struct batch_result {
            engine::sequence_number sequence = 0;
            std::unique_ptr<write_batch> write_batch_ptr;

            // add empty __ctor and __dtor for the rule of five
            // However, preserve the original semantics and prohibit copying
            // as the std::unique_ptr member does not copy.
            batch_result() {
            }

            ~batch_result() {
            }

            batch_result(const batch_result &) = delete;

            batch_result &operator=(const batch_result &) = delete;

            batch_result(batch_result &&bResult) :
                sequence(bResult.sequence), write_batch_ptr(std::move(bResult.write_batch_ptr)) {
            }

            batch_result &operator=(batch_result &&bResult) {
                sequence = bResult.sequence;
                write_batch_ptr = std::move(bResult.write_batch_ptr);
                return *this;
            }
        };

        // A transaction_log_iterator is used to iterate over the transactions in a db.
        // One run of the iterator is continuous, i.e. the iterator will stop at the
        // beginning of any gap in sequences
        class transaction_log_iterator {
        public:
            transaction_log_iterator() = default;

            virtual ~transaction_log_iterator() = default;

            // An iterator is either positioned at a write_batch or not valid.
            // This method returns true if the iterator is valid.
            // Can read data from a valid iterator.
            virtual bool valid() = 0;

            // Moves the iterator to the next write_batch.
            // REQUIRES: valid() to be true.
            virtual void next() = 0;

            // Returns is_ok if the iterator is valid.
            // Returns the Error when something has gone wrong.
            virtual status_type status() = 0;

            // If valid return's the current write_batch and the sequence number of the
            // earliest transaction contained in the batch.
            // ONLY use if valid() is true and status() is is_ok.
            virtual batch_result get_batch() = 0;

            // The read opts for transaction_log_iterator.
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
    }    // namespace engine
}    //  namespace nil
