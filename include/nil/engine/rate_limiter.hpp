#pragma once

#include <nil/engine/environment.hpp>
#include <nil/engine/statistics.hpp>

namespace nil {
    namespace dcdb {

        class rate_limiter {
        public:
            enum class op_type {
                // Limitation: we currently only invoke request() with op_type::kRead for
                // compactions when db_options::new_table_reader_for_compaction_inputs is set
                        kRead, kWrite,
            };
            enum class mode {
                kReadsOnly, kWritesOnly, kAllIo,
            };

            // For API compatibility, default to rate-limiting writes only.
            explicit rate_limiter(mode mode = mode::kWritesOnly) : mode_(mode) {
            }

            virtual ~rate_limiter() {
            }

            // This API allows user to dynamically change rate limiter's bytes per second.
            // REQUIRED: bytes_per_second > 0
            virtual void set_bytes_per_second(int64_t bytes_per_second) = 0;

            // Deprecated. New limiter derived classes should override
            // request(const int64_t, const environment_type::io_priority, get_statistics*) or
            // request(const int64_t, const environment_type::io_priority, get_statistics*, op_type)
            // instead.
            //
            // request for token for bytes. If this request can not be satisfied, the call
            // is blocked. Caller is responsible to make sure
            // bytes <= get_single_burst_bytes()
            virtual void request(const int64_t bytes, const environment_type::io_priority pri) {
                assert(false);
            }

            // request for token for bytes and potentially update get_statistics. If this
            // request can not be satisfied, the call is blocked. Caller is responsible to
            // make sure bytes <= get_single_burst_bytes().
            virtual void request(const int64_t bytes, const environment_type::io_priority pri, statistics *stats) {
                // For API compatibility, default implementation calls the older API in
                // which get_statistics are unsupported.
                request(bytes, pri);
            }

            // Requests token to read or write bytes and potentially updates get_statistics.
            //
            // If this request can not be satisfied, the call is blocked. Caller is
            // responsible to make sure bytes <= get_single_burst_bytes().
            virtual void request(const int64_t bytes, const environment_type::io_priority pri, statistics *stats,
                                 op_type op_type) {
                if (is_rate_limited(op_type)) {
                    request(bytes, pri, stats);
                }
            }

            // Requests token to read or write bytes and potentially updates get_statistics.
            // Takes into account get_single_burst_bytes() and alignment (e.g., in case of
            // direct I/O) to allocate an appropriate number of bytes, which may be less
            // than the number of bytes requested.
            virtual size_t request_token(size_t bytes, size_t alignment, environment_type::io_priority io_priority,
                                         statistics *stats, rate_limiter::op_type op_type);

            // Max bytes can be granted in a single burst
            virtual int64_t get_single_burst_bytes() const = 0;

            // Total bytes that go through rate limiter
            virtual int64_t get_total_bytes_through(const environment_type::io_priority pri = environment_type::IO_TOTAL) const = 0;

            // Total # of requests that go through rate limiter
            virtual int64_t get_total_requests(const environment_type::io_priority pri = environment_type::IO_TOTAL) const = 0;

            virtual int64_t get_bytes_per_second() const = 0;

            virtual bool is_rate_limited(op_type op_type) {
                if ((mode_ == rate_limiter::mode::kWritesOnly && op_type == rate_limiter::op_type::kRead) ||
                    (mode_ == rate_limiter::mode::kReadsOnly && op_type == rate_limiter::op_type::kWrite)) {
                    return false;
                }
                return true;
            }

        protected:
            mode get_mode() {
                return mode_;
            }

        private:
            const mode mode_;
        };

// Create a limiter object, which can be shared among RocksDB instances to
// control write rate of flush and compaction.
// @rate_bytes_per_sec: this is the only parameter you want to set most of the
// time. It controls the total write rate of compaction and flush in bytes per
// second. Currently, RocksDB does not enforce rate limit for anything other
// than flush and compaction, e.g. write to WAL.
// @refill_period_us: this controls how often tokens are refilled. For example,
// when rate_bytes_per_sec is set to 10MB/s and refill_period_us is set to
// 100ms, then 1MB is refilled every 100ms internally. Larger value can lead to
// burstier writes while smaller value introduces more CPU overhead.
// The default should work for most cases.
// @fairness: limiter accepts high-pri requests and low-pri requests.
// A low-pri request is usually blocked in favor of hi-pri request. Currently,
// RocksDB assigns low-pri to request from compaction and high-pri to request
// from flush. Low-pri requests can get blocked if flush requests come in
// continuously. This fairness parameter grants low-pri requests permission by
// 1/fairness chance even though high-pri requests exist to avoid starvation.
// You should be good by leaving it at default 10.
// @mode: mode indicates which types of operations count against the limit.
// @auto_tuned: Enables dynamic adjustment of rate limit within the range
//              `[rate_bytes_per_sec / 20, rate_bytes_per_sec]`, according to
//              the recent demand for background I/O.
        extern rate_limiter *new_generic_rate_limiter(int64_t rate_bytes_per_sec, int64_t refill_period_us = 100 * 1000,
                                                      int32_t fairness = 10,
                                                      rate_limiter::mode mode = rate_limiter::mode::kWritesOnly,
                                                      bool auto_tuned = false);

    }
} // namespace nil
