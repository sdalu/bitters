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
# Which of them you need, and nothing more: the subsystems never reach into
# one another, and CORE reaches into the ones this build says it has. Any
# combination links, CORE alone included.
#
# Name the ones you took, with -DBITTERS_WITH_GPIO, -DBITTERS_WITH_SPI and
# -DBITTERS_WITH_I2C, and put them on *your* files as well as on bitters.c
# -- INTERFACE definitions, so they reach both. They are what the headers
# are gated by, so a subsystem you did not name is not declared either, and
# a call to it is a compile error in the file that made it: earlier than a
# missing symbol at link time, and unmissable next to a runtime code a
# caller can ignore.
#
# Unlike BITTERS_WITH_THREADS they say nothing about how bitters itself was
# built -- they say which sources your target carries, which is yours to
# decide. A consumer of an installed library gets them from bitters.pc
# instead, having picked no source list of its own.
#
# DELAY has no flag: nothing reaches into it, so it is simply left out.
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
#                                                BITTERS_SILENCE_WARNING
#                                                BITTERS_WITH_GPIO
#                                                BITTERS_WITH_SPI)
#   target_sources(bitters INTERFACE ${BITTERS_SOURCES_CORE}
#                                    ${BITTERS_SOURCES_GPIO}
#                                    ${BITTERS_SOURCES_SPI})
#   target_link_libraries(daemon PRIVATE bitters)
#   target_link_libraries(gui    PRIVATE bitters Threads::Threads)
#   target_compile_definitions(gui PRIVATE BITTERS_WITH_THREADS)
#
# I2C is left out there, so BITTERS_WITH_I2C is not among the definitions
# -- and because they are INTERFACE, an I2C call in either program does not
# compile either. bitters_init() remains the one call both make. delay.c is
# left out as well and needs no flag.
#
# This file is for vendored trees. Installing bitters gives you a
# pkg-config file instead; see the README.

set(BITTERS_VERSION       1.1.1)

# The subsystems bitters_init() brings up, and so the ones whose headers
# are gated by BITTERS_WITH_<SUBSYSTEM>. CORE and DELAY have no gate:
# nothing reaches into DELAY, and CORE is what does the reaching.
set(BITTERS_SUBSYSTEMS    gpio i2c spi)

# What a hosted link needs, given BITTERS_WITH_THREADS.
set(BITTERS_LIBS          pthread)

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
