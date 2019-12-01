//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_DATABASE_HPP
#define STORAGE_ENGINE_DATABASE_HPP

#include <nil/storage/engine/slice.hpp>
#include <nil/storage/engine/range.hpp>
#include <nil/storage/engine/iterator.hpp>
#include <nil/storage/engine/options/read_options.hpp>
#include <nil/storage/engine/options/write_options.hpp>
#include <nil/storage/engine/write_batch.hpp>

#include <nil/storage/engine/column_family_handle.hpp>

namespace nil {
    namespace engine {
        class database {
        public:
            virtual ~database() = default;

            virtual column_family_handle *default_column_family() const = 0;

            // create a column_family and return the handle of column family
            // through the argument handle.
            virtual status_type create_column_family(const column_family_options &options,
                                                     const std::string &column_family_name,
                                                     column_family_handle **handle) = 0;

            // Bulk create column families with the same column family opts.
            // Return the handles of the column families through the argument handles.
            // In case of error, the request may succeed partially, and handles will
            // contain column family handles that it managed to create, and have size
            // equal to the number of created column families.
            virtual status_type create_column_families(const column_family_options &options,
                                                       const std::vector<std::string> &column_family_names,
                                                       std::vector<column_family_handle *> *handles) = 0;

            // Bulk create column families.
            // Return the handles of the column families through the argument handles.
            // In case of error, the request may succeed partially, and handles will
            // contain column family handles that it managed to create, and have size
            // equal to the number of created column families.
            virtual status_type create_column_families(const std::vector<column_family_descriptor> &column_families,
                                                       std::vector<column_family_handle *> *handles) = 0;

            // Drop a column family specified by column_family handle. This call
            // only records a drop record in the manifest and prevents the column
            // family from flushing and compacting.
            virtual status_type drop_column_family(column_family_handle *column_family) = 0;

            // Bulk drop column families. This call only records drop records in the
            // manifest and prevents the column families from flushing and compacting.
            // In case of error, the request may succeed partially. User may call
            // list_column_families to check the result.
            virtual status_type drop_column_families(const std::vector<column_family_handle *> &column_families) = 0;

            // close a column family specified by column_family handle and destroy
            // the column family handle specified to avoid double deletion. This call
            // deletes the column family handle by default. Use this method to
            // close column family instead of deleting column family handle directly
            virtual status_type destroy_column_family_handle(column_family_handle *column_family) = 0;

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

            // apply the specified updates to the database.
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
                                           const slice &key, std::string *value) = 0;

            virtual status_type get(const read_options &options, const slice &key, std::string *value) {
                return get(options, default_column_family(), key, value);
            }

            // If keys[i] does not exist in the database, then the i'th returned
            // status will be one for which status_type::is_not_found() is true, and
            // (*values)[i] will be set to some arbitrary value (often ""). Otherwise,
            // the i'th returned status will have status_type::ok() true, and (*values)[i]
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

            virtual std::vector<status_type> multi_get(const read_options &options,
                                                       const std::vector<slice> &keys,
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
            virtual iterator *new_iterator(const read_options &options,
                                           engine::column_family_handle *column_family) = 0;

            virtual iterator *new_iterator(const read_options &options) {
                return new_iterator(options, default_column_family());
            }

            // Returns iterators from a consistent database state across multiple
            // column families. Iterators are heap allocated and need to be deleted
            // before the db is deleted
            virtual engine::status_type
                new_iterators(const read_options &options,
                              const std::vector<engine::column_family_handle *> &column_families,
                              std::vector<iterator *> *iterators) = 0;
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_DATABASE_HPP
