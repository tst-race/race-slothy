#include <stdint.h>
#include <stddef.h>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <vector>
#include <stdio.h>
#include <cstdio>
#include "file_descriptor.hpp"

extern "C"{
    #include <stdio.h>
    #include <stdlib.h>
    #include <assert.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #include <sys/fcntl.h>
    #include <openssl/rand.h>


//    #include "randombytes.h"
//    #include "tweetnacl.h"
    #include "sodium.h"


    void crypto_secretbox_encrypt_file(const char * infile, const char * outfile, const unsigned char * key, const unsigned char * nonce);
    void crypto_secretbox_decrypt_file(const char * infile, const char * outfile, const unsigned char * key, const unsigned char * nonce);
    void crypto_secretbox_decrypt_fd(const struct file_descriptor* infile, const struct file_descriptor* outfile, const unsigned char * key, const unsigned char * nonce);                    
}