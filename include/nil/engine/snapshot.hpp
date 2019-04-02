#pragma once

#include <nil/engine/types.hpp>

namespace nil {
    namespace dcdb {

        class database;

// Abstract handle to particular state of a database.
// A Snapshot is an immutable object and can therefore be safely
// accessed from multiple threads without any external synchronization.
//
// To Create a Snapshot, call database::GetSnapshot().
// To Destroy a Snapshot, call database::ReleaseSnapshot(snapshot).
        class Snapshot {
        public:
            // returns Snapshot's sequence number
            virtual sequence_number GetSequenceNumber() const = 0;

        protected:
            virtual ~Snapshot();
        };

// Simple RAII wrapper class for Snapshot.
// Constructing this object will create a snapshot.  Destructing will
// release the snapshot.
        class ManagedSnapshot {
        public:
            explicit ManagedSnapshot(database *db);

            // Instead of creating a snapshot, take ownership of the input snapshot.
            ManagedSnapshot(database *db, const Snapshot *_snapshot);

            ~ManagedSnapshot();

            const Snapshot *snapshot();

        private:
            database *db_;
            const Snapshot *snapshot_;
        };

    }
} // namespace nil
