// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once

#include <cstdint>
#include <stdio.h>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <nil/dcdb/iterator.hpp>
#include <nil/dcdb/listener.hpp>
#include <nil/dcdb/metadata.hpp>
#include <nil/dcdb/options.hpp>
#include <nil/dcdb/snapshot.hpp>
#include <nil/dcdb/sst_file_writer.hpp>
#include <nil/dcdb/thread_status.hpp>
#include <nil/dcdb/transaction_log.hpp>
#include <nil/dcdb/types.hpp>
#include <nil/dcdb/version.hpp>

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

        struct Options;
        struct DBOptions;
        struct ReadOptions;
        struct WriteOptions;
        struct FlushOptions;
        struct CompactionOptions;
        struct CompactRangeOptions;
        struct table_properties;
        struct ExternalSstFileInfo;

        class WriteBatch;

        class environment_type;

        class EventListener;

        class StatsHistoryIterator;

        class TraceWriter;

#ifdef ROCKSDB_LITE
        class CompactionJobInfo;
#endif

        using std::unique_ptr;

        extern const std::string kDefaultColumnFamilyName;

        struct ColumnFamilyDescriptor {
            std::string name;
            ColumnFamilyOptions options;

            ColumnFamilyDescriptor() : name(kDefaultColumnFamilyName), options(ColumnFamilyOptions()) {
            }

            ColumnFamilyDescriptor(const std::string &_name, const ColumnFamilyOptions &_options) : name(_name),
                    options(_options) {
            }
        };

        class column_family_handle {
        public:
            virtual ~column_family_handle() {
            }

            // Returns the name of the column family associated with the current handle.
            virtual const std::string &GetName() const = 0;

            // Returns the ID of the column family associated with the current handle.
            virtual uint32_t get_id() const = 0;

            // Fills "*desc" with the up-to-date descriptor of the column family
            // associated with this handle. Since it fills "*desc" with the up-to-date
            // information, this call might internally lock and release DB mutex to
            // access the up-to-date CF options.  In addition, all the pointer-typed
            // options cannot be referenced any longer than the original options exist.
            //
            // Note that this function is not supported in RocksDBLite.
            virtual status_type GetDescriptor(ColumnFamilyDescriptor *desc) = 0;

            // Returns the comparator of the column family associated with the
            // current handle.
            virtual const Comparator *GetComparator() const = 0;
        };

        static const int kMajorVersion = __ROCKSDB_MAJOR__;
        static const int kMinorVersion = __ROCKSDB_MINOR__;

// A range of keys
        struct Range {
            slice start;
            slice limit;

            Range() {
            }

            Range(const slice &s, const slice &l) : start(s), limit(l) {
            }
        };

        struct RangePtr {
            const slice *start;
            const slice *limit;

            RangePtr() : start(nullptr), limit(nullptr) {
            }

            RangePtr(const slice *s, const slice *l) : start(s), limit(l) {
            }
        };

        struct IngestExternalFileArg {
            column_family_handle *column_family = nullptr;
            std::vector<std::string> external_files;
            IngestExternalFileOptions options;
        };

// A collections of table properties objects, where
//  key: is the table's file name.
//  value: the table properties object of the given table.
        typedef std::unordered_map<std::string, std::shared_ptr<const table_properties>> TablePropertiesCollection;

// A DB is a persistent ordered map from keys to values.
// A DB is safe for concurrent access from multiple threads without
// any external synchronization.
        class DB {
        public:
            // Open the database with the specified "name".
            // Stores a pointer to a heap-allocated database in *dbptr and returns
            // OK on success.
            // Stores nullptr in *dbptr and returns a non-OK status on error.
            // Caller should delete *dbptr when it is no longer needed.
            static status_type Open(const Options &options, const std::string &name, DB **dbptr);

            // Open the database for read only. All DB interfaces
            // that modify data, like put/delete, will return error.
            // If the db is opened in read only mode, then no compactions
            // will happen.
            //
            // Not supported in ROCKSDB_LITE, in which case the function will
            // return status_type::NotSupported.
            static status_type OpenForReadOnly(const Options &options, const std::string &name, DB **dbptr,
                                               bool error_if_log_file_exist = false);

            // Open the database for read only with column families. When opening DB with
            // read only, you can specify only a subset of column families in the
            // database that should be opened. However, you always need to specify default
            // column family. The default column family name is 'default' and it's stored
            // in nil::dcdb::kDefaultColumnFamilyName
            //
            // Not supported in ROCKSDB_LITE, in which case the function will
            // return status_type::NotSupported.
            static status_type OpenForReadOnly(const DBOptions &db_options, const std::string &name,
                                               const std::vector<ColumnFamilyDescriptor> &column_families,
                                               std::vector<column_family_handle *> *handles, DB **dbptr,
                                               bool error_if_log_file_exist = false);

            // Open DB with column families.
            // db_options specify database specific options
            // column_families is the vector of all column families in the database,
            // containing column family name and options. You need to open ALL column
            // families in the database. To get the list of column families, you can use
            // ListColumnFamilies(). Also, you can open only a subset of column families
            // for read-only access.
            // The default column family name is 'default' and it's stored
            // in nil::dcdb::kDefaultColumnFamilyName.
            // If everything is OK, handles will on return be the same size
            // as column_families --- handles[i] will be a handle that you
            // will use to operate on column family column_family[i].
            // Before delete DB, you have to close All column families by calling
            // DestroyColumnFamilyHandle() with all the handles.
            static status_type Open(const DBOptions &db_options, const std::string &name,
                                    const std::vector<ColumnFamilyDescriptor> &column_families,
                                    std::vector<column_family_handle *> *handles, DB **dbptr);

            virtual status_type Resume() {
                return status_type::NotSupported();
            }

            // Close the DB by releasing resources, closing files etc. This should be
            // called before calling the destructor so that the caller can get back a
            // status in case there are any errors. This will not fsync the WAL files.
            // If syncing is required, the caller must first call SyncWAL(), or Write()
            // using an empty write batch with WriteOptions.sync=true.
            // Regardless of the return status, the DB must be freed. If the return
            // status is NotSupported(), then the DB implementation does cleanup in the
            // destructor
            virtual status_type Close() {
                return status_type::NotSupported();
            }

            // ListColumnFamilies will open the DB specified by argument name
            // and return the list of all column families in that DB
            // through column_families argument. The ordering of
            // column families in column_families is unspecified.
            static status_type ListColumnFamilies(const DBOptions &db_options, const std::string &name,
                                                  std::vector<std::string> *column_families);

            DB() {
            }

            virtual ~DB();

            // Create a column_family and return the handle of column family
            // through the argument handle.
            virtual status_type CreateColumnFamily(const ColumnFamilyOptions &options,
                                                   const std::string &column_family_name,
                                                   column_family_handle **handle);

            // Bulk create column families with the same column family options.
            // Return the handles of the column families through the argument handles.
            // In case of error, the request may succeed partially, and handles will
            // contain column family handles that it managed to create, and have size
            // equal to the number of created column families.
            virtual status_type CreateColumnFamilies(const ColumnFamilyOptions &options,
                                                     const std::vector<std::string> &column_family_names,
                                                     std::vector<column_family_handle *> *handles);

            // Bulk create column families.
            // Return the handles of the column families through the argument handles.
            // In case of error, the request may succeed partially, and handles will
            // contain column family handles that it managed to create, and have size
            // equal to the number of created column families.
            virtual status_type CreateColumnFamilies(const std::vector<ColumnFamilyDescriptor> &column_families,
                                                     std::vector<column_family_handle *> *handles);

            // Drop a column family specified by column_family handle. This call
            // only records a drop record in the manifest and prevents the column
            // family from flushing and compacting.
            virtual status_type DropColumnFamily(column_family_handle *column_family);

            // Bulk drop column families. This call only records drop records in the
            // manifest and prevents the column families from flushing and compacting.
            // In case of error, the request may succeed partially. User may call
            // ListColumnFamilies to check the result.
            virtual status_type DropColumnFamilies(const std::vector<column_family_handle *> &column_families);

            // Close a column family specified by column_family handle and destroy
            // the column family handle specified to avoid double deletion. This call
            // deletes the column family handle by default. Use this method to
            // close column family instead of deleting column family handle directly
            virtual status_type DestroyColumnFamilyHandle(column_family_handle *column_family);

            // Set the database entry for "key" to "value".
            // If "key" already exists, it will be overwritten.
            // Returns OK on success, and a non-OK status on error.
            // Note: consider setting options.sync = true.
            virtual status_type Put(const WriteOptions &options, column_family_handle *column_family, const slice &key,
                                    const slice &value) = 0;

            virtual status_type Put(const WriteOptions &options, const slice &key, const slice &value) {
                return Put(options, DefaultColumnFamily(), key, value);
            }

            // remove the database entry (if any) for "key".  Returns OK on
            // success, and a non-OK status on error.  It is not an error if "key"
            // did not exist in the database.
            // Note: consider setting options.sync = true.
            virtual status_type Delete(const WriteOptions &options, column_family_handle *column_family,
                                       const slice &key) = 0;

            virtual status_type Delete(const WriteOptions &options, const slice &key) {
                return Delete(options, DefaultColumnFamily(), key);
            }

            // remove the database entry for "key". Requires that the key exists
            // and was not overwritten. Returns OK on success, and a non-OK status
            // on error.  It is not an error if "key" did not exist in the database.
            //
            // If a key is overwritten (by calling Put() multiple times), then the result
            // of calling SingleDelete() on this key is undefined.  SingleDelete() only
            // behaves correctly if there has been only one Put() for this key since the
            // previous call to SingleDelete() for this key.
            //
            // This feature is currently an experimental performance optimization
            // for a very specific workload.  It is up to the caller to ensure that
            // SingleDelete is only used for a key that is not deleted using Delete() or
            // written using Merge().  Mixing SingleDelete operations with Deletes and
            // Merges can result in undefined behavior.
            //
            // Note: consider setting options.sync = true.
            virtual status_type SingleDelete(const WriteOptions &options, column_family_handle *column_family,
                                             const slice &key) = 0;

            virtual status_type SingleDelete(const WriteOptions &options, const slice &key) {
                return SingleDelete(options, DefaultColumnFamily(), key);
            }

            // Removes the database entries in the range ["begin_key", "end_key"), i.e.,
            // including "begin_key" and excluding "end_key". Returns OK on success, and
            // a non-OK status on error. It is not an error if no keys exist in the range
            // ["begin_key", "end_key").
            //
            // This feature is now usable in production, with the following caveats:
            // 1) Accumulating many range tombstones in the memtable will degrade read
            // performance; this can be avoided by manually flushing occasionally.
            // 2) Limiting the maximum number of open files in the presence of range
            // tombstones can degrade read performance. To avoid this problem, set
            // max_open_files to -1 whenever possible.
            virtual status_type DeleteRange(const WriteOptions &options, column_family_handle *column_family,
                                            const slice &begin_key, const slice &end_key);

            // Merge the database entry for "key" with "value".  Returns OK on success,
            // and a non-OK status on error. The semantics of this operation is
            // determined by the user provided merge_operator when opening DB.
            // Note: consider setting options.sync = true.
            virtual status_type Merge(const WriteOptions &options, column_family_handle *column_family,
                                      const slice &key, const slice &value) = 0;

            virtual status_type Merge(const WriteOptions &options, const slice &key, const slice &value) {
                return Merge(options, DefaultColumnFamily(), key, value);
            }

            // Apply the specified updates to the database.
            // If `updates` contains no update, WAL will still be synced if
            // options.sync=true.
            // Returns OK on success, non-OK on failure.
            // Note: consider setting options.sync = true.
            virtual status_type Write(const WriteOptions &options, WriteBatch *updates) = 0;

            // If the database contains an entry for "key" store the
            // corresponding value in *value and return OK.
            //
            // If there is no entry for "key" leave *value unchanged and return
            // a status for which status_type::IsNotFound() returns true.
            //
            // May return some other status_type on an error.
            virtual inline status_type Get(const ReadOptions &options, column_family_handle *column_family,
                                           const slice &key, std::string *value) {
                assert(value != nullptr);
                PinnableSlice pinnable_val(value);
                assert(!pinnable_val.IsPinned());
                auto s = Get(options, column_family, key, &pinnable_val);
                if (s.ok() && pinnable_val.IsPinned()) {
                    value->assign(pinnable_val.data(), pinnable_val.size());
                }  // else value is already assigned
                return s;
            }

            virtual status_type Get(const ReadOptions &options, column_family_handle *column_family, const slice &key,
                                    PinnableSlice *value) = 0;

            virtual status_type Get(const ReadOptions &options, const slice &key, std::string *value) {
                return Get(options, DefaultColumnFamily(), key, value);
            }

            // If keys[i] does not exist in the database, then the i'th returned
            // status will be one for which status_type::IsNotFound() is true, and
            // (*values)[i] will be set to some arbitrary value (often ""). Otherwise,
            // the i'th returned status will have status_type::ok() true, and (*values)[i]
            // will store the value associated with keys[i].
            //
            // (*values) will always be resized to be the same size as (keys).
            // Similarly, the number of returned statuses will be the number of keys.
            // Note: keys will not be "de-duplicated". Duplicate keys will return
            // duplicate values in order.
            virtual std::vector<status_type> MultiGet(const ReadOptions &options,
                                                      const std::vector<column_family_handle *> &column_family,
                                                      const std::vector<slice> &keys,
                                                      std::vector<std::string> *values) = 0;

            virtual std::vector<status_type> MultiGet(const ReadOptions &options, const std::vector<slice> &keys,
                                                      std::vector<std::string> *values) {
                return MultiGet(options, std::vector<column_family_handle *>(keys.size(), DefaultColumnFamily()), keys,
                        values);
            }

            // If the key definitely does not exist in the database, then this method
            // returns false, else true. If the caller wants to obtain value when the key
            // is found in memory, a bool for 'value_found' must be passed. 'value_found'
            // will be true on return if value has been set properly.
            // This check is potentially lighter-weight than invoking DB::Get(). One way
            // to make this lighter weight is to avoid doing any IOs.
            // Default implementation here returns true and sets 'value_found' to false
            virtual bool KeyMayExist(const ReadOptions & /*options*/, column_family_handle * /*column_family*/,
                                     const slice & /*key*/, std::string * /*value*/, bool *value_found = nullptr) {
                if (value_found != nullptr) {
                    *value_found = false;
                }
                return true;
            }

            virtual bool KeyMayExist(const ReadOptions &options, const slice &key, std::string *value,
                                     bool *value_found = nullptr) {
                return KeyMayExist(options, DefaultColumnFamily(), key, value, value_found);
            }

            // Return a heap-allocated iterator over the contents of the database.
            // The result of NewIterator() is initially invalid (caller must
            // call one of the Seek methods on the iterator before using it).
            //
            // Caller should delete the iterator when it is no longer needed.
            // The returned iterator should be deleted before this db is deleted.
            virtual Iterator *NewIterator(const ReadOptions &options, column_family_handle *column_family) = 0;

            virtual Iterator *NewIterator(const ReadOptions &options) {
                return NewIterator(options, DefaultColumnFamily());
            }

            // Returns iterators from a consistent database state across multiple
            // column families. Iterators are heap allocated and need to be deleted
            // before the db is deleted
            virtual status_type NewIterators(const ReadOptions &options,
                                             const std::vector<column_family_handle *> &column_families,
                                             std::vector<Iterator *> *iterators) = 0;

            // Return a handle to the current DB state.  Iterators created with
            // this handle will all observe a stable snapshot of the current DB
            // state.  The caller must call ReleaseSnapshot(result) when the
            // snapshot is no longer needed.
            //
            // nullptr will be returned if the DB fails to take a snapshot or does
            // not support snapshot.
            virtual const Snapshot *GetSnapshot() = 0;

            // release a previously acquired snapshot.  The caller must not
            // use "snapshot" after this call.
            virtual void ReleaseSnapshot(const Snapshot *snapshot) = 0;

#ifndef ROCKSDB_LITE
            // Contains all valid property arguments for GetProperty().
            //
            // NOTE: Property names cannot end in numbers since those are interpreted as
            //       arguments, e.g., see kNumFilesAtLevelPrefix.
            struct Properties {
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
                //      the active and unflushed immutable memtables and storage.
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

                //  "rocksdb.oldest-snapshot-time" - returns number representing unix
                //      timestamp of oldest unreleased snapshot.
                static const std::string kOldestSnapshotTime;

                //  "rocksdb.num-live-versions" - returns number of live versions. `Version`
                //      is an internal data structure. See version_set.h for details. More
                //      live versions often mean more SST files are held from being deleted,
                //      by iterators or unfinished compactions.
                static const std::string kNumLiveVersions;

                //  "rocksdb.current-super-version-number" - returns number of current LSM
                //  version. It is a uint64_t integer number, incremented after there is
                //  any change to the LSM tree. The number is not preserved after restarting
                //  the DB. After DB restart, it will start from 0 again.
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
                //      oldest key timestamp in the DB. Currently only available for
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

                // "rocksdb.options-statistics" - returns multi-line string
                //      of options.statistics
                static const std::string kOptionsStatistics;
            };
#endif /* ROCKSDB_LITE */

            // DB implementations can export properties about their state via this method.
            // If "property" is a valid property understood by this DB implementation (see
            // Properties struct above for valid options), fills "*value" with its current
            // value and returns true.  Otherwise, returns false.
            virtual bool GetProperty(column_family_handle *column_family, const slice &property,
                                     std::string *value) = 0;

            virtual bool GetProperty(const slice &property, std::string *value) {
                return GetProperty(DefaultColumnFamily(), property, value);
            }

            virtual bool GetMapProperty(column_family_handle *column_family, const slice &property,
                                        std::map<std::string, std::string> *value) = 0;

            virtual bool GetMapProperty(const slice &property, std::map<std::string, std::string> *value) {
                return GetMapProperty(DefaultColumnFamily(), property, value);
            }

            // Similar to GetProperty(), but only works for a subset of properties whose
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
            //  "rocksdb.oldest-snapshot-time"
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
            virtual bool GetIntProperty(column_family_handle *column_family, const slice &property,
                                        uint64_t *value) = 0;

            virtual bool GetIntProperty(const slice &property, uint64_t *value) {
                return GetIntProperty(DefaultColumnFamily(), property, value);
            }

            // reset internal stats for DB and all column families.
            // Note this doesn't reset options.statistics as it is not owned by
            // DB.
            virtual status_type ResetStats() {
                return status_type::NotSupported("Not implemented");
            }

            // Same as GetIntProperty(), but this one returns the aggregated int
            // property from all column families.
            virtual bool GetAggregatedIntProperty(const slice &property, uint64_t *value) = 0;

            // flags for DB::GetSizeApproximation that specify whether memtable
            // stats should be included, or file stats approximation or both
            enum SizeApproximationFlags : uint8_t {
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
            // include_flags should be of type DB::SizeApproximationFlags
            virtual void GetApproximateSizes(column_family_handle *column_family, const Range *range, int n,
                                             uint64_t *sizes, uint8_t include_flags = INCLUDE_FILES) = 0;

            virtual void GetApproximateSizes(const Range *range, int n, uint64_t *sizes,
                                             uint8_t include_flags = INCLUDE_FILES) {
                GetApproximateSizes(DefaultColumnFamily(), range, n, sizes, include_flags);
            }

            // The method is similar to GetApproximateSizes, except it
            // returns approximate number of records in memtables.
            virtual void GetApproximateMemTableStats(column_family_handle *column_family, const Range &range,
                                                     uint64_t *const count, uint64_t *const size) = 0;

            virtual void GetApproximateMemTableStats(const Range &range, uint64_t *const count, uint64_t *const size) {
                GetApproximateMemTableStats(DefaultColumnFamily(), range, count, size);
            }

            // Deprecated versions of GetApproximateSizes
            ROCKSDB_DEPRECATED_FUNC virtual void GetApproximateSizes(const Range *range, int n, uint64_t *sizes,
                                                                     bool include_memtable) {
                uint8_t include_flags = SizeApproximationFlags::INCLUDE_FILES;
                if (include_memtable) {
                    include_flags |= SizeApproximationFlags::INCLUDE_MEMTABLES;
                }
                GetApproximateSizes(DefaultColumnFamily(), range, n, sizes, include_flags);
            }

            ROCKSDB_DEPRECATED_FUNC virtual void GetApproximateSizes(column_family_handle *column_family,
                                                                     const Range *range, int n, uint64_t *sizes,
                                                                     bool include_memtable) {
                uint8_t include_flags = SizeApproximationFlags::INCLUDE_FILES;
                if (include_memtable) {
                    include_flags |= SizeApproximationFlags::INCLUDE_MEMTABLES;
                }
                GetApproximateSizes(column_family, range, n, sizes, include_flags);
            }

            // Compact the underlying storage for the key range [*begin,*end].
            // The actual compaction interval might be superset of [*begin, *end].
            // In particular, deleted and overwritten versions are discarded,
            // and the data is rearranged to reduce the cost of operations
            // needed to access the data.  This operation should typically only
            // be invoked by users who understand the underlying implementation.
            //
            // begin==nullptr is treated as a key before all keys in the database.
            // end==nullptr is treated as a key after all keys in the database.
            // Therefore the following call will compact the entire database:
            //    db->CompactRange(options, nullptr, nullptr);
            // Note that after the entire database is compacted, all data are pushed
            // down to the last level containing any data. If the total data size after
            // compaction is reduced, that level might not be appropriate for hosting all
            // the files. In this case, client could set options.change_level to true, to
            // move the files back to the minimum level capable of holding the data set
            // or a given level (specified by non-negative options.target_level).
            virtual status_type CompactRange(const CompactRangeOptions &options, column_family_handle *column_family,
                                             const slice *begin, const slice *end) = 0;

            virtual status_type CompactRange(const CompactRangeOptions &options, const slice *begin, const slice *end) {
                return CompactRange(options, DefaultColumnFamily(), begin, end);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type CompactRange(column_family_handle *column_family,
                                                                     const slice *begin, const slice *end,
                                                                     bool change_level = false, int target_level = -1,
                                                                     uint32_t target_path_id = 0) {
                CompactRangeOptions options;
                options.change_level = change_level;
                options.target_level = target_level;
                options.target_path_id = target_path_id;
                return CompactRange(options, column_family, begin, end);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type CompactRange(const slice *begin, const slice *end,
                                                                     bool change_level = false, int target_level = -1,
                                                                     uint32_t target_path_id = 0) {
                CompactRangeOptions options;
                options.change_level = change_level;
                options.target_level = target_level;
                options.target_path_id = target_path_id;
                return CompactRange(options, DefaultColumnFamily(), begin, end);
            }

            virtual status_type SetOptions(column_family_handle * /*column_family*/,
                                           const std::unordered_map<std::string, std::string> & /*new_options*/) {
                return status_type::NotSupported("Not implemented");
            }

            virtual status_type SetOptions(const std::unordered_map<std::string, std::string> &new_options) {
                return SetOptions(DefaultColumnFamily(), new_options);
            }

            virtual status_type SetDBOptions(const std::unordered_map<std::string, std::string> &new_options) = 0;

            // CompactFiles() inputs a list of files specified by file numbers and
            // compacts them to the specified level. Note that the behavior is different
            // from CompactRange() in that CompactFiles() performs the compaction job
            // using the CURRENT thread.
            //
            // @see GetDataBaseMetaData
            // @see GetColumnFamilyMetaData
            virtual status_type CompactFiles(const CompactionOptions &compact_options,
                                             column_family_handle *column_family,
                                             const std::vector<std::string> &input_file_names, const int output_level,
                                             const int output_path_id = -1,
                                             std::vector<std::string> *const output_file_names = nullptr,
                                             CompactionJobInfo *compaction_job_info = nullptr) = 0;

            virtual status_type CompactFiles(const CompactionOptions &compact_options,
                                             const std::vector<std::string> &input_file_names, const int output_level,
                                             const int output_path_id = -1,
                                             std::vector<std::string> *const output_file_names = nullptr,
                                             CompactionJobInfo *compaction_job_info = nullptr) {
                return CompactFiles(compact_options, DefaultColumnFamily(), input_file_names, output_level,
                        output_path_id, output_file_names, compaction_job_info);
            }

            // This function will wait until all currently running background processes
            // finish. After it returns, no background process will be run until
            // ContinueBackgroundWork is called
            virtual status_type PauseBackgroundWork() = 0;

            virtual status_type ContinueBackgroundWork() = 0;

            // This function will enable automatic compactions for the given column
            // families if they were previously disabled. The function will first set the
            // disable_auto_compactions option for each column family to 'false', after
            // which it will schedule a flush/compaction.
            //
            // NOTE: Setting disable_auto_compactions to 'false' through SetOptions() API
            // does NOT schedule a flush/compaction afterwards, and only changes the
            // parameter itself within the column family option.
            //
            virtual status_type EnableAutoCompaction(
                    const std::vector<column_family_handle *> &column_family_handles) = 0;

            // Number of levels used for this DB.
            virtual int NumberLevels(column_family_handle *column_family) = 0;

            virtual int NumberLevels() {
                return NumberLevels(DefaultColumnFamily());
            }

            // Maximum level to which a new compacted memtable is pushed if it
            // does not create overlap.
            virtual int MaxMemCompactionLevel(column_family_handle *column_family) = 0;

            virtual int MaxMemCompactionLevel() {
                return MaxMemCompactionLevel(DefaultColumnFamily());
            }

            // Number of files in level-0 that would stop writes.
            virtual int Level0StopWriteTrigger(column_family_handle *column_family) = 0;

            virtual int Level0StopWriteTrigger() {
                return Level0StopWriteTrigger(DefaultColumnFamily());
            }

            // Get DB name -- the exact same name that was provided as an argument to
            // DB::Open()
            virtual const std::string &GetName() const = 0;

            // Get environment_type object from the DB
            virtual environment_type *GetEnv() const = 0;

            // Get DB Options that we use.  During the process of opening the
            // column family, the options provided when calling DB::Open() or
            // DB::CreateColumnFamily() will have been "sanitized" and transformed
            // in an implementation-defined manner.
            virtual Options GetOptions(column_family_handle *column_family) const = 0;

            virtual Options GetOptions() const {
                return GetOptions(DefaultColumnFamily());
            }

            virtual DBOptions GetDBOptions() const = 0;

            // Flush all mem-table data.
            // Flush a single column family, even when atomic flush is enabled. To flush
            // multiple column families, use Flush(options, column_families).
            virtual status_type Flush(const FlushOptions &options, column_family_handle *column_family) = 0;

            virtual status_type Flush(const FlushOptions &options) {
                return Flush(options, DefaultColumnFamily());
            }

            // Flushes multiple column families.
            // If atomic flush is not enabled, Flush(options, column_families) is
            // equivalent to calling Flush(options, column_family) multiple times.
            // If atomic flush is enabled, Flush(options, column_families) will flush all
            // column families specified in 'column_families' up to the latest sequence
            // number at the time when flush is requested.
            // Note that RocksDB 5.15 and earlier may not be able to open later versions
            // with atomic flush enabled.
            virtual status_type Flush(const FlushOptions &options,
                                      const std::vector<column_family_handle *> &column_families) = 0;

            // Flush the WAL memory buffer to the file. If sync is true, it calls SyncWAL
            // afterwards.
            virtual status_type FlushWAL(bool /*sync*/) {
                return status_type::NotSupported("FlushWAL not implemented");
            }

            // Sync the wal. Note that Write() followed by SyncWAL() is not exactly the
            // same as Write() with sync=true: in the latter case the changes won't be
            // visible until the sync is done.
            // Currently only works if allow_mmap_writes = false in Options.
            virtual status_type SyncWAL() = 0;

            // The sequence number of the most recent transaction.
            virtual SequenceNumber GetLatestSequenceNumber() const = 0;

            // Instructs DB to preserve deletes with sequence numbers >= passed seqnum.
            // Has no effect if DBOptions.preserve_deletes is set to false.
            // This function assumes that user calls this function with monotonically
            // increasing seqnums (otherwise we can't guarantee that a particular delete
            // hasn't been already processed); returns true if the value was successfully
            // updated, false if user attempted to call if with seqnum <= current value.
            virtual bool SetPreserveDeletesSequenceNumber(SequenceNumber seqnum) = 0;

#ifndef ROCKSDB_LITE

            // Prevent file deletions. Compactions will continue to occur,
            // but no obsolete files will be deleted. Calling this multiple
            // times have the same effect as calling it once.
            virtual status_type DisableFileDeletions() = 0;

            // Allow compactions to delete obsolete files.
            // If force == true, the call to EnableFileDeletions() will guarantee that
            // file deletions are enabled after the call, even if DisableFileDeletions()
            // was called multiple times before.
            // If force == false, EnableFileDeletions will only enable file deletion
            // after it's been called at least as many times as DisableFileDeletions(),
            // enabling the two methods to be called by two threads concurrently without
            // synchronization -- i.e., file deletions will be enabled only after both
            // threads call EnableFileDeletions()
            virtual status_type EnableFileDeletions(bool force = true) = 0;

            // GetLiveFiles followed by GetSortedWalFiles can generate a lossless backup

            // Retrieve the list of all files in the database. The files are
            // relative to the dbname and are not absolute paths. Despite being relative
            // paths, the file names begin with "/". The valid size of the manifest file
            // is returned in manifest_file_size. The manifest file is an ever growing
            // file, but only the portion specified by manifest_file_size is valid for
            // this snapshot. Setting flush_memtable to true does Flush before recording
            // the live files. Setting flush_memtable to false is useful when we don't
            // want to wait for flush which may have to wait for compaction to complete
            // taking an indeterminate time.
            //
            // In case you have multiple column families, even if flush_memtable is true,
            // you still need to call GetSortedWalFiles after GetLiveFiles to compensate
            // for new data that arrived to already-flushed column families while other
            // column families were flushing
            virtual status_type GetLiveFiles(std::vector<std::string> &, uint64_t *manifest_file_size,
                                             bool flush_memtable = true) = 0;

            // Retrieve the sorted list of all wal files with earliest file first
            virtual status_type GetSortedWalFiles(VectorLogPtr &files) = 0;

            // Note: this API is not yet consistent with WritePrepared transactions.
            // Sets iter to an iterator that is positioned at a write-batch containing
            // seq_number. If the sequence number is non existent, it returns an iterator
            // at the first available seq_no after the requested seq_no
            // Returns status_type::OK if iterator is valid
            // Must set WAL_ttl_seconds or WAL_size_limit_MB to large values to
            // use this api, else the WAL files will get
            // cleared aggressively and the iterator might keep getting invalid before
            // an update is read.
            virtual status_type GetUpdatesSince(SequenceNumber seq_number,
                                                std::unique_ptr<TransactionLogIterator> *iter,
                                                const TransactionLogIterator::ReadOptions &read_options = TransactionLogIterator::ReadOptions()) = 0;

// Windows API macro interference
#undef DeleteFile

            // Delete the file name from the db directory and update the internal state to
            // reflect that. Supports deletion of sst and log files only. 'name' must be
            // path relative to the db directory. eg. 000001.sst, /archive/000003.log
            virtual status_type DeleteFile(std::string name) = 0;

            // Returns a list of all table files with their level, start key
            // and end key
            virtual void GetLiveFilesMetaData(std::vector<LiveFileMetaData> * /*metadata*/) {
            }

            // Obtains the meta data of the specified column family of the DB.
            virtual void GetColumnFamilyMetaData(column_family_handle * /*column_family*/,
                                                 ColumnFamilyMetaData * /*metadata*/) {
            }

            // Get the metadata of the default column family.
            void GetColumnFamilyMetaData(ColumnFamilyMetaData *metadata) {
                GetColumnFamilyMetaData(DefaultColumnFamily(), metadata);
            }

            // IngestExternalFile() will load a list of external SST files (1) into the DB
            // Two primary modes are supported:
            // - Duplicate keys in the new files will overwrite exiting keys (default)
            // - Duplicate keys will be skipped (set ingest_behind=true)
            // In the first mode we will try to find the lowest possible level that
            // the file can fit in, and ingest the file into this level (2). A file that
            // have a key range that overlap with the memtable key range will require us
            // to Flush the memtable first before ingesting the file.
            // In the second mode we will always ingest in the bottom most level (see
            // docs to IngestExternalFileOptions::ingest_behind).
            //
            // (1) External SST files can be created using SstFileWriter
            // (2) We will try to ingest the files to the lowest possible level
            //     even if the file compression doesn't match the level compression
            // (3) If IngestExternalFileOptions->ingest_behind is set to true,
            //     we always ingest at the bottommost level, which should be reserved
            //     for this purpose (see DBOPtions::allow_ingest_behind flag).
            virtual status_type IngestExternalFile(column_family_handle *column_family,
                                                   const std::vector<std::string> &external_files,
                                                   const IngestExternalFileOptions &options) = 0;

            virtual status_type IngestExternalFile(const std::vector<std::string> &external_files,
                                                   const IngestExternalFileOptions &options) {
                return IngestExternalFile(DefaultColumnFamily(), external_files, options);
            }

            // IngestExternalFiles() will ingest files for multiple column families, and
            // record the result atomically to the MANIFEST.
            // If this function returns OK, all column families' ingestion must succeed.
            // If this function returns NOK, or the process crashes, then non-of the
            // files will be ingested into the database after recovery.
            // Note that it is possible for application to observe a mixed state during
            // the execution of this function. If the user performs range scan over the
            // column families with iterators, iterator on one column family may return
            // ingested data, while iterator on other column family returns old data.
            // Users can use snapshot for a consistent view of data.
            // If your db ingests multiple SST files using this API, i.e. args.size()
            // > 1, then RocksDB 5.15 and earlier will not be able to open it.
            //
            // REQUIRES: each arg corresponds to a different column family: namely, for
            // 0 <= i < j < len(args), args[i].column_family != args[j].column_family.
            virtual status_type IngestExternalFiles(const std::vector<IngestExternalFileArg> &args) = 0;

            virtual status_type VerifyChecksum() = 0;

            // add_file() is deprecated, please use IngestExternalFile()
            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(column_family_handle *column_family,
                                                                const std::vector<std::string> &file_path_list,
                                                                bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(column_family, file_path_list, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(const std::vector<std::string> &file_path_list,
                                                                bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(DefaultColumnFamily(), file_path_list, ifo);
            }

            // add_file() is deprecated, please use IngestExternalFile()
            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(column_family_handle *column_family,
                                                                const std::string &file_path, bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(column_family, {file_path}, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(const std::string &file_path, bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(DefaultColumnFamily(), {file_path}, ifo);
            }

            // Load table file with information "file_info" into "column_family"
            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(column_family_handle *column_family,
                                                                const std::vector<ExternalSstFileInfo> &file_info_list,
                                                                bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                std::vector<std::string> external_files;
                for (const ExternalSstFileInfo &file_info : file_info_list) {
                    external_files.push_back(file_info.file_path);
                }
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(column_family, external_files, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(const std::vector<ExternalSstFileInfo> &file_info_list,
                                                                bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                std::vector<std::string> external_files;
                for (const ExternalSstFileInfo &file_info : file_info_list) {
                    external_files.push_back(file_info.file_path);
                }
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(DefaultColumnFamily(), external_files, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(column_family_handle *column_family,
                                                                const ExternalSstFileInfo *file_info,
                                                                bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(column_family, {file_info->file_path}, ifo);
            }

            ROCKSDB_DEPRECATED_FUNC virtual status_type AddFile(const ExternalSstFileInfo *file_info,
                                                                bool move_file = false,
                                                                bool skip_snapshot_check = false) {
                IngestExternalFileOptions ifo;
                ifo.move_files = move_file;
                ifo.snapshot_consistency = !skip_snapshot_check;
                ifo.allow_global_seqno = false;
                ifo.allow_blocking_flush = false;
                return IngestExternalFile(DefaultColumnFamily(), {file_info->file_path}, ifo);
            }

#endif  // ROCKSDB_LITE

            // Sets the globally unique ID created at database creation time by invoking
            // environment_type::GenerateUniqueId(), in identity. Returns status_type::OK if identity could
            // be set properly
            virtual status_type GetDbIdentity(std::string &identity) const = 0;

            // Returns default column family handle
            virtual column_family_handle *DefaultColumnFamily() const = 0;

#ifndef ROCKSDB_LITE

            virtual status_type GetPropertiesOfAllTables(column_family_handle *column_family,
                                                         TablePropertiesCollection *props) = 0;

            virtual status_type GetPropertiesOfAllTables(TablePropertiesCollection *props) {
                return GetPropertiesOfAllTables(DefaultColumnFamily(), props);
            }

            virtual status_type GetPropertiesOfTablesInRange(column_family_handle *column_family, const Range *range,
                                                             std::size_t n, TablePropertiesCollection *props) = 0;

            virtual status_type SuggestCompactRange(column_family_handle * /*column_family*/, const slice * /*begin*/,
                                                    const slice * /*end*/) {
                return status_type::NotSupported("SuggestCompactRange() is not implemented.");
            }

            virtual status_type PromoteL0(column_family_handle * /*column_family*/, int /*target_level*/) {
                return status_type::NotSupported("PromoteL0() is not implemented.");
            }

            // Trace DB operations. Use EndTrace() to stop tracing.
            virtual status_type StartTrace(const TraceOptions & /*options*/,
                                           std::unique_ptr<TraceWriter> && /*trace_writer*/) {
                return status_type::NotSupported("StartTrace() is not implemented.");
            }

            virtual status_type EndTrace() {
                return status_type::NotSupported("EndTrace() is not implemented.");
            }

#endif  // ROCKSDB_LITE

            // Needed for StackableDB
            virtual DB *GetRootDB() {
                return this;
            }

            // Given a time window, return an iterator for accessing stats history
            // User is responsible for deleting StatsHistoryIterator after use
            virtual status_type GetStatsHistory(uint64_t /*start_time*/, uint64_t /*end_time*/,
                                                std::unique_ptr<StatsHistoryIterator> * /*stats_iterator*/) {
                return status_type::NotSupported("GetStatsHistory() is not implemented.");
            }

        private:
            // No copying allowed
            DB(const DB &);

            void operator=(const DB &);
        };

// Destroy the contents of the specified database.
// Be very careful using this method.
        status_type DestroyDB(const std::string &name, const Options &options,
                              const std::vector<ColumnFamilyDescriptor> &column_families = std::vector<
                                      ColumnFamilyDescriptor>());

#ifndef ROCKSDB_LITE

// If a DB cannot be opened, you may attempt to call this method to
// resurrect as much of the contents of the database as possible.
// Some data may be lost, so be careful when calling this function
// on a database that contains important information.
//
// With this API, we will warn and skip data associated with column families not
// specified in column_families.
//
// @param column_families Descriptors for known column families
        status_type RepairDB(const std::string &dbname, const DBOptions &db_options,
                             const std::vector<ColumnFamilyDescriptor> &column_families);

// @param unknown_cf_opts Options for column families encountered during the
//                        repair that were not specified in column_families.
        status_type RepairDB(const std::string &dbname, const DBOptions &db_options,
                             const std::vector<ColumnFamilyDescriptor> &column_families,
                             const ColumnFamilyOptions &unknown_cf_opts);

// @param options These options will be used for the database and for ALL column
//                families encountered during the repair
        status_type RepairDB(const std::string &dbname, const Options &options);

#endif

    }
} // namespace nil
