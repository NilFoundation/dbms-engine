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

#ifndef STORAGE_ENGINE_KEYS_HPP
#define STORAGE_ENGINE_KEYS_HPP

namespace nil {
    namespace dbms {
        namespace engine {
            struct exploded_clustering_prefix { };
            struct partition_key { };
            struct partition_key_view { };
            struct clustering_key_prefix { };
            struct clustering_key_prefix_view { };
        }    // namespace engine
    }        // namespace dbms
}    // namespace nil

#endif    // DCDB_DATABASE_HPP
