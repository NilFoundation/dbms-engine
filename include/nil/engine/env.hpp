// An environment_type is an interface used by the rocksdb implementation to access
// operating system functionality like the filesystem etc.  Callers
// may wish to provide a custom environment_type object when opening a database to
// get fine gain control; e.g., to rate limit file system operations.
//
// All environment_type implementations are safe for concurrent access from
// multiple threads without any external synchronization.

#pragma once

#include <cstdint>
#include <cstdarg>
#include <functional>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <nil/engine/status.hpp>
#include <nil/engine/thread_status.hpp>

#ifdef _WIN32
// Windows API macro interference
#undef DeleteFile
#undef GetCurrentTime
#endif

namespace nil {
    namespace dcdb {

        class FileLock;

        class Logger;

        class random_access_file;

        class sequential_file;

        class slice;

        class writable_file;

        class random_rw_file;

        class memory_mapped_file_buffer;

        class directory;

        struct db_options;
        struct immutable_db_options;

        class RateLimiter;

        class thread_status_updater;

        struct thread_status;

        using std::unique_ptr;
        using std::shared_ptr;

        const size_t kDefaultPageSize = 4 * 1024;

// Options while opening a file to read/write
        struct environment_options {

            // Construct with default Options
            environment_options();

            // Construct from Options
            explicit environment_options(const db_options &options);

            // If true, then use mmap to read data
            bool use_mmap_reads = false;

            // If true, then use mmap to write data
            bool use_mmap_writes = true;

            // If true, then use O_DIRECT for reading data
            bool use_direct_reads = false;

            // If true, then use O_DIRECT for writing data
            bool use_direct_writes = false;

            // If false, fallocate() calls are bypassed
            bool allow_fallocate = true;

            // If true, set the FD_CLOEXEC on open fd.
            bool set_fd_cloexec = true;

            // Allows OS to incrementally sync files to disk while they are being
            // written, in the background. Issue one request for every bytes_per_sync
            // written. 0 turns it off.
            // default_environment: 0
            uint64_t bytes_per_sync = 0;

            // If true, we will preallocate the file with FALLOC_FL_KEEP_SIZE flag, which
            // means that file size won't change as part of preallocation.
            // If false, preallocation will also change the file size. This option will
            // improve the performance in workloads where you sync the data on every
            // write. By default, we set it to true for MANIFEST writes and false for
            // WAL writes
            bool fallocate_with_keep_size = true;

            // See db_options doc
            size_t compaction_readahead_size;

            // See db_options doc
            size_t random_access_max_buffer_size;

            // See db_options doc
            size_t writable_file_max_buffer_size = 1024 * 1024;

            // If not nullptr, write rate limiting is enabled for flush and compaction
            RateLimiter *rate_limiter = nullptr;
        };

        class environment_type {
        public:
            struct file_attributes {
                // File name
                std::string name;

                // Size of file in bytes
                uint64_t size_bytes;
            };

            environment_type() : thread_status_updater_(nullptr) {
            }

            virtual ~environment_type();

            // Return a default environment suitable for the current operating
            // system.  Sophisticated users may wish to provide their own environment_type
            // implementation instead of relying on this default environment.
            //
            // The result of default_environment() belongs to rocksdb and must never be deleted.
            static environment_type *default_environment();

            // Create a brand new sequentially-readable file with the specified name.
            // On success, stores a pointer to the new file in *result and returns OK.
            // On failure stores nullptr in *result and returns non-OK.  If the file does
            // not exist, returns a non-OK status.
            //
            // The returned file will only be accessed by one thread at a time.
            virtual status_type new_sequential_file(const std::string &fname, std::unique_ptr<sequential_file> *result,
                                                    const environment_options &options) = 0;

            // Create a brand new random access read-only file with the
            // specified name.  On success, stores a pointer to the new file in
            // *result and returns OK.  On failure stores nullptr in *result and
            // returns non-OK.  If the file does not exist, returns a non-OK
            // status.
            //
            // The returned file may be concurrently accessed by multiple threads.
            virtual status_type new_random_access_file(const std::string &fname,
                                                       std::unique_ptr<random_access_file> *result,
                                                       const environment_options &options) = 0;

            // These values match Linux definition
            // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/include/uapi/linux/fcntl.h#n56
            enum write_life_time_hint {
                WLTH_NOT_SET = 0, // No hint information set
                WLTH_NONE,        // No hints about write life time
                WLTH_SHORT,       // Data written has a short life time
                WLTH_MEDIUM,      // Data written has a medium life time
                WLTH_LONG,        // Data written has a long life time
                WLTH_EXTREME,     // Data written has an extremely long life time
            };

            // Create an object that writes to a new file with the specified
            // name.  Deletes any existing file with the same name and creates a
            // new file.  On success, stores a pointer to the new file in
            // *result and returns OK.  On failure stores nullptr in *result and
            // returns non-OK.
            //
            // The returned file will only be accessed by one thread at a time.
            virtual status_type new_writable_file(const std::string &fname, std::unique_ptr<writable_file> *result,
                                                  const environment_options &options) = 0;

            // Create an object that writes to a new file with the specified
            // name.  Deletes any existing file with the same name and creates a
            // new file.  On success, stores a pointer to the new file in
            // *result and returns OK.  On failure stores nullptr in *result and
            // returns non-OK.
            //
            // The returned file will only be accessed by one thread at a time.
            virtual status_type reopen_writable_file(const std::string &fname, std::unique_ptr<writable_file> *result,
                                                     const environment_options &options) {
                return status_type::NotSupported();
            }

            // Reuse an existing file by renaming it and opening it as writable.
            virtual status_type reuse_writable_file(const std::string &fname, const std::string &old_fname,
                                                    std::unique_ptr<writable_file> *result,
                                                    const environment_options &options);

            // open `fname` for random read and write, if file doesn't exist the file
            // will be created.  On success, stores a pointer to the new file in
            // *result and returns OK.  On failure returns non-OK.
            //
            // The returned file will only be accessed by one thread at a time.
            virtual status_type new_random_rw_file(const std::string &fname, std::unique_ptr<random_rw_file> *result,
                                                   const environment_options &options) {
                return status_type::NotSupported("random_rw_file is not implemented in this environment_type");
            }

            // Opens `fname` as a memory-mapped file for read and write (in-place updates
            // only, i.e., no appends). On success, stores a raw buffer covering the whole
            // file in `*result`. The file must exist prior to this call.
            virtual status_type new_memory_mapped_file_buffer(const std::string &fname,
                                                              std::unique_ptr<memory_mapped_file_buffer> *result) {
                return status_type::NotSupported(
                        "memory_mapped_file_buffer is not implemented in this environment_type");
            }

            // Create an object that represents a directory. Will fail if directory
            // doesn't exist. If the directory exists, it will open the directory
            // and create a new directory object.
            //
            // On success, stores a pointer to the new directory in
            // *result and returns OK. On failure stores nullptr in *result and
            // returns non-OK.
            virtual status_type new_directory(const std::string &name, std::unique_ptr<directory> *result) = 0;

            // Returns OK if the named file exists.
            //         NotFound if the named file does not exist,
            //                  the calling process does not have permission to determine
            //                  whether this file exists, or if the path is invalid.
            //         IOError if an IO Error was encountered
            virtual status_type file_exists(const std::string &fname) = 0;

            // Store in *result the names of the children of the specified directory.
            // The names are relative to "dir".
            // Original contents of *results are dropped.
            // Returns OK if "dir" exists and "*result" contains its children.
            //         NotFound if "dir" does not exist, the calling process does not have
            //                  permission to access "dir", or if "dir" is invalid.
            //         IOError if an IO Error was encountered
            virtual status_type get_children(const std::string &dir, std::vector<std::string> *result) = 0;

            // Store in *result the attributes of the children of the specified directory.
            // In case the implementation lists the directory prior to iterating the files
            // and files are concurrently deleted, the deleted files will be omitted from
            // result.
            // The name attributes are relative to "dir".
            // Original contents of *results are dropped.
            // Returns OK if "dir" exists and "*result" contains its children.
            //         NotFound if "dir" does not exist, the calling process does not have
            //                  permission to access "dir", or if "dir" is invalid.
            //         IOError if an IO Error was encountered
            virtual status_type get_children_file_attributes(const std::string &dir,
                                                             std::vector<file_attributes> *result);

            // remove the named file.
            virtual status_type delete_file(const std::string &fname) = 0;

            // truncate the named file to the specified size.
            virtual status_type truncate(const std::string &fname, size_t size) {
                return status_type::NotSupported("truncate is not supported for this environment_type");
            }

            // Create the specified directory. Returns error if directory exists.
            virtual status_type create_dir(const std::string &dirname) = 0;

            // Creates directory if missing. Return Ok if it exists, or successful in
            // Creating.
            virtual status_type create_dir_if_missing(const std::string &dirname) = 0;

            // remove the specified directory.
            virtual status_type delete_dir(const std::string &dirname) = 0;

            // Store the size of fname in *file_size.
            virtual status_type get_file_size(const std::string &fname, uint64_t *file_size) = 0;

            // Store the last modification time of fname in *file_mtime.
            virtual status_type get_file_modification_time(const std::string &fname, uint64_t *file_mtime) = 0;

            // Rename file src to target.
            virtual status_type rename_file(const std::string &src, const std::string &target) = 0;

            // Hard Link file src to target.
            virtual status_type link_file(const std::string &src, const std::string &target) {
                return status_type::NotSupported("link_file is not supported for this environment_type");
            }

            virtual status_type num_file_links(const std::string &fname, uint64_t *count) {
                return status_type::NotSupported(
                        "Getting number of file links is not supported for this environment_type");
            }

            virtual status_type are_files_same(const std::string &first, const std::string &second, bool *res) {
                return status_type::NotSupported("are_files_same is not supported for this environment_type");
            }

            // Lock the specified file.  Used to prevent concurrent access to
            // the same db by multiple processes.  On failure, stores nullptr in
            // *lock and returns non-OK.
            //
            // On success, stores a pointer to the object that represents the
            // acquired lock in *lock and returns OK.  The caller should call
            // unlock_file(*lock) to release the lock.  If the process exits,
            // the lock will be automatically released.
            //
            // If somebody else already holds the lock, finishes immediately
            // with a failure.  I.e., this call does not wait for existing locks
            // to go away.
            //
            // May create the named file if it does not already exist.
            virtual status_type lock_file(const std::string &fname, FileLock **lock) = 0;

            // release the lock acquired by a previous successful call to lock_file.
            // REQUIRES: lock was returned by a successful lock_file() call
            // REQUIRES: lock has not already been unlocked.
            virtual status_type unlock_file(FileLock *lock) = 0;

            // priority for scheduling job in thread pool
            enum Priority {
                BOTTOM, LOW, HIGH, USER, TOTAL
            };

            static std::string priority_to_string(Priority priority);

            // priority for requesting bytes in rate limiter scheduler
            enum io_priority {
                IO_LOW = 0, IO_HIGH = 1, IO_TOTAL = 2
            };

            // Arrange to run "(*function)(arg)" once in a background thread, in
            // the thread pool specified by pri. By default, jobs go to the 'LOW'
            // priority thread pool.

            // "function" may run in an unspecified thread.  Multiple functions
            // added to the same environment_type may run concurrently in different threads.
            // I.e., the caller may not assume that background work items are
            // serialized.
            // When the unschedule function is called, the unschedFunction
            // registered at the time of schedule is invoked with arg as a parameter.
            virtual void schedule(void (*function)(void *arg), void *arg, Priority pri = LOW, void *tag = nullptr,
                                  void (*unschedFunction)(void *arg) = nullptr) = 0;

            // Arrange to remove jobs for given arg from the queue_ if they are not
            // already scheduled. Caller is expected to have exclusive lock on arg.
            virtual int unschedule(void *arg, Priority pri) {
                return 0;
            }

            // Start a new thread, invoking "function(arg)" within the new thread.
            // When "function(arg)" returns, the thread will be destroyed.
            virtual void start_thread(void (*function)(void *arg), void *arg) = 0;

            // Wait for all threads started by start_thread to terminate.
            virtual void wait_for_join() {
            }

            // get thread pool queue length for specific thread pool.
            virtual unsigned int get_thread_pool_queue_len(Priority pri = LOW) const {
                return 0;
            }

            // *path is set to a temporary directory that can be used for testing. It may
            // or many not have just been created. The directory may or may not differ
            // between runs of the same process, but subsequent calls will return the
            // same directory.
            virtual status_type get_test_directory(std::string *path) = 0;

            // Create and return a log file for storing informational messages.
            virtual status_type new_logger(const std::string &fname, std::shared_ptr<Logger> *result) = 0;

            // Returns the number of micro-seconds since some fixed point in time.
            // It is often used as system time such as in GenericRateLimiter
            // and other places so a port needs to return system time in order to work.
            virtual uint64_t now_micros() = 0;

            // Returns the number of nano-seconds since some fixed point in time. Only
            // useful for computing deltas of time in one run.
            // default_environment implementation simply relies on now_micros.
            // In platform-specific implementations, now_nanos() should return time points
            // that are MONOTONIC.
            virtual uint64_t now_nanos() {
                return now_micros() * 1000;
            }

            // 0 indicates not supported.
            virtual uint64_t now_cpu_nanos() {
                return 0;
            }

            // Sleep/delay the thread for the prescribed number of micro-seconds.
            virtual void sleep_for_microseconds(int micros) = 0;

            // get the current host name.
            virtual status_type get_host_name(char *name, uint64_t len) = 0;

            // get the number of seconds since the Epoch, 1970-01-01 00:00:00 (UTC).
            // Only overwrites *unix_time on success.
            virtual status_type get_current_time(int64_t *unix_time) = 0;

            // get full directory name for this db.
            virtual status_type get_absolute_path(const std::string &db_path, std::string *output_path) = 0;

            // The number of background worker threads of a specific thread pool
            // for this environment. 'LOW' is the default pool.
            // default number: 1
            virtual void set_background_threads(int number, Priority pri = LOW) = 0;

            virtual int get_background_threads(Priority pri = LOW) = 0;

            virtual status_type set_allow_non_owner_access(bool allow_non_owner_access) {
                return status_type::NotSupported("Not supported.");
            }

            // Enlarge number of background worker threads of a specific thread pool
            // for this environment if it is smaller than specified. 'LOW' is the default
            // pool.
            virtual void inc_background_threads_if_needed(int number, Priority pri) = 0;

            // Lower IO priority for threads from the specified pool.
            virtual void lower_thread_pool_io_priority(Priority pool = LOW) {
            }

            // Lower CPU priority for threads from the specified pool.
            virtual void lower_thread_pool_cpu_priority(Priority pool = LOW) {
            }

            // Converts seconds-since-Jan-01-1970 to a printable string
            virtual std::string time_to_string(uint64_t time) = 0;

            // Generates a unique id that can be used to identify a db
            virtual std::string generate_unique_id();

            // optimize_for_log_write will create a new environment_options object that is a copy of
            // the environment_options in the parameters, but is optimized for reading log files.
            virtual environment_options optimize_for_log_read(const environment_options &env_options) const;

            // optimize_for_manifest_read will create a new environment_options object that is a copy
            // of the environment_options in the parameters, but is optimized for reading manifest
            // files.
            virtual environment_options optimize_for_manifest_read(const environment_options &env_options) const;

            // optimize_for_log_write will create a new environment_options object that is a copy of
            // the environment_options in the parameters, but is optimized for writing log files.
            // default_environment implementation returns the copy of the same object.
            virtual environment_options optimize_for_log_write(const environment_options &env_options,
                                                               const db_options &db_options) const;

            // optimize_for_manifest_write will create a new environment_options object that is a copy
            // of the environment_options in the parameters, but is optimized for writing manifest
            // files. default_environment implementation returns the copy of the same object.
            virtual environment_options optimize_for_manifest_write(const environment_options &env_options) const;

            // optimize_for_compaction_table_write will create a new environment_options object that is
            // a copy of the environment_options in the parameters, but is optimized for writing
            // table files.
            virtual environment_options optimize_for_compaction_table_write(const environment_options &env_options,
                                                                            const immutable_db_options &immutable_ops) const;

            // optimize_for_compaction_table_write will create a new environment_options object that
            // is a copy of the environment_options in the parameters, but is optimized for reading
            // table files.
            virtual environment_options optimize_for_compaction_table_read(const environment_options &env_options,
                                                                           const immutable_db_options &db_options) const;

            // Returns the status of all threads that belong to the current environment_type.
            virtual status_type get_thread_list(std::vector<thread_status> *thread_list) {
                return status_type::NotSupported("Not supported.");
            }

            // Returns the pointer to thread_status_updater.  This function will be
            // used in RocksDB internally to update thread status and supports
            // get_thread_list().
            virtual thread_status_updater *get_thread_status_updater() const {
                return thread_status_updater_;
            }

            // Returns the ID of the current thread.
            virtual uint64_t get_thread_id() const;

// This seems to clash with a macro on Windows, so #undef it here
#undef GetFreeSpace

            // get the amount of free disk space
            virtual status_type get_free_space(const std::string &path, uint64_t *diskfree) {
                return status_type::NotSupported();
            }

        protected:
            // The pointer to an internal structure that will update the
            // status of each thread.
            thread_status_updater *thread_status_updater_;

        private:
            // No copying allowed
            environment_type(const environment_type &);

            void operator=(const environment_type &);
        };

// The factory function to construct a thread_status_updater.  Any environment_type
// that supports get_thread_list() feature should call this function in its
// constructor to initialize thread_status_updater_.
        thread_status_updater *create_thread_status_updater();

// A file abstraction for reading sequentially through a file
        class sequential_file {
        public:
            sequential_file() {
            }

            virtual ~sequential_file();

            // read up to "n" bytes from the file.  "scratch[0..n-1]" may be
            // written by this routine.  Sets "*result" to the data that was
            // read (including if fewer than "n" bytes were successfully read).
            // May set "*result" to point at data in "scratch[0..n-1]", so
            // "scratch[0..n-1]" must be live when "*result" is used.
            // If an error was encountered, returns a non-OK status.
            //
            // REQUIRES: External synchronization
            virtual status_type read(size_t n, slice *result, char *scratch) = 0;

            // skip "n" bytes from the file. This is guaranteed to be no
            // slower that reading the same data, but may be faster.
            //
            // If end of file is reached, skipping will stop at the end of the
            // file, and skip will return OK.
            //
            // REQUIRES: External synchronization
            virtual status_type skip(uint64_t n) = 0;

            // Indicates the upper layers if the current sequential_file implementation
            // uses direct IO.
            virtual bool use_direct_io() const {
                return false;
            }

            // Use the returned alignment value to allocate
            // aligned buffer for Direct I/O
            virtual size_t get_required_buffer_alignment() const {
                return kDefaultPageSize;
            }

            // remove any kind of caching of data from the offset to offset+length
            // of this file. If the length is 0, then it refers to the end of file.
            // If the system is not caching the file contents, then this is a noop.
            virtual status_type invalidate_cache(size_t /*offset*/, size_t /*length*/) {
                return status_type::NotSupported("invalidate_cache not supported.");
            }

            // Positioned read for direct I/O
            // If Direct I/O enabled, offset, n, and scratch should be properly aligned
            virtual status_type positioned_read(uint64_t offset, size_t n, slice *result, char *scratch) {
                return status_type::NotSupported();
            }
        };

// A file abstraction for randomly reading the contents of a file.
        class random_access_file {
        public:

            random_access_file() {
            }

            virtual ~random_access_file();

            // read up to "n" bytes from the file starting at "offset".
            // "scratch[0..n-1]" may be written by this routine.  Sets "*result"
            // to the data that was read (including if fewer than "n" bytes were
            // successfully read).  May set "*result" to point at data in
            // "scratch[0..n-1]", so "scratch[0..n-1]" must be live when
            // "*result" is used.  If an error was encountered, returns a non-OK
            // status.
            //
            // Safe for concurrent use by multiple threads.
            // If Direct I/O enabled, offset, n, and scratch should be aligned properly.
            virtual status_type read(uint64_t offset, size_t n, slice *result, char *scratch) const = 0;

            // Readahead the file starting from offset by n bytes for caching.
            virtual status_type prefetch(uint64_t offset, size_t n) {
                return status_type();
            }

            // Tries to get an unique ID for this file that will be the same each time
            // the file is opened (and will stay the same while the file is open).
            // Furthermore, it tries to make this ID at most "max_size" bytes. If such an
            // ID can be created this function returns the length of the ID and places it
            // in "id"; otherwise, this function returns 0, in which case "id"
            // may not have been modified.
            //
            // This function guarantees, for IDs from a given environment, two unique ids
            // cannot be made equal to each other by adding arbitrary bytes to one of
            // them. That is, no unique ID is the prefix of another.
            //
            // This function guarantees that the returned ID will not be interpretable as
            // a single varint.
            //
            // Note: these IDs are only valid for the duration of the process.
            virtual size_t get_unique_id(char *id, size_t max_size) const {
                return 0; // default_environment implementation to prevent issues with backwards
                // compatibility.
            };

            enum access_pattern {
                NORMAL, RANDOM, SEQUENTIAL, WILLNEED, DONTNEED
            };

            virtual void hint(access_pattern pattern) {
            }

            // Indicates the upper layers if the current random_access_file implementation
            // uses direct IO.
            virtual bool use_direct_io() const {
                return false;
            }

            // Use the returned alignment value to allocate
            // aligned buffer for Direct I/O
            virtual size_t get_required_buffer_alignment() const {
                return kDefaultPageSize;
            }

            // remove any kind of caching of data from the offset to offset+length
            // of this file. If the length is 0, then it refers to the end of file.
            // If the system is not caching the file contents, then this is a noop.
            virtual status_type invalidate_cache(size_t offset, size_t length) {
                return status_type::NotSupported("invalidate_cache not supported.");
            }
        };

// A file abstraction for sequential writing.  The implementation
// must provide buffering since callers may append small fragments
// at a time to the file.
        class writable_file {
        public:
            writable_file() : last_preallocated_block_(0), preallocation_block_size_(0),
                    io_priority_(environment_type::IO_TOTAL), write_hint_(environment_type::WLTH_NOT_SET) {
            }

            virtual ~writable_file();

            // append data to the end of the file
            // Note: A WriteabelFile object must support either append or
            // positioned_append, so the users cannot mix the two.
            virtual status_type append(const slice &data) = 0;

            // positioned_append data to the specified offset. The new EOF after append
            // must be larger than the previous EOF. This is to be used when writes are
            // not backed by OS buffers and hence has to always start from the start of
            // the sector. The implementation thus needs to also rewrite the last
            // partial sector.
            // Note: PositionAppend does not guarantee moving the file offset after the
            // write. A writable_file object must support either append or
            // positioned_append, so the users cannot mix the two.
            //
            // positioned_append() can only happen on the page/sector boundaries. For that
            // reason, if the last write was an incomplete sector we still need to rewind
            // back to the nearest sector/page and rewrite the portion of it with whatever
            // we need to add. We need to keep where we stop writing.
            //
            // positioned_append() can only write whole sectors. For that reason we have to
            // pad with zeros for the last write and trim the file when closing according
            // to the position we keep in the previous step.
            //
            // positioned_append() requires aligned buffer to be passed in. The alignment
            // required is queried via get_required_buffer_alignment()
            virtual status_type positioned_append(const slice &data, uint64_t offset) {
                return status_type::NotSupported();
            }

            // truncate is necessary to trim the file to the correct size
            // before closing. It is not always possible to keep track of the file
            // size due to whole pages writes. The behavior is undefined if called
            // with other writes to follow.
            virtual status_type truncate(uint64_t size) {
                return status_type();
            }

            virtual status_type close() = 0;

            virtual status_type flush() = 0;

            virtual status_type sync() = 0; // sync data

            /*
             * sync data and/or metadata as well.
             * By default, sync only data.
             * Override this method for environments where we need to sync
             * metadata as well.
             */
            virtual status_type fsync() {
                return sync();
            }

            // true if sync() and fsync() are safe to call concurrently with append()
            // and flush().
            virtual bool is_sync_thread_safe() const {
                return false;
            }

            // Indicates the upper layers if the current writable_file implementation
            // uses direct IO.
            virtual bool use_direct_io() const {
                return false;
            }

            // Use the returned alignment value to allocate
            // aligned buffer for Direct I/O
            virtual size_t get_required_buffer_alignment() const {
                return kDefaultPageSize;
            }

            /*
             * Change the priority in rate limiter if rate limiting is enabled.
             * If rate limiting is not enabled, this call has no effect.
             */
            virtual void set_io_priority(environment_type::io_priority pri) {
                io_priority_ = pri;
            }

            virtual environment_type::io_priority get_io_priority() {
                return io_priority_;
            }

            virtual void set_write_life_time_hint(environment_type::write_life_time_hint hint) {
                write_hint_ = hint;
            }

            virtual environment_type::write_life_time_hint get_write_life_time_hint() {
                return write_hint_;
            }

            /*
             * get the size of valid data in the file.
             */
            virtual uint64_t get_file_size() {
                return 0;
            }

            /*
             * get and set the default pre-allocation block size for writes to
             * this file.  If non-zero, then allocate will be used to extend the
             * underlying engine of a file (generally via fallocate) if the environment_type
             * instance supports it.
             */
            virtual void set_preallocation_block_size(size_t size) {
                preallocation_block_size_ = size;
            }

            virtual void get_preallocation_status(size_t *block_size, size_t *last_allocated_block) {
                *last_allocated_block = last_preallocated_block_;
                *block_size = preallocation_block_size_;
            }

            // For documentation, refer to random_access_file::get_unique_id()
            virtual size_t get_unique_id(char *id, size_t max_size) const {
                return 0; // default_environment implementation to prevent issues with backwards
            }

            // remove any kind of caching of data from the offset to offset+length
            // of this file. If the length is 0, then it refers to the end of file.
            // If the system is not caching the file contents, then this is a noop.
            // This call has no effect on dirty pages in the cache.
            virtual status_type invalidate_cache(size_t offset, size_t length) {
                return status_type::NotSupported("invalidate_cache not supported.");
            }

            // sync a file range with disk.
            // offset is the starting byte of the file range to be synchronized.
            // nbytes specifies the length of the range to be synchronized.
            // This asks the OS to initiate flushing the cached data to disk,
            // without waiting for completion.
            // default_environment implementation does nothing.
            virtual status_type range_sync(uint64_t offset, uint64_t nbytes) {
                return status_type();
            }

            // prepare_write performs any necessary preparation for a write
            // before the write actually occurs.  This allows for pre-allocation
            // of space on devices where it can result in less file
            // fragmentation and/or less waste from over-zealous filesystem
            // pre-allocation.
            virtual void prepare_write(size_t offset, size_t len) {
                if (preallocation_block_size_ == 0) {
                    return;
                }
                // If this write would cross one or more preallocation blocks,
                // determine what the last preallocation block necessary to
                // cover this write would be and allocate to that point.
                const auto block_size = preallocation_block_size_;
                size_t new_last_preallocated_block = (offset + len + block_size - 1) / block_size;
                if (new_last_preallocated_block > last_preallocated_block_) {
                    size_t num_spanned_blocks = new_last_preallocated_block - last_preallocated_block_;
                    allocate(block_size * last_preallocated_block_, block_size * num_spanned_blocks);
                    last_preallocated_block_ = new_last_preallocated_block;
                }
            }

            // Pre-allocates space for a file.
            virtual status_type allocate(uint64_t offset, uint64_t len) {
                return status_type();
            }

        protected:
            size_t preallocation_block_size() {
                return preallocation_block_size_;
            }

        private:
            size_t last_preallocated_block_;
            size_t preallocation_block_size_;

            // No copying allowed
            writable_file(const writable_file &);

            void operator=(const writable_file &);

        protected:
            friend class WritableFileWrapper;

            friend class WritableFileMirror;

            environment_type::io_priority io_priority_;
            environment_type::write_life_time_hint write_hint_;
        };

// A file abstraction for random reading and writing.
        class random_rw_file {
        public:
            random_rw_file() {
            }

            virtual ~random_rw_file() {
            }

            // Indicates if the class makes use of direct I/O
            // If false you must pass aligned buffer to write()
            virtual bool use_direct_io() const {
                return false;
            }

            // Use the returned alignment value to allocate
            // aligned buffer for Direct I/O
            virtual size_t get_required_buffer_alignment() const {
                return kDefaultPageSize;
            }

            // write bytes in `data` at  offset `offset`, Returns status_type::OK() on success.
            // Pass aligned buffer when use_direct_io() returns true.
            virtual status_type write(uint64_t offset, const slice &data) = 0;

            // read up to `n` bytes starting from offset `offset` and store them in
            // result, provided `scratch` size should be at least `n`.
            // Returns status_type::OK() on success.
            virtual status_type read(uint64_t offset, size_t n, slice *result, char *scratch) const = 0;

            virtual status_type flush() = 0;

            virtual status_type sync() = 0;

            virtual status_type fsync() {
                return sync();
            }

            virtual status_type close() = 0;

            // No copying allowed
            random_rw_file(const random_rw_file &) = delete;

            random_rw_file &operator=(const random_rw_file &) = delete;
        };

// memory_mapped_file_buffer object represents a memory-mapped file's raw buffer.
// Subclasses should release the mapping upon destruction.
        class memory_mapped_file_buffer {
        public:
            memory_mapped_file_buffer(void *_base, size_t _length) : base_(_base), length_(_length) {
            }

            virtual ~memory_mapped_file_buffer() = 0;

            // We do not want to unmap this twice. We can make this class
            // movable if desired, however, since
            memory_mapped_file_buffer(const memory_mapped_file_buffer &) = delete;

            memory_mapped_file_buffer &operator=(const memory_mapped_file_buffer &) = delete;

            void *get_base() const {
                return base_;
            }

            size_t get_len() const {
                return length_;
            }

        protected:
            void *base_;
            const size_t length_;
        };

// directory object represents collection of files and implements
// filesystem operations that can be executed on directories.
        class directory {
        public:
            virtual ~directory() {
            }

            // fsync directory. Can be called concurrently from multiple threads.
            virtual status_type fsync() = 0;

            virtual size_t get_unique_id(char *id, size_t max_size) const {
                return 0;
            }
        };

        enum info_log_level : unsigned char {
            DEBUG_LEVEL = 0, INFO_LEVEL, WARN_LEVEL, ERROR_LEVEL, FATAL_LEVEL, HEADER_LEVEL, NUM_INFO_LOG_LEVELS,
        };

// An interface for writing log messages.
        class Logger {
        public:
            size_t kDoNotSupportGetLogFileSize = (std::numeric_limits<size_t>::max)();

            explicit Logger(const info_log_level log_level = info_log_level::INFO_LEVEL) : closed_(false),
                    log_level_(log_level) {
            }

            virtual ~Logger();

            // close the log file. Must be called before destructor. If the return
            // status is NotSupported(), it means the implementation does cleanup in
            // the destructor
            virtual status_type Close();

            // write a header to the log file with the specified format
            // It is recommended that you log all header information at the start of the
            // application. But it is not enforced.
            virtual void LogHeader(const char *format, va_list ap) {
                // default_environment implementation does a simple INFO level log write.
                // Please override as per the logger class requirement.
                Logv(format, ap);
            }

            // write an entry to the log file with the specified format.
            virtual void Logv(const char *format, va_list ap) = 0;

            // write an entry to the log file with the specified log level
            // and format.  Any log with level under the internal log level
            // of *this (see @SetInfoLogLevel and @GetInfoLogLevel) will not be
            // printed.
            virtual void Logv(const info_log_level log_level, const char *format, va_list ap);

            virtual size_t GetLogFileSize() const {
                return kDoNotSupportGetLogFileSize;
            }

            // flush to the OS buffers
            virtual void Flush() {
            }

            virtual info_log_level GetInfoLogLevel() const {
                return log_level_;
            }

            virtual void SetInfoLogLevel(const info_log_level log_level) {
                log_level_ = log_level;
            }

        protected:
            virtual status_type CloseImpl();

            bool closed_;

        private:
            // No copying allowed
            Logger(const Logger &);

            void operator=(const Logger &);

            info_log_level log_level_;
        };


// Identifies a locked file.
        class FileLock {
        public:
            FileLock() {
            }

            virtual ~FileLock();

        private:
            // No copying allowed
            FileLock(const FileLock &);

            void operator=(const FileLock &);
        };

        extern void LogFlush(const std::shared_ptr<Logger> &info_log);

        extern void Log(const info_log_level log_level, const std::shared_ptr<Logger> &info_log, const char *format, ...);

// a set of log functions with different log levels.
        extern void Header(const std::shared_ptr<Logger> &info_log, const char *format, ...);

        extern void Debug(const std::shared_ptr<Logger> &info_log, const char *format, ...);

        extern void Info(const std::shared_ptr<Logger> &info_log, const char *format, ...);

        extern void Warn(const std::shared_ptr<Logger> &info_log, const char *format, ...);

        extern void Error(const std::shared_ptr<Logger> &info_log, const char *format, ...);

        extern void Fatal(const std::shared_ptr<Logger> &info_log, const char *format, ...);

// Log the specified data to *info_log if info_log is non-nullptr.
// The default info log level is info_log_level::INFO_LEVEL.
        extern void Log(const std::shared_ptr<Logger> &info_log, const char *format, ...)
#   if defined(__GNUC__) || defined(__clang__)
        __attribute__((__format__(__printf__, 2, 3)))
#   endif
        ;

        extern void LogFlush(Logger *info_log);

        extern void Log(const info_log_level log_level, Logger *info_log, const char *format, ...);

// The default info log level is info_log_level::INFO_LEVEL.
        extern void Log(Logger *info_log, const char *format, ...)
#   if defined(__GNUC__) || defined(__clang__)
        __attribute__((__format__ (__printf__, 2, 3)))
#   endif
        ;

// a set of log functions with different log levels.
        extern void Header(Logger *info_log, const char *format, ...);

        extern void Debug(Logger *info_log, const char *format, ...);

        extern void Info(Logger *info_log, const char *format, ...);

        extern void Warn(Logger *info_log, const char *format, ...);

        extern void Error(Logger *info_log, const char *format, ...);

        extern void Fatal(Logger *info_log, const char *format, ...);

// A utility routine: write "data" to the named file.
        extern status_type WriteStringToFile(environment_type *env, const slice &data, const std::string &fname,
                                             bool should_sync = false);

// A utility routine: read contents of named file into *data
        extern status_type ReadFileToString(environment_type *env, const std::string &fname, std::string *data);

// An implementation of environment_type that forwards all calls to another environment_type.
// May be useful to clients who wish to override just part of the
// functionality of another environment_type.
        class EnvWrapper : public environment_type {
        public:
            // Initialize an EnvWrapper that delegates all calls to *t
            explicit EnvWrapper(environment_type *t) : target_(t) {
            }

            ~EnvWrapper() override;

            // Return the target to which this environment_type forwards all calls
            environment_type *target() const {
                return target_;
            }

            // The following text is boilerplate that forwards all methods to target()
            status_type new_sequential_file(const std::string &f, std::unique_ptr<sequential_file> *r,
                                            const environment_options &options) override {
                return target_->new_sequential_file(f, r, options);
            }

            status_type new_random_access_file(const std::string &f, std::unique_ptr<random_access_file> *r,
                                               const environment_options &options) override {
                return target_->new_random_access_file(f, r, options);
            }

            status_type new_writable_file(const std::string &f, std::unique_ptr<writable_file> *r,
                                          const environment_options &options) override {
                return target_->new_writable_file(f, r, options);
            }

            status_type reopen_writable_file(const std::string &fname, std::unique_ptr<writable_file> *result,
                                             const environment_options &options) override {
                return target_->reopen_writable_file(fname, result, options);
            }

            status_type reuse_writable_file(const std::string &fname, const std::string &old_fname,
                                            std::unique_ptr<writable_file> *r,
                                            const environment_options &options) override {
                return target_->reuse_writable_file(fname, old_fname, r, options);
            }

            status_type new_random_rw_file(const std::string &fname, std::unique_ptr<random_rw_file> *result,
                                           const environment_options &options) override {
                return target_->new_random_rw_file(fname, result, options);
            }

            status_type new_directory(const std::string &name, std::unique_ptr<directory> *result) override {
                return target_->new_directory(name, result);
            }

            status_type file_exists(const std::string &f) override {
                return target_->file_exists(f);
            }

            status_type get_children(const std::string &dir, std::vector<std::string> *r) override {
                return target_->get_children(dir, r);
            }

            status_type get_children_file_attributes(const std::string &dir,
                                                     std::vector<file_attributes> *result) override {
                return target_->get_children_file_attributes(dir, result);
            }

            status_type delete_file(const std::string &f) override {
                return target_->delete_file(f);
            }

            status_type create_dir(const std::string &d) override {
                return target_->create_dir(d);
            }

            status_type create_dir_if_missing(const std::string &d) override {
                return target_->create_dir_if_missing(d);
            }

            status_type delete_dir(const std::string &d) override {
                return target_->delete_dir(d);
            }

            status_type get_file_size(const std::string &f, uint64_t *s) override {
                return target_->get_file_size(f, s);
            }

            status_type get_file_modification_time(const std::string &fname, uint64_t *file_mtime) override {
                return target_->get_file_modification_time(fname, file_mtime);
            }

            status_type rename_file(const std::string &s, const std::string &t) override {
                return target_->rename_file(s, t);
            }

            status_type link_file(const std::string &s, const std::string &t) override {
                return target_->link_file(s, t);
            }

            status_type num_file_links(const std::string &fname, uint64_t *count) override {
                return target_->num_file_links(fname, count);
            }

            status_type are_files_same(const std::string &first, const std::string &second, bool *res) override {
                return target_->are_files_same(first, second, res);
            }

            status_type lock_file(const std::string &f, FileLock **l) override {
                return target_->lock_file(f, l);
            }

            status_type unlock_file(FileLock *l) override {
                return target_->unlock_file(l);
            }

            void schedule(void (*f)(void *arg), void *a, Priority pri, void *tag = nullptr,
                          void (*u)(void *arg) = nullptr) override {
                return target_->schedule(f, a, pri, tag, u);
            }

            int unschedule(void *tag, Priority pri) override {
                return target_->unschedule(tag, pri);
            }

            void start_thread(void (*f)(void *), void *a) override {
                return target_->start_thread(f, a);
            }

            void wait_for_join() override {
                return target_->wait_for_join();
            }

            unsigned int get_thread_pool_queue_len(Priority pri = LOW) const override {
                return target_->get_thread_pool_queue_len(pri);
            }

            status_type get_test_directory(std::string *path) override {
                return target_->get_test_directory(path);
            }

            status_type new_logger(const std::string &fname, std::shared_ptr<Logger> *result) override {
                return target_->new_logger(fname, result);
            }

            uint64_t now_micros() override {
                return target_->now_micros();
            }

            uint64_t now_nanos() override {
                return target_->now_nanos();
            }

            void sleep_for_microseconds(int micros) override {
                target_->sleep_for_microseconds(micros);
            }

            status_type get_host_name(char *name, uint64_t len) override {
                return target_->get_host_name(name, len);
            }

            status_type get_current_time(int64_t *unix_time) override {
                return target_->get_current_time(unix_time);
            }

            status_type get_absolute_path(const std::string &db_path, std::string *output_path) override {
                return target_->get_absolute_path(db_path, output_path);
            }

            void set_background_threads(int num, Priority pri) override {
                return target_->set_background_threads(num, pri);
            }

            int get_background_threads(Priority pri) override {
                return target_->get_background_threads(pri);
            }

            status_type set_allow_non_owner_access(bool allow_non_owner_access) override {
                return target_->set_allow_non_owner_access(allow_non_owner_access);
            }

            void inc_background_threads_if_needed(int num, Priority pri) override {
                return target_->inc_background_threads_if_needed(num, pri);
            }

            void lower_thread_pool_io_priority(Priority pool = LOW) override {
                target_->lower_thread_pool_io_priority(pool);
            }

            void lower_thread_pool_cpu_priority(Priority pool = LOW) override {
                target_->lower_thread_pool_cpu_priority(pool);
            }

            std::string time_to_string(uint64_t time) override {
                return target_->time_to_string(time);
            }

            status_type get_thread_list(std::vector<thread_status> *thread_list) override {
                return target_->get_thread_list(thread_list);
            }

            thread_status_updater *get_thread_status_updater() const override {
                return target_->get_thread_status_updater();
            }

            uint64_t get_thread_id() const override {
                return target_->get_thread_id();
            }

            std::string generate_unique_id() override {
                return target_->generate_unique_id();
            }

            environment_options optimize_for_log_read(const environment_options &env_options) const override {
                return target_->optimize_for_log_read(env_options);
            }

            environment_options optimize_for_manifest_read(const environment_options &env_options) const override {
                return target_->optimize_for_manifest_read(env_options);
            }

            environment_options optimize_for_log_write(const environment_options &env_options,
                                                       const db_options &db_options) const override {
                return target_->optimize_for_log_write(env_options, db_options);
            }

            environment_options optimize_for_manifest_write(const environment_options &env_options) const override {
                return target_->optimize_for_manifest_write(env_options);
            }

            environment_options optimize_for_compaction_table_write(const environment_options &env_options,
                                                                    const immutable_db_options &immutable_ops) const override {
                return target_->optimize_for_compaction_table_write(env_options, immutable_ops);
            }

            environment_options optimize_for_compaction_table_read(const environment_options &env_options,
                                                                   const immutable_db_options &db_options) const override {
                return target_->optimize_for_compaction_table_read(env_options, db_options);
            }

        private:
            environment_type *target_;
        };

// An implementation of writable_file that forwards all calls to another
// writable_file. May be useful to clients who wish to override just part of the
// functionality of another writable_file.
// It's declared as friend of writable_file to allow forwarding calls to
// protected virtual methods.
        class WritableFileWrapper : public writable_file {
        public:
            explicit WritableFileWrapper(writable_file *t) : target_(t) {
            }

            status_type append(const slice &data) override {
                return target_->append(data);
            }

            status_type positioned_append(const slice &data, uint64_t offset) override {
                return target_->positioned_append(data, offset);
            }

            status_type truncate(uint64_t size) override {
                return target_->truncate(size);
            }

            status_type close() override {
                return target_->close();
            }

            status_type flush() override {
                return target_->flush();
            }

            status_type sync() override {
                return target_->sync();
            }

            status_type fsync() override {
                return target_->fsync();
            }

            bool is_sync_thread_safe() const override {
                return target_->is_sync_thread_safe();
            }

            bool use_direct_io() const override {
                return target_->use_direct_io();
            }

            size_t get_required_buffer_alignment() const override {
                return target_->get_required_buffer_alignment();
            }

            void set_io_priority(environment_type::io_priority pri) override {
                target_->set_io_priority(pri);
            }

            environment_type::io_priority get_io_priority() override {
                return target_->get_io_priority();
            }

            void set_write_life_time_hint(environment_type::write_life_time_hint hint) override {
                target_->set_write_life_time_hint(hint);
            }

            environment_type::write_life_time_hint get_write_life_time_hint() override {
                return target_->get_write_life_time_hint();
            }

            uint64_t get_file_size() override {
                return target_->get_file_size();
            }

            void set_preallocation_block_size(size_t size) override {
                target_->set_preallocation_block_size(size);
            }

            void get_preallocation_status(size_t *block_size, size_t *last_allocated_block) override {
                target_->get_preallocation_status(block_size, last_allocated_block);
            }

            size_t get_unique_id(char *id, size_t max_size) const override {
                return target_->get_unique_id(id, max_size);
            }

            status_type invalidate_cache(size_t offset, size_t length) override {
                return target_->invalidate_cache(offset, length);
            }

            status_type range_sync(uint64_t offset, uint64_t nbytes) override {
                return target_->range_sync(offset, nbytes);
            }

            void prepare_write(size_t offset, size_t len) override {
                target_->prepare_write(offset, len);
            }

            status_type allocate(uint64_t offset, uint64_t len) override {
                return target_->allocate(offset, len);
            }

        private:
            writable_file *target_;
        };

// Returns a new environment that stores its data in memory and delegates
// all non-file-engine tasks to base_env. The caller must delete the result
// when it is no longer needed.
// *base_env must remain live while the result is in use.
        environment_type *NewMemEnv(environment_type *base_env);

// Returns a new environment that is used for HDFS environment.
// This is a factory method for HdfsEnv declared in hdfs/env_hdfs.h
        status_type NewHdfsEnv(environment_type **hdfs_env, const std::string &fsname);

// Returns a new environment that measures function call times for filesystem
// operations, reporting results to variables in PerfContext.
// This is a factory method for TimedEnv defined in utilities/env_timed.cc.
        environment_type *NewTimedEnv(environment_type *base_env);

    }
} // namespace nil
