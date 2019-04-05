// A database can be configured with a custom filter_policy object.
// This object is responsible for creating a small filter from a set
// of keys.  These filters are stored in rocksdb and are consulted
// automatically by rocksdb to decide whether or not to read some
// information from disk. In many cases, a filter can cut down the
// number of disk seeks form a handful to a single disk seek per
// database::get() call.
//
// Most people will want to use the builtin bloom filter support (see
// new_bloom_filter_policy() below).

#pragma once

#include <memory>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <vector>

namespace nil {
    namespace dcdb {

        class slice;

// A class that takes a bunch of keys, then generates filter
        class filter_bits_builder {
        public:
            virtual ~filter_bits_builder() {
            }

            // add Key to filter, you could use any way to store the key.
            // Such as: storing hashes or original keys
            // Keys are in sorted order and duplicated keys are possible.
            virtual void add_key(const slice &key) = 0;

            // Generate the filter using the keys that are added
            // The return value of this function would be the filter bits,
            // The ownership of actual data is set to buf
            virtual slice finish(std::unique_ptr<const char[]> *buf) = 0;

            // Calculate num of entries fit into a space.
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4702) // unreachable get_code
#endif

            virtual int calculate_num_entry(const uint32_t space) {
#ifndef DCDB_LITE
                throw std::runtime_error("calculate_num_entry not Implemented");
#else
                abort();
#endif
                return 0;
            }

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
        };

// A class that checks if a key can be in filter
// It should be initialized by slice generated by BitsBuilder
        class filter_bits_reader {
        public:
            virtual ~filter_bits_reader() {
            }

            // Check if the entry match the bits in filter
            virtual bool may_match(const slice &entry) = 0;
        };

// We add a new format of filter block called full filter block
// This new interface gives you more space of customization
//
// For the full filter block, you can plug in your version by implement
// the filter_bits_builder and filter_bits_reader
//
// There are two sets of interface in filter_policy
// Set 1: create_filter, key_may_match: used for blockbased filter
// Set 2: get_filter_bits_builder, get_filter_bits_reader, they are used for
// full filter.
// Set 1 MUST be implemented correctly, Set 2 is optional
// RocksDB would first try using functions in Set 2. if they return nullptr,
// it would use Set 1 instead.
// You can choose filter type in new_bloom_filter_policy
        class filter_policy {
        public:
            virtual ~filter_policy();

            // Return the name of this policy.  Note that if the filter encoding
            // changes in an incompatible way, the name returned by this method
            // must be changed.  Otherwise, old incompatible filters may be
            // passed to methods of this type.
            virtual const char *name() const = 0;

            // keys[0,n-1] contains a list of keys (potentially with duplicates)
            // that are ordered according to the user supplied comparator.
            // append a filter that summarizes keys[0,n-1] to *dst.
            //
            // Warning: do not change the initial contents of *dst.  Instead,
            // append the newly constructed filter to *dst.
            virtual void create_filter(const slice *keys, int n, std::string *dst) const = 0;

            // "filter" contains the data appended by a preceding call to
            // create_filter() on this class.  This method must return true if
            // the key was in the list of keys passed to create_filter().
            // This method may return true or false if the key was not on the
            // list, but it should aim to return false with a high probability.
            virtual bool key_may_match(const slice &key, const slice &filter) const = 0;

            // get the filter_bits_builder, which is ONLY used for full filter block
            // It contains interface to take individual key, then generate filter
            virtual filter_bits_builder *get_filter_bits_builder() const {
                return nullptr;
            }

            // get the filter_bits_reader, which is ONLY used for full filter block
            // It contains interface to tell if key can be in filter
            // The input slice should NOT be deleted by filter_policy
            virtual filter_bits_reader *get_filter_bits_reader(const slice &contents) const {
                return nullptr;
            }
        };

// Return a new filter policy that uses a bloom filter with approximately
// the specified number of bits per key.
//
// bits_per_key: bits per key in bloom filter. A good value for bits_per_key
// is 10, which yields a filter with ~ 1% false positive rate.
// use_block_based_builder: use block based filter rather than full filter.
// If you want to builder full filter, it needs to be set to false.
//
// Callers must delete the result after any database that is using the
// result has been closed.
//
// Note: if you are using a custom comparator that ignores some parts
// of the keys being compared, you must not use new_bloom_filter_policy()
// and must provide your own filter_policy that also ignores the
// corresponding parts of the keys.  For example, if the comparator
// ignores trailing spaces, it would be incorrect to use a
// filter_policy (like new_bloom_filter_policy) that does not ignore
// trailing spaces in keys.
        extern const filter_policy *new_bloom_filter_policy(int bits_per_key, bool use_block_based_builder = false);
    }
}