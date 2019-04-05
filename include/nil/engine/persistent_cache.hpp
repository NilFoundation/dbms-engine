#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <nil/engine/env.hpp>
#include <nil/engine/slice.hpp>
#include <nil/engine/statistics.hpp>
#include <nil/engine/status.hpp>

namespace nil {
    namespace dcdb {

// persistent_cache
//
// Persistent cache interface for caching IO pages on a persistent medium. The
// cache interface is specifically designed for persistent read cache.
        class persistent_cache {
        public:
            typedef std::vector<std::map<std::string, double>> StatsType;

            virtual ~persistent_cache() {
            }

            // insert to page cache
            //
            // page_key   Identifier to identify a page uniquely across restarts
            // data       Page data
            // size       Size of the page
            virtual status_type insert(const slice &key, const char *data, const size_t size) = 0;

            // lookup page cache by page identifier
            //
            // page_key   Page identifier
            // buf        Buffer where the data should be copied
            // size       Size of the page
            virtual status_type lookup(const slice &key, std::unique_ptr<char[]> *data, size_t *size) = 0;

            // Is cache storing uncompressed data ?
            //
            // True if the cache is configured to store uncompressed data else false
            virtual bool is_compressed() = 0;

            // Return stats as map of {string, double} per-tier
            //
            // Persistent cache can be initialized as a tier of caches. The stats are per
            // tire top-down
            virtual StatsType stats() = 0;

            virtual std::string get_printable_options() const = 0;
        };

// Factor method to create a new persistent cache
        status_type new_persistent_cache(environment_type *const env, const std::string &path, const uint64_t size,
                                         const std::shared_ptr<Logger> &log, const bool optimized_for_nvm,
                                         std::shared_ptr<persistent_cache> *cache);
    }
} // namespace nil
