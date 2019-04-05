#pragma once

#include <deque>
#include <memory>
#include <string>
#include <vector>

#include <nil/engine/slice.hpp>

namespace nil {
    namespace dcdb {

        class slice;

        class Logger;

// The merge Operator
//
// Essentially, a merge_operator specifies the SEMANTICS of a merge, which only
// client knows. It could be numeric addition, list append, string
// concatenation, edit data structure, ... , anything.
// The library, on the other hand, is concerned with the exercise of this
// interface, at the right time (during get, iteration, compaction...)
//
// To use merge, the client needs to provide an object implementing one of
// the following interfaces:
//  a) associative_merge_operator - for most simple semantics (always take
//    two values, and merge them into one value, which is then insert back
//    into rocksdb); numeric addition and string concatenation are examples;
//
//  b) merge_operator - the generic class for all the more abstract / complex
//    operations; one method (full_merge_v2) to merge a insert/remove value with a
//    merge operand; and another method (partial_merge) that merges multiple
//    operands together. this is especially useful if your key values have
//    complex structures but you would still like to support client-specific
//    incremental updates.
//
// associative_merge_operator is simpler to implement. merge_operator is simply
// more powerful.
//
// Refer to rocksdb-merge wiki for more details and example implementations.
//
        class merge_operator {
        public:
            virtual ~merge_operator() {
            }

            // Gives the client a way to express the read -> modify -> write semantics
            // key:      (IN)    The key that's associated with this merge operation.
            //                   Client could multiplex the merge operator based on it
            //                   if the key space is partitioned and different subspaces
            //                   refer to different types of data which have different
            //                   merge operation semantics
            // existing: (IN)    null indicates that the key does not exist before this op
            // operand_list:(IN) the sequence of merge operations to apply, front() first.
            // new_value:(OUT)   Client is responsible for filling the merge result here.
            // The string that new_value is pointing to will be empty.
            // logger:   (IN)    Client could use this to log errors during merge.
            //
            // Return true on success.
            // All values passed in will be client-specific values. So if this method
            // returns false, it is because client specified bad data or there was
            // internal corruption. This will be treated as an error by the library.
            //
            // Also make use of the *logger for error messages.
            virtual bool full_merge(const slice &key, const slice *existing_value,
                                    const std::deque<std::string> &operand_list, std::string *new_value,
                                    Logger *logger) const {
                // deprecated, please use full_merge_v2()
                assert(false);
                return false;
            }

            struct merge_operation_input {
                explicit merge_operation_input(const slice &_key, const slice *_existing_value,
                                               const std::vector<slice> &_operand_list, Logger *_logger) : key(_key),
                        existing_value(_existing_value), operand_list(_operand_list), logger(_logger) {
                }

                // The key associated with the merge operation.
                const slice &key;
                // The existing value of the current key, nullptr means that the
                // value doesn't exist.
                const slice *existing_value;
                // A list of operands to apply.
                const std::vector<slice> &operand_list;
                // Logger could be used by client to log any errors that happen during
                // the merge operation.
                Logger *logger;
            };

            struct merge_operation_output {
                explicit merge_operation_output(std::string &_new_value, slice &_existing_operand) : new_value(
                        _new_value), existing_operand(_existing_operand) {
                }

                // Client is responsible for filling the merge result here.
                std::string &new_value;
                // If the merge result is one of the existing operands (or existing_value),
                // client can set this field to the operand (or existing_value) instead of
                // using new_value.
                slice &existing_operand;
            };

            // This function applies a stack of merge operands in chrionological order
            // on top of an existing value. There are two ways in which this method is
            // being used:
            // a) During get() operation, it used to calculate the final value of a key
            // b) During compaction, in order to collapse some operands with the based
            //    value.
            //
            // Note: The name of the method is somewhat misleading, as both in the cases
            // of get() or compaction it may be called on a subset of operands:
            // K:    0    +1    +2    +7    +4     +5      2     +1     +2
            //                              ^
            //                              |
            //                          get_snapshot
            // In the example above, get(K) operation will call full_merge with a base
            // value of 2 and operands [+1, +2]. Compaction process might decide to
            // collapse the beginning of the history up to the get_snapshot by performing
            // full merge with base value of 0 and operands [+1, +2, +7, +3].
            virtual bool full_merge_v2(const merge_operation_input &merge_in, merge_operation_output *merge_out) const;

            // This function performs merge(left_op, right_op)
            // when both the operands are themselves merge operation types
            // that you would have passed to a database::merge() call in the same order
            // (i.e.: database::merge(key,left_op), followed by database::merge(key,right_op)).
            //
            // partial_merge should combine them into a single merge operation that is
            // saved into *new_value, and then it should return true.
            // *new_value should be constructed such that a call to
            // database::merge(key, *new_value) would yield the same result as a call
            // to database::merge(key, left_op) followed by database::merge(key, right_op).
            //
            // The string that new_value is pointing to will be empty.
            //
            // The default implementation of partial_merge_multi will use this function
            // as a helper, for backward compatibility.  Any successor class of
            // merge_operator should either implement partial_merge or partial_merge_multi,
            // although implementing partial_merge_multi is suggested as it is in general
            // more effective to merge multiple operands at a time instead of two
            // operands at a time.
            //
            // If it is impossible or infeasible to combine the two operations,
            // leave new_value unchanged and return false. The library will
            // internally keep track of the operations, and apply them in the
            // correct order once a base-value (a insert/remove/End-of-Database) is seen.
            //
            // TODO: Presently there is no way to differentiate between error/corruption
            // and simply "return false". For now, the client should simply return
            // false in any case it cannot perform partial-merge, regardless of reason.
            // If there is corruption in the data, handle it in the full_merge_v2() function
            // and return false there.  The default implementation of partial_merge will
            // always return false.
            virtual bool partial_merge(const slice &key, const slice &left_operand, const slice &right_operand,
                                       std::string *new_value, Logger *logger) const {
                return false;
            }

            // This function performs merge when all the operands are themselves merge
            // operation types that you would have passed to a database::merge() call in the
            // same order (front() first)
            // (i.e. database::merge(key, operand_list[0]), followed by
            //  database::merge(key, operand_list[1]), ...)
            //
            // partial_merge_multi should combine them into a single merge operation that is
            // saved into *new_value, and then it should return true.  *new_value should
            // be constructed such that a call to database::merge(key, *new_value) would yield
            // the same result as subquential individual calls to database::merge(key, operand)
            // for each operand in operand_list from front() to back().
            //
            // The string that new_value is pointing to will be empty.
            //
            // The partial_merge_multi function will be called when there are at least two
            // operands.
            //
            // In the default implementation, partial_merge_multi will invoke partial_merge
            // multiple times, where each time it only merges two operands.  Developers
            // should either implement partial_merge_multi, or implement partial_merge which
            // is served as the helper function of the default partial_merge_multi.
            virtual bool partial_merge_multi(const slice &key, const std::deque<slice> &operand_list,
                                             std::string *new_value, Logger *logger) const;

            // The name of the merge_operator. Used to check for merge_operator
            // mismatches (i.e., a database created with one merge_operator is
            // accessed using a different merge_operator)
            // TODO: the name is currently not stored persistently and thus
            //       no checking is enforced. Client is responsible for providing
            //       consistent merge_operator between database opens.
            virtual const char *name() const = 0;

            // Determines whether the partial_merge can be called with just a single
            // merge operand.
            // Override and return true for allowing a single operand. partial_merge
            // and partial_merge_multi should be overridden and implemented
            // correctly to properly handle a single operand.
            virtual bool allow_single_operand() const {
                return false;
            }

            // Allows to control when to invoke a full merge during get.
            // This could be used to limit the number of merge operands that are looked at
            // during a point lookup, thereby helping in limiting the number of levels to
            // read from.
            // Doesn't help with iterators.
            //
            // Note: the merge operands are passed to this function in the reversed order
            // relative to how they were merged (passed to full_merge or full_merge_v2)
            // for performance reasons, see also:
            // https://github.com/facebook/rocksdb/issues/3865
            virtual bool should_merge(const std::vector<slice> &operands) const {
                return false;
            }
        };

// The simpler, associative merge operator.
        class associative_merge_operator : public merge_operator {
        public:
            ~associative_merge_operator() override {
            }

            // Gives the client a way to express the read -> modify -> write semantics
            // key:           (IN) The key that's associated with this merge operation.
            // existing_value:(IN) null indicates the key does not exist before this op
            // value:         (IN) the value to update/merge the existing_value with
            // new_value:    (OUT) Client is responsible for filling the merge result
            // here. The string that new_value is pointing to will be empty.
            // logger:        (IN) Client could use this to log errors during merge.
            //
            // Return true on success.
            // All values passed in will be client-specific values. So if this method
            // returns false, it is because client specified bad data or there was
            // internal corruption. The client should assume that this will be treated
            // as an error by the library.
            virtual bool merge(const slice &key, const slice *existing_value, const slice &value,
                               std::string *new_value, Logger *logger) const = 0;


        private:
            // default_environment implementations of the merge_operator functions
            bool full_merge_v2(const merge_operation_input &merge_in, merge_operation_output *merge_out) const override;

            bool partial_merge(const slice &key, const slice &left_operand, const slice &right_operand,
                               std::string *new_value, Logger *logger) const override;
        };

    }
} // namespace nil
