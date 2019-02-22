#ifndef FRAMEWORK_WAL_CONTROLLER_HPP
#define FRAMEWORK_WAL_CONTROLLER_HPP

#include <list>
#include <set>

#include <nil/storage/layer.hpp>

#include <nil/storage/wal/wal_iterator.hpp>

#include <nil/storage/engine/wal.hpp>

namespace nil {
    namespace storage {
        class wal {
        public:
            typedef wal_iterator iterator;

            class impl {
            public:

            };

            wal(const std::list<std::shared_ptr<layer>> &layers);

        protected:
            std::shared_ptr<impl> pimpl;
            std::set<std::shared_ptr<engine::wal>> wals_ptr;
        };
    }
}

#endif //FRAMEWORK_WAL_CONTROLLER_HPP
