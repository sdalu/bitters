#ifndef __BITTERS__H
#define __BITTERS__H

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

#endif
