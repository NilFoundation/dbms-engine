//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include <nil/engine/slice.hpp>
#include <nil/engine/slice_transform.hpp>

namespace nil {
    namespace engine {

        // context information of a compaction run
        struct compaction_filter_context {
            // Does this compaction run include all data files
            bool is_full_compaction;
            // Is this compaction requested by the client (true),
            // or is it occurring as an automatic compaction process
            bool is_manual_compaction;
        };

        // compaction_filter allows an application to modify/delete a key-value at
        // the time of compaction.

        class compaction_filter {
        public:
            enum value_type {
                kValue,
                kMergeOperand,
                kBlobIndex,    // used internally by blob_db.
            };

            enum class decision {
                kKeep,
                kRemove,
                kChangeValue,
                kRemoveAndSkipUntil,
            };

            // context information of a compaction run
            struct context {
                // Does this compaction run include all data files
                bool is_full_compaction;
                // Is this compaction requested by the client (true),
                // or is it occurring as an automatic compaction process
                bool is_manual_compaction;
                // Which column family this compaction is for.
                uint32_t column_family_id;
            };

            virtual ~compaction_filter() {
            }

            // The compaction process invokes this
            // method for kv that is being compacted. A return value
            // of false indicates that the kv should be preserved in the
            // output of this compaction run and a return value of true
            // indicates that this key-value should be removed from the
            // output of the compaction.  The application can inspect
            // the existing value of the key and make decision based on it.
            //
            // Key-Values that are results of merge operation during compaction are not
            // passed into this function. Currently, when you have a mix of insert()s and
            // merge()s on a same key, we only guarantee to process the merge operands
            // through the compaction filters. insert()s might be processed, or might not.
            //
            // When the value is to be preserved, the application has the option
            // to modify the existing_value and pass it back through new_value.
            // value_changed needs to be set to true in this case.
            //
            // Note that RocksDB snapshots (i.e. call get_snapshot() API on a
            // database* object) will not guarantee to preserve the state of the database with
            // compaction_filter. data seen from a get_snapshot might disppear after a
            // compaction finishes. If you use snapshots, think twice about whether you
            // want to use compaction filter and whether you are using it in a safe way.
            //
            // If multithreaded compaction is being used *and* a single compaction_filter
            // instance was supplied via opts::compaction_filter, this method may be
            // called from different threads concurrently.  The application must ensure
            // that the call is thread-safe.
            //
            // If the compaction_filter was created by a factory, then it will only ever
            // be used by a single thread that is doing the compaction run, and this
            // call does not need to be thread-safe.  However, multiple filters may be
            // in existence and operating concurrently.
            virtual bool filter(int level, const engine::slice &key, const engine::slice &existing_value, std::string *new_value,
                                bool *value_changed) const {
                return false;
            }

            // The compaction process invokes this method on every merge operand. If this
            // method returns true, the merge operand will be ignored and not written out
            // in the compaction output
            //
            // Note: If you are using a transaction_db, it is not recommended to implement
            // filter_merge_operand().  If a merge operation is filtered out, transaction_db
            // may not realize there is a write conflict and may allow a Transaction to
            // commit that should have failed.  Instead, it is better to implement any
            // merge filtering inside the merge_operator.
            virtual bool filter_merge_operand(int level, const engine::slice &key, const engine::slice &operand) const {
                return false;
            }

            // An extended API. Called for both values and merge operands.
            // Allows changing value and skipping ranges of keys.
            // The default implementation uses filter() and filter_merge_operand().
            // If you're overriding this method, no need to override the other two.
            // `value_type` indicates whether this key-value corresponds to a normal
            // value (e.g. written with insert())  or a merge operand (written with merge()).
            //
            // Possible return values:
            //  * kKeep - keep the key-value pair.
            //  * kRemove - remove the key-value pair or merge operand.
            //  * kChangeValue - keep the key and change the value/operand to *new_value.
            //  * kRemoveAndSkipUntil - remove this key-value pair, and also remove
            //      all key-value pairs with key in [key, *skip_until). This range
            //      of keys will be skipped without reading, potentially saving some
            //      IO operations compared to removing the keys one by one.
            //
            //      *skip_until <= key is treated the same as decision::kKeep
            //      (since the range [key, *skip_until) is empty).
            //
            //      Caveats:
            //       - The keys are skipped even if there are snapshots containing them,
            //         i.e. values removed by kRemoveAndSkipUntil can disappear from a
            //         get_snapshot - beware if you're using transaction_db or
            //         database::get_snapshot().
            //       - If value for a key was overwritten or merged into (multiple insert()s
            //         or merge()s), and compaction filter skips this key with
            //         kRemoveAndSkipUntil, it's possible that it will remove only
            //         the new value, exposing the old value that was supposed to be
            //         overwritten.
            //       - Doesn't work with PlainTableFactory in prefix mode.
            //       - If you use kRemoveAndSkipUntil, consider also reducing
            //         compaction_readahead_size option.
            //
            // Note: If you are using a transaction_db, it is not recommended to filter
            // out or modify merge operands (value_type::kMergeOperand).
            // If a merge operation is filtered out, transaction_db may not realize there
            // is a write conflict and may allow a Transaction to commit that should have
            // failed. Instead, it is better to implement any merge filtering inside the
            // merge_operator.
            virtual decision filter_v2(int level, const engine::slice &key, value_type vt, const engine::slice &existing_value,
                                       std::string *new_value, std::string *skip_until) const {
                switch (vt) {
                    case value_type::kValue: {
                        bool value_changed = false;
                        bool rv = filter(level, key, existing_value, new_value, &value_changed);
                        if (rv) {
                            return decision::kRemove;
                        }
                        return value_changed ? decision::kChangeValue : decision::kKeep;
                    }
                    case value_type::kMergeOperand: {
                        bool rv = filter_merge_operand(level, key, existing_value);
                        return rv ? decision::kRemove : decision::kKeep;
                    }
                    case value_type::kBlobIndex:
                        return decision::kKeep;
                }
                assert(false);
                return decision::kKeep;
            }

            // This function is deprecated. Snapshots will always be ignored for
            // compaction filters, because we realized that not ignoring snapshots doesn't
            // provide the gurantee we initially thought it would provide. Repeatable
            // reads will not be guaranteed anyway. If you override the function and
            // returns false, we will fail the compaction.
            virtual bool ignore_snapshots() const {
                return true;
            }

            // Returns a name that identifies this compaction filter.
            // The name will be printed to LOG file on start up for diagnosis.
            virtual const char *name() const = 0;
        };

        // Each compaction will create a new compaction_filter allowing the
        // application to know about different compactions
        class compaction_filter_factory {
        public:
            virtual ~compaction_filter_factory() {
            }

            virtual std::unique_ptr<compaction_filter>
                create_compaction_filter(const compaction_filter::context &context) = 0;

            // Returns a name that identifies this compaction filter factory.
            virtual const char *name() const = 0;
        };

    }    // namespace dcdb
}    // namespace nil
