// Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// An iterator yields a sequence of key/value pairs from a source.
// The following class defines the interface.  Multiple implementations
// are provided by this library.  In particular, iterators are provided
// to access the contents of a Table or a DB.
//
// Multiple threads can invoke const methods on an Iterator without
// external synchronization, but if any of the threads may call a
// non-const method, all threads accessing the same Iterator must use
// external synchronization.

#pragma once

namespace nil {
    namespace dcdb {

        class cleanable {
        public:
            cleanable();

            ~cleanable();

            // No copy constructor and copy assignment allowed.
            cleanable(cleanable &) = delete;

            cleanable &operator=(cleanable &) = delete;

            // Move constructor and move assignment is allowed.
            cleanable(cleanable &&);

            cleanable &operator=(cleanable &&);

            // Clients are allowed to register function/arg1/arg2 triples that
            // will be invoked when this iterator is destroyed.
            //
            // Note that unlike all of the preceding methods, this method is
            // not abstract and therefore clients should not override it.
            typedef void (*CleanupFunction)(void *arg1, void *arg2);

            void register_cleanup(CleanupFunction function, void *arg1, void *arg2);

            void delegate_cleanups_to(cleanable *other);

            // do_cleanup and also resets the pointers for reuse
            inline void reset() {
                do_cleanup();
                cleanup_.function = nullptr;
                cleanup_.next = nullptr;
            }

        protected:
            struct Cleanup {
                CleanupFunction function;
                void *arg1;
                void *arg2;
                Cleanup *next;
            };
            Cleanup cleanup_;

            // It also becomes the owner of c
            void register_cleanup(Cleanup *c);

        private:
            // Performs all the cleanups. It does not reset the pointers. Making it
            // private
            // to prevent misuse
            inline void do_cleanup() {
                if (cleanup_.function != nullptr) {
                    (*cleanup_.function)(cleanup_.arg1, cleanup_.arg2);
                    for (Cleanup *c = cleanup_.next; c != nullptr;) {
                        (*c->function)(c->arg1, c->arg2);
                        Cleanup *next = c->next;
                        delete c;
                        c = next;
                    }
                }
            }
        };
    }
} // namespace nil
