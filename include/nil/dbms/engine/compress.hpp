//---------------------------------------------------------------------------//
// Copyright (c) 2018-2021 Mikhail Komarov <nemo@nil.foundation>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the Server Side Public License, version 1,
// as published by the author.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// Server Side Public License for more details.
//
// You should have received a copy of the Server Side Public License
// along with this program. If not, see
// <https://github.com/NilFoundation/dbms/blob/master/LICENSE_1_0.txt>.
//---------------------------------------------------------------------------//

#pragma once

#include <map>
#include <set>

#include <nil/actor/core/future.hh>
#include <nil/actor/core/shared_ptr.hh>
#include <nil/actor/core/sstring.hh>

#include <nil/dbms/exceptions/exceptions.hh>

namespace nil {
    namespace engine {

        class compressor {
            actor::sstring _name;

        public:
            compressor(actor::sstring name) : _name(std::move(name)) {
            }

            virtual ~compressor() {
            }

            /**
             * Unpacks data in "input" to output. If output_len is of insufficient size,
             * exception is thrown. I.e. you should keep track of the uncompressed size.
             */
            virtual size_t uncompress(const char *input, size_t input_len, char *output, size_t output_len) const = 0;
            /**
             * Packs data in "input" to output. If output_len is of insufficient size,
             * exception is thrown. Maximum required size is obtained via "compress_max_size"
             */
            virtual size_t compress(const char *input, size_t input_len, char *output, size_t output_len) const = 0;
            /**
             * Returns the maximum output size for compressing data on "input_len" size.
             */
            virtual size_t compress_max_size(size_t input_len) const = 0;

            /**
             * Returns accepted option names for this compressor
             */
            virtual std::set<actor::sstring> option_names() const = 0;
            /**
             * Returns original options used in instantiating this compressor
             */
            virtual std::map<actor::sstring, actor::sstring> options() const = 0;

            /**
             * Compressor class name.
             */
            const actor::sstring &name() const {
                return _name;
            }

            // to cheaply bridge sstable compression options / maps
            using opt_string = boost::optional<actor::sstring>;
            using opt_getter = std::function<opt_string(const actor::sstring &)>;

            static thread_local const actor::shared_ptr<compressor> lz4;
            static thread_local const actor::shared_ptr<compressor> snappy;
            static thread_local const actor::shared_ptr<compressor> deflate;

            static const actor::sstring namespace_prefix;
        };

        template<typename BaseType, typename... Args>
        class class_registry;

        using compressor_ptr = actor::shared_ptr<compressor>;
        using compressor_registry = class_registry<compressor_ptr, const typename compressor::opt_getter &>;
    }    // namespace cluster
}    // namespace nil