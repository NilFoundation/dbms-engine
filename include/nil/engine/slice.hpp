// Slice is a simple structure containing a pointer into some external
// engine and a size.  The user of a slice must ensure that the slice
// is not used after the corresponding external engine has been
// deallocated.
//
// Multiple threads can invoke const methods on a slice without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same slice must use
// external synchronization.

#pragma once

#include <assert.h>
#include <cstdio>
#include <stddef.h>
#include <string.h>
#include <string>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

#include <nil/engine/cleanable.hpp>

namespace nil {
    namespace dcdb {

        class slice {
        public:
            // Create an empty slice.
            slice() : data_(""), size_(0) {
            }

            // Create a slice that refers to d[0,n-1].
            slice(const char *d, size_t n) : data_(d), size_(n) {
            }

            // Create a slice that refers to the contents of "s"
            /* implicit */
            slice(const std::string &s) : data_(s.data()), size_(s.size()) {
            }

#ifdef __cpp_lib_string_view
            // Create a slice that refers to the same contents as "sv"
            /* implicit */
            slice(std::string_view sv) : data_(sv.data()), size_(sv.size()) {}
#endif

            // Create a slice that refers to s[0,strlen(s)-1]
            /* implicit */
            slice(const char *s) : data_(s) {
                size_ = (s == nullptr) ? 0 : strlen(s);
            }

            // Create a single slice from slice_parts using buf as engine.
            // buf must exist as long as the returned slice exists.
            slice(const struct slice_parts &parts, std::string *buf);

            // Return a pointer to the beginning of the referenced data
            const char *data() const {
                return data_;
            }

            // Return the length (in bytes) of the referenced data
            size_t size() const {
                return size_;
            }

            // Return true iff the length of the referenced data is zero
            bool empty() const {
                return size_ == 0;
            }

            // Return the ith byte in the referenced data.
            // REQUIRES: n < size()
            char operator[](size_t n) const {
                assert(n < size());
                return data_[n];
            }

            // Change this slice to refer to an empty array
            void clear() {
                data_ = "";
                size_ = 0;
            }

            // Drop the first "n" bytes from this slice.
            void remove_prefix(size_t n) {
                assert(n <= size());
                data_ += n;
                size_ -= n;
            }

            void remove_suffix(size_t n) {
                assert(n <= size());
                size_ -= n;
            }

            // Return a string that contains the copy of the referenced data.
            // when hex is true, returns a string of twice the length hex encoded (0-9A-F)
            std::string ToString(bool hex = false) const;

#ifdef __cpp_lib_string_view
            // Return a string_view that references the same data as this slice.
            std::string_view ToStringView() const {
              return std::string_view(data_, size_);
            }
#endif

            // Decodes the current slice interpreted as an hexadecimal string into result,
            // if successful returns true, if this isn't a valid hex string
            // (e.g not coming from slice::ToString(true)) DecodeHex returns false.
            // This slice is expected to have an even number of 0-9A-F characters
            // also accepts lowercase (a-f)
            bool DecodeHex(std::string *result) const;

            // Three-way comparison.  Returns value:
            //   <  0 iff "*this" <  "b",
            //   == 0 iff "*this" == "b",
            //   >  0 iff "*this" >  "b"
            int compare(const slice &b) const;

            // Return true iff "x" is a prefix of "*this"
            bool starts_with(const slice &x) const {
                return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
            }

            bool ends_with(const slice &x) const {
                return ((size_ >= x.size_) && (memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
            }

            // compare two slices and returns the first byte where they differ
            size_t difference_offset(const slice &b) const;

            // private: make these public for rocksdbjni access
            const char *data_;
            size_t size_;

            // Intentionally copyable
        };

/**
 * A Slice that can be pinned with some cleanup tasks, which will be run upon
 * ::Reset() or object destruction, whichever is invoked first. This can be used
 * to avoid memcpy by having the PinnableSlice object referring to the data
 * that is locked in the memory and release them after the data is consumed.
 */
        class PinnableSlice : public slice, public cleanable {
        public:
            PinnableSlice() {
                buf_ = &self_space_;
            }

            explicit PinnableSlice(std::string *buf) {
                buf_ = buf;
            }

            // No copy constructor and copy assignment allowed.
            PinnableSlice(PinnableSlice &) = delete;

            PinnableSlice &operator=(PinnableSlice &) = delete;

            inline void PinSlice(const slice &s, cleanup_function f, void *arg1, void *arg2) {
                assert(!pinned_);
                pinned_ = true;
                data_ = s.data();
                size_ = s.size();
                register_cleanup(f, arg1, arg2);
                assert(pinned_);
            }

            inline void PinSlice(const slice &s, cleanable *cleanable) {
                assert(!pinned_);
                pinned_ = true;
                data_ = s.data();
                size_ = s.size();
                cleanable->delegate_cleanups_to(this);
                assert(pinned_);
            }

            inline void PinSelf(const slice &slice) {
                assert(!pinned_);
                buf_->assign(slice.data(), slice.size());
                data_ = buf_->data();
                size_ = buf_->size();
                assert(!pinned_);
            }

            inline void PinSelf() {
                assert(!pinned_);
                data_ = buf_->data();
                size_ = buf_->size();
                assert(!pinned_);
            }

            void remove_suffix(size_t n) {
                assert(n <= size());
                if (pinned_) {
                    size_ -= n;
                } else {
                    buf_->erase(size() - n, n);
                    PinSelf();
                }
            }

            void remove_prefix(size_t /*n*/) {
                assert(0);  // Not implemented
            }

            void reset() {
                cleanable::reset();
                pinned_ = false;
                size_ = 0;
            }

            inline std::string *GetSelf() {
                return buf_;
            }

            inline bool IsPinned() {
                return pinned_;
            }

        private:
            friend class PinnableSlice4Test;

            std::string self_space_;
            std::string *buf_;
            bool pinned_ = false;
        };

// A set of Slices that are virtually concatenated together.  'parts' points
// to an array of Slices.  The number of elements in the array is 'num_parts'.
        struct slice_parts {
            slice_parts(const slice *_parts, int _num_parts) : parts(_parts), num_parts(_num_parts) {
            }

            slice_parts() : parts(nullptr), num_parts(0) {
            }

            const slice *parts;
            int num_parts;
        };

        inline bool operator==(const slice &x, const slice &y) {
            return ((x.size() == y.size()) && (memcmp(x.data(), y.data(), x.size()) == 0));
        }

        inline bool operator!=(const slice &x, const slice &y) {
            return !(x == y);
        }

        inline int slice::compare(const slice &b) const {
            assert(data_ != nullptr && b.data_ != nullptr);
            const size_t min_len = (size_ < b.size_) ? size_ : b.size_;
            int r = memcmp(data_, b.data_, min_len);
            if (r == 0) {
                if (size_ < b.size_) {
                    r = -1;
                } else if (size_ > b.size_) {
                    r = +1;
                }
            }
            return r;
        }

        inline size_t slice::difference_offset(const slice &b) const {
            size_t off = 0;
            const size_t len = (size_ < b.size_) ? size_ : b.size_;
            for (; off < len; off++) {
                if (data_[off] != b.data_[off]) {
                    break;
                }
            }
            return off;
        }
    }
} // namespace nil
