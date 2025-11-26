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

#include "bitters.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"
#include "bitters/i2c.h"

int
bitters_init(void)
{
    // Allows calling init multiple times
    static unsigned int initialized = 0;
    if (initialized++) { return 0; }

    int rc = 0;

    /* Initialize GPIO */
    if ((rc = bitters_gpio_init()) < 0)
	return rc;

    /* Initialize SPI */
    if ((rc = bitters_spi_init()) < 0)
	return rc;

    /* Initialize I2C */
    if ((rc = bitters_i2c_init()) < 0)
	return rc;

    /* Job's done */
    return rc;
}


int
bitters_reduced_latency(void) {
    /* Change scheduler priority to be more "real-time" */
    struct sched_param sp = {
        .sched_priority = sched_get_priority_max(SCHED_FIFO),
    };
    sched_setscheduler(0, SCHED_FIFO, &sp);

    /* Avoid swapping by locking page in memory */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    return 0;
}
