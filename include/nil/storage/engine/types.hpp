//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <cstdint>

#include <nil/storage/engine/slice.hpp>

namespace nil {
    namespace engine {

        // Define all public custom types here.

        // Represents a sequence number in a WAL file.
        typedef uint64_t sequence_number;

        // User-oriented representation of internal key types.
        enum entry_type {
            kEntryPut,
            kEntryDelete,
            kEntrySingleDelete,
            kEntryMerge,
            kEntryRangeDeletion,
            kEntryBlobIndex,
            kEntryOther,
        };

        // <user key, sequence number, and entry type> tuple.
        struct full_key {
            slice user_key;
            sequence_number sequence;
            entry_type type;

            full_key() : sequence(0) {
            }    // Intentionally left uninitialized (for speed)
            full_key(const slice &u, const sequence_number &seq, entry_type t) : user_key(u), sequence(seq), type(t) {
            }

            std::string debug_string(bool hex = false) const;

            void clear() {
                user_key.clear();
                sequence = 0;
                type = entry_type::kEntryPut;
            }
        };
    }    // namespace engine
}    // namespace nil
