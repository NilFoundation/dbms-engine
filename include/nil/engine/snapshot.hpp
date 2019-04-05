#pragma once

#include <nil/engine/types.hpp>

namespace nil {
    namespace dcdb {

        class database;

// Abstract handle to particular state of a database.
// A get_snapshot is an immutable object and can therefore be safely
// accessed from multiple threads without any external synchronization.
//
// To Create a get_snapshot, call database::GetSnapshot().
// To Destroy a snapshot, call database::ReleaseSnapshot(get_snapshot).
        class snapshot {
        public:
            // returns get_snapshot's sequence number
            virtual sequence_number get_sequence_number() const = 0;

        protected:
            virtual ~snapshot();
        };

// Simple RAII wrapper class for get_snapshot.
// Constructing this object will create a get_snapshot.  Destructing will
// release the get_snapshot.
        class managed_snapshot {
        public:
            explicit managed_snapshot(database *db);

            // Instead of creating a get_snapshot, take ownership of the input snapshot.
            managed_snapshot(database *db, const snapshot *_snapshot);

            ~managed_snapshot();

            const snapshot *get_snapshot();

        private:
            database *db_;
            const snapshot *snapshot_;
        };

    }
} // namespace nil
