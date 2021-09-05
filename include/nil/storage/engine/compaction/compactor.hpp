//---------------------------------------------------------------------------//
// Copyright (c) 2018-2020 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef STORAGE_ENGINE_COMPACTOR_HPP
#define STORAGE_ENGINE_COMPACTOR_HPP

#include <nil/storage/engine/compaction/compaction_options.hpp>
#include <nil/storage/engine/column_family/column_family_handle.hpp>

namespace nil {
    namespace engine {
        struct compactor {
            // Compact the underlying engine for the key range [*begin,*end].
            // The actual compaction interval might be superset of [*begin, *end].
            // In particular, deleted and overwritten versions are discarded,
            // and the data is rearranged to reduce the cost of operations
            // needed to access the data.  This operation should typically only
            // be invoked by users who understand the underlying implementation.
            //
            // begin==nullptr is treated as a key before all keys in the database.
            // end==nullptr is treated as a key after all keys in the database.
            // Therefore the following call will compact the entire database:
            //    db->compact_range(opts, nullptr, nullptr);
            // Note that after the entire database is compacted, all data are pushed
            // down to the last level containing any data. If the total data size after
            // compaction is reduced, that level might not be appropriate for hosting all
            // the files. In this case, client could set opts.change_level to true, to
            // move the files back to the minimum level capable of holding the data set
            // or a given level (specified by non-negative opts.target_level).
            virtual engine::status_type compact_range(const compact_range_options &options,
                                                      column_family_handle *column_family, const slice *begin,
                                                      const slice *end) = 0;

            // compact_files() inputs a list of files specified by file numbers and
            // compacts them to the specified level. Note that the behavior is different
            // from compact_range() in that compact_files() performs the compaction job
            // using the CURRENT thread.
            //
            // @see GetDataBaseMetaData
            // @see get_column_family_meta_data
            virtual engine::status_type
                compact_files(const compaction_options &compact_opts, engine::column_family_handle *column_family,
                              const std::vector<std::string> &input_file_names, int output_level, int output_path_id,
                              std::vector<std::string> *output_file_names, compaction_job_info *compaction_job_inf) = 0;
        };
    }    // namespace engine
}    // namespace nil

#endif    // DCDB_COMPACTOR_HPP
