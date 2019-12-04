#pragma once

#include <string>
#include <utility>
#include <vector>

#include <nil/storage/engine/database.hpp>

#include <nil/storage/engine/column_family_descriptor.hpp>
#include <nil/storage/engine/comparator.hpp>
#include <nil/storage/engine/transaction.hpp>

// Database with transaction support.
//
// See transaction.h and examples/transaction_example.cc

namespace nil {
    namespace engine {

        class transaction_db_mutex_factory;

        enum txn_db_write_policy {
            WRITE_COMMITTED = 0,    // write only the committed data
                                    // TODO(myabandeh): Not implemented yet
            WRITE_PREPARED,         // write data after the prepare phase of 2pc
                                    // TODO(myabandeh): Not implemented yet
            WRITE_UNPREPARED        // write data before the prepare phase of 2pc
        };

        const uint32_t kInitialMaxDeadlocks = 5;

        struct transaction_db_options {
            // Specifies the maximum number of keys that can be locked at the same time
            // per column family.
            // If the number of locked keys is greater than max_num_locks, transaction
            // writes (or get_for_update) will return an error.
            // If this value is not positive, no limit will be enforced.
            int64_t max_num_locks = -1;

            // Stores the number of latest deadlocks to track
            uint32_t max_num_deadlocks = kInitialMaxDeadlocks;

            // Increasing this value will increase the concurrency by dividing the lock
            // table (per column family) into more sub-tables, each with their own
            // separate
            // mutex.
            size_t num_stripes = 16;

            // If positive, specifies the default wait timeout in milliseconds when
            // a transaction attempts to lock a key if not specified by
            // transaction_options::lock_timeout.
            //
            // If 0, no waiting is done if a lock cannot instantly be acquired.
            // If negative, there is no timeout.  Not using a timeout is not recommended
            // as it can lead to deadlocks.  Currently, there is no deadlock-detection to
            // recover
            // from a deadlock.
            int64_t transaction_lock_timeout = 1000;    // 1 second

            // If positive, specifies the wait timeout in milliseconds when writing a key
            // OUTSIDE of a transaction (ie by calling database::insert(),merge(),remove(),write()
            // directly).
            // If 0, no waiting is done if a lock cannot instantly be acquired.
            // If negative, there is no timeout and will block indefinitely when acquiring
            // a lock.
            //
            // Not using a timeout can lead to deadlocks.  Currently, there
            // is no deadlock-detection to recover from a deadlock.  While database writes
            // cannot deadlock with other database writes, they can deadlock with a transaction.
            // A negative timeout should only be used if all transactions have a small
            // expiration set.
            int64_t default_lock_timeout = 1000;    // 1 second

            // If set, the transaction_db will use this implementation of a mutex and
            // condition variable for all transaction locking instead of the default
            // mutex/condvar implementation.
            std::shared_ptr<transaction_db_mutex_factory> custom_mutex_factory;

            // The policy for when to write the data into the database. The default policy is to
            // write only the committed data (WRITE_COMMITTED). The data could be written
            // before the commit phase. The database then needs to provide the mechanisms to
            // tell apart committed from uncommitted data.
            txn_db_write_policy write_policy = txn_db_write_policy::WRITE_COMMITTED;

            // TODO(myabandeh): remove this option
            // Note: this is a temporary option as a hot fix in rollback of writeprepared
            // txns in myrocks. MyRocks uses merge operands for autoinc column id without
            // however obtaining locks. This breaks the assumption behind the rollback
            // logic in myrocks. This hack of simply not rolling back merge operands works
            // for the special way that myrocks uses this operands.
            bool rollback_merge_operands = false;

        private:
            // 128 entries
            size_t wp_snapshot_cache_bits = static_cast<size_t>(7);
            // 8m entry, 64MB size
            size_t wp_commit_cache_bits = static_cast<size_t>(23);

            friend class write_prepared_txn_db;

            friend class WritePreparedTransactionTestBase;

            friend class MySQLStyleTransactionTest;
        };

        struct transaction_options {
            // Setting set_snapshot=true is the same as calling
            // transaction::set_snapshot().
            bool set_snapshot = false;

            // Setting to true means that before acquiring locks, this transaction will
            // check if doing so will cause a deadlock. If so, it will return with
            // engine::status_type::busy.  The user should retry their transaction.
            bool deadlock_detect = false;

            // If set, it states that the CommitTimeWriteBatch represents the latest state
            // of the application, has only one sub-batch, i.e., no duplicate keys,  and
            // meant to be used later during recovery. It enables an optimization to
            // postpone updating the memtable with CommitTimeWriteBatch to only
            // SwitchMemtable or recovery.
            bool use_only_the_last_commit_time_batch_for_recovery = false;

            // TODO(agiardullo): transaction_db does not yet support comparators that allow
            // two non-equal keys to be equivalent.  Ie, cmp->compare(a,b) should only
            // return 0 if
            // a.compare(b) returns 0.

            // If positive, specifies the wait timeout in milliseconds when
            // a transaction attempts to lock a key.
            //
            // If 0, no waiting is done if a lock cannot instantly be acquired.
            // If negative, transaction_db_options::transaction_lock_timeout will be used.
            int64_t lock_timeout = -1;

            // Expiration duration in milliseconds.  If non-negative, transactions that
            // last longer than this many milliseconds will fail to commit.  If not set,
            // a forgotten transaction that is never committed, rolled back, or deleted
            // will never relinquish any locks it holds.  This could prevent keys from
            // being written by other writers.
            int64_t expiration = -1;

            // The number of traversals to make during deadlock detection.
            int64_t deadlock_detect_depth = 50;

            // The maximum number of bytes used for the write batch. 0 means no limit.
            size_t max_write_batch_size = 0;

            // skip Concurrency Control. This could be as an optimization if the
            // application knows that the transaction would not have any conflict with
            // concurrent transactions. It could also be used during recovery if (i)
            // application guarantees no conflict between prepared transactions in the WAL
            // (ii) application guarantees that recovered transactions will be rolled
            // back/commit before new transactions start.
            // default_environment: false
            bool skip_concurrency_control = false;
        };

        // The per-write optimizations that do not involve transactions. transaction_db
        // implementation might or might not make use of the specified optimizations.
        struct transaction_db_write_optimizations {
            // If it is true it means that the application guarantees that the
            // key-set in the write batch do not conflict with any concurrent transaction
            // and hence the concurrency control mechanism could be skipped for this
            // write.
            bool skip_concurrency_control = false;
            // If true, the application guarantees that there is no duplicate <column
            // family, key> in the write batch and any employed mechanism to handle
            // duplicate keys could be skipped.
            bool skip_duplicate_key_check = false;
        };

        struct key_lock_info {
            std::string key;
            std::vector<engine::transaction_id> ids;
            bool exclusive;
        };

        struct deadlock_info {
            engine::transaction_id m_txn_id;
            uint32_t m_cf_id;
            bool m_exclusive;
            std::string m_waiting_key;
        };

        struct deadlock_path {
            std::vector<deadlock_info> path;
            bool limit_exceeded;
            int64_t deadlock_time;

            explicit deadlock_path(const std::vector<deadlock_info> &path_entry, const int64_t &dl_time) :
                path(path_entry), limit_exceeded(false), deadlock_time(dl_time) {
            }

            // empty path, limit exceeded constructor and default constructor
            explicit deadlock_path(const int64_t &dl_time = 0, bool limit = false) :
                path(0), limit_exceeded(limit), deadlock_time(dl_time) {
            }

            bool empty() {
                return path.empty() && !limit_exceeded;
            }
        };

        class transaction_db : public database {
        public:
            // Optimized version of ::write that receives more optimization request such
            // as skip_concurrency_control.
            using database::write;

            virtual engine::status_type write(const write_options &opts, const transaction_db_write_optimizations &,
                                              write_batch *updates) {
                // The default implementation ignores transaction_db_write_optimizations and
                // falls back to the un-optimized version of ::write
                return write(opts, updates);
            }

            // open a transaction_db similar to database::open().
            // Internally call prepare_wrap() and wrap_db()
            // If the return status is not is_ok, then dbptr is set to nullptr.
            static engine::status_type open(const database_options &options,
                                            const transaction_db_options &txn_db_options, const std::string &dbname,
                                            transaction_db **dbptr);

            static engine::status_type open(const db_options &db_opts, const transaction_db_options &txn_db_opts,
                                            const std::string &dbname,
                                            const std::vector<engine::column_family_descriptor> &column_families,
                                            std::vector<engine::column_family_handle *> *handles,
                                            transaction_db **dbptr);

            // Note: prepare_wrap() may change parameters, make copies before the
            // invocation if needed.
            static void prepare_wrap(db_options *db_options,
                                     std::vector<engine::column_family_descriptor> *column_families,
                                     std::vector<size_t> *compaction_enabled_cf_indices);

            // If the return status is not is_ok, then dbptr will bet set to nullptr. The
            // input db parameter might or might not be deleted as a result of the
            // failure. If it is properly deleted it will be set to nullptr. If the return
            // status is is_ok, the ownership of db is transferred to dbptr.
            static engine::status_type wrap_db(database *db, const transaction_db_options &txn_db_options,
                                               const std::vector<size_t> &compaction_enabled_cf_indices,
                                               const std::vector<engine::column_family_handle *> &handles,
                                               transaction_db **dbptr);

            // If the return status is not is_ok, then dbptr will bet set to nullptr. The
            // input db parameter might or might not be deleted as a result of the
            // failure. If it is properly deleted it will be set to nullptr. If the return
            // status is is_ok, the ownership of db is transferred to dbptr.
            static engine::status_type wrap_stackable_db(stackable_db *db, const transaction_db_options &txn_db_options,
                                                         const std::vector<size_t> &compaction_enabled_cf_indices,
                                                         const std::vector<engine::column_family_handle *> &handles,
                                                         transaction_db **dbptr);

            // Since the destructor in stackable_db is virtual, this destructor is virtual
            // too. The root db will be deleted by the base's destructor.
            ~transaction_db() override {
            }

            // Starts a new transaction.
            //
            // Caller is responsible for deleting the returned transaction when no
            // longer needed.
            //
            // If old_txn is not null, BeginTransaction will reuse this transaction
            // handle instead of allocating a new one.  This is an optimization to avoid
            // extra allocations when repeatedly creating transactions.
            virtual engine::transaction *
                begin_transaction(const write_options &write_options,
                                  const transaction_options &txn_options = transaction_options(),
                                  engine::transaction *old_txn = nullptr) = 0;

            virtual engine::transaction *get_transaction_by_name(const engine::transaction_name &name) = 0;

            virtual void get_all_prepared_transactions(std::vector<engine::transaction *> *trans) = 0;

            // Returns set of all locks held.
            //
            // The mapping is column family id -> key_lock_info
            virtual std::unordered_multimap<uint32_t, key_lock_info> get_lock_status_data() = 0;

            virtual std::vector<deadlock_path> get_deadlock_info_buffer() = 0;

            virtual void set_deadlock_info_buffer_size(uint32_t target_size) = 0;

        protected:
            // To create an transaction_db, call open()
            // The ownership of db is transferred to the base stackable_db
            explicit transaction_db(database *db) : stackable_db(db) {
            }

        private:
            // No copying allowed
            transaction_db(const transaction_db &);

            void operator=(const transaction_db &);
        };

    }    // namespace engine
}    // namespace nil
