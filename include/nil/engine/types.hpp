#pragma once

#include <cstdint>

#include <nil/engine/slice.hpp>

namespace nil {
    namespace dcdb {

// Define all public custom types here.

// Represents a sequence number in a WAL file.
        typedef uint64_t sequence_number;

// User-oriented representation of internal key types.
        enum EntryType {
            kEntryPut, kEntryDelete, kEntrySingleDelete, kEntryMerge, kEntryRangeDeletion, kEntryBlobIndex, kEntryOther,
        };

// <user key, sequence number, and entry type> tuple.
        struct FullKey {
            slice user_key;
            sequence_number sequence;
            EntryType type;

            FullKey() : sequence(0) {
            }  // Intentionally left uninitialized (for speed)
            FullKey(const slice &u, const sequence_number &seq, EntryType t) : user_key(u), sequence(seq), type(t) {
            }

            std::string DebugString(bool hex = false) const;

            void clear() {
                user_key.clear();
                sequence = 0;
                type = EntryType::kEntryPut;
            }
        };

// Parse slice representing internal key to FullKey
// Parsed FullKey is valid for as long as the memory pointed to by
// internal_key is alive.
        bool ParseFullKey(const slice &internal_key, FullKey *result);

    }
} // namespace nil
