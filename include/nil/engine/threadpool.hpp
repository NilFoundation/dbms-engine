#pragma once

#include <functional>

namespace nil {
    namespace dcdb {

/*
 * ThreadPool is a component that will spawn N background threads that will
 * be used to execute scheduled work, The number of background threads could
 * be modified by calling set_background_threads().
 * */
        class ThreadPool {
        public:
            virtual ~ThreadPool() {
            }

            // Wait for all threads to finish.
            // Discard those threads that did not start
            // executing
            virtual void JoinAllThreads() = 0;

            // Set the number of background threads that will be executing the
            // scheduled jobs.
            virtual void SetBackgroundThreads(int num) = 0;

            virtual int GetBackgroundThreads() = 0;

            // get the number of jobs scheduled in the ThreadPool queue.
            virtual unsigned int GetQueueLen() const = 0;

            // Waits for all jobs to complete those
            // that already started running and those that did not
            // start yet. This ensures that everything that was thrown
            // on the TP runs even though
            // we may not have specified enough threads for the amount
            // of jobs
            virtual void WaitForJobsAndJoinAllThreads() = 0;

            // Submit a fire and forget jobs
            // This allows to submit the same job multiple times
            virtual void SubmitJob(const std::function<void()> &) = 0;

            // This moves the function in for efficiency
            virtual void SubmitJob(std::function<void()> &&) = 0;

        };

// NewThreadPool() is a function that could be used to create a ThreadPool
// with `num_threads` background threads.
        extern ThreadPool *NewThreadPool(int num_threads);

    }
} // namespace nil
