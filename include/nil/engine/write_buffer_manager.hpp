// write_buffer_manager is for managing memory allocation for one or more
// MemTables.

#pragma once

#include <atomic>
#include <cstddef>

#include <nil/engine/cache.hpp>

namespace nil {
    namespace dcdb {

        class write_buffer_manager {
        public:
            // _buffer_size = 0 indicates no limit. Memory won't be capped.
            // memory_usage() won't be valid and should_flush() will always return true.
            // if `cache` is provided, we'll insert dummy entries in the cache and cost
            // the memory allocated to the cache. It can be used even if _buffer_size = 0.
            explicit write_buffer_manager(size_t _buffer_size, std::shared_ptr<cache> cache = {});

            ~write_buffer_manager();

            bool enabled() const {
                return buffer_size_ != 0;
            }

            bool cost_to_cache() const {
                return cache_rep_ != nullptr;
            }

            // Only valid if enabled()
            size_t memory_usage() const {
                return memory_used_.load(std::memory_order_relaxed);
            }

            size_t mutable_memtable_memory_usage() const {
                return memory_active_.load(std::memory_order_relaxed);
            }

            size_t buffer_size() const {
                return buffer_size_;
            }

            // Should only be called from write thread
            bool should_flush() const {
                if (enabled()) {
                    if (mutable_memtable_memory_usage() > mutable_limit_) {
                        return true;
                    }
                    if (memory_usage() >= buffer_size_ && mutable_memtable_memory_usage() >= buffer_size_ / 2) {
                        // If the memory exceeds the buffer size, we trigger more aggressive
                        // flush. But if already more than half memory is being flushed,
                        // triggering more flush may not help. We will hold it instead.
                        return true;
                    }
                }
                return false;
            }

            void reserve_mem(size_t mem) {
                if (cache_rep_ != nullptr) {
                    reserve_mem_with_cache(mem);
                } else if (enabled()) {
                    memory_used_.fetch_add(mem, std::memory_order_relaxed);
                }
                if (enabled()) {
                    memory_active_.fetch_add(mem, std::memory_order_relaxed);
                }
            }

            // We are in the process of freeing `mem` bytes, so it is not considered
            // when checking the soft limit.
            void schedule_free_mem(size_t mem) {
                if (enabled()) {
                    memory_active_.fetch_sub(mem, std::memory_order_relaxed);
                }
            }

            void free_mem(size_t mem) {
                if (cache_rep_ != nullptr) {
                    free_mem_with_cache(mem);
                } else if (enabled()) {
                    memory_used_.fetch_sub(mem, std::memory_order_relaxed);
                }
            }

        private:
            const size_t buffer_size_;
            const size_t mutable_limit_;
            std::atomic<size_t> memory_used_;
            // Memory that hasn't been scheduled to free.
            std::atomic<size_t> memory_active_;
            struct CacheRep;
            std::unique_ptr<CacheRep> cache_rep_;

            void reserve_mem_with_cache(size_t mem);

            void free_mem_with_cache(size_t mem);

            // No copying allowed
            write_buffer_manager(const write_buffer_manager &) = delete;

            write_buffer_manager &operator=(const write_buffer_manager &) = delete;
        };
    }
} // namespace nil
