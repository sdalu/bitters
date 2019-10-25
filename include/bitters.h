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
#define BITTERS_LOG(x, ...)						\
    fprintf(stderr, x "\n", ##__VA_ARGS__)
#endif

#ifndef BITTERS_ASSERT
#include <assert.h>
#define BITTERS_ASSERT(x)						\
    assert(x)
#endif


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
