#include <nil/engine/environment.hpp>

#include <thread>

#include <boost/log/sources/severity_logger.hpp>

namespace nil {
    namespace dcdb {

        environment_type::~environment_type() {
        }

        std::string environment_type::priority_to_string(environment_type::priority_type priority) {
            switch (priority) {
                case environment_type::priority_type::BOTTOM:
                    return "Bottom";
                case environment_type::priority_type::LOW:
                    return "Low";
                case environment_type::priority_type::HIGH:
                    return "High";
                case environment_type::priority_type::USER:
                    return "User";
                case environment_type::priority_type::TOTAL:
                    assert(false);
            }
            return "Invalid";
        }

        uint64_t environment_type::get_thread_id() const {
            std::hash<std::thread::id> hasher;
            return hasher(std::this_thread::get_id());
        }

        status_type environment_type::reuse_writable_file(const std::string &fname, const std::string &old_fname,
                                                          std::unique_ptr<writable_file> *result,
                                                          const environment_options &options) {
            status_type s = rename_file(old_fname, fname);
            if (!s.is_ok()) {
                return s;
            }
            return new_writable_file(fname, result, options);
        }

        status_type environment_type::get_children_file_attributes(const std::string &dir,
                                                                   std::vector<file_attributes> *result) {
            assert(result != nullptr);
            std::vector<std::string> child_fnames;
            status_type s = get_children(dir, &child_fnames);
            if (!s.is_ok()) {
                return s;
            }
            result->resize(child_fnames.size());
            size_t result_size = 0;
            for (auto & child_fname : child_fnames) {
                const std::string path = dir + "/" += child_fname;
                if (!(s = get_file_size(path, &(*result)[result_size].size_bytes)).is_ok()) {
                    if (file_exists(path).is_not_found()) {
                        // The file may have been deleted since we listed the directory
                        continue;
                    }
                    return s;
                }
                (*result)[result_size].name = std::move(child_fname);
                result_size++;
            }
            result->resize(result_size);
            return status_type();
        }

        sequential_file::~sequential_file() {
        }

        random_access_file::~random_access_file() {
        }

        writable_file::~writable_file() {
        }

        memory_mapped_file_buffer::~memory_mapped_file_buffer() {
        }

        Logger::~Logger() {
        }

        status_type Logger::close() {
            if (!closed_) {
                closed_ = true;
                return CloseImpl();
            } else {
                return status_type();
            }
        }

        status_type Logger::CloseImpl() {
            return status_type::not_supported();
        }

        FileLock::~FileLock() {
        }

        void LogFlush(boost::log::sources::severity_logger_mt<info_log_level> *info_log) {
            if (info_log) {
                info_log->flush();
            }
        }

        static void Logv(boost::log::sources::severity_logger_mt<info_log_level> *info_log, const char *format, va_list ap) {
            if (info_log && info_log->GetInfoLogLevel() <= info_log_level::INFO_LEVEL) {
                info_log->Logv(info_log_level::INFO_LEVEL, format, ap);
            }
        }

        void Log(boost::log::sources::severity_logger_mt<info_log_level> *info_log, const char *format, ...) {
            va_list ap;
            va_start(ap, format);
            Logv(info_log, format, ap);
            va_end(ap);
        }

        void Logger::Logv(const info_log_level log_level, const char *format, va_list ap) {
            static const char *kInfoLogLevelNames[5] = {
                    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
            };
            if (log_level < log_level_) {
                return;
            }

            if (log_level == info_log_level::INFO_LEVEL) {
                // Doesn't print log level if it is INFO level.
                // This is to avoid unexpected performance regression after we add
                // the feature of log level. All the logs before we add the feature
                // are INFO level. We don't want to add extra costs to those existing
                // logging.
                Logv(format, ap);
            } else if (log_level == info_log_level::HEADER_LEVEL) {
                LogHeader(format, ap);
            } else {
                char new_format[500];
                snprintf(new_format, sizeof(new_format) - 1, "[%s] %s", kInfoLogLevelNames[log_level], format);
                Logv(new_format, ap);
            }
        }

        static void Logv(const info_log_level log_level, boost::log::sources::severity_logger_mt<info_log_level> *info_log, const char *format, va_list ap) {
            if (info_log && info_log->GetInfoLogLevel() <= log_level) {
                if (log_level == info_log_level::HEADER_LEVEL) {
                    info_log->LogHeader(format, ap);
                } else {
                    info_log->Logv(log_level, format, ap);
                }
            }
        }

        void Log(const info_log_level log_level, boost::log::sources::severity_logger_mt<info_log_level> *info_log, const char *format, ...) {
            va_list ap;
            va_start(ap, format);
            Logv(log_level, info_log, format, ap);
            va_end(ap);
        }

        void LogFlush(const std::shared_ptr<Logger> &info_log) {
            LogFlush(info_log.get());
        }

        void Log(const info_log_level log_level, const std::shared_ptr<Logger> &info_log, const char *format, ...) {
            va_list ap;
            va_start(ap, format);
            Logv(log_level, info_log.get(), format, ap);
            va_end(ap);
        }

        void Log(const std::shared_ptr<Logger> &info_log, const char *format, ...) {
            va_list ap;
            va_start(ap, format);
            Logv(info_log.get(), format, ap);
            va_end(ap);
        }

        status_type WriteStringToFile(environment_type *env, const slice &data, const std::string &fname,
                                      bool should_sync) {
            std::unique_ptr<writable_file> file;
            environment_options soptions;
            status_type s = env->new_writable_file(fname, &file, soptions);
            if (!s.is_ok()) {
                return s;
            }
            s = file->append(data);
            if (s.is_ok() && should_sync) {
                s = file->sync();
            }
            if (!s.is_ok()) {
                env->delete_file(fname);
            }
            return s;
        }

        status_type ReadFileToString(environment_type *env, const std::string &fname, std::string *data) {
            environment_options soptions;
            data->clear();
            std::unique_ptr<sequential_file> file;
            status_type s = env->new_sequential_file(fname, &file, soptions);
            if (!s.is_ok()) {
                return s;
            }
            static const int kBufferSize = 8192;
            char *space = new char[kBufferSize];
            while (true) {
                slice fragment;
                s = file->read(kBufferSize, &fragment, space);
                if (!s.is_ok()) {
                    break;
                }
                data->append(fragment.data(), fragment.size());
                if (fragment.empty()) {
                    break;
                }
            }
            delete[] space;
            return s;
        }

        environment_wrapper::~environment_wrapper() {
        }

        namespace {  // anonymous namespace

            void assign_env_options(environment_options *env_options, const db_options &options) {
                env_options->use_mmap_reads = options.allow_mmap_reads;
                env_options->use_mmap_writes = options.allow_mmap_writes;
                env_options->use_direct_reads = options.use_direct_reads;
                env_options->set_fd_cloexec = options.is_fd_close_on_exec;
                env_options->bytes_per_sync = options.bytes_per_sync;
                env_options->compaction_readahead_size = options.compaction_readahead_size;
                env_options->random_access_max_buffer_size = options.random_access_max_buffer_size;
                env_options->rate_limiter = options.rate_limiter.get();
                env_options->writable_file_max_buffer_size = options.writable_file_max_buffer_size;
                env_options->allow_fallocate = options.allow_fallocate;
            }

        }

        environment_options environment_type::optimize_for_log_write(const environment_options &env_options,
                                                                     const db_options &db_options) const {
            environment_options optimized_env_options(env_options);
            optimized_env_options.bytes_per_sync = db_options.wal_bytes_per_sync;
            optimized_env_options.writable_file_max_buffer_size = db_options.writable_file_max_buffer_size;
            return optimized_env_options;
        }

        environment_options environment_type::optimize_for_manifest_write(const environment_options &env_options) const {
            return env_options;
        }

        environment_options environment_type::optimize_for_log_read(const environment_options &env_options) const {
            environment_options optimized_env_options(env_options);
            optimized_env_options.use_direct_reads = false;
            return optimized_env_options;
        }

        environment_options environment_type::optimize_for_manifest_read(const environment_options &env_options) const {
            environment_options optimized_env_options(env_options);
            optimized_env_options.use_direct_reads = false;
            return optimized_env_options;
        }

        environment_options environment_type::optimize_for_compaction_table_write(
                const environment_options &env_options, const immutable_db_options &immutable_ops) const {
            environment_options optimized_env_options(env_options);
            optimized_env_options.use_direct_writes = immutable_ops.use_direct_io_for_flush_and_compaction;
            return optimized_env_options;
        }

        environment_options environment_type::optimize_for_compaction_table_read(const environment_options &env_options,
                                                                                 const immutable_db_options &db_options) const {
            environment_options optimized_env_options(env_options);
            optimized_env_options.use_direct_reads = db_options.use_direct_reads;
            return optimized_env_options;
        }

        environment_options::environment_options(const db_options &options) {
            assign_env_options(this, options);
        }

        environment_options::environment_options() {
            db_options options;
            assign_env_options(this, options);
        }
    }
} // namespace nil
