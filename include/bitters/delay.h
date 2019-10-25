#ifndef __BITTERS__DELAY__H
#define __BITTERS__DELAY__H

/**
 * @file  delay.c
 * @brief Generating delay
 *
 * If using thread, file must be compiled with -DBITTERS_WITH_THREADS
 *
 * @addtogroup Bitters
 * @{
 */

/**
 * Generate a delay of a few microseconds
 * @param us		number of microseconds to wait for
 */
void bitters_delay_usec(uint16_t us);

/**
 * Generate a delay of a few milliseconds
 * @param ms		number of milliseconds to wait for
 */
void bitters_delay_msec(uint16_t ms);


/** @} */

#endif
