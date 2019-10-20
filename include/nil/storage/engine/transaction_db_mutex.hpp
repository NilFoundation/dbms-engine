//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <memory>

#include <nil/storage/engine/status.hpp>

namespace nil {
    namespace engine {

        // transaction_db_mutex and transaction_db_cond_var APIs allows applications to
        // implement custom mutexes and condition variables to be used by a
        // transaction_db when locking keys.
        //
        // To open a transaction_db with a custom transaction_db_mutex_factory, set
        // transaction_db_options.custom_mutex_factory.

        class transaction_db_mutex {
        public:
            virtual ~transaction_db_mutex() {
            }

            // Attempt to acquire lock.  Return is_ok on success, or other engine::status_type on failure.
            // If returned status is is_ok, transaction_db will eventually call unlock().
            virtual engine::status_type lock() = 0;

            // Attempt to acquire lock.  If timeout is non-negative, operation may be
            // failed after this many microseconds.
            // Returns is_ok on success,
            //         timed_out if timed out,
            //         or other engine::status_type on failure.
            // If returned status is is_ok, transaction_db will eventually call unlock().
            virtual engine::status_type try_lock_for(int64_t timeout_time) = 0;

            // Unlock Mutex that was successfully locked by lock() or TryLockUntil()
            virtual void unlock() = 0;
        };

        class transaction_db_cond_var {
        public:
            virtual ~transaction_db_cond_var() {
            }

            // Block current thread until condition variable is notified by a call to
            // notify() or notify_all().  Wait() will be called with mutex locked.
            // Returns is_ok if notified.
            // Returns non-is_ok if transaction_db should stop waiting and fail the operation.
            // May return is_ok spuriously even if not notified.
            virtual engine::status_type wait(std::shared_ptr<transaction_db_mutex> mutex) = 0;

            // Block current thread until condition variable is notified by a call to
            // notify() or notify_all(), or if the timeout is reached.
            // Wait() will be called with mutex locked.
            //
            // If timeout is non-negative, operation should be failed after this many
            // microseconds.
            // If implementing a custom version of this class, the implementation may
            // choose to ignore the timeout.
            //
            // Returns is_ok if notified.
            // Returns timed_out if timeout is reached.
            // Returns other status if transaction_db should otherwis stop waiting and
            //  fail the operation.
            // May return is_ok spuriously even if not notified.
            virtual engine::status_type wait_for(std::shared_ptr<transaction_db_mutex> mutex, int64_t timeout_time) = 0;

            // If any threads are waiting on *this, unblock at least one of the
            // waiting threads.
            virtual void notify() = 0;

            // Unblocks all threads waiting on *this.
            virtual void notify_all() = 0;
        };

        // Factory class that can allocate mutexes and condition variables.
        class transaction_db_mutex_factory {
        public:
            // create a transaction_db_mutex object.
            virtual std::shared_ptr<transaction_db_mutex> allocate_mutex() = 0;

            // create a transaction_db_cond_var object.
            virtual std::shared_ptr<transaction_db_cond_var> allocate_cond_var() = 0;

            virtual ~transaction_db_mutex_factory() {
            }
        };
    }    // namespace engine
}    // namespace nil
