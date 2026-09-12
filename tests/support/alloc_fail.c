/* Fails the Nth calloc/malloc (N from BH_FAIL_AT), to exercise the
 * allocation-failure paths in _bitters_gpio_ctrl_create including the
 * newly added ctrl->pfds. */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
static void *(*real_calloc)(size_t,size_t);
static void *(*real_malloc)(size_t);
static long  counter = 0, fail_at = -1;
static int   armed = 0;
static void init(void){
    if (armed) return;
    armed = 1;
    real_calloc = dlsym(RTLD_NEXT, "calloc");
    real_malloc = dlsym(RTLD_NEXT, "malloc");
    const char *e = getenv("BH_FAIL_AT");
    fail_at = e ? atol(e) : -1;
}
void *calloc(size_t n, size_t s){
    if (!armed) { init(); }
    if (!real_calloc) return NULL;
    if (fail_at >= 0 && ++counter == fail_at) { errno = ENOMEM; return NULL; }
    return real_calloc(n,s);
}
void *malloc(size_t s){
    if (!armed) { init(); }
    if (!real_malloc) return NULL;
    if (fail_at >= 0 && ++counter == fail_at) { errno = ENOMEM; return NULL; }
    return real_malloc(s);
}
