#pragma once

#if !defined(DCDB_LITE)

#include <string>

#include "env.hpp"

namespace nil {
    namespace dcdb {

        class encryption_provider;

// Returns an environment_type that encrypts data when stored on disk and decrypts data when
// read from disk.
        environment_type *new_encrypted_env(environment_type *base_env, encryption_provider *provider);

// block_access_cipher_stream is the base class for any cipher stream that
// supports random access at block level (without requiring data from other blocks).
// E.g. CTR (Counter operation mode) supports this requirement.
        class block_access_cipher_stream {
        public:
            virtual ~block_access_cipher_stream() {
            };

            // block_size returns the size of each block supported by this cipher stream.
            virtual size_t block_size() = 0;

            // encrypt one or more (partial) blocks of data at the file offset.
            // Length of data is given in dataSize.
            virtual status_type encrypt(uint64_t fileOffset, char *data, size_t dataSize);

            // decrypt one or more (partial) blocks of data at the file offset.
            // Length of data is given in dataSize.
            virtual status_type decrypt(uint64_t fileOffset, char *data, size_t dataSize);

        protected:
            // allocate scratch space which is passed to encrypt_block/decrypt_block.
            virtual void allocate_scratch(std::string &scratch) = 0;

            // encrypt a block of data at the given block index.
            // Length of data is equal to block_size();
            virtual status_type encrypt_block(uint64_t blockIndex, char *data, char *scratch) = 0;

            // decrypt a block of data at the given block index.
            // Length of data is equal to block_size();
            virtual status_type decrypt_block(uint64_t blockIndex, char *data, char *scratch) = 0;
        };

// block_cipher
        class block_cipher {
        public:
            virtual ~block_cipher() {
            };

            // block_size returns the size of each block supported by this cipher stream.
            virtual size_t block_size() = 0;

            // encrypt a block of data.
            // Length of data is equal to block_size().
            virtual status_type encrypt(char *data) = 0;

            // decrypt a block of data.
            // Length of data is equal to block_size().
            virtual status_type decrypt(char *data) = 0;
        };

// Implements a block_cipher using ROT13.
//
// Note: This is a sample implementation of block_cipher,
// it is NOT considered safe and should NOT be used in production.
        class rot13_block_cipher : public block_cipher {
        private:
            size_t blockSize_;
        public:
            rot13_block_cipher(size_t blockSize) : blockSize_(blockSize) {
            }

            virtual ~rot13_block_cipher() {
            };

            // block_size returns the size of each block supported by this cipher stream.
            virtual size_t block_size() override {
                return blockSize_;
            }

            // encrypt a block of data.
            // Length of data is equal to block_size().
            virtual status_type encrypt(char *data) override;

            // decrypt a block of data.
            // Length of data is equal to block_size().
            virtual status_type decrypt(char *data) override;
        };

// ctr_cipher_stream implements block_access_cipher_stream using an
// Counter operations mode. 
// See https://en.wikipedia.org/wiki/Block_cipher_mode_of_operation
//
// Note: This is a possible implementation of block_access_cipher_stream,
// it is considered suitable for use.
        class ctr_cipher_stream final : public block_access_cipher_stream {
        private:
            block_cipher &cipher_;
            std::string iv_;
            uint64_t initialCounter_;
        public:
            ctr_cipher_stream(block_cipher &c, const char *iv, uint64_t initialCounter) : cipher_(c),
                    iv_(iv, c.block_size()), initialCounter_(initialCounter) {
            };

            virtual ~ctr_cipher_stream() {
            };

            // block_size returns the size of each block supported by this cipher stream.
            virtual size_t block_size() override {
                return cipher_.block_size();
            }

        protected:
            // allocate scratch space which is passed to encrypt_block/decrypt_block.
            virtual void allocate_scratch(std::string &scratch) override;

            // encrypt a block of data at the given block index.
            // Length of data is equal to block_size();
            virtual status_type encrypt_block(uint64_t blockIndex, char *data, char *scratch) override;

            // decrypt a block of data at the given block index.
            // Length of data is equal to block_size();
            virtual status_type decrypt_block(uint64_t blockIndex, char *data, char *scratch) override;
        };

// The encryption provider is used to create a cipher stream for a specific file.
// The returned cipher stream will be used for actual encryption/decryption 
// actions.
        class encryption_provider {
        public:
            virtual ~encryption_provider() {
            };

            // get_prefix_length returns the length of the prefix that is added to every file
            // and used for storing encryption options.
            // For optimal performance, the prefix length should be a multiple of
            // the page size.
            virtual size_t get_prefix_length() = 0;

            // create_new_prefix initialized an allocated block of prefix memory
            // for a new file.
            virtual status_type create_new_prefix(const std::string &fname, char *prefix, size_t prefixLength) = 0;

            // create_cipher_stream creates a block access cipher stream for a file given
            // given name and options.
            virtual status_type create_cipher_stream(const std::string &fname, const environment_options &options,
                                                     slice &prefix, std::unique_ptr<block_access_cipher_stream> *result) = 0;
        };

// This encryption provider uses a CTR cipher stream, with a given block cipher 
// and IV.
//
// Note: This is a possible implementation of encryption_provider,
// it is considered suitable for use, provided a safe block_cipher is used.
        class ctr_encryption_provider : public encryption_provider {
        private:
            block_cipher &cipher_;
        protected:
            const static size_t defaultPrefixLength = 4096;

        public:
            ctr_encryption_provider(block_cipher &c) : cipher_(c) {
            };

            virtual ~ctr_encryption_provider() {
            }

            // get_prefix_length returns the length of the prefix that is added to every file
            // and used for storing encryption options.
            // For optimal performance, the prefix length should be a multiple of
            // the page size.
            virtual size_t get_prefix_length() override;

            // create_new_prefix initialized an allocated block of prefix memory
            // for a new file.
            virtual status_type create_new_prefix(const std::string &fname, char *prefix, size_t prefixLength) override;

            // create_cipher_stream creates a block access cipher stream for a file given
            // given name and options.
            virtual status_type create_cipher_stream(const std::string &fname, const environment_options &options,
                                                     slice &prefix, std::unique_ptr<block_access_cipher_stream> *result) override;

        protected:
            // populate_secret_prefix_part initializes the data into a new prefix block
            // that will be encrypted. This function will store the data in plain text.
            // It will be encrypted later (before written to disk).
            // Returns the amount of space (starting from the start of the prefix)
            // that has been initialized.
            virtual size_t populate_secret_prefix_part(char *prefix, size_t prefixLength, size_t blockSize);

            // create_cipher_stream_from_prefix creates a block access cipher stream for a file given
            // given name and options. The given prefix is already decrypted.
            virtual status_type create_cipher_stream_from_prefix(const std::string &fname,
                                                                 const environment_options &options,
                                                                 uint64_t initialCounter, const slice &iv,
                                                                 const slice &prefix,
                                                                 std::unique_ptr<block_access_cipher_stream> *result);
        };

    }
} // namespace nil

#endif  // !defined(DCDB_LITE)
