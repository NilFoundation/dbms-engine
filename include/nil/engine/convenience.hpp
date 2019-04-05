#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <nil/engine/db.hpp>
#include <nil/engine/options.hpp>
#include <nil/engine/table.hpp>

namespace nil {
    namespace dcdb {

#ifndef DCDB_LITE
// The following set of functions provide a way to construct RocksDB opts
// from a string or a string-to-string map.  Here're the general rule of
// setting option values from strings by type.  Some RocksDB types are also
// supported in these APIs.  Please refer to the comment of the function itself
// to find more information about how to config those RocksDB types.
//
// * Strings:
//   Strings will be used as values directly without any truncating or
//   trimming.
//
// * Booleans:
//   - "true" or "1" => true
//   - "false" or "0" => false.
//   [Example]:
//   - {"optimize_filters_for_hits", "1"} in get_column_family_options_from_map, or
//   - "optimize_filters_for_hits=true" in get_column_family_options_from_string.
//
// * Integers:
//   Integers are converted directly from string, in addition to the following
//   units that we support:
//   - 'k' or 'K' => 2^10
//   - 'm' or 'M' => 2^20
//   - 'g' or 'G' => 2^30
//   - 't' or 'T' => 2^40  // only for unsigned int with sufficient bits.
//   [Example]:
//   - {"arena_block_size", "19G"} in get_column_family_options_from_map, or
//   - "arena_block_size=19G" in get_column_family_options_from_string.
//
// * Doubles / Floating Points:
//   Doubles / Floating Points are converted directly from string.  Note that
//   currently we do not support units.
//   [Example]:
//   - {"hard_rate_limit", "2.1"} in get_column_family_options_from_map, or
//   - "hard_rate_limit=2.1" in get_column_family_options_from_string.
// * Array / Vectors:
//   An array is specified by a list of values, where ':' is used as
//   the delimiter to separate each value.
//   [Example]:
//   - {"compression_per_level", "kNoCompression:kSnappyCompression"}
//     in get_column_family_options_from_map, or
//   - "compression_per_level=kNoCompression:kSnappyCompression" in
//     GetColumnFamilyOptionsFromMapString
// * Enums:
//   The valid values of each enum are identical to the names of its constants.
//   [Example]:
//   - comp_type: valid values are "kNoCompression",
//     "kSnappyCompression", "kZlibCompression", "kBZip2Compression", ...
//   - compaction_style: valid values are "kCompactionStyleLevel",
//     "kCompactionStyleUniversal", "kCompactionStyleFIFO", and
//     "kCompactionStyleNone".
//

// Take a default column_family_options "base_options" in addition to a
// map "opts_map" of option name to option value to construct the new
// column_family_options "new_options".
//
// Below are the instructions of how to config some non-primitive-typed
// opts in ColumnFOptions:
//
// * table_factory:
//   table_factory can be configured using our custom nested-option syntax.
//
//   {option_a=value_a; option_b=value_b; option_c=value_c; ... }
//
//   A nested option is enclosed by two curly braces, within which there are
//   multiple option assignments.  Each assignment is of the form
//   "variable_name=value;".
//
//   Currently we support the following types of table_factory:
//   - BlockBasedTableFactory:
//     Use name "block_based_table_factory" to initialize table_factory with
//     BlockBasedTableFactory.  Its BlockBasedTableFactoryOptions can be
//     configured using the nested-option syntax.
//     [Example]:
//     * {"block_based_table_factory", "{block_cache=1M;block_size=4k;}"}
//       is equivalent to assigning table_factory with a BlockBasedTableFactory
//       that has 1M LRU block-cache with block size equals to 4k:
//         column_family_options cf_opt;
//         block_based_table_options blk_opt;
//         blk_opt.block_cache = new_lru_cache(1 * 1024 * 1024);
//         blk_opt.block_size = 4 * 1024;
//         cf_opt.table_factory.reset(new_block_based_table_factory(blk_opt));
//   - PlainTableFactory:
//     Use name "plain_table_factory" to initialize table_factory with
//     PlainTableFactory.  Its PlainTableFactoryOptions can be configured using
//     the nested-option syntax.
//     [Example]:
//     * {"plain_table_factory", "{user_key_len=66;bloom_bits_per_key=20;}"}
//
// * memtable_factory:
//   Use "memtable" to config memtable_factory.  Here are the supported
//   memtable factories:
//   - SkipList:
//     Pass "skip_list:<lookahead>" to config memtable to use SkipList,
//     or simply "skip_list" to use the default SkipList.
//     [Example]:
//     * {"memtable", "skip_list:5"} is equivalent to setting
//       memtable to skip_list_factory(5).
//   - PrefixHash:
//     Pass "prfix_hash:<hash_bucket_count>" to config memtable
//     to use PrefixHash, or simply "prefix_hash" to use the default
//     PrefixHash.
//     [Example]:
//     * {"memtable", "prefix_hash:1000"} is equivalent to setting
//       memtable to new_hash_skip_list_rep_factory(hash_bucket_count).
//   - HashLinkedList:
//     Pass "hash_linkedlist:<hash_bucket_count>" to config memtable
//     to use HashLinkedList, or simply "hash_linkedlist" to use the default
//     HashLinkedList.
//     [Example]:
//     * {"memtable", "hash_linkedlist:1000"} is equivalent to
//       setting memtable to new_hash_link_list_rep_factory(1000).
//   - vector_rep_factory:
//     Pass "vector:<count>" to config memtable to use vector_rep_factory,
//     or simply "vector" to use the default Vector memtable.
//     [Example]:
//     * {"memtable", "vector:1024"} is equivalent to setting memtable
//       to vector_rep_factory(1024).
//   - HashCuckooRepFactory:
//     Pass "cuckoo:<write_buffer_size>" to use HashCuckooRepFactory with the
//     specified write buffer size, or simply "cuckoo" to use the default
//     HashCuckooRepFactory.
//     [Example]:
//     * {"memtable", "cuckoo:1024"} is equivalent to setting memtable
//       to NewHashCuckooRepFactory(1024).
//
//  * compression_opts:
//    Use "compression_opts" to config compression_opts.  The value format
//    is of the form "<window_bits>:<level>:<strategy>:<max_dict_bytes>".
//    [Example]:
//    * {"compression_opts", "4:5:6:7"} is equivalent to setting:
//        column_family_options cf_opt;
//        cf_opt.compression_opts.window_bits = 4;
//        cf_opt.compression_opts.level = 5;
//        cf_opt.compression_opts.strategy = 6;
//        cf_opt.compression_opts.max_dict_bytes = 7;
//
// @param base_options the default opts of the output "new_options".
// @param opts_map an option name to value map for specifying how "new_options"
//     should be set.
// @param new_options the resulting opts based on "base_options" with the
//     change specified in "opts_map".
// @param input_strings_escaped when set to true, each escaped characters
//     prefixed by '\' in the values of the opts_map will be further converted
//     back to the raw string before assigning to the associated opts.
// @param ignore_unknown_options when set to true, unknown opts are ignored
//     instead of resulting in an unknown-option error.
// @return status_type::ok() on success.  Otherwise, a non-is_ok status indicating
//     error will be returned, and "new_options" will be set to "base_options".
        status_type get_column_family_options_from_map(const column_family_options &base_options,
                                                       const std::unordered_map<std::string, std::string> &opts_map,
                                                       column_family_options *new_options,
                                                       bool input_strings_escaped = false,
                                                       bool ignore_unknown_options = false);

// Take a default db_options "base_options" in addition to a
// map "opts_map" of option name to option value to construct the new
// db_options "new_options".
//
// Below are the instructions of how to config some non-primitive-typed
// opts in db_options:
//
// * rate_limiter_bytes_per_sec:
//   limiter can be configured directly by specifying its bytes_per_sec.
//   [Example]:
//   - Passing {"rate_limiter_bytes_per_sec", "1024"} is equivalent to
//     passing new_generic_rate_limiter(1024) to rate_limiter_bytes_per_sec.
//
// @param base_options the default opts of the output "new_options".
// @param opts_map an option name to value map for specifying how "new_options"
//     should be set.
// @param new_options the resulting opts based on "base_options" with the
//     change specified in "opts_map".
// @param input_strings_escaped when set to true, each escaped characters
//     prefixed by '\' in the values of the opts_map will be further converted
//     back to the raw string before assigning to the associated opts.
// @param ignore_unknown_options when set to true, unknown opts are ignored
//     instead of resulting in an unknown-option error.
// @return status_type::ok() on success.  Otherwise, a non-is_ok status indicating
//     error will be returned, and "new_options" will be set to "base_options".
        status_type get_db_options_from_map(const db_options &base_options,
                                            const std::unordered_map<std::string, std::string> &opts_map,
                                            db_options *new_options, bool input_strings_escaped = false,
                                            bool ignore_unknown_options = false);

// Take a default block_based_table_options "table_options" in addition to a
// map "opts_map" of option name to option value to construct the new
// block_based_table_options "new_table_options".
//
// Below are the instructions of how to config some non-primitive-typed
// opts in block_based_table_options:
//
// * filter_policy:
//   We currently only support the following filter_policy in the convenience
//   functions:
//   - BloomFilter: use "bloomfilter:[bits_per_key]:[use_block_based_builder]"
//     to specify BloomFilter.  The above string is equivalent to calling
//     new_bloom_filter_policy(bits_per_key, use_block_based_builder).
//     [Example]:
//     - Pass {"filter_policy", "bloomfilter:4:true"} in
//       get_block_based_table_options_from_map to use a BloomFilter with 4-bits
//       per key and use_block_based_builder enabled.
//
// * block_cache / block_cache_compressed:
//   We currently only support LRU cache in the get_options API.  The LRU
//   cache can be set by directly specifying its size.
//   [Example]:
//   - Passing {"block_cache", "1M"} in get_block_based_table_options_from_map is
//     equivalent to setting block_cache using new_lru_cache(1024 * 1024).
//
// @param table_options the default opts of the output "new_table_options".
// @param opts_map an option name to value map for specifying how
//     "new_table_options" should be set.
// @param new_table_options the resulting opts based on "table_options"
//     with the change specified in "opts_map".
// @param input_strings_escaped when set to true, each escaped characters
//     prefixed by '\' in the values of the opts_map will be further converted
//     back to the raw string before assigning to the associated opts.
// @param ignore_unknown_options when set to true, unknown opts are ignored
//     instead of resulting in an unknown-option error.
// @return status_type::ok() on success.  Otherwise, a non-is_ok status indicating
//     error will be returned, and "new_table_options" will be set to
//     "table_options".
        status_type get_block_based_table_options_from_map(const block_based_table_options &table_options,
                                                           const std::unordered_map<std::string, std::string> &opts_map,
                                                           block_based_table_options *new_table_options,
                                                           bool input_strings_escaped = false,
                                                           bool ignore_unknown_options = false);

// Take a default plain_table_options "table_options" in addition to a
// map "opts_map" of option name to option value to construct the new
// plain_table_options "new_table_options".
//
// @param table_options the default opts of the output "new_table_options".
// @param opts_map an option name to value map for specifying how
//     "new_table_options" should be set.
// @param new_table_options the resulting opts based on "table_options"
//     with the change specified in "opts_map".
// @param input_strings_escaped when set to true, each escaped characters
//     prefixed by '\' in the values of the opts_map will be further converted
//     back to the raw string before assigning to the associated opts.
// @param ignore_unknown_options when set to true, unknown opts are ignored
//     instead of resulting in an unknown-option error.
// @return status_type::ok() on success.  Otherwise, a non-is_ok status indicating
//     error will be returned, and "new_table_options" will be set to
//     "table_options".
        status_type get_plain_table_options_from_map(const plain_table_options &table_options,
                                                     const std::unordered_map<std::string, std::string> &opts_map,
                                                     plain_table_options *new_table_options,
                                                     bool input_strings_escaped = false,
                                                     bool ignore_unknown_options = false);

// Take a string representation of option names and  values, apply them into the
// base_options, and return the new opts as a result. The string has the
// following format:
//   "write_buffer_size=1024;max_write_buffer_number=2"
// Nested opts config is also possible. For example, you can define
// block_based_table_options as part of the string for block-based table factory:
//   "write_buffer_size=1024;block_based_table_factory={block_size=4k};"
//   "max_write_buffer_num=2"
        status_type get_column_family_options_from_string(const column_family_options &base_options,
                                                          const std::string &opts_str,
                                                          column_family_options *new_options);

        status_type get_db_options_from_string(const db_options &base_options, const std::string &opts_str,
                                               db_options *new_options);

        status_type get_string_from_db_options(std::string *opts_str, const db_options &db_options,
                                               const std::string &delimiter = ";  ");

        status_type get_string_from_column_family_options(std::string *opts_str,
                                                          const column_family_options &cf_options,
                                                          const std::string &delimiter = ";  ");

        status_type get_string_from_compression_type(std::string *compression_str, compression_type compression_type);

        std::vector<compression_type> get_supported_compressions();

        status_type get_block_based_table_options_from_string(const block_based_table_options &table_options,
                                                              const std::string &opts_str,
                                                              block_based_table_options *new_table_options);

        status_type get_plain_table_options_from_string(const plain_table_options &table_options,
                                                        const std::string &opts_str,
                                                        plain_table_options *new_table_options);

        status_type get_mem_table_rep_factory_from_string(const std::string &opts_str,
                                                          std::unique_ptr<mem_table_rep_factory> *new_mem_factory);

        status_type get_options_from_string(const options &base_options, const std::string &opts_str,
                                            options *new_options);

        status_type string_to_map(const std::string &opts_str, std::unordered_map<std::string, std::string> *opts_map);

// request stopping background work, if wait is true wait until it's done
        void cancel_all_background_work(database *db, bool wait = false);

// remove files which are entirely in the given range
// Could leave some keys in the range which are in files which are not
// entirely in the range. Also leaves L0 files regardless of whether they're
// in the range.
// Snapshots before the delete might not see the data in the given range.
        status_type delete_files_in_range(database *db, column_family_handle *column_family, const slice *begin,
                                          const slice *end, bool include_end = true);

// remove files in multiple ranges at once
// remove files in a lot of ranges one at a time can be slow, use this API for
// better performance in that case.
        status_type delete_files_in_ranges(database *db, column_family_handle *column_family, const range_ptr *ranges,
                                           size_t n, bool include_end = true);

// Verify the checksum of file
        status_type verify_sst_file_checksum(const options &options, const environment_options &env_options,
                                             const std::string &file_path);

#endif  // DCDB_LITE

    }
} // namespace nil