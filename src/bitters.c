#include <sched.h>
#include <sys/mman.h>
#include <errno.h>

#include "bitters.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"

#if defined(BITTERS_WITH_THREADS)
#include <pthread.h>
#include <signal.h>
#endif


static void
_bitters_sigusr1(int a) {
    /* Nothing */
}


int 
bitters_init(void) 
{
    int rc = 0;

#if defined(BITTERS_WITH_THREADS)
    /* Install dummy signal handler (to have interrupted system call) */
    struct sigaction sigact = { .sa_handler = _bitters_sigusr1 };
    struct sigaction oldsigact;
    rc = sigaction(SIGUSR1, &sigact, &oldsigact);
    if (rc < 0) {
	BITTERS_LOG("failed to install signal hander USR1");
	return -errno;
    }    
    BITTERS_ASSERT((oldsigact.sa_handler   == NULL) &&
		   (oldsigact.sa_sigaction == NULL));
    if ((oldsigact.sa_handler   != NULL) ||
	(oldsigact.sa_sigaction != NULL)) {
	sigaction(SIGUSR1, &oldsigact, NULL);
	BITTERS_LOG("process is already intercepting SIGUSR1, fix it!");
	return -EBUSY;
    }
#endif

    /* Initialize GPIO */
    if ((rc = bitters_gpio_init()) < 0)
	return rc;
	
    /* Initialize SPI */
    if ((rc = bitters_spi_init()) < 0)
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
