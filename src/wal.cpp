#include <nil/storage/wal/wal.hpp>

namespace nil {
    namespace storage {
        wal::wal(const std::list<std::shared_ptr<layer>> &layers) {
            for (const std::list<std::shared_ptr<layer>>::value_type &r : layers) {
                wals_ptr.insert(r->engine_ptr()->wal_ptr());
            }
        }
    }
}