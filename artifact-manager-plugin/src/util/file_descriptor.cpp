#include "util/file_descriptor.hpp"

// SELinux on Android causes issues with shm_fd and ramfs
#if (defined __i386__ || defined __amd64__) && !defined __android__
// #define __NR_memfd_create 319 // https://code.woboq.org/qt5/include/asm/unistd_64.h.html
#define FD_ARCHDEP_INMEMORY // defining this will make it so the file descriptors are not closed, since closing the file descriptor will lose the allocated memory.
// Wrapper to call memfd_create syscall
// (should be used for older kernels. Ubuntu18.04 doesn't need this, but 16.04 does.)
//static inline int memfd_create(const char *name, unsigned int flags) {
//    return syscall(__NR_memfd_create, name, flags);
//}

// Returns a file descriptor where we can write our shared object
struct file_descriptor* get_fd_archdep(void) {
    struct file_descriptor *f = (struct file_descriptor*) malloc(sizeof(struct file_descriptor));
    if (f == NULL) {
        fprintf(stderr, "[-] 'malloc' failed to allocate file_descriptor\n");
        exit(-1);
    }
    f->filename = NULL;

    //If we have a kernel < 3.17
    // We need to use the less fancy way
    if (kernel_version() < 3.17) {
        f->fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRWXU);
        f->filename = concat("/dev/shm/", SHM_NAME);
        if (f->fd < 0) { //Something went wrong :(
            fprintf(stderr, "[-] 'shm_open' failed to get file descriptor to %s\n", f->filename);
            exit(-1);
        }
    }
    // If we have a kernel >= 3.17
    // We can use the funky style
    else {
        f->fd = syscall(__NR_memfd_create, SHM_NAME, 1);
        f->filename = (char*) malloc(sizeof(char)*MAX_FILEPATH_LENGTH); // TODO: fix hardcoded max path length
        snprintf(f->filename, MAX_FILEPATH_LENGTH, "/proc/%d/fd/%d", getpid(), f->fd);
        if (f->fd < 0) { //Something went wrong :(
            fprintf(stderr, "[-] 'memfd_create' failed to get file descriptor to %s\n", f->filename);
            exit(-1);
        }
    }

    #ifdef DEBUG
    printf("get_fd_archdep(void) allocated file at: `%s`\n", f->filename);
    #endif

    return f;
}


// Callback to write the shared object
size_t write_data (void *ptr, size_t size, size_t nmemb, int shm_fd) {
    int bytes_written = write(shm_fd, ptr, nmemb);
    if (bytes_written < 0) {
        fprintf(stderr, "[-] Could not write file :'(\n%s\n",strerror(errno));
        close(shm_fd);
        exit(-1);
    }
    //printf("Bytes written: %d\n", bytes_written);
    return bytes_written;
}

#endif