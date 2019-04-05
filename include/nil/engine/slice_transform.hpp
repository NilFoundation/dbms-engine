// Class for specifying user-defined functions which perform a
// transformation on a slice.  It is not required that every slice
// belong to the domain and/or range of a function.  Subclasses should
// define in_domain and in_range to determine which slices are in either
// of these sets respectively.

#pragma once

#include <string>

namespace nil {
    namespace dcdb {

        class slice;

/*
 * A slice_transform is a generic pluggable way of transforming one string
 * to another. Its primary use-case is in configuring rocksdb
 * to store prefix blooms by setting prefix_extractor in
 * column_family_options.
 */
        class slice_transform {
        public:
            virtual ~slice_transform() {
            };

            // Return the name of this transformation.
            virtual const char *name() const = 0;

            // Extract a prefix from a specified key. This method is called when
            // a key is inserted into the db, and the returned slice is used to
            // create a bloom filter.
            virtual slice transform(const slice &key) const = 0;

            // Determine whether the specified key is compatible with the logic
            // specified in the transform method. This method is invoked for every
            // key that is inserted into the db. If this method returns true,
            // then transform is called to translate the key to its prefix and
            // that returned prefix is inserted into the bloom filter. If this
            // method returns false, then the call to transform is skipped and
            // no prefix is inserted into the bloom filters.
            //
            // For example, if the transform method operates on a fixed length
            // prefix of size 4, then an invocation to in_domain("abc") returns
            // false because the specified key length(3) is shorter than the
            // prefix size of 4.
            //
            // Wiki documentation here:
            // https://github.com/facebook/rocksdb/wiki/Prefix-Seek-API-Changes
            //
            virtual bool in_domain(const slice &key) const = 0;

            // This is currently not used and remains here for backward compatibility.
            virtual bool in_range(const slice &dst) const {
                return false;
            }

            // Some slice_transform will have a full length which can be used to
            // determine if two keys are consecuitive. Can be disabled by always
            // returning 0
            virtual bool full_length_enabled(size_t *len) const {
                return false;
            }

            // Transform(s)=transform(`prefix`) for any s with `prefix` as a prefix.
            //
            // This function is not used by RocksDB, but for users. If users pass
            // opts by string to RocksDB, they might not know what prefix extractor
            // they are using. This function is to help users can determine:
            //   if they want to iterate all keys prefixing `prefix`, whether it is
            //   safe to use prefix bloom filter and seek to key `prefix`.
            // If this function returns true, this means a user can seek() to a prefix
            // using the bloom filter. Otherwise, user needs to skip the bloom filter
            // by setting read_options.total_order_seek = true.
            //
            // Here is an example: Suppose we implement a slice transform that returns
            // the first part of the string after splitting it using delimiter ",":
            // 1. same_result_when_appended("abc,") should return true. If applying prefix
            //    bloom filter using it, all slices matching "abc:.*" will be extracted
            //    to "abc,", so any SST file or memtable containing any of those key
            //    will not be filtered out.
            // 2. same_result_when_appended("abc") should return false. A user will not
            //    guaranteed to see all the keys matching "abc.*" if a user seek to "abc"
            //    against a database with the same setting. If one SST file only contains
            //    "abcd,e", the file can be filtered out and the key will be invisible.
            //
            // i.e., an implementation always returning false is safe.
            virtual bool same_result_when_appended(const slice &prefix) const {
                return false;
            }
        };

        extern const slice_transform *new_fixed_prefix_transform(size_t prefix_len);

        extern const slice_transform *new_capped_prefix_transform(size_t cap_len);

        extern const slice_transform *new_noop_transform();

    }
}