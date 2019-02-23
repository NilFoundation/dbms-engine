#ifndef FRAMEWORK_ENGINE_HPP
#define FRAMEWORK_ENGINE_HPP

#include <nil/storage/engine/wal.hpp>

#include <nil/storage/transaction.hpp>

namespace nil {
    namespace storage {
        namespace engine {
            class virtual_engine {
            public:
                virtual_engine(const std::unique_ptr<engine::wal> &input_wal_ptr) : log_ptr(input_wal_ptr) {
                }

                std::shared_ptr<engine::wal> wal_ptr() const {
                    return log_ptr;
                }

                /*!
                 * @brief Begin a new single or multi-statement transaction.
                 * Called on first statement in a transaction, not when
                 * a user said begin(). Effectively it means that
                 * transaction in the engine begins with the first
                 * statement.
                 */
                virtual bool begin(const transaction &trx) = 0;

                /*!
                 * @brief Begin one statement in existing transaction.
                 */
                virtual bool begin_statement(const transaction &trx) = 0;

                /*!
                 * @brief Called before a WAL write is made to prepare
                 * a transaction for commit in the engine.
                 */
                virtual bool prepare(const transaction &trx) = 0;

                /*!
                 * @brief End the transaction in the engine, the transaction
                 * has been successfully written to the WAL.
                 * This method can't throw: if any error happens here,
                 * there is no better option than panic.
                 */
                virtual void commit(const transaction &trx) = 0;

                /*!
                 * @brief Called to roll back effects of a statement if an
                 * error happens, e.g., in a trigger.
                 */
                virtual void rollback_statement(const transaction &trx) = 0;

                /*!
                 * @brief Roll back and end the transaction in the engine.
                 */
                virtual void rollback(const transaction &trx) = 0;

                /*!
                 * @brief Bootstrap an initial data engine state
                 * @return true if bootstrapped successfully
                 */
                virtual bool bootstrap(const transaction &trx) = 0;

            protected:
                std::shared_ptr<engine::wal> log_ptr;
            };
        }
    }
}

#endif //FRAMEWORK_ENGINE_HPP
