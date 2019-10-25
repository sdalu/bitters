#include <sched.h>
#include <sys/mman.h>

#include "bitters/gpio.h"
#include "bitters/spi.h"


int 
bitters_init(void) 
{
    int rc = 0;
    
    if ((rc = bitters_gpio_init()) < 0)
	return rc;
	
    if ((rc = bitters_spi_init()) < 0)
	return rc;

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
