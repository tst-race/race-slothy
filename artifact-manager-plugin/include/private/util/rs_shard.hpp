#ifndef __RS_SHARD_H__
#define __RS_SHARD_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <string.h>
#include <openssl/md5.h>

#include <openssl/sha.h>
#include <openssl/rand.h>


#include "jerasure.h"
#include "reed_sol.h"

//#include <jerasure.h>
//#include <jerasure/reed_sol.h>

#include "file_descriptor.hpp"

#if defined(_RS_MODE_1_)
	#define WORD_SIZE 32
	#define META_SIZE 1024
	#define META_BUFFERSIZE 1024
	#define META_READINS 1

	#define RS_SHARDS_TOTAL 16
	#define RS_SHARDS_REQUIRED 8
#elif defined(_RS_MODE_2_)
	#define RS_WORD_SIZE 32
	#define RS_META_SIZE 1024
	#define RS_META_BUFFERSIZE 1024
	#define RS_META_READINS 1

	#define RS_SHARDS_TOTAL 16
	#define RS_SHARDS_REQUIRED 8
#else
#define _RS_DEFAULT_MODE_
	#define RS_WORD_SIZE 32
	#define RS_META_SIZE 1024
	#define RS_META_BUFFERSIZE 1024
	#define RS_META_READINS 1

	#define RS_SHARDS_TOTAL 16
	#define RS_SHARDS_REQUIRED 8
#endif

#define NUM_HASHES 100
#define RANDOMIZE_BYTES_DEBUG 160 // multiple of 32


typedef struct {
	char* key;
	int value;
}item;

typedef struct {
    int key;
    struct file_descriptor* file;
}file_info;

extern "C" {
	int rs_shard_fd(const struct file_descriptor*, struct file_descriptor*[], int threshold, int num_shards);
	int rs_shard_file(const char*, const char*, int threshold, int num_shards);
	int rs_unshard_fd(struct file_descriptor*[], int num_fds, const struct file_descriptor*, int threshold, int num_shards);
	int rs_unshard_file(const char*, const char*, int threshold, int num_shards);

	int rs_shard_meta(struct file_descriptor*[], char* buff, int len, int k, int m, int w);

	int randomize_header(const struct file_descriptor*);
	int unrandomize_header(const struct file_descriptor*);
}
#endif
