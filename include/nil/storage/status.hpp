// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// A status_type encapsulates the result of an operation.  It may indicate success,
// or it may indicate an error with an associated error message.
//
// Multiple threads can invoke const methods on a status_type without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same status_type must use
// external synchronization.

#pragma once

#include <string>

#include <nil/storage/slice.hpp>

namespace nil {
    namespace dcdb {

        class status_type {
        public:
            // Create a success status.
            status_type() : code_(kOk), subcode_(kNone), sev_(kNoError), state_(nullptr) {
            }

            ~status_type() {
                delete[] state_;
            }

            // Copy the specified status.
            status_type(const status_type &s);

            status_type &operator=(const status_type &s);

            status_type(status_type &&s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
            noexcept
#endif
            ;

            status_type &operator=(status_type &&s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
            noexcept
#endif
            ;

            bool operator==(const status_type &rhs) const;

            bool operator!=(const status_type &rhs) const;

            enum Code : unsigned char {
                kOk = 0,
                kNotFound = 1,
                kCorruption = 2,
                kNotSupported = 3,
                kInvalidArgument = 4,
                kIOError = 5,
                kMergeInProgress = 6,
                kIncomplete = 7,
                kShutdownInProgress = 8,
                kTimedOut = 9,
                kAborted = 10,
                kBusy = 11,
                kExpired = 12,
                kTryAgain = 13,
                kCompactionTooLarge = 14
            };

            Code code() const {
                return code_;
            }

            enum SubCode : unsigned char {
                kNone = 0,
                kMutexTimeout = 1,
                kLockTimeout = 2,
                kLockLimit = 3,
                kNoSpace = 4,
                kDeadlock = 5,
                kStaleFile = 6,
                kMemoryLimit = 7,
                kSpaceLimit = 8,
                kMaxSubCode
            };

            SubCode subcode() const {
                return subcode_;
            }

            enum Severity : unsigned char {
                kNoError = 0, kSoftError = 1, kHardError = 2, kFatalError = 3, kUnrecoverableError = 4, kMaxSeverity
            };

            status_type(const status_type &s, Severity sev);

            Severity severity() const {
                return sev_;
            }

            // Returns a C style string indicating the message of the status_type
            const char *get_state() const {
                return state_;
            }

            // Return a success status.
            static status_type OK() {
                return status_type();
            }

            // Return error status of an appropriate type.
            static status_type NotFound(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kNotFound, msg, msg2);
            }

            // Fast path for not found without malloc;
            static status_type NotFound(SubCode msg = kNone) {
                return status_type(kNotFound, msg);
            }

            static status_type Corruption(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kCorruption, msg, msg2);
            }

            static status_type Corruption(SubCode msg = kNone) {
                return status_type(kCorruption, msg);
            }

            static status_type NotSupported(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kNotSupported, msg, msg2);
            }

            static status_type NotSupported(SubCode msg = kNone) {
                return status_type(kNotSupported, msg);
            }

            static status_type InvalidArgument(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kInvalidArgument, msg, msg2);
            }

            static status_type InvalidArgument(SubCode msg = kNone) {
                return status_type(kInvalidArgument, msg);
            }

            static status_type IOError(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIOError, msg, msg2);
            }

            static status_type IOError(SubCode msg = kNone) {
                return status_type(kIOError, msg);
            }

            static status_type MergeInProgress(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kMergeInProgress, msg, msg2);
            }

            static status_type MergeInProgress(SubCode msg = kNone) {
                return status_type(kMergeInProgress, msg);
            }

            static status_type Incomplete(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIncomplete, msg, msg2);
            }

            static status_type Incomplete(SubCode msg = kNone) {
                return status_type(kIncomplete, msg);
            }

            static status_type ShutdownInProgress(SubCode msg = kNone) {
                return status_type(kShutdownInProgress, msg);
            }

            static status_type ShutdownInProgress(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kShutdownInProgress, msg, msg2);
            }

            static status_type Aborted(SubCode msg = kNone) {
                return status_type(kAborted, msg);
            }

            static status_type Aborted(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kAborted, msg, msg2);
            }

            static status_type Busy(SubCode msg = kNone) {
                return status_type(kBusy, msg);
            }

            static status_type Busy(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kBusy, msg, msg2);
            }

            static status_type TimedOut(SubCode msg = kNone) {
                return status_type(kTimedOut, msg);
            }

            static status_type TimedOut(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kTimedOut, msg, msg2);
            }

            static status_type Expired(SubCode msg = kNone) {
                return status_type(kExpired, msg);
            }

            static status_type Expired(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kExpired, msg, msg2);
            }

            static status_type TryAgain(SubCode msg = kNone) {
                return status_type(kTryAgain, msg);
            }

            static status_type TryAgain(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kTryAgain, msg, msg2);
            }

            static status_type CompactionTooLarge(SubCode msg = kNone) {
                return status_type(kCompactionTooLarge, msg);
            }

            static status_type CompactionTooLarge(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kCompactionTooLarge, msg, msg2);
            }

            static status_type NoSpace() {
                return status_type(kIOError, kNoSpace);
            }

            static status_type NoSpace(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIOError, kNoSpace, msg, msg2);
            }

            static status_type MemoryLimit() {
                return status_type(kAborted, kMemoryLimit);
            }

            static status_type MemoryLimit(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kAborted, kMemoryLimit, msg, msg2);
            }

            static status_type SpaceLimit() {
                return status_type(kIOError, kSpaceLimit);
            }

            static status_type SpaceLimit(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIOError, kSpaceLimit, msg, msg2);
            }

            // Returns true iff the status indicates success.
            bool ok() const {
                return code() == kOk;
            }

            // Returns true iff the status indicates a NotFound error.
            bool IsNotFound() const {
                return code() == kNotFound;
            }

            // Returns true iff the status indicates a Corruption error.
            bool IsCorruption() const {
                return code() == kCorruption;
            }

            // Returns true iff the status indicates a NotSupported error.
            bool IsNotSupported() const {
                return code() == kNotSupported;
            }

            // Returns true iff the status indicates an InvalidArgument error.
            bool IsInvalidArgument() const {
                return code() == kInvalidArgument;
            }

            // Returns true iff the status indicates an IOError.
            bool IsIOError() const {
                return code() == kIOError;
            }

            // Returns true iff the status indicates an MergeInProgress.
            bool IsMergeInProgress() const {
                return code() == kMergeInProgress;
            }

            // Returns true iff the status indicates Incomplete
            bool IsIncomplete() const {
                return code() == kIncomplete;
            }

            // Returns true iff the status indicates Shutdown In progress
            bool IsShutdownInProgress() const {
                return code() == kShutdownInProgress;
            }

            bool IsTimedOut() const {
                return code() == kTimedOut;
            }

            bool IsAborted() const {
                return code() == kAborted;
            }

            bool IsLockLimit() const {
                return code() == kAborted && subcode() == kLockLimit;
            }

            // Returns true iff the status indicates that a resource is Busy and
            // temporarily could not be acquired.
            bool IsBusy() const {
                return code() == kBusy;
            }

            bool IsDeadlock() const {
                return code() == kBusy && subcode() == kDeadlock;
            }

            // Returns true iff the status indicated that the operation has Expired.
            bool IsExpired() const {
                return code() == kExpired;
            }

            // Returns true iff the status indicates a TryAgain error.
            // This usually means that the operation failed, but may succeed if
            // re-attempted.
            bool IsTryAgain() const {
                return code() == kTryAgain;
            }

            // Returns true iff the status indicates the proposed compaction is too large
            bool IsCompactionTooLarge() const {
                return code() == kCompactionTooLarge;
            }

            // Returns true iff the status indicates a NoSpace error
            // This is caused by an I/O error returning the specific "out of space"
            // error condition. Stricto sensu, an NoSpace error is an I/O error
            // with a specific subcode, enabling users to take the appropriate action
            // if needed
            bool IsNoSpace() const {
                return (code() == kIOError) && (subcode() == kNoSpace);
            }

            // Returns true iff the status indicates a memory limit error.  There may be
            // cases where we limit the memory used in certain operations (eg. the size
            // of a write batch) in order to avoid out of memory exceptions.
            bool IsMemoryLimit() const {
                return (code() == kAborted) && (subcode() == kMemoryLimit);
            }

            // Return a string representation of this status suitable for printing.
            // Returns the string "OK" for success.
            std::string ToString() const;

        private:
            // A nullptr state_ (which is always the case for OK) means the message
            // is empty.
            // of the following form:
            //    state_[0..3] == length of message
            //    state_[4..]  == message
            Code code_;
            SubCode subcode_;
            Severity sev_;
            const char *state_;

            explicit status_type(Code _code, SubCode _subcode = kNone) : code_(_code), subcode_(_subcode),
                    sev_(kNoError), state_(nullptr) {
            }

            status_type(Code _code, SubCode _subcode, const slice &msg, const slice &msg2);

            status_type(Code _code, const slice &msg, const slice &msg2) : status_type(_code, kNone, msg, msg2) {
            }

            static const char *CopyState(const char *s);
        };

        inline status_type::status_type(const status_type &s) : code_(s.code_), subcode_(s.subcode_), sev_(s.sev_) {
            state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_);
        }

        inline status_type::status_type(const status_type &s, Severity sev) : code_(s.code_), subcode_(s.subcode_),
                sev_(sev) {
            state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_);
        }

        inline status_type &status_type::operator=(const status_type &s) {
            // The following condition catches both aliasing (when this == &s),
            // and the common case where both s and *this are ok.
            if (this != &s) {
                code_ = s.code_;
                subcode_ = s.subcode_;
                sev_ = s.sev_;
                delete[] state_;
                state_ = (s.state_ == nullptr) ? nullptr : CopyState(s.state_);
            }
            return *this;
        }

        inline status_type::status_type(status_type &&s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
        noexcept
#endif
                : status_type() {
            *this = std::move(s);
        }

        inline status_type &status_type::operator=(status_type &&s)
#if !(defined _MSC_VER) || ((defined _MSC_VER) && (_MSC_VER >= 1900))
        noexcept
#endif
        {
            if (this != &s) {
                code_ = std::move(s.code_);
                s.code_ = kOk;
                subcode_ = std::move(s.subcode_);
                s.subcode_ = kNone;
                sev_ = std::move(s.sev_);
                s.sev_ = kNoError;
                delete[] state_;
                state_ = nullptr;
                std::swap(state_, s.state_);
            }
            return *this;
        }

        inline bool status_type::operator==(const status_type &rhs) const {
            return (code_ == rhs.code_);
        }

        inline bool status_type::operator!=(const status_type &rhs) const {
            return !(*this == rhs);
        }
    }
} // namespace nil
