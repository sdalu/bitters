/*
 * Copyright (c) 2019-2020,2025
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BITTERS__H
#define __BITTERS__H

/**
 * @file  bitters.c
 * @brief Library initialization and global configuration
 *
 * @addtogroup Bitters
 * @{
 */

/**
 * Logging macro used internally to report errors and debug information.
 * The default prints to @c stderr (appending a newline) while preserving
 * @c errno. Define your own before including this header, or on the
 * compiler command line, to redirect or silence logging.
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

/**
 * Assertion macro used internally when assertions are enabled.
 * Defaults to the standard @c assert(). Define your own before including
 * this header to override it.
 */
#ifndef BITTERS_ASSERT
#include <assert.h>
#define BITTERS_ASSERT(x)						\
    assert(x)
#endif


/**
 * Signal used internally to notify the GPIO IRQ processing thread that the
 * set of registered callbacks has changed (only relevant when built with
 * BITTERS_WITH_GPIO_IRQ). Override with @c -DBITTERS_SIGIRQ=SIGNAME if
 * @c SIGUSR1 is already used by your application.
 */
#ifndef BITTERS_SIGIRQ
#define BITTERS_SIGIRQ SIGUSR1
#endif


/**
 * Initialize library.
 * Library can be left in an half initialized state in case of error.
 *
 * @return < 0 in case of error (-errno)
 */
int bitters_init(void);

/**
 * Try to reduce latency of IO access.
 * It will raise scheduling priority and lock page in memory
 * to avoid swapping.
 *
 * @return < 0 in case of error (-errno)
 */
int bitters_reduced_latency(void);

/** @} */

#endif
