/* Interposes close() with one that leaves errno modified -- which a
 * conforming close() is free to do on failure. Proves whether the caller's
 * reported error survives the cleanup path. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <unistd.h>
int close(int fd) {
    static int (*real)(int);
    if (!real) real = dlsym(RTLD_NEXT, "close");
    int r = real(fd);
    errno = EIO;            /* the clobber */
    return r;
}
