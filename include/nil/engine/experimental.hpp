// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <nil/storage/db.hpp>
#include <nil/storage/status.hpp>

namespace nil {
    namespace dcdb {
        namespace experimental {

// Supported only for Leveled compaction
            status_type SuggestCompactRange(DB *db, column_family_handle *column_family, const slice *begin,
                                            const slice *end);

            status_type SuggestCompactRange(DB *db, const slice *begin, const slice *end);

// Move all L0 files to target_level skipping compaction.
// This operation succeeds only if the files in L0 have disjoint ranges; this
// is guaranteed to happen, for instance, if keys are inserted in sorted
// order. Furthermore, all levels between 1 and target_level must be empty.
// If any of the above condition is violated, InvalidArgument will be
// returned.
            status_type PromoteL0(DB *db, column_family_handle *column_family, int target_level = 1);

        }  // namespace experimental
    }
} // namespace nil
