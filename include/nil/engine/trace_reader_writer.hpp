#pragma once

#include <nil/engine/env.hpp>

namespace nil {
    namespace dcdb {

// Allow custom implementations of trace_writer and trace_reader.
// By default, RocksDB provides a way to capture the traces to a file using the
// factory new_file_trace_writer(). But users could also choose to export traces to
// any other system by providing custom implementations of trace_writer and
// trace_reader.

// trace_writer allows exporting RocksDB traces to any system, one operation at
// a time.
        class trace_writer {
        public:
            trace_writer() {
            }

            virtual ~trace_writer() {
            }

            virtual status_type write(const slice &data) = 0;

            virtual status_type close() = 0;

            virtual uint64_t get_file_size() = 0;
        };

// trace_reader allows reading RocksDB traces from any system, one operation at
// a time. A RocksDB Replayer could depend on this to replay opertions.
        class trace_reader {
        public:
            trace_reader() {
            }

            virtual ~trace_reader() {
            }

            virtual status_type read(std::string *data) = 0;

            virtual status_type close() = 0;
        };

// Factory methods to read/write traces from/to a file.
        status_type new_file_trace_writer(environment_type *env, const environment_options &env_options,
                                          const std::string &trace_filename, std::unique_ptr<trace_writer> *trace_writer);

        status_type new_file_trace_reader(environment_type *env, const environment_options &env_options,
                                          const std::string &trace_filename, std::unique_ptr<trace_reader> *trace_reader);
    }
} // namespace nil
