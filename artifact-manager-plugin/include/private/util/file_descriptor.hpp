
#ifndef __FILE_DESCRIPTOR_H__
#define __FILE_DESCRIPTOR_H__

#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>

#include "kernel.hpp"
#include "concat.hpp"

#define MAX_FILEPATH_LENGTH 1024 // TODO: remove this hardcoded value
#define SHM_NAME "slothy" // TODO: randomize this hardcoded path

struct file_descriptor {
    int fd;
    char* filename;
};

struct file_descriptor* get_fd_archdep(void);

size_t write_data (void *ptr, size_t size, size_t nmemb, int shm_fd);
#endif // __FILE_DESCRIPTOR_H__