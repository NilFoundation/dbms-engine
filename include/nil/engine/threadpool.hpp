#pragma once

#include <functional>

namespace nil {
    namespace dcdb {

/*
 * thread_pool is a component that will spawn N background threads that will
 * be used to execute scheduled work, The number of background threads could
 * be modified by calling set_background_threads().
 * */
        class thread_pool {
        public:
            virtual ~thread_pool() {
            }

            // Wait for all threads to finish.
            // Discard those threads that did not start
            // executing
            virtual void join_all_threads() = 0;

            // Set the number of background threads that will be executing the
            // scheduled jobs.
            virtual void set_background_threads(int num) = 0;

            virtual int get_background_threads() = 0;

            // get the number of jobs scheduled in the thread_pool queue.
            virtual unsigned int get_queue_len() const = 0;

            // Waits for all jobs to complete those
            // that already started running and those that did not
            // start yet. This ensures that everything that was thrown
            // on the TP runs even though
            // we may not have specified enough threads for the amount
            // of jobs
            virtual void wait_for_jobs_and_join_all_threads() = 0;

            // Submit a fire and forget jobs
            // This allows to submit the same job multiple times
            virtual void submit_job(const std::function<void()> &) = 0;

            // This moves the function in for efficiency
            virtual void submit_job(std::function<void()> &&) = 0;

        };

// new_thread_pool() is a function that could be used to create a thread_pool
// with `num_threads` background threads.
        extern thread_pool *new_thread_pool(int num_threads);

    }
} // namespace nil
