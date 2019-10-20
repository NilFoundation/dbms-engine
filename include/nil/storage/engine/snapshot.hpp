//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <nil/storage/engine/types.hpp>
#include <nil/storage/engine/database.hpp>

namespace nil {
    namespace engine {

        // Abstract handle to particular state of a database.
        // A get_snapshot is an immutable object and can therefore be safely
        // accessed from multiple threads without any external synchronization.
        //
        // To create a get_snapshot, call database::get_snapshot().
        // To Destroy a snapshot, call database::release_snapshot(get_snapshot).
        class snapshot {
        public:
            // returns get_snapshot's sequence number
            virtual engine::sequence_number get_sequence_number() const = 0;

        protected:
            virtual ~snapshot();
        };

        // Simple RAII wrapper class for get_snapshot.
        // Constructing this object will create a get_snapshot.  Destructing will
        // release the get_snapshot.
        class managed_snapshot {
        public:
            explicit managed_snapshot(database *db) : db_(db), snapshot_(db->get_snapshot()) {
            }

            // Instead of creating a get_snapshot, take ownership of the input snapshot.
            managed_snapshot(database *db, const snapshot *_snapshot) : db_(db), snapshot_(_snapshot) {
            }

            ~managed_snapshot() {
                if (snapshot_) {
                    db_->release_snapshot(snapshot_);
                }
            }

            const snapshot *get_snapshot() {
                return snapshot_;
            }

        private:
            database *db_;
            const snapshot *snapshot_;
        };
    }    // namespace dcdb
}    // namespace nil
