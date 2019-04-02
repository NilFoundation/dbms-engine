#pragma once

#include <cstdint>
#include <string>

namespace nil {
    namespace dcdb {

// How much perf stats to collect. Affects perf_context and iostats_context.
        enum PerfLevel : unsigned char {
            kUninitialized = 0,             // unknown setting
            kDisable = 1,                   // disable perf stats
            kEnableCount = 2,               // enable only count stats
            kEnableTimeExceptForMutex = 3,  // Other than count stats, also enable time
            // stats except for mutexes
            // Other than time, also measure CPU time counters. Still don't measure
            // time (neither wall time nor CPU time) for mutexes.
                    kEnableTimeAndCPUTimeExceptForMutex = 4, kEnableTime = 5,  // enable count and time stats
            kOutOfBounds = 6  // N.B. Must always be the last value!
        };

// set the perf stats level for current thread
        void SetPerfLevel(PerfLevel level);

// get current perf stats level for current thread
        PerfLevel GetPerfLevel();

    }
} // namespace nil
