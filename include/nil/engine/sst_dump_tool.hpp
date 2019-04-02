#ifndef ROCKSDB_LITE
#pragma once

#include <nil/engine/options.hpp>

namespace nil {
    namespace dcdb {

        class SSTDumpTool {
        public:
            int Run(int argc, char **argv, Options options = Options());
        };

    }
} // namespace nil

#endif  // ROCKSDB_LITE
