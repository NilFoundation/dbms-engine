//
// A write_batch_with_index with a binary searchable index built for all the keys
// inserted.
#pragma once

#include <memory>
#include <string>

#include <nil/engine/comparator.hpp>
#include <nil/engine/iterator.hpp>
#include <nil/engine/slice.hpp>
#include <nil/engine/status.hpp>
#include <nil/engine/write_batch.hpp>

namespace nil {
    namespace engine {

        class column_family_handle;

        class comparator;

        class database;

        class read_callback;

        struct read_options;
        struct db_options;

        enum write_type {
            kPutRecord,
            kMergeRecord,
            kDeleteRecord,
            kSingleDeleteRecord,
            kDeleteRangeRecord,
            kLogDataRecord,
            kXIDRecord,
        };

        // an entry for insert, merge, remove, or single_remove entry for write batches.
        // Used in write_batch_with_index_iterator.
        struct write_entry {
            write_type type;
            engine::slice key;
            engine::slice value;
        };

        // iterator of one column family out of a write_batch_with_index.
        class write_batch_with_index_iterator {
        public:
            virtual ~write_batch_with_index_iterator() {
            }

            virtual bool valid() const = 0;

            virtual void seek_to_first() = 0;

            virtual void seek_to_last() = 0;

            virtual void seek(const engine::slice &key) = 0;

            virtual void seek_for_prev(const engine::slice &key) = 0;

            virtual void next() = 0;

            virtual void prev() = 0;

            // the return write_entry is only valid until the next mutation of
            // write_batch_with_index
            virtual write_entry entry() const = 0;

            virtual engine::status_type status() const = 0;
        };

        // A write_batch_with_index with a binary searchable index built for all the keys
        // inserted.
        // In insert(), merge() remove(), or single_remove(), the same function of the
        // wrapped will be called. At the same time, indexes will be built.
        // By calling get_write_batch(), a user will get the write_batch for the data
        // they inserted, which can be used for database::write().
        // A user can call new_iterator() to create an iterator.
        class write_batch_with_index : public engine::write_batch {
        public:
            using engine::write_batch::insert;

            virtual engine::status_type insert(column_family_handle *column_family, const engine::slice &key,
                                               const engine::slice &value) override = 0;

            virtual engine::status_type insert(const engine::slice &key, const engine::slice &value) override = 0;

            using engine::write_batch::merge;

            virtual engine::status_type merge(column_family_handle *column_family, const engine::slice &key,
                                              const engine::slice &value) override = 0;

            virtual engine::status_type merge(const engine::slice &key, const engine::slice &value) override = 0;

            using engine::write_batch::remove;

            virtual engine::status_type remove(column_family_handle *column_family,
                                               const engine::slice &key) override = 0;

            virtual engine::status_type remove(const engine::slice &key) override = 0;

            using engine::write_batch::single_remove;

            virtual engine::status_type single_remove(column_family_handle *column_family,
                                                      const engine::slice &key) override = 0;

            virtual engine::status_type single_remove(const engine::slice &key) override = 0;

            using engine::write_batch::remove_range;

            virtual engine::status_type remove_range(column_family_handle *column_family,
                                                     const engine::slice &begin_key,
                                                     const engine::slice &end_key) override = 0;

            virtual engine::status_type remove_range(const engine::slice &begin_key,
                                                     const engine::slice &end_key) override = 0;

            using engine::write_batch::put_log_data;

            virtual engine::status_type put_log_data(const engine::slice &blob) override = 0;

            using engine::write_batch::clear;

            virtual void clear() override = 0;

            virtual write_batch *get_write_batch() = 0;

            // Create an iterator of a column family. User can call iterator.seek() to
            // search to the next entry of or after a key. Keys will be iterated in the
            // order given by index_comparator. For multiple updates on the same key,
            // each update will be returned as a separate entry, in the order of update
            // time.
            //
            // The returned iterator should be deleted by the caller.
            virtual write_batch_with_index_iterator *new_iterator(column_family_handle *column_family) = 0;

            // Create an iterator of the default column family.
            virtual write_batch_with_index_iterator *new_iterator() = 0;

            // Will create a new iterator that will use write_batch_with_index_iterator as a delta and
            // base_iterator as base.
            //
            // This function is only supported if the write_batch_with_index was
            // constructed with overwrite_key=true.
            //
            // The returned iterator should be deleted by the caller.
            // The base_iterator is now 'owned' by the returned iterator. Deleting the
            // returned iterator will also delete the base_iterator.
            //
            // Updating write batch with the current key of the iterator is not safe.
            // We strongly recommand users not to do it. It will invalidate the current
            // key() and value() of the iterator. This invalidation happens even before
            // the write batch update finishes. The state may recover after next() is
            // called.
            virtual engine::iterator *new_iterator_with_base(column_family_handle *column_family,
                                                             engine::iterator *base_iterator) = 0;

            // default column family
            virtual engine::iterator *new_iterator_with_base(engine::iterator *base_iterator) = 0;

            // Similar to database::get() but will only read the key from this batch.
            // If the batch does not have enough data to resolve merge operations,
            // merge_in_progress status may be returned.
            virtual engine::status_type get_from_batch(column_family_handle *column_family, const db_options &options,
                                                       const engine::slice &key, std::string *value) = 0;

            // Similar to previous function but does not require a column_family.
            // Note:  An invalid_argument status will be returned if there are any merge
            // operators for this key.  Use previous method instead.
            virtual engine::status_type get_from_batch(const db_options &options, const engine::slice &key,
                                                       std::string *value) {
                return get_from_batch(nullptr, options, key, value);
            }

            // Similar to database::get() but will also read writes from this batch.
            //
            // This function will query both this batch and the database and then merge
            // the results using the database's merge operator (if the batch contains any
            // merge requests).
            //
            // Setting read_options.get_snapshot will affect what is read from the database
            // but will NOT change which keys are read from the batch (the keys in
            // this batch do not yet belong to any get_snapshot and will be fetched
            // regardless).
            virtual engine::status_type get_from_batch_and_db(database *db, const read_options &input_read_options,
                                                              const engine::slice &key, std::string *value) = 0;

            virtual engine::status_type get_from_batch_and_db(database *db, const read_options &input_read_options,
                                                              column_family_handle *column_family,
                                                              const engine::slice &key, std::string *value) = 0;

            // Records the state of the batch for future calls to rollback_to_save_point().
            // May be called multiple times to set multiple save points.
            virtual void set_save_point() override = 0;

            // remove all entries in this batch (insert, merge, remove, single_remove,
            // put_log_data) since the most recent call to set_save_point() and removes the
            // most recent save point.
            // If there is no previous call to set_save_point(), behaves the same as
            // clear().
            //
            // Calling rollback_to_save_point invalidates any open iterators on this batch.
            //
            // Returns engine::status_type::ok() on success,
            //         engine::status_type::not_found() if no previous call to set_save_point(),
            //         or other engine::status_type on corruption.
            virtual engine::status_type rollback_to_save_point() override = 0;

            // Pop the most recent save point.
            // If there is no previous call to set_save_point(), engine::status_type::not_found()
            // will be returned.
            // Otherwise returns engine::status_type::ok().
            virtual engine::status_type pop_save_point() override = 0;

            virtual void set_max_bytes(size_t max_bytes) override = 0;

            virtual std::size_t get_data_size() const = 0;
        };
    }    // namespace engine
}    // namespace nil
