#ifndef __BITTERS__H
#define __BITTERS__H

/**
 * @file  bitters.c
 * @brief Various 
 *
 * @addtogroup Bitters
 * @{
 */

#ifndef BITTERS_LOG
#include <stdio.h>
#include <errno.h>
#define BITTERS_LOG(x, ...) do {					\
	int errno_saved = errno;					\
	fprintf(stderr, x "\n", ##__VA_ARGS__);				\
	errno = errno_saved;						\
    } while(0)
#endif

#ifndef BITTERS_ASSERT
#include <assert.h>
#define BITTERS_ASSERT(x)						\
    assert(x)
#endif


#ifndef BITTERS_SIGIRQ
#define BITTERS_SIGIRQ SIGUSR1
#endif


/**
 * Initialize library.
 * Library can be left in an half initialized state in case of error.
 *
 * @return < 0 in case of error
 */
int bitters_init(void);

/**
 * Try to reduce lattency of IO access.
 * It will raise scheduling priority and lock page in memory
 * to avoid swapping.
 *
 * @return < 0 in case of error
 */
int bitters_reduced_lattency(void);

/** @} */

#endif
