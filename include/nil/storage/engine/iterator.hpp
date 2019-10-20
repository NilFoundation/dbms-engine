//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

// An iterator yields a sequence of key/value pairs from a source.
// The following class defines the interface.  Multiple implementations
// are provided by this library.  In particular, iterators are provided
// to access the contents of a Table or a database.
//
// Multiple threads can invoke const methods on an iterator without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same iterator must use
// external synchronization.

#pragma once

#include <string>

#include <nil/storage/engine/slice.hpp>
#include <nil/storage/engine/status.hpp>

namespace nil {
    namespace engine {

        class iterator {
        public:
            iterator() {
            }

            virtual ~iterator() {
            }

            // An iterator is either positioned at a key/value pair, or
            // not valid.  This method returns true iff the iterator is valid.
            // Always returns false if !status().is_ok().
            virtual bool valid() const = 0;

            // Position at the first key in the source.  The iterator is valid()
            // after this call iff the source is not empty.
            virtual void seek_to_first() = 0;

            // Position at the last key in the source.  The iterator is
            // valid() after this call iff the source is not empty.
            virtual void seek_to_last() = 0;

            // Position at the first key in the source that at or past target.
            // The iterator is valid() after this call iff the source contains
            // an entry that comes at or past target.
            // All seek*() methods clear any error status() that the iterator had prior to
            // the call; after the seek, status() indicates only the error (if any) that
            // happened during the seek, not any past errors.
            virtual void seek(const engine::slice &target) = 0;

            // Position at the last key in the source that at or before target.
            // The iterator is valid() after this call iff the source contains
            // an entry that comes at or before target.
            virtual void seek_for_prev(const engine::slice &target) = 0;

            // Moves to the next entry in the source.  After this call, valid() is
            // true iff the iterator was not positioned at the last entry in the source.
            // REQUIRES: valid()
            virtual void next() = 0;

            // Moves to the previous entry in the source.  After this call, valid() is
            // true iff the iterator was not positioned at the first entry in source.
            // REQUIRES: valid()
            virtual void prev() = 0;

            // Return the key for the current entry.  The underlying engine for
            // the returned engine::slice is valid only until the next modification of
            // the iterator.
            // REQUIRES: valid()
            virtual engine::slice key() const = 0;

            // Return the value for the current entry.  The underlying engine for
            // the returned engine::slice is valid only until the next modification of
            // the iterator.
            // REQUIRES: valid()
            virtual engine::slice value() const = 0;

            // If an error has occurred, return it.  Else return an is_ok status.
            // If non-blocking IO is requested and this operation cannot be
            // satisfied without doing some IO, then this returns engine::status_type::incomplete().
            virtual engine::status_type status() const = 0;

            // If supported, renew the iterator to represent the latest state. The
            // iterator will be invalidated after the call. Not supported if
            // read_options.get_snapshot is given when creating the iterator.
            virtual engine::status_type refresh() {
                return engine::status_type::not_supported("refresh() is not supported");
            }

            // Property "rocksdb.iterator.is-key-pinned":
            //   If returning "1", this means that the engine::slice returned by key() is valid
            //   as long as the iterator is not deleted.
            //   It is guaranteed to always return "1" if
            //      - iterator created with read_options::pin_data = true
            //      - database tables were created with
            //        block_based_table_options::use_delta_encoding = false.
            // Property "rocksdb.iterator.super-version-number":
            //   LSM version used by the iterator. The same format as database Property
            //   kCurrentSuperVersionNumber. See its comment for more information.
            // Property "rocksdb.iterator.internal-key":
            //   get the user-key portion of the internal key at which the iteration
            //   stopped.
            virtual engine::status_type get_property(const std::string &prop_name, std::string *prop) {
                if (prop == nullptr) {
                    return engine::status_type::invalid_argument("prop is nullptr");
                }
                if (prop_name == "rocksdb.iterator.is-key-pinned") {
                    *prop = "0";
                    return engine::status_type();
                }
                return engine::status_type::invalid_argument("Unidentified property.");
            }

        private:
            // No copying allowed
            iterator(const iterator &);

            void operator=(const iterator &);
        };
    }    // namespace dcdb
}    // namespace nil
