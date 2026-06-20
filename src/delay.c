/*
 * Copyright (c) 2019
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#if defined(BITTERS_WITH_THREADS)
#include <pthread.h>
#endif

void
bitters_delay_usec(uint16_t us) {
    sigset_t mask, oldmask;
    sigfillset(&mask);
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, &mask, &oldmask);
#else
    sigprocmask(SIG_SETMASK, &mask, &oldmask);
#endif
    usleep(us);
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
#else
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
#endif
}

void
bitters_delay_msec(uint16_t ms) {
    sigset_t mask, oldmask;
    sigfillset(&mask);
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, &mask, &oldmask);
#else
    sigprocmask(SIG_SETMASK, &mask, &oldmask);
#endif
    /* usleep() is only defined for values < 1000000, so use nanosleep()
     * which has no such restriction
     */
    struct timespec ts = { .tv_sec  =  ms / 1000,
			   .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
#else
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
#endif
}
