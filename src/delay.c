/*
 * Copyright (c) 2019
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <unistd.h>
#include <signal.h>

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
    usleep(ms * 1000);
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, &oldmask, NULL);
#else
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
#endif
}
