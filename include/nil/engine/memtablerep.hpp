// This file contains the interface that must be implemented by any collection
// to be used as the backing store for a MemTable. Such a collection must
// satisfy the following properties:
//  (1) It does not store duplicate items.
//  (2) It uses mem_table_rep::key_comparator to compare items for iteration and
//     equality.
//  (3) It can be accessed concurrently by multiple readers and can support
//     during reads. However, it needn't support multiple concurrent writes.
//  (4) Items are never deleted.
// The liberal use of assertions is encouraged to enforce (1).
//
// The factory will be passed an MemTableAllocator object when a new mem_table_rep
// is requested.
//
// Users can implement their own memtable representations. We include three
// types built in:
//  - SkipListRep: This is the default; it is backed by a skip list.
//  - HashSkipListRep: The memtable rep that is best used for keys that are
//  structured like "prefix:suffix" where iteration within a prefix is
//  common and iteration across different prefixes is rare. It is backed by
//  a hash map where each bucket is a skip list.
//  - VectorRep: This is backed by an unordered std::vector. On iteration, the
// vector is sorted. It is intelligent about sorting; once the mark_read_only()
// has been called, the vector will only be sorted once. It is optimized for
// random-write-heavy workloads.
//
// The last four implementations are designed for situations in which
// iteration over the entire collection is rare since doing so requires all the
// keys to be copied into a sorted data structure.

#pragma once

#include <memory>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>

#include <nil/engine/slice.hpp>

namespace nil {
    namespace dcdb {

        class Arena;

        class allocator;

        class LookupKey;

        class slice_transform;

        class Logger;

        typedef void *KeyHandle;

        extern slice get_length_prefixed_slice(const char *data);

        class mem_table_rep {
        public:
            // key_comparator provides a means to compare keys, which are internal keys
            // concatenated with values.
            class key_comparator {
            public:
                typedef nil::dcdb::slice decoded_type;

                virtual decoded_type decode_key(const char *key) const {
                    // The format of key is frozen and can be terated as a part of the API
                    // contract. Refer to MemTable::add for details.
                    return get_length_prefixed_slice(key);
                }

                // compare a and b. Return a negative value if a is less than b, 0 if they
                // are equal, and a positive value if a is greater than b
                virtual int operator()(const char *prefix_len_key1, const char *prefix_len_key2) const = 0;

                virtual int operator()(const char *prefix_len_key, const slice &key) const = 0;

                virtual ~key_comparator() {
                }
            };

            explicit mem_table_rep(allocator *a) : allocator_(a) {
            }

            // allocate a buf of len size for storing key. The idea is that a
            // specific memtable representation knows its underlying data structure
            // better. By allowing it to allocate memory, it can possibly insert
            // correlated stuff in consecutive memory area to make processor
            // prefetching more efficient.
            virtual KeyHandle allocate(const size_t len, char **buf);

            // insert key into the collection. (The caller will pack key and value into a
            // single buffer and pass that in as the parameter to insert).
            // REQUIRES: nothing that compares equal to key is currently in the
            // collection, and no concurrent modifications to the table in progress
            virtual void insert(KeyHandle handle) = 0;

            // Same as ::insert
            // Returns false if mem_table_rep_factory::can_handle_duplicated_key() is true and
            // the <key, seq> already exists.
            virtual bool insert_key(KeyHandle handle) {
                insert(handle);
                return true;
            }

            // Same as insert(), but in additional pass a hint to insert location for
            // the key. If hint points to nullptr, a new hint will be populated.
            // otherwise the hint will be updated to reflect the last insert location.
            //
            // Currently only skip-list based memtable implement the interface. Other
            // implementations will fallback to insert() by default.
            virtual void insert_with_hint(KeyHandle handle, void **hint) {
                // Ignore the hint by default.
                insert(handle);
            }

            // Same as ::insert_with_hint
            // Returns false if mem_table_rep_factory::can_handle_duplicated_key() is true and
            // the <key, seq> already exists.
            virtual bool insert_key_with_hint(KeyHandle handle, void **hint) {
                insert_with_hint(handle, hint);
                return true;
            }

            // Like insert(handle), but may be called concurrent with other calls
            // to insert_concurrently for other handles.
            //
            // Returns false if mem_table_rep_factory::can_handle_duplicated_key() is true and
            // the <key, seq> already exists.
            virtual void insert_concurrently(KeyHandle handle);

            // Same as ::insert_concurrently
            // Returns false if mem_table_rep_factory::can_handle_duplicated_key() is true and
            // the <key, seq> already exists.
            virtual bool insert_key_concurrently(KeyHandle handle) {
                insert_concurrently(handle);
                return true;
            }

            // Returns true iff an entry that compares equal to key is in the collection.
            virtual bool contains(const char *key) const = 0;

            // Notify this table rep that it will no longer be added to. By default,
            // does nothing.  After mark_read_only() is called, this table rep will
            // not be written to (ie No more calls to allocate(), insert(),
            // or any writes done directly to entries accessed through the iterator.)
            virtual void mark_read_only() {
            }

            // Notify this table rep that it has been flushed to stable engine.
            // By default, does nothing.
            //
            // Invariant: mark_read_only() is called, before mark_flushed().
            // Note that this method if overridden, should not run for an extended period
            // of time. Otherwise, RocksDB may be blocked.
            virtual void mark_flushed() {
            }

            // Look up key from the mem table, since the first key in the mem table whose
            // user_key matches the one given k, call the function callback_func(), with
            // callback_args directly forwarded as the first parameter, and the mem table
            // key as the second parameter. If the return value is false, then terminates.
            // Otherwise, go through the next key.
            //
            // It's safe for get() to terminate after having finished all the potential
            // key for the k.user_key(), or not.
            //
            // default_environment:
            // get() function with a default value of dynamically construct an iterator,
            // seek and call the call back function.
            virtual void get(const LookupKey &k, void *callback_args,
                             bool (*callback_func)(void *arg, const char *entry));

            virtual uint64_t approximate_num_entries(const slice &start_ikey, const slice &end_key) {
                return 0;
            }

            // Report an approximation of how much memory has been used other than memory
            // that was allocated through the allocator.  Safe to call from any thread.
            virtual size_t approximate_memory_usage() = 0;

            virtual ~mem_table_rep() {
            }

            // Iteration over the contents of a skip collection
            class iterator {
            public:
                // Initialize an iterator over the specified collection.
                // The returned iterator is not valid.
                // explicit iterator(const mem_table_rep* collection);
                virtual ~iterator() {
                }

                // Returns true iff the iterator is positioned at a valid node.
                virtual bool valid() const = 0;

                // Returns the key at the current position.
                // REQUIRES: valid()
                virtual const char *key() const = 0;

                // Advances to the next position.
                // REQUIRES: valid()
                virtual void next() = 0;

                // Advances to the previous position.
                // REQUIRES: valid()
                virtual void prev() = 0;

                // Advance to the first entry with a key >= target
                virtual void seek(const slice &internal_key, const char *memtable_key) = 0;

                // retreat to the first entry with a key <= target
                virtual void seek_for_prev(const slice &internal_key, const char *memtable_key) = 0;

                // Position at the first entry in collection.
                // Final state of iterator is valid() iff collection is not empty.
                virtual void seek_to_first() = 0;

                // Position at the last entry in collection.
                // Final state of iterator is valid() iff collection is not empty.
                virtual void seek_to_last() = 0;
            };

            // Return an iterator over the keys in this representation.
            // arena: If not null, the arena needs to be used to allocate the iterator.
            //        When destroying the iterator, the caller will not call "delete"
            //        but iterator::iterator() directly. The destructor needs to destroy
            //        all the states but those allocated in arena.
            virtual iterator *get_iterator(Arena *arena = nullptr) = 0;

            // Return an iterator that has a special seek semantics. The result of
            // a seek might only include keys with the same prefix as the target key.
            // arena: If not null, the arena is used to allocate the iterator.
            //        When destroying the iterator, the caller will not call "delete"
            //        but iterator::iterator() directly. The destructor needs to destroy
            //        all the states but those allocated in arena.
            virtual iterator *get_dynamic_prefix_iterator(Arena *arena = nullptr) {
                return get_iterator(arena);
            }

            // Return true if the current mem_table_rep supports merge operator.
            // default_environment: true
            virtual bool is_merge_operator_supported() const {
                return true;
            }

            // Return true if the current mem_table_rep supports get_snapshot
            // default_environment: true
            virtual bool is_snapshot_supported() const {
                return true;
            }

        protected:
            // When *key is an internal key concatenated with the value, returns the
            // user key.
            virtual slice user_key(const char *key) const;

            allocator *allocator_;
        };

// This is the base class for all factories that are used by RocksDB to create
// new mem_table_rep objects
        class mem_table_rep_factory {
        public:
            virtual ~mem_table_rep_factory() {
            }

            virtual mem_table_rep *create_mem_table_rep(const mem_table_rep::key_comparator &key_cmp, allocator *a,
                                                        const slice_transform *st, Logger *l) = 0;

            virtual mem_table_rep *create_mem_table_rep(const mem_table_rep::key_comparator &key_cmp,
                                                        allocator *allocator, const slice_transform *slice_transform,
                                                        Logger *logger, uint32_t column_family_id) {
                return create_mem_table_rep(key_cmp, allocator, slice_transform, logger);
            }

            virtual const char *name() const = 0;

            // Return true if the current mem_table_rep supports concurrent inserts
            // default_environment: false
            virtual bool is_insert_concurrently_supported() const {
                return false;
            }

            // Return true if the current mem_table_rep supports detecting duplicate
            // <key,seq> at insertion time. If true, then mem_table_rep::insert* returns
            // false when if the <key,seq> already exists.
            // default_environment: false
            virtual bool can_handle_duplicated_key() const {
                return false;
            }
        };

// This uses a skip list to store keys. It is the default.
//
// Parameters:
//   lookahead: If non-zero, each iterator's seek operation will start the
//     search from the previously visited record (doing at most 'lookahead'
//     steps). This is an optimization for the access pattern including many
//     seeks with consecutive keys.
        class skip_list_factory : public mem_table_rep_factory {
        public:
            explicit skip_list_factory(size_t lookahead = 0) : lookahead_(lookahead) {
            }

            using mem_table_rep_factory::create_mem_table_rep;

            virtual mem_table_rep *create_mem_table_rep(const mem_table_rep::key_comparator &key_cmp, allocator *a,
                                                        const slice_transform *st, Logger *l) override;

            virtual const char *name() const override {
                return "skip_list_factory";
            }

            bool is_insert_concurrently_supported() const override {
                return true;
            }

            bool can_handle_duplicated_key() const override {
                return true;
            }

        private:
            const size_t lookahead_;
        };



// This creates MemTableReps that are backed by an std::vector. On iteration,
// the vector is sorted. This is useful for workloads where iteration is very
// rare and writes are generally not issued after reads begin.
//
// Parameters:
//   count: Passed to the constructor of the underlying std::vector of each
//     VectorRep. On initialization, the underlying array will be at least count
//     bytes reserved for usage.
        class vector_rep_factory : public mem_table_rep_factory {
            const size_t count_;

        public:
            explicit vector_rep_factory(size_t count = 0) : count_(count) {
            }

            using mem_table_rep_factory::create_mem_table_rep;

            virtual mem_table_rep *create_mem_table_rep(const mem_table_rep::key_comparator &key_cmp, allocator *a,
                                                        const slice_transform *st, Logger *l) override;

            virtual const char *name() const override {
                return "vector_rep_factory";
            }
        };

// This class contains a fixed array of buckets, each
// pointing to a skiplist (null if the bucket is empty).
// bucket_count: number of fixed array buckets
// skiplist_height: the max height of the skiplist
// skiplist_branching_factor: probabilistic size ratio between adjacent
//                            link lists in the skiplist
        extern mem_table_rep_factory *new_hash_skip_list_rep_factory(size_t bucket_count = 1000000,
                                                                     int32_t skiplist_height = 4,
                                                                     int32_t skiplist_branching_factor = 4);

// The factory is to create memtables based on a hash table:
// it contains a fixed array of buckets, each pointing to either a linked list
// or a skip list if number of entries inside the bucket exceeds
// threshold_use_skiplist.
// @bucket_count: number of fixed array buckets
// @huge_page_tlb_size: if <=0, allocate the hash table bytes from malloc.
//                      Otherwise from huge page TLB. The user needs to reserve
//                      huge pages for it to be allocated, like:
//                          sysctl -w vm.nr_hugepages=20
//                      See linux doc Documentation/vm/hugetlbpage.txt
// @bucket_entries_logging_threshold: if number of entries in one bucket
//                                    exceeds this number, log about it.
// @if_log_bucket_dist_when_flash: if true, log distribution of number of
//                                 entries when flushing.
// @threshold_use_skiplist: a bucket switches to skip list if number of
//                          entries exceed this parameter.
        extern mem_table_rep_factory *new_hash_link_list_rep_factory(size_t bucket_count = 50000,
                                                                     size_t huge_page_tlb_size = 0,
                                                                     int bucket_entries_logging_threshold = 4096,
                                                                     bool if_log_bucket_dist_when_flash = true,
                                                                     uint32_t threshold_use_skiplist = 256);


    }
} // namespace nil
