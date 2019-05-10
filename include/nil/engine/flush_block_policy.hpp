#pragma once

#include <string>

#include <nil/engine/table.hpp>

namespace nil {
    namespace dcdb {

        class slice;

        class block_builder;

        struct database_options;

// flush_block_policy provides a configurable way to determine when to flush a
// block in the block based tables,
        class flush_block_policy {
        public:
            // Keep track of the key/value sequences and return the boolean value to
            // determine if table builder should flush current data block.
            virtual bool update(const slice &key, const slice &value) = 0;

            virtual ~flush_block_policy() {
            }
        };

        class flush_block_policy_factory {
        public:
            // Return the name of the flush block policy.
            virtual const char *name() const = 0;

            // Return a new block flush policy that flushes data blocks by data size.
            // flush_block_policy may need to access the metadata of the data block
            // builder to determine when to flush the blocks.
            //
            // Callers must delete the result after any database that is using the
            // result has been closed.
            virtual flush_block_policy *new_flush_block_policy(const block_based_table_options &table_options,
                                                               const block_builder &data_block_builder) const = 0;

            virtual ~flush_block_policy_factory() {
            }
        };

        class flush_block_by_size_policy_factory : public flush_block_policy_factory {
        public:
            flush_block_by_size_policy_factory() {
            }

            const char *name() const override {
                return "flush_block_by_size_policy_factory";
            }

            flush_block_policy *new_flush_block_policy(const block_based_table_options &table_options,
                                                       const block_builder &data_block_builder) const override;

            static flush_block_policy *new_flush_block_policy(const uint64_t size, const int deviation,
                                                              const block_builder &data_block_builder);
        };

    }
}  // namespace nil
