//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef DCDB_SNAPSHOT_CHECKER_HPP
#define DCDB_SNAPSHOT_CHECKER_HPP

#include <nil/storage/engine/types.hpp>

namespace nil {
    namespace engine {
        enum class snapshot_checker_result : int {
            kInSnapshot = 0,
            kNotInSnapshot = 1,    // In case get_snapshot is released and the checker has no clue whether
                                   // the given sequence is visible to the get_snapshot.
            kSnapshotReleased = 2,
        };

        // callback class that control GC of duplicate keys in flush/compaction.
        class snapshot_checker {
        public:
            virtual ~snapshot_checker() {
            }

            virtual snapshot_checker_result check_in_snapshot(engine::sequence_number sequence,
                                                              engine::sequence_number snapshot_sequence) const = 0;
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_SNAPSHOT_CHECKER_HPP
