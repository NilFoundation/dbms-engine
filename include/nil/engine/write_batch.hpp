// write_batch holds a collection of updates to apply atomically to a database.
//
// The updates are applied in the order in which they are added
// to the write_batch.  For example, the value of "key" will be "v3"
// after the following batch is written:
//
//    batch.insert("key", "v1");
//    batch.remove("key");
//    batch.insert("key", "v2");
//    batch.insert("key", "v3");
//
// Multiple threads can invoke const methods on a write_batch without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same write_batch must use
// external synchronization.

#pragma once

#include <atomic>
#include <stack>
#include <string>
#include <cstdint>

#include <nil/engine/status.hpp>
#include <nil/engine/write_batch_base.hpp>

namespace nil {
    namespace dcdb {

        class slice;

        class column_family_handle;

        struct SavePoints;
        struct slice_parts;

        struct SavePoint {
            size_t size;  // size of rep_
            int count;    // count of elements in rep_
            uint32_t content_flags;

            SavePoint() : size(0), count(0), content_flags(0) {
            }

            SavePoint(size_t _size, int _count, uint32_t _flags) : size(_size), count(_count), content_flags(_flags) {
            }

            void clear() {
                size = 0;
                count = 0;
                content_flags = 0;
            }

            bool is_cleared() const {
                return (size | count | content_flags) == 0;
            }
        };

        class write_batch : public write_batch_base {
        public:
            explicit write_batch(size_t reserved_bytes = 0, size_t max_bytes = 0);

            ~write_batch() override;

            using write_batch_base::insert;

            // Store the mapping "key->value" in the database.
            status_type insert(column_family_handle *column_family, const slice &key, const slice &value) override;

            status_type insert(const slice &key, const slice &value) override {
                return insert(nullptr, key, value);
            }

            // Variant of insert() that gathers output like writev(2).  The key and value
            // that will be written to the database are concatenations of arrays of
            // slices.
            status_type insert(column_family_handle *column_family, const slice_parts &key, const slice_parts &value) override;

            status_type insert(const slice_parts &key, const slice_parts &value) override {
                return insert(nullptr, key, value);
            }

            using write_batch_base::remove;

            // If the database contains a mapping for "key", erase it.  Else do nothing.
            status_type remove(column_family_handle *column_family, const slice &key) override;

            status_type remove(const slice &key) override {
                return remove(nullptr, key);
            }

            // variant that takes slice_parts
            status_type remove(column_family_handle *column_family, const slice_parts &key) override;

            status_type remove(const slice_parts &key) override {
                return remove(nullptr, key);
            }

            using write_batch_base::single_remove;

            // write_batch implementation of database::single_remove().  See db.h.
            status_type single_remove(column_family_handle *column_family, const slice &key) override;

            status_type single_remove(const slice &key) override {
                return single_remove(nullptr, key);
            }

            // variant that takes slice_parts
            status_type single_remove(column_family_handle *column_family, const slice_parts &key) override;

            status_type single_remove(const slice_parts &key) override {
                return single_remove(nullptr, key);
            }

            using write_batch_base::remove_range;

            // write_batch implementation of database::remove_range().  See db.h.
            status_type remove_range(column_family_handle *column_family, const slice &begin_key,
                                     const slice &end_key) override;

            status_type remove_range(const slice &begin_key, const slice &end_key) override {
                return remove_range(nullptr, begin_key, end_key);
            }

            // variant that takes slice_parts
            status_type remove_range(column_family_handle *column_family, const slice_parts &begin_key,
                                     const slice_parts &end_key) override;

            status_type remove_range(const slice_parts &begin_key, const slice_parts &end_key) override {
                return remove_range(nullptr, begin_key, end_key);
            }

            using write_batch_base::merge;

            // merge "value" with the existing value of "key" in the database.
            // "key->merge(existing, value)"
            status_type merge(column_family_handle *column_family, const slice &key, const slice &value) override;

            status_type merge(const slice &key, const slice &value) override {
                return merge(nullptr, key, value);
            }

            // variant that takes slice_parts
            status_type merge(column_family_handle *column_family, const slice_parts &key,
                              const slice_parts &value) override;

            status_type merge(const slice_parts &key, const slice_parts &value) override {
                return merge(nullptr, key, value);
            }

            using write_batch_base::put_log_data;

            // append a blob of arbitrary size to the records in this batch. The blob will
            // be stored in the transaction log but not in any other file. In particular,
            // it will not be persisted to the SST files. When iterating over this
            // write_batch, write_batch::handler::LogData will be called with the contents
            // of the blob as it is encountered. Blobs, puts, deletes, and merges will be
            // encountered in the same order in which they were inserted. The blob will
            // NOT consume sequence number(s) and will NOT increase the count of the batch
            //
            // Example application: add timestamps to the transaction log for use in
            // replication.
            status_type put_log_data(const slice &blob) override;

            using write_batch_base::clear;

            // clear all updates buffered in this batch.
            void clear() override;

            // Records the state of the batch for future calls to rollback_to_save_point().
            // May be called multiple times to set multiple save points.
            void set_save_point() override;

            // remove all entries in this batch (insert, merge, remove, put_log_data) since the
            // most recent call to set_save_point() and removes the most recent save point.
            // If there is no previous call to set_save_point(), status_type::NotFound()
            // will be returned.
            // Otherwise returns status_type::OK().
            status_type rollback_to_save_point() override;

            // Pop the most recent save point.
            // If there is no previous call to set_save_point(), status_type::NotFound()
            // will be returned.
            // Otherwise returns status_type::OK().
            status_type pop_save_point() override;

            // Support for iterating over the contents of a batch.
            class handler {
            public:
                virtual ~handler();
                // All handler functions in this class provide default implementations so
                // we won't break existing clients of handler on a source code level when
                // adding a new member function.

                // default implementation will just call insert without column family for
                // backwards compatibility. If the column family is not default,
                // the function is noop
                virtual status_type PutCF(uint32_t column_family_id, const slice &key, const slice &value) {
                    if (column_family_id == 0) {
                        // insert() historically doesn't return status. We didn't want to be
                        // backwards incompatible so we didn't change the return status
                        // (this is a public API). We do an ordinary get and return status_type::OK()
                        Put(key, value);
                        return status_type();
                    }
                    return status_type::InvalidArgument("non-default column family and PutCF not implemented");
                }

                virtual void Put(const slice & /*key*/, const slice & /*value*/) {
                }

                virtual status_type DeleteCF(uint32_t column_family_id, const slice &key) {
                    if (column_family_id == 0) {
                        Delete(key);
                        return status_type();
                    }
                    return status_type::InvalidArgument("non-default column family and DeleteCF not implemented");
                }

                virtual void Delete(const slice & /*key*/) {
                }

                virtual status_type SingleDeleteCF(uint32_t column_family_id, const slice &key) {
                    if (column_family_id == 0) {
                        SingleDelete(key);
                        return status_type();
                    }
                    return status_type::InvalidArgument("non-default column family and SingleDeleteCF not implemented");
                }

                virtual void SingleDelete(const slice & /*key*/) {
                }

                virtual status_type DeleteRangeCF(uint32_t /*column_family_id*/, const slice & /*begin_key*/,
                                                  const slice & /*end_key*/) {
                    return status_type::InvalidArgument("DeleteRangeCF not implemented");
                }

                virtual status_type MergeCF(uint32_t column_family_id, const slice &key, const slice &value) {
                    if (column_family_id == 0) {
                        Merge(key, value);
                        return status_type();
                    }
                    return status_type::InvalidArgument("non-default column family and MergeCF not implemented");
                }

                virtual void Merge(const slice & /*key*/, const slice & /*value*/) {
                }

                virtual status_type PutBlobIndexCF(uint32_t /*column_family_id*/, const slice & /*key*/,
                                                   const slice & /*value*/) {
                    return status_type::InvalidArgument("PutBlobIndexCF not implemented");
                }

                // The default implementation of LogData does nothing.
                virtual void LogData(const slice &blob);

                virtual status_type MarkBeginPrepare(bool = false) {
                    return status_type::InvalidArgument("MarkBeginPrepare() handler not defined.");
                }

                virtual status_type MarkEndPrepare(const slice & /*xid*/) {
                    return status_type::InvalidArgument("MarkEndPrepare() handler not defined.");
                }

                virtual status_type MarkNoop(bool /*empty_batch*/) {
                    return status_type::InvalidArgument("MarkNoop() handler not defined.");
                }

                virtual status_type MarkRollback(const slice & /*xid*/) {
                    return status_type::InvalidArgument("MarkRollbackPrepare() handler not defined.");
                }

                virtual status_type MarkCommit(const slice & /*xid*/) {
                    return status_type::InvalidArgument("MarkCommit() handler not defined.");
                }

                // Continue is called by write_batch::Iterate. If it returns false,
                // iteration is halted. Otherwise, it continues iterating. The default
                // implementation always returns true.
                virtual bool Continue();

            protected:
                friend class write_batch;

                virtual bool WriteAfterCommit() const {
                    return true;
                }

                virtual bool WriteBeforePrepare() const {
                    return false;
                }
            };

            status_type Iterate(handler *handler) const;

            // Retrieve the serialized version of this batch.
            const std::string &Data() const {
                return rep_;
            }

            // Retrieve data size of the batch.
            size_t GetDataSize() const {
                return rep_.size();
            }

            // Returns the number of updates in the batch
            int Count() const;

            // Returns true if PutCF will be called during Iterate
            bool HasPut() const;

            // Returns true if DeleteCF will be called during Iterate
            bool HasDelete() const;

            // Returns true if SingleDeleteCF will be called during Iterate
            bool HasSingleDelete() const;

            // Returns true if DeleteRangeCF will be called during Iterate
            bool HasDeleteRange() const;

            // Returns true if MergeCF will be called during Iterate
            bool HasMerge() const;

            // Returns true if MarkBeginPrepare will be called during Iterate
            bool HasBeginPrepare() const;

            // Returns true if MarkEndPrepare will be called during Iterate
            bool HasEndPrepare() const;

            // Returns trie if MarkCommit will be called during Iterate
            bool HasCommit() const;

            // Returns trie if MarkRollback will be called during Iterate
            bool HasRollback() const;

            using write_batch_base::GetWriteBatch;

            write_batch *GetWriteBatch() override {
                return this;
            }

            // Constructor with a serialized string object
            explicit write_batch(const std::string &rep);

            explicit write_batch(std::string &&rep);

            write_batch(const write_batch &src);

            write_batch(write_batch &&src) noexcept;

            write_batch &operator=(const write_batch &src);

            write_batch &operator=(write_batch &&src);

            // marks this point in the write_batch as the last record to
            // be inserted into the WAL, provided the WAL is enabled
            void MarkWalTerminationPoint();

            const SavePoint &GetWalTerminationPoint() const {
                return wal_term_point_;
            }

            void SetMaxBytes(size_t max_bytes) override {
                max_bytes_ = max_bytes;
            }

        private:
            friend class WriteBatchInternal;

            friend class LocalSavePoint;

            // TODO(myabandeh): this is needed for a hack to collapse the write batch and
            // remove duplicate keys. remove it when the hack is replaced with a proper
            // solution.
            friend class WriteBatchWithIndex;

            SavePoints *save_points_;

            // When sending a write_batch through WriteImpl we might want to
            // specify that only the first x records of the batch be written to
            // the WAL.
            SavePoint wal_term_point_;

            // For HasXYZ.  Mutable to allow lazy computation of results
            mutable std::atomic<uint32_t> content_flags_;

            // Performs deferred computation of content_flags if necessary
            uint32_t ComputeContentFlags() const;

            // Maximum size of rep_.
            size_t max_bytes_;

            // Is the content of the batch the application's latest state that meant only
            // to be used for recovery? Refer to
            // TransactionOptions::use_only_the_last_commit_time_batch_for_recovery for
            // more details.
            bool is_latest_persistent_state_ = false;

        protected:
            std::string rep_;  // See comment in write_batch.cc for the format of rep_

            // Intentionally copyable
        };

    }
} // namespace nil
