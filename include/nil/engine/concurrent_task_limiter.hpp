#pragma once

#include <nil/engine/environment.hpp>
#include <nil/engine/statistics.hpp>

namespace nil {
    namespace dcdb {

        class concurrent_task_limiter {
        public:

            virtual ~concurrent_task_limiter() {
            }

            // Returns a name that identifies this concurrent task limiter.
            virtual const std::string &get_name() const = 0;

            // Set max concurrent tasks.
            // limit = 0 means no new task allowed.
            // limit < 0 means no limitation.
            virtual void set_max_outstanding_task(int32_t limit) = 0;

            // reset to unlimited max concurrent task.
            virtual void reset_max_outstanding_task() = 0;

            // Returns current outstanding task count.
            virtual int32_t get_outstanding_task() const = 0;
        };

// Create a concurrent_task_limiter that can be shared with mulitple CFs
// across RocksDB instances to control concurrent tasks.
//
// @param name: name of the limiter.
// @param limit: max concurrent tasks.
//        limit = 0 means no new task allowed.
//        limit < 0 means no limitation.
        extern concurrent_task_limiter *new_concurrent_task_limiter(const std::string &name, int32_t limit);

    }
} // namespace nil
