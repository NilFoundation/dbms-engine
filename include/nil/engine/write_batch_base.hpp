// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <cstddef>

namespace nil {
    namespace dcdb {

        class slice;

        class status_type;

        class column_family_handle;

        class WriteBatch;

        struct SliceParts;

// Abstract base class that defines the basic interface for a write batch.
// See WriteBatch for a basic implementation and WrithBatchWithIndex for an
// indexed implementation.
        class WriteBatchBase {
        public:
            virtual ~WriteBatchBase() {
            }

            // Store the mapping "key->value" in the database.
            virtual status_type Put(column_family_handle *column_family, const slice &key, const slice &value) = 0;

            virtual status_type Put(const slice &key, const slice &value) = 0;

            // Variant of Put() that gathers output like writev(2).  The key and value
            // that will be written to the database are concatenations of arrays of
            // slices.
            virtual status_type Put(column_family_handle *column_family, const SliceParts &key,
                                    const SliceParts &value);

            virtual status_type Put(const SliceParts &key, const SliceParts &value);

            // Merge "value" with the existing value of "key" in the database.
            // "key->merge(existing, value)"
            virtual status_type Merge(column_family_handle *column_family, const slice &key, const slice &value) = 0;

            virtual status_type Merge(const slice &key, const slice &value) = 0;

            // variant that takes SliceParts
            virtual status_type Merge(column_family_handle *column_family, const SliceParts &key,
                                      const SliceParts &value);

            virtual status_type Merge(const SliceParts &key, const SliceParts &value);

            // If the database contains a mapping for "key", erase it.  Else do nothing.
            virtual status_type Delete(column_family_handle *column_family, const slice &key) = 0;

            virtual status_type Delete(const slice &key) = 0;

            // variant that takes SliceParts
            virtual status_type Delete(column_family_handle *column_family, const SliceParts &key);

            virtual status_type Delete(const SliceParts &key);

            // If the database contains a mapping for "key", erase it. Expects that the
            // key was not overwritten. Else do nothing.
            virtual status_type SingleDelete(column_family_handle *column_family, const slice &key) = 0;

            virtual status_type SingleDelete(const slice &key) = 0;

            // variant that takes SliceParts
            virtual status_type SingleDelete(column_family_handle *column_family, const SliceParts &key);

            virtual status_type SingleDelete(const SliceParts &key);

            // If the database contains mappings in the range ["begin_key", "end_key"),
            // erase them. Else do nothing.
            virtual status_type DeleteRange(column_family_handle *column_family, const slice &begin_key,
                                            const slice &end_key) = 0;

            virtual status_type DeleteRange(const slice &begin_key, const slice &end_key) = 0;

            // variant that takes SliceParts
            virtual status_type DeleteRange(column_family_handle *column_family, const SliceParts &begin_key,
                                            const SliceParts &end_key);

            virtual status_type DeleteRange(const SliceParts &begin_key, const SliceParts &end_key);

            // Append a blob of arbitrary size to the records in this batch. The blob will
            // be stored in the transaction log but not in any other file. In particular,
            // it will not be persisted to the SST files. When iterating over this
            // WriteBatch, WriteBatch::Handler::LogData will be called with the contents
            // of the blob as it is encountered. Blobs, puts, deletes, and merges will be
            // encountered in the same order in which they were inserted. The blob will
            // NOT consume sequence number(s) and will NOT increase the count of the batch
            //
            // Example application: add timestamps to the transaction log for use in
            // replication.
            virtual status_type PutLogData(const slice &blob) = 0;

            // Clear all updates buffered in this batch.
            virtual void Clear() = 0;

            // Covert this batch into a WriteBatch.  This is an abstracted way of
            // converting any WriteBatchBase(eg WriteBatchWithIndex) into a basic
            // WriteBatch.
            virtual WriteBatch *GetWriteBatch() = 0;

            // Records the state of the batch for future calls to RollbackToSavePoint().
            // May be called multiple times to set multiple save points.
            virtual void SetSavePoint() = 0;

            // remove all entries in this batch (Put, Merge, Delete, PutLogData) since the
            // most recent call to SetSavePoint() and removes the most recent save point.
            // If there is no previous call to SetSavePoint(), behaves the same as
            // Clear().
            virtual status_type RollbackToSavePoint() = 0;

            // Pop the most recent save point.
            // If there is no previous call to SetSavePoint(), status_type::NotFound()
            // will be returned.
            // Otherwise returns status_type::OK().
            virtual status_type PopSavePoint() = 0;

            // Sets the maximum size of the write batch in bytes. 0 means no limit.
            virtual void SetMaxBytes(size_t max_bytes) = 0;
        };

    }
} // namespace nil
