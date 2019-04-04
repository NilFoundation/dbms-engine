#pragma once

#include <nil/engine/env.hpp>

namespace nil {
    namespace dcdb {

// Allow custom implementations of trace_writer and TraceReader.
// By default, RocksDB provides a way to capture the traces to a file using the
// factory NewFileTraceWriter(). But users could also choose to export traces to
// any other system by providing custom implementations of trace_writer and
// TraceReader.

// trace_writer allows exporting RocksDB traces to any system, one operation at
// a time.
        class trace_writer {
        public:
            trace_writer() {
            }

            virtual ~trace_writer() {
            }

            virtual status_type Write(const slice &data) = 0;

            virtual status_type Close() = 0;

            virtual uint64_t GetFileSize() = 0;
        };

// TraceReader allows reading RocksDB traces from any system, one operation at
// a time. A RocksDB Replayer could depend on this to replay opertions.
        class TraceReader {
        public:
            TraceReader() {
            }

            virtual ~TraceReader() {
            }

            virtual status_type Read(std::string *data) = 0;

            virtual status_type Close() = 0;
        };

// Factory methods to read/write traces from/to a file.
        status_type NewFileTraceWriter(environment_type *env, const environment_options &env_options,
                                       const std::string &trace_filename, std::unique_ptr<trace_writer> *trace_writer);

        status_type NewFileTraceReader(environment_type *env, const environment_options &env_options,
                                       const std::string &trace_filename, std::unique_ptr<TraceReader> *trace_reader);
    }
} // namespace nil
