//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <cstddef>

#include <nil/storage/engine/slice.hpp>
#include <nil/storage/engine/status.hpp>

namespace nil {
    namespace engine {
        class column_family_handle;

        // Abstract base class that defines the basic interface for a write batch.
        // See write_batch for a basic implementation and write_batch_with_index for an
        // indexed implementation.
        class write_batch {
        public:
            virtual ~write_batch() {
            }

            // Store the mapping "key->value" in the database.
            virtual status_type insert(column_family_handle *column_family, const slice &key, const engine::slice &value) = 0;

            virtual status_type insert(const slice &key, const slice &value) = 0;

            // Variant of insert() that gathers output like writev(2).  The key and value
            // that will be written to the database are concatenations of arrays of
            // slices.
            // Simple implementation of SlicePart variants of insert().  Child classes
            // can override these method with more performant solutions if they choose.
            virtual status_type insert(column_family_handle *column_family, const slice_parts &key,
                                       const slice_parts &value) {
                std::string key_buf, value_buf;
                slice key_slice(key, &key_buf);
                slice value_slice(value, &value_buf);

                return insert(column_family, key_slice, value_slice);
            }

            virtual status_type insert(const slice_parts &key, const slice_parts &value) {
                std::string key_buf, value_buf;
                slice key_slice(key, &key_buf);
                slice value_slice(value, &value_buf);

                return insert(key_slice, value_slice);
            }

            // merge "value" with the existing value of "key" in the database.
            // "key->merge(existing, value)"
            virtual status_type merge(column_family_handle *column_family, const slice &key, const engine::slice &value) = 0;

            virtual status_type merge(const slice &key, const slice &value) = 0;

            // variant that takes slice_parts
            virtual status_type merge(column_family_handle *column_family, const slice_parts &key,
                                      const slice_parts &value) {
                std::string key_buf, value_buf;
                slice key_slice(key, &key_buf);
                slice value_slice(value, &value_buf);

                return merge(column_family, key_slice, value_slice);
            }

            virtual status_type merge(const slice_parts &key, const slice_parts &value) {
                std::string key_buf, value_buf;
                slice key_slice(key, &key_buf);
                slice value_slice(value, &value_buf);

                return merge(key_slice, value_slice);
            }

            // If the database contains a mapping for "key", erase it.  Else do nothing.
            virtual status_type remove(column_family_handle *column_family, const slice &key) = 0;

            virtual status_type remove(const slice &key) = 0;

            // variant that takes slice_parts
            virtual status_type remove(column_family_handle *column_family, const slice_parts &key) {
                std::string key_buf;
                slice key_slice(key, &key_buf);
                return remove(column_family, key_slice);
            }

            virtual status_type remove(const slice_parts &key) {
                std::string key_buf;
                slice key_slice(key, &key_buf);
                return remove(key_slice);
            }

            // If the database contains a mapping for "key", erase it. Expects that the
            // key was not overwritten. Else do nothing.
            virtual status_type single_remove(column_family_handle *column_family, const slice &key) = 0;

            virtual status_type single_remove(const slice &key) = 0;

            // variant that takes slice_parts
            virtual status_type single_remove(column_family_handle *column_family, const slice_parts &key) {
                std::string key_buf;
                slice key_slice(key, &key_buf);
                return single_remove(column_family, key_slice);
            }

            virtual status_type single_remove(const slice_parts &key) {
                std::string key_buf;
                slice key_slice(key, &key_buf);
                return single_remove(key_slice);
            }

            // If the database contains mappings in the range ["begin_key", "end_key"),
            // erase them. Else do nothing.
            virtual status_type remove_range(column_family_handle *column_family, const slice &begin_key,
                                             const slice &end_key) = 0;

            virtual status_type remove_range(const slice &begin_key, const slice &end_key) = 0;

            // variant that takes slice_parts
            virtual status_type remove_range(column_family_handle *column_family, const slice_parts &begin_key,
                                             const slice_parts &end_key) {
                std::string begin_key_buf, end_key_buf;
                slice begin_key_slice(begin_key, &begin_key_buf);
                slice end_key_slice(end_key, &end_key_buf);
                return remove_range(column_family, begin_key_slice, end_key_slice);
            }

            virtual status_type remove_range(const slice_parts &begin_key, const slice_parts &end_key) {
                std::string begin_key_buf, end_key_buf;
                slice begin_key_slice(begin_key, &begin_key_buf);
                slice end_key_slice(end_key, &end_key_buf);
                return remove_range(begin_key_slice, end_key_slice);
            }

            // append a blob of arbitrary size to the records in this batch. The blob will
            // be stored in the transaction log but not in any other file. In particular,
            // it will not be persisted to the SST files. When iterating over this
            // write_batch, write_batch::handler::log_data will be called with the contents
            // of the blob as it is encountered. Blobs, puts, deletes, and merges will be
            // encountered in the same order in which they were inserted. The blob will
            // NOT consume sequence number(s) and will NOT increase the count of the batch
            //
            // Example application: add timestamps to the transaction log for use in
            // replication.
            virtual status_type put_log_data(const slice &blob) = 0;

            // clear all updates buffered in this batch.
            virtual void clear() = 0;

            // Records the state of the batch for future calls to rollback_to_save_point().
            // May be called multiple times to set multiple save points.
            virtual void set_save_point() = 0;

            // remove all entries in this batch (insert, merge, remove, put_log_data) since the
            // most recent call to set_save_point() and removes the most recent save point.
            // If there is no previous call to set_save_point(), behaves the same as
            // clear().
            virtual status_type rollback_to_save_point() = 0;

            // Pop the most recent save point.
            // If there is no previous call to set_save_point(), status_type::not_found()
            // will be returned.
            // Otherwise returns status_type::ok().
            virtual status_type pop_save_point() = 0;

            // Sets the maximum size of the write batch in bytes. 0 means no limit.
            virtual void set_max_bytes(size_t max_bytes) = 0;
        };
    }    // namespace engine
}    // namespace nil
