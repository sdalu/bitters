/* The spidev buffer warning: it must explain a transfer that ran into the
 * limit, and say nothing otherwise -- in particular nothing at init time,
 * which is where it used to be said, to every program, before anything
 * had been transferred.
 *
 * Touches no SPI bus: the interface is never enabled, so its descriptor
 * is -1 and every transfer fails with EBADF once it reaches the ioctl.
 * That is a real failure, which is what the warning hangs off. The
 * oversized length is never dereferenced for the same reason. Needs no
 * privilege. */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "bitters/spi.h"

/* Comfortably over any spidev bufsiz, default (one page) or raised, while
 * staying inside the 32-bit kernel length field. */
#define OVERSIZED (64u * 1024u * 1024u)

static int bad = 0;
static int cap = -1;                /* where stderr has been redirected */

static void expect(const char *what, int got, int want) {
    printf("  %-46s %-5d (want %d)%s\n", what, got, want,
           got == want ? "" : "  <-- MISMATCH");
    if (got != want) bad = 1;
}

/* How much the library has written to stderr since the last reset. */
static off_t said(void) {
    struct stat st;
    fflush(stderr);
    return (fstat(cap, &st) == 0) ? st.st_size : -1;
}

static void reset(void) {
    fflush(stderr);
    if (ftruncate(cap, 0) != 0) bad = 1;
    if (lseek(cap, 0, SEEK_SET) == (off_t)-1) bad = 1;
}

static void expect_said(const char *what, int want) {
    off_t n = said();
    expect(what, n > 0, want);
    reset();
}

int main(void) {
    char path[] = "/tmp/t_spi_bufsize.XXXXXX";

    /* The suite silences warnings for every other test; this one is about
     * a warning, so it speaks for itself whatever it was invoked with */
    unsetenv("BITTERS_SILENCE_WARNING");
    unsetenv("BITTERS_SILENCE_SPI_BUFSIZE_WARNING");

    cap = mkstemp(path);
    if ((cap < 0) || (unlink(path) != 0) || (dup2(cap, STDERR_FILENO) < 0)) {
        printf("spi bufsize warning: cannot capture stderr\n");
        return 1;
    }

    bitters_spi_t spi = BITTERS_SPI_INITIALIZER(0, 0);  /* never enabled */
    uint8_t buf[4] = { 0 };
    struct bitters_spi_transfer small = { .tx = buf, .rx = buf,
                                          .len = sizeof buf };
    struct bitters_spi_transfer over  = { .tx = buf, .rx = buf,
                                          .len = OVERSIZED };

    expect("init() does no I/O", bitters_spi_init(), 0);
    expect_said("  ... and says nothing about the buffer", 0);

    expect("a small transfer fails on the descriptor",
           bitters_spi_transfer(&spi, &small, 1), -EBADF);
    expect_said("  ... and says nothing: it fits the buffer", 0);

    expect("an oversized transfer fails the same way",
           bitters_spi_transfer(&spi, &over, 1), -EBADF);
    expect_said("  ... and explains the buffer limit", 1);

    expect("a second oversized transfer fails the same way",
           bitters_spi_transfer(&spi, &over, 1), -EBADF);
    expect_said("  ... and does not repeat itself", 0);

    /* Restore stderr so the verdict is visible even when it is a failure */
    printf("spi bufsize warning: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
