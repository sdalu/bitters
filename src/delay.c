/*
 * Copyright (c) 2019,2026
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

/* Block everything that could cut the delay short, but leave the
 * synchronous fault signals deliverable: blocking those is undefined if
 * one is actually raised, and it would cost the application its own
 * handler. The sleeps below cannot raise them anyway. */
static void
_bitters_delay_block(sigset_t *oldmask) {
    static const int sync_signal[] = {
	SIGBUS, SIGFPE, SIGILL, SIGSEGV, SIGSYS, SIGTRAP,
    };
    sigset_t mask;
    sigfillset(&mask);
    for (unsigned int i = 0 ;
	 i < sizeof(sync_signal) / sizeof(sync_signal[0]) ; i++)
	sigdelset(&mask, sync_signal[i]);
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, &mask, oldmask);
#else
    sigprocmask(SIG_SETMASK, &mask, oldmask);
#endif
}

static void
_bitters_delay_restore(const sigset_t *oldmask) {
#if defined(BITTERS_WITH_THREADS)
    pthread_sigmask(SIG_SETMASK, oldmask, NULL);
#else
    sigprocmask(SIG_SETMASK, oldmask, NULL);
#endif
}


void
bitters_delay_usec(uint16_t us) {
    sigset_t oldmask;
    _bitters_delay_block(&oldmask);
    usleep(us);
    _bitters_delay_restore(&oldmask);
}

void
bitters_delay_msec(uint16_t ms) {
    sigset_t oldmask;
    _bitters_delay_block(&oldmask);
    /* usleep() is only defined for values < 1000000, so use nanosleep()
     * which has no such restriction
     */
    struct timespec ts = { .tv_sec  =  ms / 1000,
			   .tv_nsec = (ms % 1000) * 1000000L };
    nanosleep(&ts, NULL);
    _bitters_delay_restore(&oldmask);
}
