#ifndef FRAMEWORK_SINK_HPP
#define FRAMEWORK_SINK_HPP

#include <vector>

#include <nil/storage/layer.hpp>

namespace nil {
    namespace storage {
        /*!
         * @brief Sink defines the data way through the engines
         */
        class sink {
        public:

        protected:
            std::list<std::shared_ptr<layer>> layers;
        };
    }
}

#endif //FRAMEWORK_SINK_HPP
