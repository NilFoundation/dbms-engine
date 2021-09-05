//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_FLUSHER_HPP
#define STORAGE_ENGINE_FLUSHER_HPP

#include <nil/storage/engine/column_family/column_family_handle.hpp>
#include <nil/storage/engine/flush/flush_options.hpp>
#include <nil/storage/engine/database.hpp>

namespace nil {
    namespace engine {
        /*!
         * @brief
         */
        struct flusher {
            // flush all mem-table data.
            // flush a single column family, even when atomic flush is enabled. To flush
            // multiple column families, use flush(opts, column_families).
            virtual engine::status_type flush(const flush_options &options, column_family_handle *column_family) = 0;

            // Flushes multiple column families.
            // If atomic flush is not enabled, flush(opts, column_families) is
            // equivalent to calling flush(opts, column_family) multiple times.
            // If atomic flush is enabled, flush(opts, column_families) will flush all
            // column families specified in 'column_families' up to the latest sequence
            // number at the time when flush is requested.
            virtual engine::status_type flush(const flush_options &options,
                                              const std::vector<column_family_handle *> &column_families) = 0;
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_FLUSHER_HPP
