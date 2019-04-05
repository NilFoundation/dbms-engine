#pragma once

#include <nil/engine/db.hpp>
#include <nil/engine/status.hpp>

namespace nil {
    namespace dcdb {
        namespace experimental {

// Supported only for Leveled compaction
            status_type suggest_compact_range(database *db, column_family_handle *column_family, const slice *begin,
                                              const slice *end);

            status_type suggest_compact_range(database *db, const slice *begin, const slice *end);

// Move all L0 files to target_level skipping compaction.
// This operation succeeds only if the files in L0 have disjoint ranges; this
// is guaranteed to happen, for instance, if keys are inserted in sorted
// order. Furthermore, all levels between 1 and target_level must be empty.
// If any of the above condition is violated, invalid_argument will be
// returned.
            status_type promote_l0(database *db, column_family_handle *column_family, int target_level = 1);

        }  // namespace experimental
    }
} // namespace nil
