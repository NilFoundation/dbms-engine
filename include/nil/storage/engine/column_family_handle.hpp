//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_COLUMN_FAMILY_HANDLE_HPP
#define STORAGE_ENGINE_COLUMN_FAMILY_HANDLE_HPP

#include <nil/storage/engine/column_family_descriptor.hpp>

namespace nil {
    namespace engine {

        class column_family_handle {
        public:
            virtual ~column_family_handle() {
            }

            // Returns the name of the column family associated with the current handle.
            virtual const std::string &get_name() const = 0;

            // Returns the ID of the column family associated with the current handle.
            virtual uint32_t get_id() const = 0;

            // Fills "*desc" with the up-to-date descriptor of the column family
            // associated with this handle. Since it fills "*desc" with the up-to-date
            // information, this call might internally lock and release database mutex to
            // access the up-to-date CF opts.  In addition, all the pointer-typed
            // opts cannot be referenced any longer than the original opts exist.
            virtual status_type get_descriptor(column_family_descriptor *desc) = 0;

            // Returns the comparator of the column family associated with the
            // current handle.
            virtual const comparator *get_comparator() const = 0;
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_COLUMN_FAMILY_HANDLE_HPP
