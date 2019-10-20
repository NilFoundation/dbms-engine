#pragma once

#include <string>
#include <vector>

#include <nil/engine/comparator.hpp>
#include <nil/engine/status.hpp>

namespace nil {
    namespace engine {

        class iterator;

        class transaction_db;

        class write_batch_with_index;

        using transaction_name = std::string;

        using transaction_id = uint64_t;

        // Provides notification to the caller of set_snapshot_on_next_operation when
        // the actual get_snapshot gets created
        class transaction_notifier {
        public:
            virtual ~transaction_notifier() {
            }

            // Implement this method to receive notification when a get_snapshot is
            // requested via set_snapshot_on_next_operation.
            virtual void snapshot_created(const snapshot *newSnapshot) = 0;
        };

        // Provides BEGIN/COMMIT/ROLLBACK transactions.
        //
        // To use transactions, you must first create either an optimistic_transaction_db
        // or a transaction_db.  See examples/[optimistic_]transaction_example.cc for
        // more information.
        //
        // To create a transaction, use [Optimistic]transaction_db::begin_transaction().
        //
        // It is up to the caller to synchronize access to this object.
        //
        // See examples/transaction_example.cc for some simple examples.
        //
        // TODO(agiardullo): Not yet implemented
        //  -perf_context_ get_statistics
        //  -Support for using Transactions with db_with_ttl
        class transaction {
        public:
            virtual ~transaction() {
            }

            // If a transaction has a get_snapshot set, the transaction will ensure that
            // any keys successfully written(or fetched via get_for_update()) have not
            // been modified outside of this transaction since the time the get_snapshot was
            // set.
            // If a get_snapshot has not been set, the transaction guarantees that keys have
            // not been modified since the time each key was first written (or fetched via
            // get_for_update()).
            //
            // Using set_snapshot() will provide stricter isolation guarantees at the
            // expense of potentially more transaction failures due to conflicts with
            // other writes.
            //
            // Calling set_snapshot() has no effect on keys written before this function
            // has been called.
            //
            // set_snapshot() may be called multiple times if you would like to change
            // the get_snapshot used for different operations in this transaction.
            //
            // Calling set_snapshot will not affect the version of data returned by get()
            // methods.  See transaction::get() for more details.
            virtual void set_snapshot() = 0;

            // Similar to set_snapshot(), but will not change the current get_snapshot
            // until insert/merge/remove/get_for_update/MultiGetForUpdate is called.
            // By calling this function, the transaction will essentially call
            // set_snapshot() for you right before performing the next write/get_for_update.
            //
            // Calling set_snapshot_on_next_operation() will not affect what get_snapshot is
            // returned by get_snapshot() until the next write/get_for_update is executed.
            //
            // When the get_snapshot is created the notifier's snapshot_created method will
            // be called so that the caller can get access to the get_snapshot.
            //
            // This is an optimization to reduce the likelihood of conflicts that
            // could occur in between the time set_snapshot() is called and the first
            // write/get_for_update operation.  Eg, this prevents the following
            // race-condition:
            //
            //   txn1->set_snapshot();
            //                             txn2->insert("A", ...);
            //                             txn2->commit();
            //   txn1->get_for_update(opts, "A", ...);  // FAIL!
            virtual void set_snapshot_on_next_operation(std::shared_ptr<transaction_notifier> notifier = nullptr) = 0;

            // Returns the get_snapshot created by the last call to set_snapshot().
            //
            // REQUIRED: The returned get_snapshot is only valid up until the next time
            // set_snapshot()/SetSnapshotOnNextSavePoint() is called, clear_snapshot()
            // is called, or the transaction is deleted.
            virtual const snapshot *get_snapshot() const = 0;

            // Clears the current snapshot (i.e. no get_snapshot will be 'set')
            //
            // This removes any get_snapshot that currently exists or is set to be created
            // on the next update operation (set_snapshot_on_next_operation).
            //
            // Calling clear_snapshot() has no effect on keys written before this function
            // has been called.
            //
            // If a reference to a get_snapshot was retrieved via get_snapshot(), it will no
            // longer be valid and should be discarded after a call to clear_snapshot().
            virtual void clear_snapshot() = 0;

            // prepare the current transaction for 2PC
            virtual engine::status_type prepare() = 0;

            // write all batched keys to the db atomically.
            //
            // Returns is_ok on success.
            //
            // May return any error status that could be returned by database:write().
            //
            // If this transaction was created by an optimistic_transaction_db(),
            // engine::status_type::busy() may be returned if the transaction could not guarantee
            // that there are no write conflicts.  engine::status_type::try_again() may be returned
            // if the memtable history size is not large enough
            //  (See max_write_buffer_number_to_maintain).
            //
            // If this transaction was created by a transaction_db(), engine::status_type::expired()
            // may be returned if this transaction has lived for longer than
            // transaction_options.expiration.
            virtual engine::status_type commit() = 0;

            // Discard all batched writes in this transaction.
            virtual engine::status_type rollback() = 0;

            // Records the state of the transaction for future calls to
            // rollback_to_save_point().  May be called multiple times to set multiple save
            // points.
            virtual void set_save_point() = 0;

            // Undo all operations in this transaction (insert, merge, remove, put_log_data)
            // since the most recent call to set_save_point() and removes the most recent
            // set_save_point().
            // If there is no previous call to set_save_point(), returns engine::status_type::not_found()
            virtual engine::status_type rollback_to_save_point() = 0;

            // Pop the most recent save point.
            // If there is no previous call to set_save_point(), engine::status_type::not_found()
            // will be returned.
            // Otherwise returns engine::status_type::ok().
            virtual engine::status_type pop_save_point() = 0;

            // This function is similar to database::get() except it will also read pending
            // changes in this transaction.  Currently, this function will return
            // engine::status_type::merge_in_progress if the most recent write to the queried key in
            // this batch is a merge.
            //
            // If read_options.get_snapshot is not set, the current version of the key will
            // be read.  Calling set_snapshot() does not affect the version of the data
            // returned.
            //
            // Note that setting read_options.get_snapshot will affect what is read from the
            // database but will NOT change which keys are read from this transaction (the keys
            // in this transaction do not yet belong to any get_snapshot and will be fetched
            // regardless).
            virtual engine::status_type get(const read_options &options, column_family_handle *column_family,
                                            const engine::slice &key, std::string *value) = 0;

            // An overload of the above method that receives a pinnable_slice
            // For backward compatibility a default implementation is provided
            virtual engine::status_type get(const read_options &options, column_family_handle *column_family,
                                            const engine::slice &key, pinnable_slice *pinnable_val) {
                assert(pinnable_val != nullptr);
                auto s = get(options, column_family, key, pinnable_val->get_self());
                pinnable_val->pin_self();
                return s;
            }

            virtual engine::status_type get(const read_options &options, const engine::slice &key,
                                            std::string *value) = 0;

            virtual engine::status_type get(const read_options &options, const engine::slice &key,
                                            pinnable_slice *pinnable_val) {
                assert(pinnable_val != nullptr);
                auto s = get(options, key, pinnable_val->get_self());
                pinnable_val->pin_self();
                return s;
            }

            virtual std::vector<engine::status_type> multi_get(const read_options &options,
                                                               const std::vector<column_family_handle *> &column_family,
                                                               const std::vector<engine::slice> &keys,
                                                               std::vector<std::string> *values) = 0;

            virtual std::vector<engine::status_type> multi_get(const read_options &options,
                                                               const std::vector<engine::slice> &keys,
                                                               std::vector<std::string> *values) = 0;

            // read this key and ensure that this transaction will only
            // be able to be committed if this key is not written outside this
            // transaction after it has first been read (or after the get_snapshot if a
            // get_snapshot is set in this transaction and do_validate is true). If
            // do_validate is false, read_options::get_snapshot is expected to be nullptr so
            // that get_for_update returns the latest committed value. The transaction
            // behavior is the same regardless of whether the key exists or not.
            //
            // Note: Currently, this function will return engine::status_type::merge_in_progress
            // if the most recent write to the queried key in this batch is a merge.
            //
            // The values returned by this function are similar to transaction::get().
            // If value==nullptr, then this function will not read any data, but will
            // still ensure that this key cannot be written to by outside of this
            // transaction.
            //
            // If this transaction was created by an optimistic_transaction, get_for_update()
            // could cause commit() to fail.  Otherwise, it could return any error
            // that could be returned by database::get().
            //
            // If this transaction was created by a transaction_db, it can return
            // engine::status_type::ok() on success,
            // engine::status_type::busy() if there is a write conflict,
            // engine::status_type::timed_out() if a lock could not be acquired,
            // engine::status_type::try_again() if the memtable history size is not large enough
            //  (See max_write_buffer_number_to_maintain)
            // engine::status_type::merge_in_progress() if merge operations cannot be resolved.
            // or other errors if this key could not be read.
            virtual engine::status_type get_for_update(const read_options &options, column_family_handle *column_family,
                                                       const engine::slice &key, std::string *value,
                                                       bool exclusive = true, bool do_validate = true) = 0;

            // An overload of the above method that receives a pinnable_slice
            // For backward compatibility a default implementation is provided
            virtual engine::status_type get_for_update(const read_options &options, column_family_handle *column_family,
                                                       const engine::slice &key, pinnable_slice *pinnable_val,
                                                       bool exclusive = true, bool do_validate = true) {
                if (pinnable_val == nullptr) {
                    std::string *null_str = nullptr;
                    return get_for_update(options, column_family, key, null_str, exclusive, do_validate);
                } else {
                    auto s =
                        get_for_update(options, column_family, key, pinnable_val->get_self(), exclusive, do_validate);
                    pinnable_val->pin_self();
                    return s;
                }
            }

            virtual engine::status_type get_for_update(const read_options &options, const engine::slice &key,
                                                       std::string *value, bool exclusive = true,
                                                       bool do_validate = true) = 0;

            virtual std::vector<engine::status_type>
                multi_get_for_update(const read_options &options,
                                     const std::vector<column_family_handle *> &column_family,
                                     const std::vector<engine::slice> &keys, std::vector<std::string> *values) = 0;

            virtual std::vector<engine::status_type> multi_get_for_update(const read_options &options,
                                                                          const std::vector<engine::slice> &keys,
                                                                          std::vector<std::string> *values) = 0;

            // Returns an iterator that will iterate on all keys in the default
            // column family including both keys in the database and uncommitted keys in this
            // transaction.
            //
            // Setting read_options.get_snapshot will affect what is read from the
            // database but will NOT change which keys are read from this transaction (the keys
            // in this transaction do not yet belong to any get_snapshot and will be fetched
            // regardless).
            //
            // Caller is responsible for deleting the returned iterator.
            //
            // The returned iterator is only valid until commit(), rollback(), or
            // rollback_to_save_point() is called.
            virtual iterator *get_iterator(const read_options &input_read_options) = 0;

            virtual iterator *get_iterator(const read_options &input_read_options,
                                           column_family_handle *column_family) = 0;

            // insert, merge, remove, and single_remove behave similarly to the corresponding
            // functions in write_batch, but will also do conflict checking on the
            // keys being written.
            //
            // assume_tracked=true expects the key be already tracked. If valid then it
            // skips ValidateSnapshot. Returns error otherwise.
            //
            // If this transaction was created on an optimistic_transaction_db, these
            // functions should always return engine::status_type::ok().
            //
            // If this transaction was created on a transaction_db, the status returned
            // can be:
            // engine::status_type::ok() on success,
            // engine::status_type::busy() if there is a write conflict,
            // engine::status_type::timed_out() if a lock could not be acquired,
            // engine::status_type::try_again() if the memtable history size is not large enough
            //  (See max_write_buffer_number_to_maintain)
            // or other errors on unexpected failures.
            virtual engine::status_type insert(column_family_handle *column_family, const engine::slice &key,
                                               const engine::slice &value, bool assume_tracked = false) = 0;

            virtual engine::status_type insert(const engine::slice &key, const engine::slice &value) = 0;

            virtual engine::status_type insert(column_family_handle *column_family, const slice_parts &key,
                                               const slice_parts &value, bool assume_tracked = false) = 0;

            virtual engine::status_type insert(const slice_parts &key, const slice_parts &value) = 0;

            virtual engine::status_type merge(column_family_handle *column_family, const engine::slice &key,
                                              const engine::slice &value, bool assume_tracked = false) = 0;

            virtual engine::status_type merge(const engine::slice &key, const engine::slice &value) = 0;

            virtual engine::status_type remove(column_family_handle *column_family, const engine::slice &key,
                                               bool assume_tracked = false) = 0;

            virtual engine::status_type remove(const engine::slice &key) = 0;

            virtual engine::status_type remove(column_family_handle *column_family, const slice_parts &key,
                                               bool assume_tracked = false) = 0;

            virtual engine::status_type remove(const slice_parts &key) = 0;

            virtual engine::status_type single_remove(column_family_handle *column_family, const engine::slice &key,
                                                      bool assume_tracked = false) = 0;

            virtual engine::status_type single_remove(const engine::slice &key) = 0;

            virtual engine::status_type single_remove(column_family_handle *column_family, const slice_parts &key,
                                                      bool assume_tracked = false) = 0;

            virtual engine::status_type single_remove(const slice_parts &key) = 0;

            // insert_untracked() will write a insert to the batch of operations to be committed
            // in this transaction.  This write will only happen if this transaction
            // gets committed successfully.  But unlike transaction::insert(),
            // no conflict checking will be done for this key.
            //
            // If this transaction was created on a pessimistic_transaction_db, this
            // function will still acquire locks necessary to make sure this write doesn't
            // cause conflicts in other transactions and may return engine::status_type::busy().
            virtual engine::status_type insert_untracked(column_family_handle *column_family, const engine::slice &key,
                                                         const engine::slice &value) = 0;

            virtual engine::status_type insert_untracked(const engine::slice &key, const engine::slice &value) = 0;

            virtual engine::status_type insert_untracked(column_family_handle *column_family, const slice_parts &key,
                                                         const slice_parts &value) = 0;

            virtual engine::status_type insert_untracked(const slice_parts &key, const slice_parts &value) = 0;

            virtual engine::status_type merge_untracked(column_family_handle *column_family, const engine::slice &key,
                                                        const engine::slice &value) = 0;

            virtual engine::status_type merge_untracked(const engine::slice &key, const engine::slice &value) = 0;

            virtual engine::status_type remove_untracked(column_family_handle *column_family,
                                                         const engine::slice &key) = 0;

            virtual engine::status_type remove_untracked(const engine::slice &key) = 0;

            virtual engine::status_type remove_untracked(column_family_handle *column_family,
                                                         const slice_parts &key) = 0;

            virtual engine::status_type remove_untracked(const slice_parts &key) = 0;

            virtual engine::status_type single_remove_untracked(column_family_handle *column_family,
                                                                const engine::slice &key) = 0;

            virtual engine::status_type single_remove_untracked(const engine::slice &key) = 0;

            // Similar to write_batch::put_log_data
            virtual void put_log_data(const engine::slice &blob) = 0;

            // By default, all insert/merge/remove operations will be indexed in the
            // transaction so that get/get_for_update/get_iterator can search for these
            // keys.
            //
            // If the caller does not want to fetch the keys about to be written,
            // they may want to avoid indexing as a performance optimization.
            // Calling disable_indexing() will turn off indexing for all future
            // insert/merge/remove operations until enable_indexing() is called.
            //
            // If a key is insert/merge/Deleted after disable_indexing is called and then
            // is fetched via get/get_for_update/get_iterator, the result of the fetch is
            // undefined.
            virtual void disable_indexing() = 0;

            virtual void enable_indexing() = 0;

            // Returns the number of distinct Keys being tracked by this transaction.
            // If this transaction was created by a transaction_db, this is the number of
            // keys that are currently locked by this transaction.
            // If this transaction was created by an optimistic_transaction_db, this is the
            // number of keys that need to be checked for conflicts at commit time.
            virtual uint64_t get_num_keys() const = 0;

            // Returns the number of Puts/Deletes/Merges that have been applied to this
            // transaction so far.
            virtual uint64_t get_num_puts() const = 0;

            virtual uint64_t get_num_deletes() const = 0;

            virtual uint64_t get_num_merges() const = 0;

            // Returns the elapsed time in milliseconds since this transaction began.
            virtual uint64_t get_elapsed_time() const = 0;

            // Fetch the underlying write batch that contains all pending changes to be
            // committed.
            //
            // Note:  You should not write or delete anything from the batch directly and
            // should only use the functions in the transaction class to
            // write to this transaction.
            virtual write_batch_with_index *get_write_batch() = 0;

            // Change the value of transaction_options.lock_timeout (in milliseconds) for
            // this transaction.
            // Has no effect on optimistic_transactions.
            virtual void set_lock_timeout(int64_t timeout) = 0;

            // Return the write_options that will be used during commit()
            virtual write_options *get_write_options() = 0;

            // reset the write_options that will be used during commit().
            virtual void set_write_options(const write_options &write_options) = 0;

            // If this key was previously fetched in this transaction using
            // get_for_update/MultiGetForUpdate(), calling undo_get_for_update will tell
            // the transaction that it no longer needs to do any conflict checking
            // for this key.
            //
            // If a key has been fetched N times via get_for_update/MultiGetForUpdate(),
            // then undo_get_for_update will only have an effect if it is also called N
            // times.  If this key has been written to in this transaction,
            // undo_get_for_update() will have no effect.
            //
            // If set_save_point() has been called after the get_for_update(),
            // undo_get_for_update() will not have any effect.
            //
            // If this transaction was created by an optimistic_transaction_db,
            // calling undo_get_for_update can affect whether this key is conflict checked
            // at commit time.
            // If this transaction was created by a transaction_db,
            // calling undo_get_for_update may release any held locks for this key.
            virtual void undo_get_for_update(column_family_handle *column_family, const engine::slice &key) = 0;

            virtual void undo_get_for_update(const engine::slice &key) = 0;

            virtual engine::status_type rebuild_from_write_batch(write_batch *src_batch) = 0;

            virtual write_batch *get_commit_time_write_batch() = 0;

            virtual void set_log_number(uint64_t log) {
                log_number_ = log;
            }

            virtual uint64_t get_log_number() const {
                return log_number_;
            }

            virtual engine::status_type set_name(const transaction_name &name) = 0;

            virtual transaction_name get_name() const {
                return name_;
            }

            virtual transaction_id get_id() const {
                return 0;
            }

            virtual bool is_deadlock_detect() const {
                return false;
            }

            virtual std::vector<transaction_id> get_waiting_txns(uint32_t *column_family_id,
                                                              std::string *key) const {
                assert(false);
                return std::vector<transaction_id>();
            }

            enum transaction_state {
                STARTED = 0,
                AWAITING_PREPARE = 1,
                PREPARED = 2,
                AWAITING_COMMIT = 3,
                COMMITED = 4,
                AWAITING_ROLLBACK = 5,
                ROLLEDBACK = 6,
                LOCKS_STOLEN = 7,
            };

            transaction_state get_state() const {
                return txn_state_;
            }

            void set_state(transaction_state state) {
                txn_state_ = state;
            }

            // NOTE: Experimental feature
            // The globally unique id with which the transaction is identified. This id
            // might or might not be set depending on the implementation. Similarly the
            // implementation decides the point in lifetime of a transaction at which it
            // assigns the id. Although currently it is the case, the id is not guaranteed
            // to remain the same across restarts.
            uint64_t get_id() {
                return id_;
            }

        protected:
            explicit transaction(const transaction_db *db) {
            }

            transaction() : log_number_(0), txn_state_(STARTED) {
            }

            // the log in which the prepared section for this txn resides
            // (for two phase commit)
            uint64_t log_number_;
            transaction_name name_;

            // Execution status of the transaction.
            std::atomic<transaction_state> txn_state_;

            uint64_t id_ = 0;

            virtual void SetId(uint64_t id) {
                assert(id_ == 0);
                id_ = id;
            }

        private:
            friend class pessimistic_transaction_db;

            friend class write_unprepared_txn_db;

            // No copying allowed
            transaction(const transaction &);

            void operator=(const transaction &);
        };

    }    // namespace dcdb
}    // namespace nil
