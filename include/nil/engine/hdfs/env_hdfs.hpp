#pragma once

#include <algorithm>
#include <cstdio>
#include <time.h>
#include <iostream>

#include "nil/dcdb/port/sys_time.hpp"
#include <nil/engine/environment.hpp>
#include <nil/engine/status.hpp>

#ifdef USE_HDFS

#include <hdfs.h>

namespace nil {
    namespace dcdb {

// Thrown during execution when there is an issue with the supplied
// arguments.
        class HdfsUsageException : public std::exception {
        };

// A simple exception that indicates something went wrong that is not
// recoverable.  The intention is for the message to be printed (with
// nothing else) and the process terminate.
        class HdfsFatalException : public std::exception {
        public:
            explicit HdfsFatalException(const std::string &s) : what_(s) {
            }

            virtual ~HdfsFatalException() throw() {
            }

            virtual const char *what() const throw() {
                return what_.c_str();
            }

        private:
            const std::string what_;
        };

//
// The HDFS environment for rocksdb. This class overrides all the
// file/dir access methods and delegates the thread-mgmt methods to the
// default posix environment.
//
        class HdfsEnv : public environment_type {

        public:
            explicit HdfsEnv(const std::string &fsname) : fsname_(fsname) {
                posixEnv = environment_type::default_environment();
                fileSys_ = connectToPath(fsname_);
            }

            virtual ~HdfsEnv() {
                fprintf(stderr, "Destroying HdfsEnv::Default()\n");
                hdfsDisconnect(fileSys_);
            }

            virtual status_type NewSequentialFile(const std::string &fname, std::unique_ptr<sequential_file> *result,
                                                  const environment_options &opts);

            virtual status_type NewRandomAccessFile(const std::string &fname,
                                                    std::unique_ptr<random_access_file> *result,
                                                    const environment_options &opts);

            virtual status_type NewWritableFile(const std::string &fname, std::unique_ptr<writable_file> *result,
                                                const environment_options &opts);

            virtual status_type NewDirectory(const std::string &name, std::unique_ptr<directory> *result);

            virtual status_type FileExists(const std::string &fname);

            virtual status_type get_children(const std::string &path, std::vector<std::string> *result);

            virtual status_type delete_file(const std::string &fname);

            virtual status_type CreateDir(const std::string &name);

            virtual status_type create_dir_if_missing(const std::string &name);

            virtual status_type DeleteDir(const std::string &name);

            virtual status_type GetFileSize(const std::string &fname, uint64_t *size);

            virtual status_type GetFileModificationTime(const std::string &fname, uint64_t *file_mtime);

            virtual status_type RenameFile(const std::string &src, const std::string &target);

            virtual status_type LinkFile(const std::string &src, const std::string &target) {
                return status_type::not_supported(); // not supported
            }

            virtual status_type LockFile(const std::string &fname, FileLock **lock);

            virtual status_type UnlockFile(FileLock *lock);

            virtual status_type NewLogger(const std::string &fname, std::shared_ptr<boost::log::sources::severity_logger_mt<info_log_level>> *result);

            virtual void Schedule(void (*function)(void *arg), void *arg, priority pri = LOW, void *tag = nullptr,
                                  void (*unschedFunction)(void *arg) = 0) {
                posixEnv->Schedule(function, arg, pri, tag, unschedFunction);
            }

            virtual int UnSchedule(void *tag, priority pri) {
                return posixEnv->UnSchedule(tag, pri);
            }

            virtual void StartThread(void (*function)(void *arg), void *arg) {
                posixEnv->StartThread(function, arg);
            }

            virtual void WaitForJoin() {
                posixEnv->WaitForJoin();
            }

            virtual unsigned int GetThreadPoolQueueLen(priority pri = LOW) const override {
                return posixEnv->GetThreadPoolQueueLen(pri);
            }

            virtual status_type GetTestDirectory(std::string *path) {
                return posixEnv->GetTestDirectory(path);
            }

            virtual uint64_t NowMicros() {
                return posixEnv->NowMicros();
            }

            virtual void sleep_for_microseconds(int micros) {
                posixEnv->sleep_for_microseconds(micros);
            }

            virtual status_type GetHostName(char *name, uint64_t len) {
                return posixEnv->GetHostName(name, len);
            }

            virtual status_type GetCurrentTime(int64_t *unix_time) {
                return posixEnv->GetCurrentTime(unix_time);
            }

            virtual status_type GetAbsolutePath(const std::string &db_path, std::string *output_path) {
                return posixEnv->GetAbsolutePath(db_path, output_path);
            }

            virtual void set_background_threads(int number, priority pri = LOW) {
                posixEnv->set_background_threads(number, pri);
            }

            virtual int GetBackgroundThreads(priority pri = LOW) {
                return posixEnv->GetBackgroundThreads(pri);
            }

            virtual void IncBackgroundThreadsIfNeeded(int number, priority pri) override {
                posixEnv->IncBackgroundThreadsIfNeeded(number, pri);
            }

            virtual std::string TimeToString(uint64_t number) {
                return posixEnv->TimeToString(number);
            }

            static uint64_t gettid() {
                assert(sizeof(pthread_t) <= sizeof(uint64_t));
                return (uint64_t) pthread_self();
            }

            virtual uint64_t GetThreadID() const override {
                return HdfsEnv::gettid();
            }

        private:
            std::string fsname_;  // string of the form "hdfs://hostname:port/"
            hdfsFS fileSys_;      //  a single FileSystem object for all files
            environment_type *posixEnv;       // This object is derived from environment_type, but not from
            // posixEnv. We have posixnv as an encapsulated
            // object here so that we can use posix timers,
            // posix threads, etc.

            static const std::string kProto;
            static const std::string pathsep;

            /**
             * If the URI is specified of the form hdfs://server:port/path,
             * then connect to the specified cluster
             * else connect to default.
             */
            hdfsFS connectToPath(const std::string &uri) {
                if (uri.empty()) {
                    return nullptr;
                }
                if (uri.find(kProto) != 0) {
                    // uri doesn't start with hdfs:// -> use default:0, which is special
                    // to libhdfs.
                    return hdfsConnectNewInstance("default", 0);
                }
                const std::string hostport = uri.substr(kProto.length());

                std::vector<std::string> parts;
                split(hostport, ':', parts);
                if (parts.size() != 2) {
                    throw HdfsFatalException("Bad uri for hdfs " + uri);
                }
                // parts[0] = hosts, parts[1] = port/xxx/yyy
                std::string host(parts[0]);
                std::string remaining(parts[1]);

                int rem = remaining.find(pathsep);
                std::string portStr = (rem == 0 ? remaining : remaining.substr(0, rem));

                tPort port;
                port = atoi(portStr.c_str());
                if (port == 0) {
                    throw HdfsFatalException("Bad host-port for hdfs " + uri);
                }
                hdfsFS fs = hdfsConnectNewInstance(host.c_str(), port);
                return fs;
            }

            void split(const std::string &s, char delim, std::vector<std::string> &elems) {
                elems.clear();
                size_t prev = 0;
                size_t pos = s.find(delim);
                while (pos != std::string::npos) {
                    elems.push_back(s.substr(prev, pos));
                    prev = pos + 1;
                    pos = s.find(delim, prev);
                }
                elems.push_back(s.substr(prev, s.size()));
            }
        };

    }
} // namespace nil

#else // USE_HDFS


namespace nil {
    namespace dcdb {

        static const status_type notsup;

        class HdfsEnv : public environment_type {

        public:
            explicit HdfsEnv(const std::string &fsname) {
                fprintf(stderr, "You have not build rocksdb with HDFS support\n");
                fprintf(stderr, "Please see hdfs/README for details\n");
                abort();
            }

            virtual ~HdfsEnv() {
            }

            virtual status_type new_sequential_file(const std::string &fname, std::unique_ptr<sequential_file> *result,
                                                    const environment_options &options) override;

            virtual status_type new_random_access_file(const std::string &fname,
                                                       std::unique_ptr<random_access_file> *result,
                                                       const environment_options &options) override {
                return notsup;
            }

            virtual status_type new_writable_file(const std::string &fname, std::unique_ptr<writable_file> *result,
                                                  const environment_options &options) override {
                return notsup;
            }

            virtual status_type new_directory(const std::string &name, std::unique_ptr<directory> *result) override {
                return notsup;
            }

            virtual status_type file_exists(const std::string &fname) override {
                return notsup;
            }

            virtual status_type get_children(const std::string &path, std::vector<std::string> *result) override {
                return notsup;
            }

            virtual status_type delete_file(const std::string &fname) override {
                return notsup;
            }

            virtual status_type create_dir(const std::string &name) override {
                return notsup;
            }

            virtual status_type create_dir_if_missing(const std::string &name) override {
                return notsup;
            }

            virtual status_type delete_dir(const std::string &name) override {
                return notsup;
            }

            virtual status_type get_file_size(const std::string &fname, uint64_t *size) override {
                return notsup;
            }

            virtual status_type get_file_modification_time(const std::string &fname, uint64_t *time) override {
                return notsup;
            }

            virtual status_type rename_file(const std::string &src, const std::string &target) override {
                return notsup;
            }

            virtual status_type link_file(const std::string &src, const std::string &target) override {
                return notsup;
            }

            virtual status_type lock_file(const std::string &fname, FileLock **lock) override {
                return notsup;
            }

            virtual status_type unlock_file(FileLock *lock) override {
                return notsup;
            }

            virtual status_type new_logger(const std::string &fname, std::shared_ptr<boost::log::sources::severity_logger_mt<info_log_level>> *result) override {
                return notsup;
            }

            virtual void schedule(void (*function)(void *arg), void *arg, priority_type pri = LOW, void *tag = nullptr,
                                  void (*unschedFunction)(void *arg) = nullptr) override {
            }

            virtual int unschedule(void *tag, priority_type pri) override {
                return 0;
            }

            virtual void start_thread(void (*function)(void *arg), void *arg) override {
            }

            virtual void wait_for_join() override {
            }

            virtual unsigned int get_thread_pool_queue_len(priority_type pri = LOW) const override {
                return 0;
            }

            virtual status_type get_test_directory(std::string *path) override {
                return notsup;
            }

            virtual uint64_t now_micros() override {
                return 0;
            }

            virtual void sleep_for_microseconds(int micros) override {
            }

            virtual status_type get_host_name(char *name, uint64_t len) override {
                return notsup;
            }

            virtual status_type get_current_time(int64_t *unix_time) override {
                return notsup;
            }

            virtual status_type get_absolute_path(const std::string &db_path, std::string *outputpath) override {
                return notsup;
            }

            virtual void set_background_threads(int number, priority_type pri = LOW) override {
            }

            virtual int get_background_threads(priority_type pri = LOW) override {
                return 0;
            }

            virtual void inc_background_threads_if_needed(int number, priority_type pri) override {
            }

            virtual std::string time_to_string(uint64_t number) override {
                return "";
            }

            virtual uint64_t get_thread_id() const override {
                return 0;
            }
        };
    }
}
#endif // USE_HDFS
