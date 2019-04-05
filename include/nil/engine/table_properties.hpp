#pragma once

#include <cstdint>
#include <map>
#include <string>

#include <nil/engine/status.hpp>
#include <nil/engine/types.hpp>

namespace nil {
    namespace dcdb {

// -- Table properties
// Other than basic table properties, each table may also have the user
// collected properties.
// The value of the user-collected properties are encoded as raw bytes --
// users have to interpret these values by themselves.
// Note: To do prefix seek/scan in `user_properties`, you can do
// something similar to:
//
// user_properties props = ...;
// for (auto pos = props.lower_bound(prefix);
//      pos != props.end() && pos->first.compare(0, prefix.size(), prefix) == 0;
//      ++pos) {
//   ...
// }
        typedef std::map<std::string, std::string> user_collected_properties;

// table properties' human-readable names in the property block.
        struct table_properties_names {
            static const std::string kDataSize;
            static const std::string kIndexSize;
            static const std::string kIndexPartitions;
            static const std::string kTopLevelIndexSize;
            static const std::string kIndexKeyIsUserKey;
            static const std::string kIndexValueIsDeltaEncoded;
            static const std::string kFilterSize;
            static const std::string kRawKeySize;
            static const std::string kRawValueSize;
            static const std::string kNumDataBlocks;
            static const std::string kNumEntries;
            static const std::string kDeletedKeys;
            static const std::string kMergeOperands;
            static const std::string kNumRangeDeletions;
            static const std::string kFormatVersion;
            static const std::string kFixedKeyLen;
            static const std::string kFilterPolicy;
            static const std::string kColumnFamilyName;
            static const std::string kColumnFamilyId;
            static const std::string kComparator;
            static const std::string kMergeOperator;
            static const std::string kPrefixExtractorName;
            static const std::string kPropertyCollectors;
            static const std::string kCompression;
            static const std::string kCreationTime;
            static const std::string kOldestKeyTime;
        };

        extern const std::string kPropertiesBlock;
        extern const std::string kCompressionDictBlock;
        extern const std::string kRangeDelBlock;

// `table_properties_collector` provides the mechanism for users to collect
// their own properties that they are interested in. This class is essentially
// a collection of callback functions that will be invoked during table
// building. It is constructed with table_properties_collector_factory. The methods
// don't need to be thread-safe, as we will create exactly one
// table_properties_collector object per table and then call it sequentially
        class table_properties_collector {
        public:
            virtual ~table_properties_collector() {
            }

            // DEPRECATE User defined collector should implement add_user_key(), though
            //           this old function still works for backward compatible reason.
            // add() will be called when a new key/value pair is inserted into the table.
            // @params key    the user key that is inserted into the table.
            // @params value  the value that is inserted into the table.
            virtual status_type add(const slice &key, const slice &value) {
                return status_type::invalid_argument("table_properties_collector::add() deprecated.");
            }

            // add_user_key() will be called when a new key/value pair is inserted into the
            // table.
            // @params key    the user key that is inserted into the table.
            // @params value  the value that is inserted into the table.
            virtual status_type add_user_key(const slice &key, const slice &value, entry_type type, sequence_number seq,
                                             uint64_t file_size) {
                // For backwards-compatibility.
                return add(key, value);
            }

            // Called after each new block is cut
            virtual void block_add(uint64_t blockRawBytes, uint64_t blockCompressedBytesFast,
                                   uint64_t blockCompressedBytesSlow) {
                // Nothing to do here. Callback registers can override.
            }

            // finish() will be called when a table has already been built and is ready
            // for writing the properties block.
            // @params properties  User will add their collected statistics to
            // `properties`.
            virtual status_type finish(user_collected_properties *properties) = 0;

            // Return the human-readable properties, where the key is property name and
            // the value is the human-readable form of value.
            virtual user_collected_properties get_readable_properties() const = 0;

            // The name of the properties collector can be used for debugging purpose.
            virtual const char *name() const = 0;

            // EXPERIMENTAL Return whether the output file should be further compacted
            virtual bool need_compact() const {
                return false;
            }
        };

// Constructs table_properties_collector. Internals create a new
// table_properties_collector for each new table
        class table_properties_collector_factory {
        public:
            struct context {
                uint32_t column_family_id;
                static const uint32_t kUnknownColumnFamily;
            };

            virtual ~table_properties_collector_factory() {
            }

            // has to be thread-safe
            virtual table_properties_collector *create_table_properties_collector(
                    table_properties_collector_factory::context context) = 0;

            // The name of the properties collector can be used for debugging purpose.
            virtual const char *name() const = 0;
        };

// table_properties contains a bunch of read-only properties of its associated
// table.
        struct table_properties {
        public:
            // the total size of all data blocks.
            uint64_t data_size = 0;
            // the size of index block.
            uint64_t index_size = 0;
            // Total number of index partitions if kTwoLevelIndexSearch is used
            uint64_t index_partitions = 0;
            // Size of the top-level index if kTwoLevelIndexSearch is used
            uint64_t top_level_index_size = 0;
            // Whether the index key is user key. Otherwise it includes 8 byte of sequence
            // number added by internal key format.
            uint64_t index_key_is_user_key = 0;
            // Whether delta encoding is used to encode the index values.
            uint64_t index_value_is_delta_encoded = 0;
            // the size of filter block.
            uint64_t filter_size = 0;
            // total raw key size
            uint64_t raw_key_size = 0;
            // total raw value size
            uint64_t raw_value_size = 0;
            // the number of blocks in this table
            uint64_t num_data_blocks = 0;
            // the number of entries in this table
            uint64_t num_entries = 0;
            // the number of deletions in the table
            uint64_t num_deletions = 0;
            // the number of merge operands in the table
            uint64_t num_merge_operands = 0;
            // the number of range deletions in this table
            uint64_t num_range_deletions = 0;
            // format version, reserved for backward compatibility
            uint64_t format_version = 0;
            // If 0, key is variable length. Otherwise number of bytes for each key.
            uint64_t fixed_key_len = 0;
            // ID of column family for this SST file, corresponding to the CF identified
            // by column_family_name.
            uint64_t column_family_id = nil::dcdb::table_properties_collector_factory::context::kUnknownColumnFamily;
            // The time when the SST file was created.
            // Since SST files are immutable, this is equivalent to last modified time.
            uint64_t creation_time = 0;
            // Timestamp of the earliest key. 0 means unknown.
            uint64_t oldest_key_time = 0;

            // name of the column family with which this SST file is associated.
            // If column family is unknown, `column_family_name` will be an empty string.
            std::string column_family_name;

            // The name of the filter policy used in this table.
            // If no filter policy is used, `filter_policy_name` will be an empty string.
            std::string filter_policy_name;

            // The name of the comparator used in this table.
            std::string comparator_name;

            // The name of the merge operator used in this table.
            // If no merge operator is used, `merge_operator_name` will be "nullptr".
            std::string merge_operator_name;

            // The name of the prefix extractor used in this table
            // If no prefix extractor is used, `prefix_extractor_name` will be "nullptr".
            std::string prefix_extractor_name;

            // The names of the property collectors factories used in this table
            // separated by commas
            // {collector_name[1]},{collector_name[2]},{collector_name[3]} ..
            std::string property_collectors_names;

            // The compression algo used to compress the SST files.
            std::string compression_name;

            // user collected properties
            user_collected_properties user_properties;
            user_collected_properties readable_properties;

            // The offset of the value of each property in the file.
            std::map<std::string, uint64_t> properties_offsets;

            // convert this object to a human readable form
            //   @prop_delim: delimiter for each property.
            std::string to_string(const std::string &prop_delim = "; ", const std::string &kv_delim = "=") const;

            // Aggregate the numerical member variables of the specified
            // table_properties.
            void add(const table_properties &tp);
        };

// Extra properties
// Below is a list of non-basic properties that are collected by database
// itself. Especially some properties regarding to the internal keys (which
// is unknown to `table`).
//
// DEPRECATED: these properties now belong as table_properties members. Please
// use table_properties::num_deletions and table_properties::num_merge_operands,
// respectively.
        extern uint64_t get_deleted_keys(const user_collected_properties &props);

        extern uint64_t get_merge_operands(const user_collected_properties &props, bool *property_present);

    }
} // namespace nil