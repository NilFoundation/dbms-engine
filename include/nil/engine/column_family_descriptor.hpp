//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef DCDB_COLUMN_FAMILY_DESCRIPTOR_HPP
#define DCDB_COLUMN_FAMILY_DESCRIPTOR_HPP

#include <string>

#include <nil/engine/column_family_options.hpp>

namespace nil {
    namespace engine {
        extern const std::string kDefaultColumnFamilyName;

        struct column_family_descriptor {
            std::string name;
            column_family_options options;

            column_family_descriptor() : name(kDefaultColumnFamilyName), options(column_family_options()) {
            }

            column_family_descriptor(const std::string &_name, const column_family_options &_options) :
                name(_name), options(_options) {
            }
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_COLUMN_FAMILY_DESCRIPTOR_HPP
