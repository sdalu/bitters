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

#if defined(BITTERS_WITH_THREADS)
#include <pthread.h>
#include <signal.h>
#endif


static void
_bitters_sigirq(int a) {
    /* Nothing */
}


int 
bitters_init(void) 
{
    int rc = 0;

#if defined(BITTERS_WITH_THREADS)
    /* Install dummy signal handler (to have interrupted system call) */
    struct sigaction sigact = { .sa_handler = _bitters_sigirq };
    struct sigaction oldsigact;
    rc = sigaction(BITTERS_SIGIRQ, &sigact, &oldsigact);
    if (rc < 0) {
	BITTERS_LOG("failed to install signal hander for %s",
		    strsignal(BITTERS_SIGIRQ));
	return -errno;
    }    
    BITTERS_ASSERT((oldsigact.sa_handler   == NULL) &&
		   (oldsigact.sa_sigaction == NULL));
    if ((oldsigact.sa_handler   != NULL) ||
	(oldsigact.sa_sigaction != NULL)) {
	// Restore original signal handler
	sigaction(BITTERS_SIGIRQ, &oldsigact, NULL);
	// Warn about it
	char *signame = strsignal(BITTERS_SIGIRQ);
	fprintf(stderr,
	"\n"
	"bitters: process is already intercepting %s\n"
	"       | orignal handler has been restored\n"
	"       | you can either:\n"
	"       |   * compile defining BITTERS_SIGIRQ to another signal\n"
	"       |   * change your program so that %s is free to be used\n"
	"       |   * don't use bitters IRQ callback\n"
	"\n", signame, signame);
	return -EBUSY;
    }
#endif

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
bitters_reduced_lattency(void) {
    /* Change scheduler priority to be more "real-time" */
    struct sched_param sp = {
        .sched_priority = sched_get_priority_max(SCHED_FIFO),
    };
    sched_setscheduler(0, SCHED_FIFO, &sp);

    /* Avoid swapping by locking page in memory */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    return 0;
}
