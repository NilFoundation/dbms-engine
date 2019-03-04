#ifndef FRAMEWORK_LAYER_HPP
#define FRAMEWORK_LAYER_HPP

#include <memory>

#include <nil/storage/engine/basic_engine.hpp>

namespace nil {
    namespace storage {
        class layer {
        public:
            std::shared_ptr<engine> engine_ptr() const {
                return eng_ptr;
            }

        protected:
            std::shared_ptr<engine> eng_ptr;
        };
    }
}

#endif //FRAMEWORK_LAYER_HPP
