/* Forces F_GETFL to fail, to exercise the path C8 is about. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
/* glibc with 64-bit time on a 32-bit ABI (_TIME_BITS=64, which Debian
 * armhf defaults to since trixie) redirects both fcntl and fcntl64 to
 * __fcntl_time64, so defining both here would define one symbol twice.
 * There, one definition covers every caller, and the real one is looked
 * up under the name the redirect gave it. */
#ifdef __USE_TIME_BITS64
#define REAL_FCNTL "__fcntl_time64"
#else
#define REAL_FCNTL "fcntl"
#endif
static int (*real_fcntl)(int, int, ...);
static int hook(int fd, int cmd, void *arg) {
    if (cmd == F_GETFL) { errno = EBADF; return -1; }
    if (!real_fcntl) real_fcntl = dlsym(RTLD_NEXT, REAL_FCNTL);
    return real_fcntl(fd, cmd, arg);
}
int fcntl(int fd, int cmd, ...) {
    va_list ap; va_start(ap, cmd); void *a = va_arg(ap, void *); va_end(ap);
    return hook(fd, cmd, a);
}
#ifndef __USE_TIME_BITS64
int fcntl64(int fd, int cmd, ...) {
    va_list ap; va_start(ap, cmd); void *a = va_arg(ap, void *); va_end(ap);
    return hook(fd, cmd, a);
}
#endif
