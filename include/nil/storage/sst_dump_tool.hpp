// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
#ifndef ROCKSDB_LITE
#pragma once

#include <nil/dcdb/options.hpp>

namespace nil {
    namespace dcdb {

        class SSTDumpTool {
        public:
            int Run(int argc, char **argv, Options options = Options());
        };

    }
} // namespace nil

#endif  // ROCKSDB_LITE
