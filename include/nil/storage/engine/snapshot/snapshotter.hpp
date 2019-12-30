#ifndef STORAGE_ENGINE_SNAPSHOTTER_HPP
#define STORAGE_ENGINE_SNAPSHOTTER_HPP

namespace nil {
    namespace engine {
        struct snapshot;

        struct snapshotter {
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
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_SNAPSHOTTER_HPP
