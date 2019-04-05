// A status_type encapsulates the result of an operation.  It may indicate success,
// or it may indicate an error with an associated error message.
//
// Multiple threads can invoke const methods on a status_type without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same status_type must use
// external synchronization.

#pragma once

#include <string>

#include <nil/engine/slice.hpp>

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

            enum code : unsigned char {
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

            code get_code() const {
                return code_;
            }

            enum sub_code : unsigned char {
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

            sub_code get_subcode() const {
                return subcode_;
            }

            enum severity : unsigned char {
                kNoError = 0, kSoftError = 1, kHardError = 2, kFatalError = 3, kUnrecoverableError = 4, kMaxSeverity
            };

            status_type(const status_type &s, severity sev);

            severity severity() const {
                return sev_;
            }

            // Returns a C style string indicating the message of the status_type
            const char *get_state() const {
                return state_;
            }

            // Return a success status.
            static status_type ok() {
                return status_type();
            }

            // Return error status of an appropriate type.
            static status_type not_found(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kNotFound, msg, msg2);
            }

            // Fast path for not found without malloc;
            static status_type not_found(sub_code msg = kNone) {
                return status_type(kNotFound, msg);
            }

            static status_type corruption(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kCorruption, msg, msg2);
            }

            static status_type corruption(sub_code msg = kNone) {
                return status_type(kCorruption, msg);
            }

            static status_type not_supported(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kNotSupported, msg, msg2);
            }

            static status_type not_supported(sub_code msg = kNone) {
                return status_type(kNotSupported, msg);
            }

            static status_type invalid_argument(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kInvalidArgument, msg, msg2);
            }

            static status_type invalid_argument(sub_code msg = kNone) {
                return status_type(kInvalidArgument, msg);
            }

            static status_type io_error(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIOError, msg, msg2);
            }

            static status_type io_error(sub_code msg = kNone) {
                return status_type(kIOError, msg);
            }

            static status_type merge_in_progress(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kMergeInProgress, msg, msg2);
            }

            static status_type merge_in_progress(sub_code msg = kNone) {
                return status_type(kMergeInProgress, msg);
            }

            static status_type incomplete(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIncomplete, msg, msg2);
            }

            static status_type incomplete(sub_code msg = kNone) {
                return status_type(kIncomplete, msg);
            }

            static status_type shutdown_in_progress(sub_code msg = kNone) {
                return status_type(kShutdownInProgress, msg);
            }

            static status_type shutdown_in_progress(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kShutdownInProgress, msg, msg2);
            }

            static status_type aborted(sub_code msg = kNone) {
                return status_type(kAborted, msg);
            }

            static status_type aborted(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kAborted, msg, msg2);
            }

            static status_type busy(sub_code msg = kNone) {
                return status_type(kBusy, msg);
            }

            static status_type busy(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kBusy, msg, msg2);
            }

            static status_type timed_out(sub_code msg = kNone) {
                return status_type(kTimedOut, msg);
            }

            static status_type timed_out(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kTimedOut, msg, msg2);
            }

            static status_type expired(sub_code msg = kNone) {
                return status_type(kExpired, msg);
            }

            static status_type expired(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kExpired, msg, msg2);
            }

            static status_type try_again(sub_code msg = kNone) {
                return status_type(kTryAgain, msg);
            }

            static status_type try_again(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kTryAgain, msg, msg2);
            }

            static status_type compaction_too_large(sub_code msg = kNone) {
                return status_type(kCompactionTooLarge, msg);
            }

            static status_type compaction_too_large(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kCompactionTooLarge, msg, msg2);
            }

            static status_type no_space() {
                return status_type(kIOError, kNoSpace);
            }

            static status_type no_space(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIOError, kNoSpace, msg, msg2);
            }

            static status_type memory_limit() {
                return status_type(kAborted, kMemoryLimit);
            }

            static status_type memory_limit(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kAborted, kMemoryLimit, msg, msg2);
            }

            static status_type space_limit() {
                return status_type(kIOError, kSpaceLimit);
            }

            static status_type space_limit(const slice &msg, const slice &msg2 = slice()) {
                return status_type(kIOError, kSpaceLimit, msg, msg2);
            }

            // Returns true iff the status indicates success.
            bool is_ok() const {
                return get_code() == kOk;
            }

            // Returns true iff the status indicates a not_found error.
            bool is_not_found() const {
                return get_code() == kNotFound;
            }

            // Returns true iff the status indicates a corruption error.
            bool is_corruption() const {
                return get_code() == kCorruption;
            }

            // Returns true iff the status indicates a not_supported error.
            bool is_not_supported() const {
                return get_code() == kNotSupported;
            }

            // Returns true iff the status indicates an invalid_argument error.
            bool is_invalid_argument() const {
                return get_code() == kInvalidArgument;
            }

            // Returns true iff the status indicates an io_error.
            bool is_io_error() const {
                return get_code() == kIOError;
            }

            // Returns true iff the status indicates an merge_in_progress.
            bool is_merge_in_progress() const {
                return get_code() == kMergeInProgress;
            }

            // Returns true iff the status indicates incomplete
            bool is_incomplete() const {
                return get_code() == kIncomplete;
            }

            // Returns true iff the status indicates Shutdown In progress
            bool is_shutdown_in_progress() const {
                return get_code() == kShutdownInProgress;
            }

            bool is_timed_out() const {
                return get_code() == kTimedOut;
            }

            bool is_aborted() const {
                return get_code() == kAborted;
            }

            bool is_lock_limit() const {
                return get_code() == kAborted && get_subcode() == kLockLimit;
            }

            // Returns true iff the status indicates that a resource is busy and
            // temporarily could not be acquired.
            bool is_busy() const {
                return get_code() == kBusy;
            }

            bool is_deadlock() const {
                return get_code() == kBusy && get_subcode() == kDeadlock;
            }

            // Returns true iff the status indicated that the operation has expired.
            bool is_expired() const {
                return get_code() == kExpired;
            }

            // Returns true iff the status indicates a try_again error.
            // This usually means that the operation failed, but may succeed if
            // re-attempted.
            bool is_try_again() const {
                return get_code() == kTryAgain;
            }

            // Returns true iff the status indicates the proposed compaction is too large
            bool is_compaction_too_large() const {
                return get_code() == kCompactionTooLarge;
            }

            // Returns true iff the status indicates a no_space error
            // This is caused by an I/O error returning the specific "out of space"
            // error condition. Stricto sensu, an no_space error is an I/O error
            // with a specific get_subcode, enabling users to take the appropriate action
            // if needed
            bool is_no_space() const {
                return (get_code() == kIOError) && (get_subcode() == kNoSpace);
            }

            // Returns true iff the status indicates a memory limit error.  There may be
            // cases where we limit the memory used in certain operations (eg. the size
            // of a write batch) in order to avoid out of memory exceptions.
            bool is_memory_limit() const {
                return (get_code() == kAborted) && (get_subcode() == kMemoryLimit);
            }

            // Return a string representation of this status suitable for printing.
            // Returns the string "is_ok" for success.
            std::string to_string() const;

        private:
            // A nullptr state_ (which is always the case for is_ok) means the message
            // is empty.
            // of the following form:
            //    state_[0..3] == length of message
            //    state_[4..]  == message
            code code_;
            sub_code subcode_;
            severity sev_;
            const char *state_;

            explicit status_type(code _code, sub_code _subcode = kNone) : code_(_code), subcode_(_subcode),
                    sev_(kNoError), state_(nullptr) {
            }

            status_type(code _code, sub_code _subcode, const slice &msg, const slice &msg2);

            status_type(code _code, const slice &msg, const slice &msg2) : status_type(_code, kNone, msg, msg2) {
            }

            static const char *copy_state(const char *s);
        };

        inline status_type::status_type(const status_type &s) : code_(s.code_), subcode_(s.subcode_), sev_(s.sev_) {
            state_ = (s.state_ == nullptr) ? nullptr : copy_state(s.state_);
        }

        inline status_type::status_type(const status_type &s, severity sev) : code_(s.code_), subcode_(s.subcode_),
                sev_(sev) {
            state_ = (s.state_ == nullptr) ? nullptr : copy_state(s.state_);
        }

        inline status_type &status_type::operator=(const status_type &s) {
            // The following condition catches both aliasing (when this == &s),
            // and the common case where both s and *this are is_ok.
            if (this != &s) {
                code_ = s.code_;
                subcode_ = s.subcode_;
                sev_ = s.sev_;
                delete[] state_;
                state_ = (s.state_ == nullptr) ? nullptr : copy_state(s.state_);
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
                code_ = s.code_;
                s.code_ = kOk;
                subcode_ = s.subcode_;
                s.subcode_ = kNone;
                sev_ = s.sev_;
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
