# bitters -- source list for CMake consumers that vendor the tree.
#
#   include(${CMAKE_CURRENT_SOURCE_DIR}/3rd/bitters/bitters.cmake)
#
# Defines variables, not targets, on purpose. A consumer composes them
# itself, which is what lets you compile bitters differently for
# different targets (give only your threaded program BITTERS_WITH_THREADS)
# and leave out a subsystem you do not use.
#
#   BITTERS_VERSION          the release this tree is
#   BITTERS_INCLUDE_DIR      add to your include path
#   BITTERS_SOURCES          every source
#   BITTERS_SOURCES_CORE     bitters_init(), bitters_reduced_latency()
#   BITTERS_SOURCES_DELAY    bitters_delay_usec(), _msec()
#   BITTERS_SOURCES_GPIO     the GPIO API
#   BITTERS_SOURCES_I2C      the I2C API
#   BITTERS_SOURCES_SPI      the SPI API
#
# The .c files need -D_GNU_SOURCE; the public headers do not, and do not
# depend on the feature flags either -- a function whose feature was not
# compiled in returns -ENOSYS rather than going missing, so the headers
# are the same whatever you build with.
#
# Feature flags, all build-time only:
#   BITTERS_WITH_THREADS          thread-safe operation
#   BITTERS_WITH_GPIO_IRQ         interrupt callbacks (needs THREADS)
#   BITTERS_{GPIO,SPI,I2C}_WITH_ASSERT
#   BITTERS_{GPIO,SPI,I2C}_WITH_LOG
#   BITTERS_SILENCE_WARNING       no run-time configuration warnings
#
# For example, a consumer that uses GPIO and SPI but not I2C or delay,
# and wants threads only in one program:
#
#   include(3rd/bitters/bitters.cmake)
#   add_library(bitters INTERFACE)
#   target_include_directories(bitters INTERFACE ${BITTERS_INCLUDE_DIR})
#   target_compile_definitions(bitters INTERFACE _GNU_SOURCE
#                                                BITTERS_SILENCE_WARNING)
#   target_sources(bitters INTERFACE ${BITTERS_SOURCES_CORE}
#                                    ${BITTERS_SOURCES_GPIO}
#                                    ${BITTERS_SOURCES_SPI})
#   target_link_libraries(daemon PRIVATE bitters)
#   target_link_libraries(gui    PRIVATE bitters Threads::Threads)
#   target_compile_definitions(gui PRIVATE BITTERS_WITH_THREADS)
#
# This file is for vendored trees. Installing bitters gives you a
# pkg-config file instead; see the README.

set(BITTERS_VERSION       1.1.0)

set(BITTERS_INCLUDE_DIR   ${CMAKE_CURRENT_LIST_DIR}/include)

set(BITTERS_SOURCES_CORE  ${CMAKE_CURRENT_LIST_DIR}/src/bitters.c)
set(BITTERS_SOURCES_DELAY ${CMAKE_CURRENT_LIST_DIR}/src/delay.c)
set(BITTERS_SOURCES_GPIO  ${CMAKE_CURRENT_LIST_DIR}/src/gpio.c)
set(BITTERS_SOURCES_I2C   ${CMAKE_CURRENT_LIST_DIR}/src/i2c.c)
set(BITTERS_SOURCES_SPI   ${CMAKE_CURRENT_LIST_DIR}/src/spi.c)

set(BITTERS_SOURCES
    ${BITTERS_SOURCES_CORE}
    ${BITTERS_SOURCES_DELAY}
    ${BITTERS_SOURCES_GPIO}
    ${BITTERS_SOURCES_I2C}
    ${BITTERS_SOURCES_SPI})
