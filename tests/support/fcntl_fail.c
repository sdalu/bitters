/* Forces F_GETFL to fail, to exercise the path C8 is about. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
static int (*real_fcntl)(int, int, ...);
static int hook(int fd, int cmd, void *arg) {
    if (cmd == F_GETFL) { errno = EBADF; return -1; }
    if (!real_fcntl) real_fcntl = dlsym(RTLD_NEXT, "fcntl");
    return real_fcntl(fd, cmd, arg);
}
int fcntl(int fd, int cmd, ...) {
    va_list ap; va_start(ap, cmd); void *a = va_arg(ap, void *); va_end(ap);
    return hook(fd, cmd, a);
}
int fcntl64(int fd, int cmd, ...) {
    va_list ap; va_start(ap, cmd); void *a = va_arg(ap, void *); va_end(ap);
    return hook(fd, cmd, a);
}
