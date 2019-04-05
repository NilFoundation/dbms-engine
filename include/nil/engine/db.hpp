#pragma once

#include <cstdint>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nil/engine/iterator.hpp>
#include <nil/engine/listener.hpp>
#include <nil/engine/metadata.hpp>
#include <nil/engine/options.hpp>
#include <nil/engine/snapshot.hpp>
#include <nil/engine/sst_file_writer.hpp>
#include <nil/engine/thread_status.hpp>
#include <nil/engine/transaction_log.hpp>
#include <nil/engine/types.hpp>

#ifdef _WIN32
// Windows API macro interference
#undef DeleteFile
#endif

#if defined(__GNUC__) || defined(__clang__)
#define ROCKSDB_DEPRECATED_FUNC __attribute__((__deprecated__))
#elif _WIN32
#define ROCKSDB_DEPRECATED_FUNC __declspec(deprecated)
#endif

namespace nil {
    namespace dcdb {

        struct options;
        struct db_options;
        struct read_options;
        struct write_options;
        struct flush_options;
        struct compaction_options;
        struct compact_range_options;
        struct table_properties;
        struct external_sst_file_info;

        class write_batch;

        class environment_type;

        class event_listener;

        class stats_history_iterator;

        class trace_writer;

#ifdef DCDB_LITE
        class compaction_job_info;
#endif

        using std::unique_ptr;

        extern const std::string kDefaultColumnFamilyName;

        struct column_family_descriptor {
            std::string name;
            column_family_options options;

            column_family_descriptor() : name(kDefaultColumnFamilyName), options(column_family_options()) {
            }

            column_family_descriptor(const std::string &_name, const column_family_options &_options) : name(_name),
                    options(_options) {
            }
        };

        class column_family_handle {
        public:
            virtual ~column_family_handle() {
            }

            // Returns the name of the column family associated with the current handle.
            virtual const std::string &get_name() const = 0;

            // Returns the ID of the column family associated with the current handle.
            virtual uint32_t get_id() const = 0;

            // Fills "*desc" with the up-to-date descriptor of the column family
            // associated with this handle. Since it fills "*desc" with the up-to-date
            // information, this call might internally lock and release database mutex to
            // access the up-to-date CF opts.  In addition, all the pointer-typed
            // opts cannot be referenced any longer than the original opts exist.
            //
            // Note that this function is not supported in RocksDBLite.
            virtual status_type get_descriptor(column_family_descriptor *desc) = 0;

            // Returns the comparator of the column family associated with the
            // current handle.
            virtual const comparator *get_comparator() const = 0;
        };

        static const int kMajorVersion = 0;//__ROCKSDB_MAJOR__;
        static const int kMinorVersion = 0;//__ROCKSDB_MINOR__;

// A range of keys
        struct range {
            slice start;
            slice limit;

            range() {
            }

            range(const slice &s, const slice &l) : start(s), limit(l) {
            }
        };

        struct range_ptr {
            const slice *start;
            const slice *limit;

            range_ptr() : start(nullptr), limit(nullptr) {
            }

            range_ptr(const slice *s, const slice *l) : start(s), limit(l) {
            }
        };

        struct ingest_external_file_arg {
            column_family_handle *column_family = nullptr;
            std::vector<std::string> external_files;
            ingest_external_file_options options;
        };

// A collections of table properties objects, where
//  key: is the table's file name.
//  value: the table properties object of the given table.
        typedef std::unordered_map<std::string, std::shared_ptr<const table_properties>> table_properties_collection;

// A database is a persistent ordered map from keys to values.
// A database is safe for concurrent access from multiple threads without
// any external synchronization.
        class database {
        public:
            // open the database with the specified "name".
            // Stores a pointer to a heap-allocated database in *dbptr and returns
            // is_ok on success.
            // Stores nullptr in *dbptr and returns a non-is_ok status on error.
            // Caller should delete *dbptr when it is no longer needed.
            static status_type open(const options &options, const std::string &name, database **dbptr);

            // open the database for read only. All database interfaces
            // that modify data, like insert/delete, will return error.
            // If the db is opened in read only mode, then no compactions
            // will happen.
            //
            // Not supported in DCDB_LITE, in which case the function will
            // return status_type::not_supported.
            static status_type open_for_read_only(const options &options, const std::string &name, database **dbptr,
                                                  bool error_if_log_file_exist = false);

            // open the database for read only with column families. When opening database with
            // read only, you can specify only a subset of column families in the
            // database that should be opened. However, you always need to specify default
            // column family. The default column family name is 'default' and it's stored
            // in nil::dcdb::kDefaultColumnFamilyName
            //
            // Not supported in DCDB_LITE, in which case the function will
            // return status_type::not_supported.
            static status_type open_for_read_only(const db_options &db_options, const std::string &name,
                                                  const std::vector<column_family_descriptor> &column_families,
                                                  std::vector<column_family_handle *> *handles, database **dbptr,
                                                  bool error_if_log_file_exist = false);

            // open database with column families.
            // db_options specify database specific opts
            // column_families is the vector of all column families in the database,
            // containing column family name and opts. You need to open ALL column
            // families in the database. To get the list of column families, you can use
            // list_column_families(). Also, you can open only a subset of column families
            // for read-only access.
            // The default column family name is 'default' and it's stored
            // in nil::dcdb::kDefaultColumnFamilyName.
            // If everything is is_ok, handles will on return be the same size
            // as column_families --- handles[i] will be a handle that you
            // will use to operate on column family column_family[i].
            // Before delete database, you have to close All column families by calling
            // destroy_column_family_handle() with all the handles.
            static status_type open(const db_options &db_options, const std::string &name,
                                    const std::vector<column_family_descriptor> &column_families,
                                    std::vector<column_family_handle *> *handles, database **dbptr);

            virtual status_type resume() {
                return status_type::not_supported();
            }

            // close the database by releasing resources, closing files etc. This should be
            // called before calling the destructor so that the caller can get back a
            // status in case there are any errors. This will not fsync the WAL files.
            // If syncing is required, the caller must first call sync_wal(), or write()
            // using an empty write batch with write_options.sync=true.
            // Regardless of the return status, the database must be freed. If the return
            // status is not_supported(), then the database implementation does cleanup in the
            // destructor
            virtual status_type close() {
                return status_type::not_supported();
            }

            // list_column_families will open the database specified by argument name
            // and return the list of all column families in that database
            // through column_families argument. The ordering of
            // column families in column_families is unspecified.
            static status_type list_column_families(const db_options &db_options, const std::string &name,
                                                    std::vector<std::string> *column_families);

            database() {
            }

            virtual ~database();

            // Create a column_family and return the handle of column family
            // through the argument handle.
            virtual status_type create_column_family(const column_family_options &options,
                                                     const std::string &column_family_name,
                                                     column_family_handle **handle);

            // Bulk create column families with the same column family opts.
            // Return the handles of the column families through the argument handles.
            // In case of error, the request may succeed partially, and handles will
            // contain column family handles that it managed to create, and have size
            // equal to the number of created column families.
            virtual status_type create_column_families(const column_family_options &options,
                                                       const std::vector<std::string> &column_family_names,
                                                       std::vector<column_family_handle *> *handles);

            // Bulk create column families.
            // Return the handles of the column families through the argument handles.
            // In case of error, the request may succeed partially, and handles will
            // contain column family handles that it managed to create, and have size
            // equal to the number of created column families.
            virtual status_type create_column_families(const std::vector<column_family_descriptor> &column_families,
                                                       std::vector<column_family_handle *> *handles);

            // Drop a column family specified by column_family handle. This call
            // only records a drop record in the manifest and prevents the column
            // family from flushing and compacting.
            virtual status_type drop_column_family(column_family_handle *column_family);

            // Bulk drop column families. This call only records drop records in the
            // manifest and prevents the column families from flushing and compacting.
            // In case of error, the request may succeed partially. User may call
            // list_column_families to check the result.
            virtual status_type drop_column_families(const std::vector<column_family_handle *> &column_families);

            // close a column family specified by column_family handle and destroy
            // the column family handle specified to avoid double deletion. This call
            // deletes the column family handle by default. Use this method to
            // close column family instead of deleting column family handle directly
            virtual status_type destroy_column_family_handle(column_family_handle *column_family);

            // Set the database entry for "key" to "value".
            // If "key" already exists, it will be overwritten.
            // Returns OK on success, and a non-is_ok status on error.
            // Note: consider setting opts.sync = true.
            virtual status_type insert(const write_options &options, column_family_handle *column_family,
                                       const slice &key, const slice &value) = 0;

            virtual status_type insert(const write_options &options, const slice &key, const slice &value) {
                return insert(options, default_column_family(), key, value);
            }

            // remove the database entry (if any) for "key".  Returns is_ok on
            // success, and a non-is_ok status on error.  It is not an error if "key"
            // did not exist in the database.
            // Note: consider setting opts.sync = true.
            virtual status_type remove(const write_options &options, column_family_handle *column_family,
                                       const slice &key) = 0;

            virtual status_type remove(const write_options &options, const slice &key) {
                return remove(options, default_column_family(), key);
            }

            // remove the database entry for "key". Requires that the key exists
            // and was not overwritten. Returns is_ok on success, and a non-OK status
            // on error.  It is not an error if "key" did not exist in the database.
            //
            // If a key is overwritten (by calling insert() multiple times), then the result
            // of calling single_remove() on this key is undefined.  single_remove() only
            // behaves correctly if there has been only one insert() for this key since the
            // previous call to single_remove() for this key.
            //
            // This feature is currently an experimental performance optimization
            // for a very specific workload.  It is up to the caller to ensure that
            // single_remove is only used for a key that is not deleted using remove() or
            // written using merge().  Mixing single_remove operations with Deletes and
            // Merges can result in undefined behavior.
            //
            // Note: consider setting opts.sync = true.
            virtual status_type single_delete(const write_options &options, column_family_handle *column_family,
                                              const slice &key) = 0;

            virtual status_type single_delete(const write_options &options, const slice &key) {
                return single_delete(options, default_column_family(), key);
            }

            // Removes the database entries in the range ["begin_key", "end_key"), i.e.,
            // including "begin_key" and excluding "end_key". Returns is_ok on success, and
            // a non-is_ok status on error. It is not an error if no keys exist in the range
            // ["begin_key", "end_key").
            //
            // This feature is now usable in production, with the following caveats:
            // 1) Accumulating many range tombstones in the memtable will degrade read
            // performance; this can be avoided by manually flushing occasionally.
            // 2) Limiting the maximum number of open files in the presence of range
            // tombstones can degrade read performance. To avoid this problem, set
            // max_open_files to -1 whenever possible.
            virtual status_type delete_range(const write_options &options, column_family_handle *column_family,
                                             const slice &begin_key, const slice &end_key);

            // merge the database entry for "key" with "value".  Returns is_ok on success,
            // and a non-is_ok status on error. The semantics of this operation is
            // determined by the user provided merge_operator when opening database.
            // Note: consider setting opts.sync = true.
            virtual status_type merge(const write_options &options, column_family_handle *column_family,
                                      const slice &key, const slice &value) = 0;

            virtual status_type merge(const write_options &options, const slice &key, const slice &value) {
                return merge(options, default_column_family(), key, value);
            }

            // Apply the specified updates to the database.
            // If `updates` contains no update, WAL will still be synced if
            // opts.sync=true.
            // Returns OK on success, non-is_ok on failure.
            // Note: consider setting opts.sync = true.
            virtual status_type write(const write_options &options, write_batch *updates) = 0;

            // If the database contains an entry for "key" store the
            // corresponding value in *value and return is_ok.
            //
            // If there is no entry for "key" leave *value unchanged and return
            // a status for which status_type::is_not_found() returns true.
            //
            // May return some other status_type on an error.
            virtual inline status_type get(const read_options &options, column_family_handle *column_family,
                                           const slice &key, std::string *value) {
                assert(value != nullptr);
                pinnable_slice pinnable_val(value);
                assert(!pinnable_val.is_pinned());
                auto s = get(options, column_family, key, &pinnable_val);
                if (s.is_ok() && pinnable_val.is_pinned()) {
                    value->assign(pinnable_val.data(), pinnable_val.size());
                }  // else value is already assigned
                return s;
            }

            virtual status_type get(const read_options &options, column_family_handle *column_family, const slice &key,
                                    pinnable_slice *value) = 0;

            virtual status_type get(const read_options &options, const slice &key, std::string *value) {
                return get(options, default_column_family(), key, value);
            }

            // If keys[i] does not exist in the database, then the i'th returned
            // status will be one for which status_type::is_not_found() is true, and
            // (*values)[i] will be set to some arbitrary value (often ""). Otherwise,
            // the i'th returned status will have status_type::is_ok() true, and (*values)[i]
            // will store the value associated with keys[i].
            //
            // (*values) will always be resized to be the same size as (keys).
            // Similarly, the number of returned statuses will be the number of keys.
            // Note: keys will not be "de-duplicated". Duplicate keys will return
            // duplicate values in order.
            virtual std::vector<status_type> multi_get(const read_options &options,
                                                       const std::vector<column_family_handle *> &column_family,
                                                       const std::vector<slice> &keys,
                                                       std::vector<std::string> *values) = 0;

            virtual std::vector<status_type> multi_get(const read_options &options, const std::vector<slice> &keys,
                                                       std::vector<std::string> *values) {
                return multi_get(options, std::vector<column_family_handle *>(keys.size(), default_column_family()),
                        keys, values);
            }

            // If the key definitely does not exist in the database, then this method
            // returns false, else true. If the caller wants to obtain value when the key
            // is found in memory, a bool for 'value_found' must be passed. 'value_found'
            // will be true on return if value has been set properly.
            // This check is potentially lighter-weight than invoking database::get(). One way
            // to make this lighter weight is to avoid doing any IOs.
            // default_environment implementation here returns true and sets 'value_found' to false
            virtual bool key_may_exist(const read_options &options, column_family_handle *column_family,
                                       const slice &key, std::string *value, bool *value_found = nullptr) {
                if (value_found != nullptr) {
                    *value_found = false;
                }
                return true;
            }

            virtual bool key_may_exist(const read_options &options, const slice &key, std::string *value,
                                       bool *value_found = nullptr) {
                return key_may_exist(options, default_column_family(), key, value, value_found);
            }

            // Return a heap-allocated iterator over the contents of the database.
            // The result of new_iterator() is initially invalid (caller must
            // call one of the seek methods on the iterator before using it).
            //
            // Caller should delete the iterator when it is no longer needed.
            // The returned iterator should be deleted before this db is deleted.
            virtual iterator *new_iterator(const read_options &options, column_family_handle *column_family) = 0;

            virtual iterator *new_iterator(const read_options &options) {
                return new_iterator(options, default_column_family());
            }

            // Returns iterators from a consistent database state across multiple
            // column families. Iterators are heap allocated and need to be deleted
            // before the db is deleted
            virtual status_type new_iterators(const read_options &options,
                                              const std::vector<column_family_handle *> &column_families,
                                              std::vector<iterator *> *iterators) = 0;

            // Return a handle to the current database state.  Iterators created with
            // this handle will all observe a stable get_snapshot of the current database
            // state.  The caller must call release_snapshot(result) when the
            // get_snapshot is no longer needed.
            //
            // nullptr will be returned if the database fails to take a get_snapshot or does
            // not support get_snapshot.
            virtual const snapshot *get_snapshot() = 0;

            // release a previously acquired get_snapshot.  The caller must not
            // use "get_snapshot" after this call.
            virtual void release_snapshot(const snapshot *snapshot) = 0;

#ifndef DCDB_LITE
            // contains all valid property arguments for get_property().
            //
            // NOTE: Property names cannot end in numbers since those are interpreted as
            //       arguments, e.g., see kNumFilesAtLevelPrefix.
            struct properties {
                //  "rocksdb.num-files-at-level<N>" - returns string containing the number
                //      of files at level <N>, where <N> is an ASCII representation of a
                //      level number (e.g., "0").
                static const std::string kNumFilesAtLevelPrefix;

                //  "rocksdb.compression-ratio-at-level<N>" - returns string containing the
                //      compression ratio of data at level <N>, where <N> is an ASCII
                //      representation of a level number (e.g., "0"). Here, compression
                //      ratio is defined as uncompressed data size / compressed file size.
                //      Returns "-1.0" if no open files at level <N>.
                static const std::string kCompressionRatioAtLevelPrefix;

                //  "rocksdb.stats" - returns a multi-line string containing the data
                //      described by kCFStats followed by the data described by kDBStats.
                static const std::string kStats;

                //  "rocksdb.sstables" - returns a multi-line string summarizing current
                //      SST files.
                static const std::string kSSTables;

                //  "rocksdb.cfstats" - Both of "rocksdb.cfstats-no-file-histogram" and
                //      "rocksdb.cf-file-histogram" together. See below for description
                //      of the two.
                static const std::string kCFStats;

                //  "rocksdb.cfstats-no-file-histogram" - returns a multi-line string with
                //      general columm family stats per-level over db's lifetime ("L<n>"),
                //      aggregated over db's lifetime ("Sum"), and aggregated over the
                //      interval since the last retrieval ("Int").
                //  It could also be used to return the stats in the format of the map.
                //  In this case there will a pair of string to array of double for
                //  each level as well as for "Sum". "Int" stats will not be affected
                //  when this form of stats are retrieved.
                static const std::string kCFStatsNoFileHistogram;

                //  "rocksdb.cf-file-histogram" - print out how many file reads to every
                //      level, as well as the histogram of latency of single requests.
                static const std::string kCFFileHistogram;

                //  "rocksdb.dbstats" - returns a multi-line string with general database
                //      stats, both cumulative (over the db's lifetime) and interval (since
                //      the last retrieval of kDBStats).
                static const std::string kDBStats;

                //  "rocksdb.levelstats" - returns multi-line string containing the number
                //      of files per level and total size of each level (MB).
                static const std::string kLevelStats;

                //  "rocksdb.num-immutable-mem-table" - returns number of immutable
                //      memtables that have not yet been flushed.
                static const std::string kNumImmutableMemTable;

                //  "rocksdb.num-immutable-mem-table-flushed" - returns number of immutable
                //      memtables that have already been flushed.
                static const std::string kNumImmutableMemTableFlushed;

                //  "rocksdb.mem-table-flush-pending" - returns 1 if a memtable flush is
                //      pending; otherwise, returns 0.
                static const std::string kMemTableFlushPending;

                //  "rocksdb.num-running-flushes" - returns the number of currently running
                //      flushes.
                static const std::string kNumRunningFlushes;

                //  "rocksdb.compaction-pending" - returns 1 if at least one compaction is
                //      pending; otherwise, returns 0.
                static const std::string kCompactionPending;

                //  "rocksdb.num-running-compactions" - returns the number of currently
                //      running compactions.
                static const std::string kNumRunningCompactions;

                //  "rocksdb.background-errors" - returns accumulated number of background
                //      errors.
                static const std::string kBackgroundErrors;

                //  "rocksdb.cur-size-active-mem-table" - returns approximate size of active
                //      memtable (bytes).
                static const std::string kCurSizeActiveMemTable;

                //  "rocksdb.cur-size-all-mem-tables" - returns approximate size of active
                //      and unflushed immutable memtables (bytes).
                static const std::string kCurSizeAllMemTables;

                //  "rocksdb.size-all-mem-tables" - returns approximate size of active,
                //      unflushed immutable, and pinned immutable memtables (bytes).
                static const std::string kSizeAllMemTables;

                //  "rocksdb.num-entries-active-mem-table" - returns total number of entries
                //      in the active memtable.
                static const std::string kNumEntriesActiveMemTable;

                //  "rocksdb.num-entries-imm-mem-tables" - returns total number of entries
                //      in the unflushed immutable memtables.
                static const std::string kNumEntriesImmMemTables;

                //  "rocksdb.num-deletes-active-mem-table" - returns total number of delete
                //      entries in the active memtable.
                static const std::string kNumDeletesActiveMemTable;

                //  "rocksdb.num-deletes-imm-mem-tables" - returns total number of delete
                //      entries in the unflushed immutable memtables.
                static const std::string kNumDeletesImmMemTables;

                //  "rocksdb.estimate-num-keys" - returns estimated number of total keys in
                //      the active and unflushed immutable memtables and engine.
                static const std::string kEstimateNumKeys;

                //  "rocksdb.estimate-table-readers-mem" - returns estimated memory used for
                //      reading SST tables, excluding memory used in block cache (e.g.,
                //      filter and index blocks).
                static const std::string kEstimateTableReadersMem;

                //  "rocksdb.is-file-deletions-enabled" - returns 0 if deletion of obsolete
                //      files is enabled; otherwise, returns a non-zero number.
                static const std::string kIsFileDeletionsEnabled;

                //  "rocksdb.num-snapshots" - returns number of unreleased snapshots of the
                //      database.
                static const std::string kNumSnapshots;

                //  "rocksdb.oldest-get_snapshot-time" - returns number representing unix
                //      timestamp of oldest unreleased get_snapshot.
                static const std::string kOldestSnapshotTime;

                //  "rocksdb.num-live-versions" - returns number of live versions. `Version`
                //      is an internal data structure. See version_set.h for details. More
                //      live versions often mean more SST files are held from being deleted,
                //      by iterators or unfinished compactions.
                static const std::string kNumLiveVersions;

                //  "rocksdb.current-super-version-number" - returns number of current LSM
                //  version. It is a uint64_t integer number, incremented after there is
                //  any change to the LSM tree. The number is not preserved after restarting
                //  the database. After database restart, it will start from 0 again.
                static const std::string kCurrentSuperVersionNumber;

                //  "rocksdb.estimate-live-data-size" - returns an estimate of the amount of
                //      live data in bytes.
                static const std::string kEstimateLiveDataSize;

                //  "rocksdb.min-log-number-to-keep" - return the minimum log number of the
                //      log files that should be kept.
                static const std::string kMinLogNumberToKeep;

                //  "rocksdb.min-obsolete-sst-number-to-keep" - return the minimum file
                //      number for an obsolete SST to be kept. The max value of `uint64_t`
                //      will be returned if all obsolete files can be deleted.
                static const std::string kMinObsoleteSstNumberToKeep;

                //  "rocksdb.total-sst-files-size" - returns total size (bytes) of all SST
                //      files.
                //  WARNING: may slow down online queries if there are too many files.
                static const std::string kTotalSstFilesSize;

                //  "rocksdb.live-sst-files-size" - returns total size (bytes) of all SST
                //      files belong to the latest LSM tree.
                static const std::string kLiveSstFilesSize;

                //  "rocksdb.base-level" - returns number of level to which L0 data will be
                //      compacted.
                static const std::string kBaseLevel;

                //  "rocksdb.estimate-pending-compaction-bytes" - returns estimated total
                //      number of bytes compaction needs to rewrite to get all levels down
                //      to under target size. Not valid for other compactions than level-
                //      based.
                static const std::string kEstimatePendingCompactionBytes;

                //  "rocksdb.aggregated-table-properties" - returns a string representation
                //      of the aggregated table properties of the target column family.
                static const std::string kAggregatedTableProperties;

                //  "rocksdb.aggregated-table-properties-at-level<N>", same as the previous
                //      one but only returns the aggregated table properties of the
                //      specified level "N" at the target column family.
                static const std::string kAggregatedTablePropertiesAtLevel;

                //  "rocksdb.actual-delayed-write-rate" - returns the current actual delayed
                //      write rate. 0 means no delay.
                static const std::string kActualDelayedWriteRate;

                //  "rocksdb.is-write-stopped" - Return 1 if write has been stopped.
                static const std::string kIsWriteStopped;

                //  "rocksdb.estimate-oldest-key-time" - returns an estimation of
                //      oldest key timestamp in the database. Currently only available for
                //      FIFO compaction with
                //      compaction_options_fifo.allow_compaction = false.
                static const std::string kEstimateOldestKeyTime;

                //  "rocksdb.block-cache-capacity" - returns block cache capacity.
                static const std::string kBlockCacheCapacity;

                //  "rocksdb.block-cache-usage" - returns the memory size for the entries
                //      residing in block cache.
                static const std::string kBlockCacheUsage;

                // "rocksdb.block-cache-pinned-usage" - returns the memory size for the
                //      entries being pinned.
                static const std::string kBlockCachePinnedUsage;

                // "rocksdb.opts-statistics" - returns multi-line string
                //      of opts.statistics
                static const std::string kOptionsStatistics;
            };
#endif /* DCDB_LITE */

            // database implementations can export properties about their state via this method.
            // If "property" is a valid property understood by this database implementation (see
            // properties struct above for valid opts), fills "*value" with its current
            // value and returns true.  Otherwise, returns false.
            virtual bool get_property(column_family_handle *column_family, const slice &property,
                                      std::string *value) = 0;

            virtual bool get_property(const slice &property, std::string *value) {
                return get_property(default_column_family(), property, value);
            }

            virtual bool get_map_property(column_family_handle *column_family, const slice &property,
                                          std::map<std::string, std::string> *value) = 0;

            virtual bool get_map_property(const slice &property, std::map<std::string, std::string> *value) {
                return get_map_property(default_column_family(), property, value);
            }

            // Similar to get_property(), but only works for a subset of properties whose
            // return value is an integer. Return the value by integer. Supported
            // properties:
            //  "rocksdb.num-immutable-mem-table"
            //  "rocksdb.mem-table-flush-pending"
            //  "rocksdb.compaction-pending"
            //  "rocksdb.background-errors"
            //  "rocksdb.cur-size-active-mem-table"
            //  "rocksdb.cur-size-all-mem-tables"
            //  "rocksdb.size-all-mem-tables"
            //  "rocksdb.num-entries-active-mem-table"
            //  "rocksdb.num-entries-imm-mem-tables"
            //  "rocksdb.num-deletes-active-mem-table"
            //  "rocksdb.num-deletes-imm-mem-tables"
            //  "rocksdb.estimate-num-keys"
            //  "rocksdb.estimate-table-readers-mem"
            //  "rocksdb.is-file-deletions-enabled"
            //  "rocksdb.num-snapshots"
            //  "rocksdb.oldest-get_snapshot-time"
            //  "rocksdb.num-live-versions"
            //  "rocksdb.current-super-version-number"
            //  "rocksdb.estimate-live-data-size"
            //  "rocksdb.min-log-number-to-keep"
            //  "rocksdb.min-obsolete-sst-number-to-keep"
            //  "rocksdb.total-sst-files-size"
            //  "rocksdb.live-sst-files-size"
            //  "rocksdb.base-level"
            //  "rocksdb.estimate-pending-compaction-bytes"
            //  "rocksdb.num-running-compactions"
            //  "rocksdb.num-running-flushes"
            //  "rocksdb.actual-delayed-write-rate"
            //  "rocksdb.is-write-stopped"
            //  "rocksdb.estimate-oldest-key-time"
            //  "rocksdb.block-cache-capacity"
            //  "rocksdb.block-cache-usage"
            //  "rocksdb.block-cache-pinned-usage"
            virtual bool get_int_property(column_family_handle *column_family, const slice &property,
                                          uint64_t *value) = 0;

            virtual bool get_int_property(const slice &property, uint64_t *value) {
                return get_int_property(default_column_family(), property, value);
            }

            // reset internal stats for database and all column families.
            // Note this doesn't reset opts.statistics as it is not owned by
            // database.
            virtual status_type reset_stats() {
                return status_type::not_supported("Not implemented");
            }

            // Same as get_int_property(), but this one returns the aggregated int
            // property from all column families.
            virtual bool get_aggregated_int_property(const slice &property, uint64_t *value) = 0;

            // flags for database::GetSizeApproximation that specify whether memtable
            // stats should be included, or file stats approximation or both
            enum size_approximation_flags : uint8_t {
                NONE = 0, INCLUDE_MEMTABLES = 1, INCLUDE_FILES = 1 << 1
            };

            // For each i in [0,n-1], store in "sizes[i]", the approximate
            // file system space used by keys in "[range[i].start .. range[i].limit)".
            //
            // Note that the returned sizes measure file system space usage, so
            // if the user data compresses by a factor of ten, the returned
            // sizes will be one-tenth the size of the corresponding user data size.
            //
            // If include_flags defines whether the returned size should include
            // the recently written data in the mem-tables (if
            // the mem-table type supports it), data serialized to disk, or both.
            // include_flags should be of type database::size_approximation_flags
            virtual void get_approximate_sizes(column_family_handle *column_family, const range *range, int n,
                                               uint64_t *sizes, uint8_t include_flags = INCLUDE_FILES) = 0;

            virtual void get_approximate_sizes(const range *range, int n, uint64_t *sizes,
                                               uint8_t include_flags = INCLUDE_FILES) {
                get_approximate_sizes(default_column_family(), range, n, sizes, include_flags);
            }

            // The method is similar to get_approximate_sizes, except it
            // returns approximate number of records in memtables.
            virtual void get_approximate_mem_table_stats(column_family_handle *column_family, const range &range,
                                                         uint64_t *const count, uint64_t *const size) = 0;

            virtual void get_approximate_mem_table_stats(const range &range, uint64_t *const count,
                                                         uint64_t *const size) {
                get_approximate_mem_table_stats(default_column_family(), range, count, size);
            }

            // Deprecated versions of get_approximate_sizes
            ROCKSDB_DEPRECATED_FUNC virtual void get_approximate_sizes(const range *range, int n, uint64_t *sizes,
                                                                       bool include_memtable) {
                uint8_t include_flags = size_approximation_flags::INCLUDE_FILES;
                if (include_memtable) {
                    include_flags |= size_approximation_flags::INCLUDE_MEMTABLES;
                }
                get_approximate_sizes(default_column_family(), range, n, sizes, include_flags);
            }

            ROCKSDB_DEPRECATED_FUNC virtual void get_approximate_sizes(column_family_handle *column_family,
                                                                       const range *range, int n, uint64_t *sizes,
                                                                       bool include_memtable) {
                uint8_t include_flags = size_approximation_flags::INCLUDE_FILES;
                if (include_memtable) {
                    include_flags |= size_approximation_flags::INCLUDE_MEMTABLES;
                }
                get_approximate_sizes(column_family, range, n, sizes, include_flags);
            }

            // Compact the underlying engine for the key range [*begin,*end].
            // The actual compaction interval might be superset of [*begin, *end].
            // In particular, deleted and overwritten versions are discarded,
            // and the data is rearranged to reduce the cost of operations
            // needed to access the data.  This operation should typically only
            // be invoked by users who understand the underlying implementation.
            //
            // begin==nullptr is treated as a key before all keys in the database.
            // end==nullptr is treated as a key after all keys in the database.
            // Therefore the following call will compact the entire database:
            //    db->compact_range(opts, nullptr, nullptr);
            // Note that after the entire database is compacted, all data are pushed
            // down to the last level containing any data. If the total data size after
            // compaction is reduced, that level might not be appropriate for hosting all
            // the files. In this case, client could set opts.change_level to true, to
            // move the files back to the minimum level capable of holding the data set
            // or a given level (specified by non-negative opts.target_level).
            virtual status_type compact_range(const compact_range_options &options, column_family_handle *column_family,
                                              const slice *begin, const slice *end) = 0;

            virtual status_type compact_range(const compact_range_options &options, const slice *begin,
                                              const slice *end) {
                return compact_range(options, default_column_family(), begin, end);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type compact_range(column_family_handle *column_family,
                                                                      const slice *begin, const slice *end,
                                                                      bool change_level = false, int target_level = -1,
                                                                      uint32_t target_path_id = 0) {
                compact_range_options options;
                options.change_level = change_level;
                options.target_level = target_level;
                options.target_path_id = target_path_id;
                return compact_range(options, column_family, begin, end);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type compact_range(const slice *begin, const slice *end,
                                                                      bool change_level = false, int target_level = -1,
                                                                      uint32_t target_path_id = 0) {
                compact_range_options options;
                options.change_level = change_level;
                options.target_level = target_level;
                options.target_path_id = target_path_id;
                return compact_range(options, default_column_family(), begin, end);
            }

            virtual status_type set_options(column_family_handle *column_family,
                                            const std::unordered_map<std::string, std::string> &new_options) {
                return status_type::not_supported("Not implemented");
            }

            virtual status_type set_options(const std::unordered_map<std::string, std::string> &new_options) {
                return set_options(default_column_family(), new_options);
            }

            virtual status_type set_db_options(const std::unordered_map<std::string, std::string> &new_options) = 0;

            // compact_files() inputs a list of files specified by file numbers and
            // compacts them to the specified level. Note that the behavior is different
            // from compact_range() in that compact_files() performs the compaction job
            // using the CURRENT thread.
            //
            // @see GetDataBaseMetaData
            // @see get_column_family_meta_data
            virtual status_type compact_files(const compaction_options &compact_opts,
                                              column_family_handle *column_family,
                                              const std::vector<std::string> &input_file_names, const int output_level,
                                              const int output_path_id = -1,
                                              std::vector<std::string> *const output_file_names = nullptr,
                                              compaction_job_info *compaction_job_inf = nullptr) = 0;

            virtual status_type compact_files(const compaction_options &compact_options,
                                              const std::vector<std::string> &input_file_names, const int output_level,
                                              const int output_path_id = -1,
                                              std::vector<std::string> *const output_file_names = nullptr,
                                              compaction_job_info *compaction_job_info = nullptr) {
                return compact_files(compact_options, default_column_family(), input_file_names, output_level,
                        output_path_id, output_file_names, compaction_job_info);
            }

            // This function will wait until all currently running background processes
            // finish. After it returns, no background process will be run until
            // continue_background_work is called
            virtual status_type pause_background_work() = 0;

            virtual status_type continue_background_work() = 0;

            // This function will enable automatic compactions for the given column
            // families if they were previously disabled. The function will first set the
            // disable_auto_compactions option for each column family to 'false', after
            // which it will schedule a flush/compaction.
            //
            // NOTE: Setting disable_auto_compactions to 'false' through set_options() API
            // does NOT schedule a flush/compaction afterwards, and only changes the
            // parameter itself within the column family option.
            //
            virtual status_type enable_auto_compaction(
                    const std::vector<column_family_handle *> &column_family_handles) = 0;

            // Number of levels used for this database.
            virtual int number_levels(column_family_handle *column_family) = 0;

            virtual int number_levels() {
                return number_levels(default_column_family());
            }

            // Maximum level to which a new compacted memtable is pushed if it
            // does not create overlap.
            virtual int max_mem_compaction_level(column_family_handle *column_family) = 0;

            virtual int max_mem_compaction_level() {
                return max_mem_compaction_level(default_column_family());
            }

            // Number of files in level-0 that would stop writes.
            virtual int level0_stop_write_trigger(column_family_handle *column_family) = 0;

            virtual int level0_stop_write_trigger() {
                return level0_stop_write_trigger(default_column_family());
            }

            // get database name -- the exact same name that was provided as an argument to
            // database::open()
            virtual const std::string &get_name() const = 0;

            // get environment_type object from the database
            virtual environment_type *get_env() const = 0;

            // get database opts that we use.  During the process of opening the
            // column family, the opts provided when calling database::open() or
            // database::create_column_family() will have been "sanitized" and transformed
            // in an implementation-defined manner.
            virtual options get_options(column_family_handle *column_family) const = 0;

            virtual options get_options() const {
                return get_options(default_column_family());
            }

            virtual db_options get_db_options() const = 0;

            // flush all mem-table data.
            // flush a single column family, even when atomic flush is enabled. To flush
            // multiple column families, use flush(opts, column_families).
            virtual status_type flush(const flush_options &options, column_family_handle *column_family) = 0;

            virtual status_type flush(const flush_options &options) {
                return flush(options, default_column_family());
            }

            // Flushes multiple column families.
            // If atomic flush is not enabled, flush(opts, column_families) is
            // equivalent to calling flush(opts, column_family) multiple times.
            // If atomic flush is enabled, flush(opts, column_families) will flush all
            // column families specified in 'column_families' up to the latest sequence
            // number at the time when flush is requested.
            // Note that RocksDB 5.15 and earlier may not be able to open later versions
            // with atomic flush enabled.
            virtual status_type flush(const flush_options &options,
                                      const std::vector<column_family_handle *> &column_families) = 0;

            // flush the WAL memory buffer to the file. If sync is true, it calls sync_wal
            // afterwards.
            virtual status_type flush_wal(bool sync) {
                return status_type::not_supported("flush_wal not implemented");
            }

            // sync the wal. Note that write() followed by sync_wal() is not exactly the
            // same as write() with sync=true: in the latter case the changes won't be
            // visible until the sync is done.
            // Currently only works if allow_mmap_writes = false in opts.
            virtual status_type sync_wal() = 0;

            // The sequence number of the most recent transaction.
            virtual sequence_number get_latest_sequence_number() const = 0;

            // Instructs database to preserve deletes with sequence numbers >= passed seqnum.
            // Has no effect if db_options.preserve_deletes is set to false.
            // This function assumes that user calls this function with monotonically
            // increasing seqnums (otherwise we can't guarantee that a particular delete
            // hasn't been already processed); returns true if the value was successfully
            // updated, false if user attempted to call if with seqnum <= current value.
            virtual bool set_preserve_deletes_sequence_number(sequence_number seqnum) = 0;

#ifndef DCDB_LITE

            // Prevent file deletions. Compactions will continue to occur,
            // but no obsolete files will be deleted. Calling this multiple
            // times have the same effect as calling it once.
            virtual status_type disable_file_deletions() = 0;

            // Allow compactions to delete obsolete files.
            // If force == true, the call to enable_file_deletions() will guarantee that
            // file deletions are enabled after the call, even if disable_file_deletions()
            // was called multiple times before.
            // If force == false, enable_file_deletions will only enable file deletion
            // after it's been called at least as many times as disable_file_deletions(),
            // enabling the two methods to be called by two threads concurrently without
            // synchronization -- i.e., file deletions will be enabled only after both
            // threads call enable_file_deletions()
            virtual status_type enable_file_deletions(bool force = true) = 0;

            // get_live_files followed by get_sorted_wal_files can generate a lossless backup

            // Retrieve the list of all files in the database. The files are
            // relative to the dbname and are not absolute paths. Despite being relative
            // paths, the file names begin with "/". The valid size of the manifest file
            // is returned in manifest_file_size. The manifest file is an ever growing
            // file, but only the portion specified by manifest_file_size is valid for
            // this get_snapshot. Setting flush_memtable to true does flush before recording
            // the live files. Setting flush_memtable to false is useful when we don't
            // want to wait for flush which may have to wait for compaction to complete
            // taking an indeterminate time.
            //
            // In case you have multiple column families, even if flush_memtable is true,
            // you still need to call get_sorted_wal_files after get_live_files to compensate
            // for new data that arrived to already-flushed column families while other
            // column families were flushing
            virtual status_type get_live_files(std::vector<std::string> &ret, uint64_t *manifest_file_size,
                                               bool flush_memtable) = 0;

            // Retrieve the sorted list of all wal files with earliest file first
            virtual status_type get_sorted_wal_files(log_ptr_vector &files) = 0;

            // Note: this API is not yet consistent with WritePrepared transactions.
            // Sets iter to an iterator that is positioned at a write-batch containing
            // seq_number. If the sequence number is non existent, it returns an iterator
            // at the first available seq_no after the requested seq_no
            // Returns status_type::is_ok if iterator is valid
            // Must set WAL_ttl_seconds or WAL_size_limit_MB to large values to
            // use this api, else the WAL files will get
            // cleared aggressively and the iterator might keep getting invalid before
            // an update is read.
            virtual status_type get_updates_since(sequence_number seq_number,
                                                  std::unique_ptr<transaction_log_iterator> *iter,
                                                  const transaction_log_iterator::read_options &read_options = transaction_log_iterator::read_options()) = 0;

// Windows API macro interference
#undef DeleteFile

            // remove the file name from the db directory and update the internal state to
            // reflect that. Supports deletion of sst and log files only. 'name' must be
            // path relative to the db directory. eg. 000001.sst, /archive/000003.log
            virtual status_type delete_file(std::string name) = 0;

            // Returns a list of all table files with their level, start key
            // and end key
            virtual void get_live_files_meta_data(std::vector<live_file_meta_data> *metadata) {
            }

            // Obtains the meta data of the specified column family of the database.
            virtual void get_column_family_meta_data(column_family_handle *column_family,
                                                     column_family_meta_data *metadata) {
            }

            // get the metadata of the default column family.
            void get_column_family_meta_data(column_family_meta_data *metadata) {
                get_column_family_meta_data(default_column_family(), metadata);
            }

            // ingest_external_file() will load a list of external SST files (1) into the database
            // Two primary modes are supported:
            // - Duplicate keys in the new files will overwrite exiting keys (default)
            // - Duplicate keys will be skipped (set ingest_behind=true)
            // In the first mode we will try to find the lowest possible level that
            // the file can fit in, and ingest the file into this level (2). A file that
            // have a key range that overlap with the memtable key range will require us
            // to flush the memtable first before ingesting the file.
            // In the second mode we will always ingest in the bottom most level (see
            // docs to ingest_external_file_options::ingest_behind).
            //
            // (1) External SST files can be created using sst_file_writer
            // (2) We will try to ingest the files to the lowest possible level
            //     even if the file compression doesn't match the level compression
            // (3) If ingest_external_file_options->ingest_behind is set to true,
            //     we always ingest at the bottommost level, which should be reserved
            //     for this purpose (see DBOPtions::allow_ingest_behind flag).
            virtual status_type ingest_external_file(column_family_handle *column_family,
                                                     const std::vector<std::string> &external_files,
                                                     const ingest_external_file_options &options) = 0;

            virtual status_type ingest_external_file(const std::vector<std::string> &external_files,
                                                     const ingest_external_file_options &options) {
                return ingest_external_file(default_column_family(), external_files, options);
            }

            // ingest_external_files() will ingest files for multiple column families, and
            // record the result atomically to the MANIFEST.
            // If this function returns is_ok, all column families' ingestion must succeed.
            // If this function returns NOK, or the process crashes, then non-of the
            // files will be ingested into the database after recovery.
            // Note that it is possible for application to observe a mixed state during
            // the execution of this function. If the user performs range scan over the
            // column families with iterators, iterator on one column family may return
            // ingested data, while iterator on other column family returns old data.
            // Users can use get_snapshot for a consistent view of data.
            // If your db ingests multiple SST files using this API, i.e. args.size()
            // > 1, then RocksDB 5.15 and earlier will not be able to open it.
            //
            // REQUIRES: each arg corresponds to a different column family: namely, for
            // 0 <= i < j < len(args), args[i].column_family != args[j].column_family.
            virtual status_type ingest_external_files(const std::vector<ingest_external_file_arg> &args) = 0;

            virtual status_type verify_checksum() = 0;

            // add_file() is deprecated, please use ingest_external_file()
            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(column_family_handle *column_family,
                                                                 const std::vector<std::string> &file_path_list,
                                                                 bool move_file = false,
                                                                 bool skip_snapshot_check = false) {
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(column_family, file_path_list, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(const std::vector<std::string> &file_path_list,
                                                                 bool move_file = false,
                                                                 bool skip_snapshot_check = false) {
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(default_column_family(), file_path_list, ifo);
            }

            // add_file() is deprecated, please use ingest_external_file()
            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(column_family_handle *column_family,
                                                                 const std::string &file_path, bool move_file = false,
                                                                 bool skip_snapshot_check = false) {
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(column_family, {file_path}, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(const std::string &file_path, bool move_file = false,
                                                                 bool skip_snapshot_check = false) {
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(default_column_family(), {file_path}, ifo);
            }

            // Load table file with information "file_info" into "column_family"
            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(column_family_handle *column_family, const std::vector<
                    external_sst_file_info> &file_info_list, bool move_file = false, bool skip_snapshot_check = false) {
                std::vector<std::string> external_files;
                for (const external_sst_file_info &file_info : file_info_list) {
                    external_files.push_back(file_info.file_path);
                }
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(column_family, external_files, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(
                    const std::vector<external_sst_file_info> &file_info_list, bool move_file = false,
                    bool skip_snapshot_check = false) {
                std::vector<std::string> external_files;
                for (const external_sst_file_info &file_info : file_info_list) {
                    external_files.push_back(file_info.file_path);
                }
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(default_column_family(), external_files, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(column_family_handle *column_family,
                                                                 const external_sst_file_info *file_info,
                                                                 bool move_file = false,
                                                                 bool skip_snapshot_check = false) {
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(column_family, {file_info->file_path}, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type add_file(const external_sst_file_info *file_info,
                                                                 bool move_file = false,
                                                                 bool skip_snapshot_check = false) {
                ingest_external_file_options ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return ingest_external_file(default_column_family(), {file_info->file_path}, ifo);
            }

#endif  // DCDB_LITE

            // Sets the globally unique ID created at database creation time by invoking
            // environment_type::generate_unique_id(), in identity. Returns status_type::is_ok if identity could
            // be set properly
            virtual status_type get_db_identity(std::string &identity) const = 0;

            // Returns default column family handle
            virtual column_family_handle *default_column_family() const = 0;

#ifndef DCDB_LITE

            virtual status_type get_properties_of_all_tables(column_family_handle *column_family,
                                                             table_properties_collection *props) = 0;

            virtual status_type get_properties_of_all_tables(table_properties_collection *props) {
                return get_properties_of_all_tables(default_column_family(), props);
            }

            virtual status_type get_properties_of_tables_in_range(column_family_handle *column_family,
                                                                  const range *range, std::size_t n,
                                                                  table_properties_collection *props) = 0;

            virtual status_type suggest_compact_range(column_family_handle *column_family, const slice *begin,
                                                      const slice *end) {
                return status_type::not_supported("suggest_compact_range() is not implemented.");
            }

            virtual status_type promote_l0(column_family_handle *column_family, int target_level) {
                return status_type::not_supported("promote_l0() is not implemented.");
            }

            // Trace database operations. Use end_trace() to stop tracing.
            virtual status_type start_trace(const trace_options &options,
                                            std::unique_ptr<trace_writer> &&trace_writer) {
                return status_type::not_supported("start_trace() is not implemented.");
            }

            virtual status_type end_trace() {
                return status_type::not_supported("end_trace() is not implemented.");
            }

#endif  // DCDB_LITE

            // Needed for StackableDB
            virtual database *get_root_db() {
                return this;
            }

            // Given a time window, return an iterator for accessing stats history
            // User is responsible for deleting stats_history_iterator after use
            virtual status_type get_stats_history(uint64_t start_time, uint64_t end_time,
                                                  std::unique_ptr<stats_history_iterator> *stats_iterator) {
                return status_type::not_supported("get_stats_history() is not implemented.");
            }

        private:
            // No copying allowed
            database(const database &);

            void operator=(const database &);
        };

// Destroy the contents of the specified database.
// Be very careful using this method.
        status_type destroy_db(const std::string &name, const options &options,
                               const std::vector<column_family_descriptor> &column_families = std::vector<
                                       column_family_descriptor>());

#ifndef DCDB_LITE

// If a database cannot be opened, you may attempt to call this method to
// resurrect as much of the contents of the database as possible.
// Some data may be lost, so be careful when calling this function
// on a database that contains important information.
//
// With this API, we will warn and skip data associated with column families not
// specified in column_families.
//
// @param column_families Descriptors for known column families
        status_type repair_db(const std::string &dbname, const db_options &db_options,
                              const std::vector<column_family_descriptor> &column_families);

// @param unknown_cf_opts opts for column families encountered during the
//                        repair that were not specified in column_families.
        status_type repair_db(const std::string &dbname, const db_options &db_options,
                              const std::vector<column_family_descriptor> &column_families,
                              const column_family_options &unknown_cf_opts);

// @param opts These opts will be used for the database and for ALL column
//                families encountered during the repair
        status_type repair_db(const std::string &dbname, const options &options);

#endif

    }
} // namespace nil
