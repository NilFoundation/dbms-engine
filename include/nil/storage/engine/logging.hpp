//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_LOGGING_HPP
#define STORAGE_ENGINE_LOGGING_HPP

namespace nil {
    namespace engine {
        /*!
         * @brief This sets the requirement for the engines logging levels available
         */
        enum info_log_level : unsigned char {
            DEBUG_LEVEL = 0,
            INFO_LEVEL,
            WARN_LEVEL,
            ERROR_LEVEL,
            FATAL_LEVEL,
            HEADER_LEVEL,
            NUM_INFO_LOG_LEVELS
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_LOGGING_HPP
