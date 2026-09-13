/*
 * Copyright (c) 2026
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BITTERS__VERSION__H
#define __BITTERS__VERSION__H

/**
 * @file  version.h
 * @brief Library version
 *
 * @addtogroup Bitters
 * @{
 */

/* The release, and the one place it is written.
 *
 * A C header can read no other file, so if a consumer is to have the
 * version without a build step, the version has to be written where the
 * header can see it -- here. Everything else reads it from here:
 * bitters.cmake parses these three lines, and the Makefile asks
 * scripts/manifest.sh, which parses them too. So `make version`,
 * bitters.pc, the soname and BITTERS_VERSION_STRING cannot disagree,
 * there being nothing left to disagree with.
 *
 * Bumping a release is editing these three numbers and tagging v<M>.<m>.<p>.
 */

/** Major version of the release */
#define BITTERS_VERSION_MAJOR 1
/** Minor version of the release */
#define BITTERS_VERSION_MINOR 1
/** Patch version of the release */
#define BITTERS_VERSION_PATCH 1

/* Composed rather than spelled out, so the numbers above stay the only
 * copy. The two levels are the usual stringify dance: the inner one is
 * what expands BITTERS_VERSION_MAJOR before # freezes it.
 */
#define __BITTERS_VERSION_STR(x)  #x
#define __BITTERS_VERSION_XSTR(x) __BITTERS_VERSION_STR(x)

/**
 * The release as a string, @c "1.2.3".
 *
 * This is what the headers are, not necessarily what the library you
 * link against is; @c bitters_version() is the one that knows that, and
 * it is also the one that carries the git part of a build made between
 * releases.
 */
#define BITTERS_VERSION_STRING						\
    __BITTERS_VERSION_XSTR(BITTERS_VERSION_MAJOR) "."			\
    __BITTERS_VERSION_XSTR(BITTERS_VERSION_MINOR) "."			\
    __BITTERS_VERSION_XSTR(BITTERS_VERSION_PATCH)

/**
 * The release as one comparable integer, for @c \#if -- 1.2.3 is 10203.
 * Each field is given two digits, so a field never reaches the next one
 * (1.1.1 is 10101, well under 1.2.0's 10200).
 */
#define BITTERS_VERSION_NUMBER						\
    (BITTERS_VERSION_MAJOR * 10000 +					\
     BITTERS_VERSION_MINOR * 100 +					\
     BITTERS_VERSION_PATCH)

/**
 * Whether these headers are release @p maj.@p min.@p pat or newer, for
 * conditional compilation:
 *
 * @code
 * #if !BITTERS_VERSION_AT_LEAST(1, 1, 0)
 * #error bitters 1.1.0 or newer is required
 * #endif
 * @endcode
 *
 * It answers for the headers, which is what a @c \#if can answer for. A
 * program that must also know what it is running against asks
 * @c bitters_version() at run time.
 */
#define BITTERS_VERSION_AT_LEAST(maj, min, pat)				\
    (BITTERS_VERSION_NUMBER >= ((maj) * 10000 + (min) * 100 + (pat)))

/**
 * What a build between releases adds to the version, and nothing (@c "")
 * for a release or wherever it could not be known.
 *
 * Set when bitters itself is compiled -- the Makefile passes
 * @c -DBITTERS_VERSION_GIT, from @c scripts/gitversion.sh -- so it
 * describes the library, not its consumer. A tree built from a tarball,
 * or vendored into somebody else's repository, leaves it empty rather
 * than reporting that repository's git state as bitters'.
 *
 * The shape is SemVer build metadata: @c "+3.gae9c67b" is three commits
 * past the release tag at that commit, and @c ".dirty" is appended when
 * the worktree had uncommitted changes.
 */
#ifndef BITTERS_VERSION_GIT
#define BITTERS_VERSION_GIT ""
#endif

/**
 * The version of the library this call reaches, as a string.
 *
 * @c BITTERS_VERSION_STRING is what the headers say and is settled when
 * your program is compiled; this is what the code answering the call
 * was, which for a shared library is only known once it is loaded --
 * the two differ exactly when the library was replaced underneath you.
 * It carries @c BITTERS_VERSION_GIT too, so a build made between
 * releases says so: @c "1.1.1+3.gae9c67b".
 *
 * @return the version, a static string that is never NULL
 */
const char *bitters_version(void);

/** @} */

#endif
