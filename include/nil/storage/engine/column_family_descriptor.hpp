//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_COLUMN_FAMILY_DESCRIPTOR_HPP
#define STORAGE_ENGINE_COLUMN_FAMILY_DESCRIPTOR_HPP

#include <string>

#include <nil/storage/engine/column_family_options.hpp>

namespace nil {
    namespace engine {
        extern const std::string kDefaultColumnFamilyName;

        struct column_family_descriptor {
            typedef column_family_options options_type;

            column_family_descriptor() : name(kDefaultColumnFamilyName), options(options_type()) {
            }

            column_family_descriptor(const std::string &_name, const options_type &_options) :
                name(_name), options(_options) {
            }

            std::string name;
            options_type options;
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_COLUMN_FAMILY_DESCRIPTOR_HPP
