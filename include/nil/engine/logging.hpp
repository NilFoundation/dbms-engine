#ifndef DCDB_LOGGING_HPP
#define DCDB_LOGGING_HPP

namespace nil {
    namespace dcdb {
        /*!
         * @brief This sets the requirement for the engines logging levels available
         */
        enum info_log_level : unsigned char {
            DEBUG_LEVEL = 0, INFO_LEVEL, WARN_LEVEL, ERROR_LEVEL, FATAL_LEVEL, HEADER_LEVEL, NUM_INFO_LOG_LEVELS
        };
    }
}

#endif //DCDB_LOGGING_HPP
