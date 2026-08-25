/*
 * Copyright (c) 2019
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sched.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#if defined(BITTERS_WITH_THREADS)
#include <pthread.h>
#endif

#include "bitters.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"
#include "bitters/i2c.h"

int
bitters_init(void)
{
    int rc = 0;

    /* Allows calling init multiple times, also concurrently: without
     * the lock, two first-time callers would both run the subsystem
     * inits, and the gpio signal-handler setup would mistake its own
     * sibling for a conflicting third-party handler */
    static unsigned int initialized = 0;
#if defined(BITTERS_WITH_THREADS)
    static pthread_mutex_t initialized_lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&initialized_lock);
#endif
    if (initialized) { goto done; }

    /* Initialize GPIO */
    if ((rc = bitters_gpio_init()) < 0)
	goto done;

    /* Initialize SPI */
    if ((rc = bitters_spi_init()) < 0)
	goto done;

    /* Initialize I2C */
    if ((rc = bitters_i2c_init()) < 0)
	goto done;

    /* Mark as initialized, only on success, so that a failed
     * initialization can be retried */
    initialized = 1;

    /* Job's done */
 done:
#if defined(BITTERS_WITH_THREADS)
    pthread_mutex_unlock(&initialized_lock);
#endif
    return rc;
}


int
bitters_reduced_latency(void) {
    int rc = 0;

    /* Change scheduler priority to be more "real-time" */
    struct sched_param sp = {
        .sched_priority = sched_get_priority_max(SCHED_FIFO),
    };
    if (sched_setscheduler(0, SCHED_FIFO, &sp) < 0)
	rc = -errno;

    /* Avoid swapping by locking page in memory
     * (still attempted even if the scheduler change failed) */
    if (mlockall(MCL_CURRENT | MCL_FUTURE) < 0 && rc == 0)
	rc = -errno;

    return rc;
}
