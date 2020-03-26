//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_RANGE_HPP
#define STORAGE_ENGINE_RANGE_HPP

#include <nil/storage/engine/slice.hpp>

namespace nil {
    namespace engine {
        // A range of keys
        struct range {
            slice start;
            slice limit;

            range() {
            }

            range(const slice &s, const slice &l) : start(s), limit(l) {
            }
        };

        struct range_ptr {
            const slice *start;
            const slice *limit;

            range_ptr() : start(nullptr), limit(nullptr) {
            }

            range_ptr(const slice *s, const slice *l) : start(s), limit(l) {
            }
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_RANGE_HPP
