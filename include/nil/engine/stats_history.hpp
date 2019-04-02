#pragma once

#include <map>
#include <string>

// #include <nil/dcdb/db/db_impl.h>
#include <nil/engine/statistics.hpp>
#include <nil/engine/status.hpp>

namespace nil {
    namespace dcdb {

        class DBImpl;

        class StatsHistoryIterator {
        public:
            StatsHistoryIterator() {
            }

            virtual ~StatsHistoryIterator() {
            }

            virtual bool Valid() const = 0;

            // Moves to the next stats history record.  After this call, Valid() is
            // true iff the iterator was not positioned at the last entry in the source.
            // REQUIRES: Valid()
            virtual void Next() = 0;

            // Return the time stamp (in microseconds) when stats history is recorded.
            // REQUIRES: Valid()
            virtual uint64_t GetStatsTime() const = 0;

            // Return the current stats history as an std::map which specifies the
            // mapping from stats name to stats value . The underlying engine
            // for the returned map is valid only until the next modification of
            // the iterator.
            // REQUIRES: Valid()
            virtual const std::map<std::string, uint64_t> &GetStatsMap() const = 0;

            // If an error has occurred, return it.  Else return an ok status.
            virtual status_type status() const = 0;
        };

    }
} // namespace nil
