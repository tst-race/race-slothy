#ifndef __SHARD_H__
#define __SHARD_H__
#include "util/file_descriptor.hpp"
#include "util/logger.hpp"
#include "util/rs_shard.hpp"
#include "util/crypto_secretbox.hpp"
#include <stdlib.h>

extern "C"{
    #include "util/sss.h"
    #include "sodium.h"
    #include <assert.h>
    #include <string.h>

    void randomize_file(const char * infile);
    void unrandomize_file(const char * infile);
    void encrypt_file (const char * infile, const char * outfile, const unsigned char * key, const unsigned char * nonce);
    void decrypt_file (const char * infile, const char * outfile, const unsigned char * key, const unsigned char * nonce);
    void shard_file(const char*, const char*,int, int);
    void unshard_fd(struct file_descriptor*[], unsigned int, struct file_descriptor*, unsigned int, unsigned int);
    void unshard_file(const char * shard_prefix, const char * outfile, int threshold, int num_shares);

}

#endif