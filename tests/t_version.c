/* The version, on both sides of the line it is drawn on:
 *
 *   the macros in bitters/version.h say what the *headers* are, settled
 *   when this file was compiled;
 *   bitters_version() says what the *library* is, settled when the
 *   library was compiled -- and it carries the git part, which is how a
 *   build made between releases admits to it.
 *
 * So this test is built the way the difference really arises: ../src/bitters.c
 * gets -DBITTERS_VERSION_GIT (a made-up one), this file does not. The two
 * answers must then differ in exactly that, which is also what proves the
 * macro is not quietly leaking into the caller's view. See tests/Makefile.
 *
 * Nothing here touches a device, and no subsystem is named, so bitters.c
 * links alone. */
#include <stdio.h>
#include <string.h>
#include "bitters/version.h"

/* What tests/Makefile compiled the library half with, "" if this test was
 * built by hand without it. */
#ifndef BH_EXPECT_GIT
#define BH_EXPECT_GIT ""
#endif

int main(void) {
    int bad = 0;
    char buf[64];

    /* The string is composed from the three numbers; a hand-written copy
     * that drifted from them is exactly what that composition prevents. */
    snprintf(buf, sizeof buf, "%d.%d.%d",
	     BITTERS_VERSION_MAJOR, BITTERS_VERSION_MINOR,
	     BITTERS_VERSION_PATCH);
    if (strcmp(buf, BITTERS_VERSION_STRING) != 0) {
	printf("  BITTERS_VERSION_STRING is \"%s\", want \"%s\"\n",
	       BITTERS_VERSION_STRING, buf); bad = 1;
    }

    /* Two digits per field, so a field never carries into the next one */
    if (BITTERS_VERSION_NUMBER != BITTERS_VERSION_MAJOR * 10000 +
				  BITTERS_VERSION_MINOR * 100 +
				  BITTERS_VERSION_PATCH) {
	printf("  BITTERS_VERSION_NUMBER is %d, does not match %s\n",
	       BITTERS_VERSION_NUMBER, BITTERS_VERSION_STRING); bad = 1;
    }
    if (BITTERS_VERSION_MINOR > 99 || BITTERS_VERSION_PATCH > 99) {
	printf("  a version field went past 99: %s no longer compares "
	       "as a number\n", BITTERS_VERSION_STRING); bad = 1;
    }

    /* AT_LEAST at its boundaries: this very version, one patch further
     * (which these headers are not), and the previous major (which they
     * are past). Written with the numbers rather than literals so the
     * test does not need editing at every release. */
    if (!BITTERS_VERSION_AT_LEAST(BITTERS_VERSION_MAJOR,
				  BITTERS_VERSION_MINOR,
				  BITTERS_VERSION_PATCH)) {
	printf("  AT_LEAST says %s is not itself\n",
	       BITTERS_VERSION_STRING); bad = 1;
    }
    if (BITTERS_VERSION_AT_LEAST(BITTERS_VERSION_MAJOR,
				 BITTERS_VERSION_MINOR,
				 BITTERS_VERSION_PATCH + 1)) {
	printf("  AT_LEAST says %s is already the next patch\n",
	       BITTERS_VERSION_STRING); bad = 1;
    }
    if (BITTERS_VERSION_MAJOR > 0 &&
	!BITTERS_VERSION_AT_LEAST(BITTERS_VERSION_MAJOR - 1, 99, 99)) {
	printf("  AT_LEAST says %s is not past the previous major\n",
	       BITTERS_VERSION_STRING); bad = 1;
    }

    /* The library's own answer: the release these headers name, plus
     * whatever the library was built with. */
    snprintf(buf, sizeof buf, "%s%s", BITTERS_VERSION_STRING, BH_EXPECT_GIT);
    if (strcmp(bitters_version(), buf) != 0) {
	printf("  bitters_version() is \"%s\", want \"%s\"\n",
	       bitters_version(), buf); bad = 1;
    }

    /* The git part reached the library and not this file: the difference
     * the two answers exist to express. Only checkable when the Makefile
     * passed one. */
    if (BH_EXPECT_GIT[0] != '\0') {
	if (BH_EXPECT_GIT[0] != '+') {
	    printf("  a git part must start with '+': \"%s\"\n",
		   BH_EXPECT_GIT); bad = 1;
	}
	if (strcmp(bitters_version(), BITTERS_VERSION_STRING) == 0) {
	    printf("  bitters_version() lost the library's git part\n");
	    bad = 1;
	}
	if (BITTERS_VERSION_GIT[0] != '\0') {
	    printf("  BITTERS_VERSION_GIT leaked into a consumer: \"%s\"\n",
		   BITTERS_VERSION_GIT); bad = 1;
	}
    }

    printf("version: headers %s, library %s%s\n",
	   BITTERS_VERSION_STRING, bitters_version(),
	   bad ? "  INCONSISTENT" : "");
    return bad;
}
