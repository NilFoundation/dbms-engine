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

        struct save_points;
        struct slice_parts;

        struct save_point {
            size_t size;  // size of rep_
            int count;    // count of elements in rep_
            uint32_t content_flags;

            save_point() : size(0), count(0), content_flags(0) {
            }

            save_point(size_t _size, int _count, uint32_t _flags) : size(_size), count(_count), content_flags(_flags) {
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
            status_type insert(column_family_handle *column_family, const slice_parts &key,
                               const slice_parts &value) override;

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
            // write_batch, write_batch::handler::log_data will be called with the contents
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
            // If there is no previous call to set_save_point(), status_type::not_found()
            // will be returned.
            // Otherwise returns status_type::ok().
            status_type rollback_to_save_point() override;

            // Pop the most recent save point.
            // If there is no previous call to set_save_point(), status_type::not_found()
            // will be returned.
            // Otherwise returns status_type::ok().
            status_type pop_save_point() override;

            // Support for iterating over the contents of a batch.
            class handler {
            public:
                virtual ~handler();
                // All handler functions in this class provide default implementations so
                // we won't break existing clients of handler on a source get_code level when
                // adding a new member function.

                // default implementation will just call insert without column family for
                // backwards compatibility. If the column family is not default,
                // the function is noop
                virtual status_type insert_cf(uint32_t column_family_id, const slice &key, const slice &value) {
                    if (column_family_id == 0) {
                        // insert() historically doesn't return status. We didn't want to be
                        // backwards incompatible so we didn't change the return status
                        // (this is a public API). We do an ordinary get and return status_type::ok()
                        insert(key, value);
                        return status_type();
                    }
                    return status_type::invalid_argument("non-default column family and insert_cf not implemented");
                }

                virtual void insert(const slice &key, const slice &value) {
                }

                virtual status_type remove_cf(uint32_t column_family_id, const slice &key) {
                    if (column_family_id == 0) {
                        remove(key);
                        return status_type();
                    }
                    return status_type::invalid_argument("non-default column family and remove_cf not implemented");
                }

                virtual void remove(const slice &key) {
                }

                virtual status_type single_remove_cf(uint32_t column_family_id, const slice &key) {
                    if (column_family_id == 0) {
                        single_remove(key);
                        return status_type();
                    }
                    return status_type::invalid_argument("non-default column family and single_remove_cf not implemented");
                }

                virtual void single_remove(const slice &key) {
                }

                virtual status_type remove_range_cf(uint32_t column_family_id, const slice &begin_key,
                                                    const slice &end_key) {
                    return status_type::invalid_argument("remove_range_cf not implemented");
                }

                virtual status_type merge_cf(uint32_t column_family_id, const slice &key, const slice &value) {
                    if (column_family_id == 0) {
                        merge(key, value);
                        return status_type();
                    }
                    return status_type::invalid_argument("non-default column family and merge_cf not implemented");
                }

                virtual void merge(const slice &key, const slice &value) {
                }

                virtual status_type put_blob_index_cf(uint32_t column_family_id, const slice &key, const slice &value) {
                    return status_type::invalid_argument("put_blob_index_cf not implemented");
                }

                // The default implementation of log_data does nothing.
                virtual void log_data(const slice &blob);

                virtual status_type mark_begin_prepare(bool unprepared = false) {
                    return status_type::invalid_argument("mark_begin_prepare() handler not defined.");
                }

                virtual status_type mark_end_prepare(const slice &xid) {
                    return status_type::invalid_argument("mark_end_prepare() handler not defined.");
                }

                virtual status_type mark_noop(bool empty_batch) {
                    return status_type::invalid_argument("mark_noop() handler not defined.");
                }

                virtual status_type mark_rollback(const slice &xid) {
                    return status_type::invalid_argument("MarkRollbackPrepare() handler not defined.");
                }

                virtual status_type mark_commit(const slice &xid) {
                    return status_type::invalid_argument("mark_commit() handler not defined.");
                }

                // continue_iterating is called by write_batch::iterate. If it returns false,
                // iteration is halted. Otherwise, it continues iterating. The default
                // implementation always returns true.
                virtual bool continue_iterating();

            protected:
                friend class write_batch;

                virtual bool write_after_commit() const {
                    return true;
                }

                virtual bool write_before_prepare() const {
                    return false;
                }
            };

            status_type iterate(handler *handler) const;

            // Retrieve the serialized version of this batch.
            const std::string &data() const {
                return rep_;
            }

            // Retrieve data size of the batch.
            size_t get_data_size() const {
                return rep_.size();
            }

            // Returns the number of updates in the batch
            int count() const;

            // Returns true if insert_cf will be called during iterate
            bool has_put() const;

            // Returns true if remove_cf will be called during iterate
            bool has_delete() const;

            // Returns true if single_remove_cf will be called during iterate
            bool has_single_delete() const;

            // Returns true if remove_range_cf will be called during iterate
            bool has_delete_range() const;

            // Returns true if merge_cf will be called during iterate
            bool has_merge() const;

            // Returns true if mark_begin_prepare will be called during iterate
            bool has_begin_prepare() const;

            // Returns true if mark_end_prepare will be called during iterate
            bool has_end_prepare() const;

            // Returns trie if mark_commit will be called during iterate
            bool has_commit() const;

            // Returns trie if mark_rollback will be called during iterate
            bool has_rollback() const;

            using write_batch_base::get_write_batch;

            write_batch *get_write_batch() override {
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
            void mark_wal_termination_point();

            const save_point &get_wal_termination_point() const {
                return wal_term_point_;
            }

            void set_max_bytes(size_t max_bytes) override {
                max_bytes_ = max_bytes;
            }

        private:
            friend class write_batch_internal;

            friend class local_save_point;

            // TODO(myabandeh): this is needed for a hack to collapse the write batch and
            // remove duplicate keys. remove it when the hack is replaced with a proper
            // solution.
            friend class write_batch_with_index;

            save_points *save_points_;

            // When sending a write_batch through WriteImpl we might want to
            // specify that only the first x records of the batch be written to
            // the WAL.
            save_point wal_term_point_;

            // For HasXYZ.  Mutable to allow lazy computation of results
            mutable std::atomic<uint32_t> content_flags_;

            // Performs deferred computation of content_flags if necessary
            uint32_t compute_content_flags() const;

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
