//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <string>

namespace nil {
    namespace dcdb {

        class engine::slice;

        // A comparator object provides a total order across slices that are
        // used as keys in an sstable or a database.  A comparator implementation
        // must be thread-safe since rocksdb may invoke its methods concurrently
        // from multiple threads.
        class comparator {
        public:
            virtual ~comparator() {
            }

            // Three-way comparison.  Returns value:
            //   < 0 iff "a" < "b",
            //   == 0 iff "a" == "b",
            //   > 0 iff "a" > "b"
            virtual int compare(const engine::slice &a, const engine::slice &b) const = 0;

            // Compares two slices for equality. The following invariant should always
            // hold (and is the default implementation):
            //   equal(a, b) iff compare(a, b) == 0
            // Overwrite only if equality comparisons can be done more efficiently than
            // three-way comparisons.
            virtual bool equal(const engine::slice &a, const engine::slice &b) const {
                return compare(a, b) == 0;
            }

            // The name of the comparator.  Used to check for comparator
            // mismatches (i.e., a database created with one comparator is
            // accessed using a different comparator.
            //
            // The client of this package should switch to a new name whenever
            // the comparator implementation changes in a way that will cause
            // the relative ordering of any two keys to change.
            //
            // Names starting with "rocksdb." are reserved and should not be used
            // by any clients of this package.
            virtual const char *name() const = 0;

            // Advanced functions: these are used to reduce the space requirements
            // for internal data structures like index blocks.

            // If *start < limit, changes *start to a short string in [start,limit).
            // Simple comparator implementations may return with *start unchanged,
            // i.e., an implementation of this method that does nothing is correct.
            virtual void find_shortest_separator(std::string *start, const engine::slice &limit) const = 0;

            // Changes *key to a short string >= *key.
            // Simple comparator implementations may return with *key unchanged,
            // i.e., an implementation of this method that does nothing is correct.
            virtual void find_short_successor(std::string *key) const = 0;

            // if it is a wrapped comparator, may return the root one.
            // return itself it is not wrapped.
            virtual const comparator *get_root_comparator() const {
                return this;
            }

            // given two keys, determine if t is the successor of s
            virtual bool is_same_length_immediate_successor(const engine::slice &s, const engine::slice &t) const {
                return false;
            }

            // return true if two keys with different byte sequences can be regarded
            // as equal by this comparator.
            // The major use case is to determine if data_block_hash_index is compatible
            // with the customized comparator.
            virtual bool can_keys_with_different_byte_contents_be_equal() const {
                return true;
            }
        };

        // Return a builtin comparator that uses lexicographic byte-wise
        // ordering.  The result remains the property of this module and
        // must not be deleted.
        extern const comparator *bytewise_comparator();

        // Return a builtin comparator that uses reverse lexicographic byte-wise
        // ordering.
        extern const comparator *reverse_bytewise_comparator();

    }    // namespace dcdb
}    // namespace nil
