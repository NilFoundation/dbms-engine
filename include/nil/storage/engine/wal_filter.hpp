//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <string>
#include <map>

namespace nil {
    namespace engine {

        class write_batch;

        // WALFilter allows an application to inspect write-ahead-log (WAL)
        // records or modify their processing on recovery.
        // Please see the details below.
        class wal_filter {
        public:
            enum class wal_processing_option {
                // continue_iterating processing as usual
                kContinueProcessing = 0,     // Ignore the current record but continue processing of log(s)
                kIgnoreCurrentRecord = 1,    // Stop replay of logs and discard logs
                                             // Logs won't be replayed on subsequent recovery
                kStopReplay = 2,             // Corrupted record detected by filter
                kCorruptedRecord = 3,        // Marker for enum count
                kWalProcessingOptionMax = 4
            };

            virtual ~wal_filter() {
            }

            // Provide ColumnFamily->log_number map to filter
            // so that filter can determine whether a log number applies to a given
            // column family (i.e. that log hasn't been flushed to SST already for the
            // column family).
            // We also pass in name->id map as only name is known during
            // recovery (as handles are opened post-recovery).
            // while write batch callbacks happen in terms of column family id.
            //
            // @params cf_lognumber_map column_family_id to lognumber map
            // @params cf_name_id_map   column_family_name to column_family_id map

            virtual void column_family_log_number_map(const std::map<uint32_t, uint64_t> &cf_lognumber_map,
                                                      const std::map<std::string, uint32_t> &cf_name_id_map) {
            }

            // log_record is invoked for each log record encountered for all the logs
            // during replay on logs on recovery. This method can be used to:
            //  * inspect the record (using the batch parameter)
            //  * ignoring current record
            //    (by returning wal_processing_option::kIgnoreCurrentRecord)
            //  * reporting corrupted record
            //    (by returning wal_processing_option::kCorruptedRecord)
            //  * stop log replay
            //    (by returning kStop replay) - please note that this implies
            //    discarding the logs from current record onwards.
            //
            // @params log_number     log_number of the current log.
            //                        filter might use this to determine if the log
            //                        record is applicable to a certain column family.
            // @params log_file_name  log file name - only for informational purposes
            // @params batch          batch encountered in the log during recovery
            // @params new_batch      new_batch to populate if filter wants to change
            //                        the batch (for example to filter some records out,
            //                        or alter some records).
            //                        Please note that the new batch MUST NOT contain
            //                        more records than original, else recovery would
            //                        be failed.
            // @params batch_changed  Whether batch was changed by the filter.
            //                        It must be set to true if new_batch was populated,
            //                        else new_batch has no effect.
            // @returns               Processing option for the current record.
            //                        Please see wal_processing_option enum above for
            //                        details.
            virtual wal_processing_option log_record_found(unsigned long long log_number,
                                                           const std::string &log_file_name, const write_batch &batch,
                                                           write_batch *new_batch, bool *batch_changed) {
                // default_environment implementation falls back to older function for compatibility
                return log_record(batch, new_batch, batch_changed);
            }

            // Please see the comments for log_record above. This function is for
            // compatibility only and contains a subset of parameters.
            // New get_code should use the function above.
            virtual wal_processing_option log_record(const write_batch &batch, write_batch *new_batch,
                                                     bool *batch_changed) const {
                return wal_processing_option::kContinueProcessing;
            }

            // Returns a name that identifies this WAL filter.
            // The name will be printed to LOG file on start up for diagnosis.
            virtual const char *name() const = 0;
        };

    }    // namespace engine
}    // namespace nil
