//---------------------------------------------------------------------------//
// Copyright (c) 2018-2019 Nil Foundation
// Copyright (c) 2018-2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

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

#include <cassert>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <string>

#ifdef __cpp_lib_string_view
#include <string_view>
#endif

namespace nil {
    namespace engine {
        class slice;

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

        class slice {
            // 2 small internal utility functions, for efficient hex conversions
            // and no need for snprintf, toupper etc...
            // Originally from wdt/system/EncryptionUtils.cpp - for to_string(true)/decode_hex:
            inline static char toHex(unsigned char v) {
                if (v <= 9) {
                    return '0' + v;
                }
                return 'A' + v - 10;
            }

            // most of the get_code is for validation/error check
            inline static int fromHex(char c) {
                // toupper:
                if (c >= 'a' && c <= 'f') {
                    c -= ('a' - 'A');    // aka 0x20
                }
                // validation
                if (c < '0' || (c > '9' && (c < 'A' || c > 'F'))) {
                    return -1;    // invalid not 0-9A-F hex char
                }
                if (c <= '9') {
                    return c - '0';
                }
                return c - 'A' + 10;
            }

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
            slice(std::string_view sv) : data_(sv.data()), size_(sv.size()) {
            }
#endif

            // Create a slice that refers to s[0,strlen(s)-1]
            /* implicit */
            slice(const char *s) : data_(s) {
                size_ = (s == nullptr) ? 0 : strlen(s);
            }

            // Create a single slice from slice_parts using buf as engine.
            // buf must exist as long as the returned slice exists.
            slice(const struct slice_parts &parts, std::string *buf) {
                size_t length = 0;
                for (int i = 0; i < parts.num_parts; ++i) {
                    length += parts.parts[i].size();
                }
                buf->reserve(length);

                for (int i = 0; i < parts.num_parts; ++i) {
                    buf->append(parts.parts[i].data(), parts.parts[i].size());
                }
                data_ = buf->data();
                size_ = buf->size();
            }

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
            std::string to_string(bool hex = false) const {
                std::string result;    // RVO/NRVO/move
                if (hex) {
                    result.reserve(2 * size_);
                    for (size_t i = 0; i < size_; ++i) {
                        unsigned char c = data_[i];
                        result.push_back(toHex(c >> 4U));
                        result.push_back(toHex(c & 0xfU));
                    }
                    return result;
                } else {
                    result.assign(data_, size_);
                    return result;
                }
            }

#ifdef __cpp_lib_string_view
            // Return a string_view that references the same data as this slice.
            std::string_view to_string_view() const {
                return std::string_view(data_, size_);
            }
#endif

            // Decodes the current slice interpreted as an hexadecimal string into result,
            // if successful returns true, if this isn't a valid hex string
            // (e.g not coming from slice::to_string(true)) decode_hex returns false.
            // This slice is expected to have an even number of 0-9A-F characters
            // also accepts lowercase (a-f)
            bool decode_hex(std::string *result) const {
                std::string::size_type len = size_;
                if (len % 2) {
                    // Hex string must be even number of hex digits to get complete bytes back
                    return false;
                }
                if (!result) {
                    return false;
                }
                result->clear();
                result->reserve(len / 2);

                for (size_t i = 0; i < len;) {
                    int h1 = fromHex(data_[i++]);
                    if (h1 < 0) {
                        return false;
                    }
                    int h2 = fromHex(data_[i++]);
                    if (h2 < 0) {
                        return false;
                    }
                    result->push_back(static_cast<char>((h1 << 4U) | h2));
                }
                return true;
            }

            // Three-way comparison.  Returns value:
            //   <  0 iff "*this" <  "b",
            //   == 0 iff "*this" == "b",
            //   >  0 iff "*this" >  "b"
            int compare(const slice &b) const {
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

            // Return true iff "x" is a prefix of "*this"
            bool starts_with(const slice &x) const {
                return ((size_ >= x.size_) && (memcmp(data_, x.data_, x.size_) == 0));
            }

            bool ends_with(const slice &x) const {
                return ((size_ >= x.size_) && (memcmp(data_ + size_ - x.size_, x.data_, x.size_) == 0));
            }

            // compare two slices and returns the first byte where they differ
            size_t difference_offset(const slice &b) const {
                size_t off = 0;
                const size_t len = (size_ < b.size_) ? size_ : b.size_;
                for (; off < len; off++) {
                    if (data_[off] != b.data_[off]) {
                        break;
                    }
                }
                return off;
            }

            // private: make these public for rocksdbjni access
            const char *data_;
            size_t size_;

            // Intentionally copyable
        };

        inline bool operator==(const slice &x, const slice &y) {
            return ((x.size() == y.size()) && (memcmp(x.data(), y.data(), x.size()) == 0));
        }

        inline bool operator!=(const slice &x, const slice &y) {
            return !(x == y);
        }
    }    // namespace engine
}    // namespace nil
