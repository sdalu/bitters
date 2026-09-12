/* SPI argument validation and error paths. Touches no SPI bus: every
 * check either returns before the descriptor is used, or deliberately
 * targets a device that does not exist. Needs no privilege. */
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "bitters/spi.h"

static int bad = 0;
static void expect(const char *what, int got, int want) {
    printf("  %-46s %-5d (want %d)%s\n", what, got, want,
           got == want ? "" : "  <-- MISMATCH");
    if (got != want) bad = 1;
}
int main(void) {
    bitters_spi_t spi = BITTERS_SPI_INITIALIZER(0, 0);   /* never enabled: fd = -1 */
    uint8_t buf[4] = { 0 };
    struct bitters_spi_transfer xfr[600];
    for (unsigned i = 0; i < sizeof xfr / sizeof xfr[0]; i++) {
        xfr[i].tx = buf; xfr[i].rx = buf; xfr[i].len = sizeof buf;
    }

    /* nothing to transfer must be a no-op, not a zero-length VLA */
    expect("transfer(count=0)", bitters_spi_transfer(&spi, xfr, 0), 0);

    /* SPI_IOC_MESSAGE() encodes the size in 14 bits: 512 * 32 == 16384 */
    expect("transfer(count=512) rejected", bitters_spi_transfer(&spi, xfr, 512), -EINVAL);
    expect("transfer(count=600) rejected", bitters_spi_transfer(&spi, xfr, 600), -EINVAL);
    /* 511 must pass validation and only then fail on the closed fd */
    expect("transfer(count=511) reaches the ioctl", bitters_spi_transfer(&spi, xfr, 511), -EBADF);

    /* spi_ioc_transfer.len is a 32-bit field */
    if (sizeof(size_t) > 4) {
        struct bitters_spi_transfer big = { .tx = buf, .rx = buf,
                                            .len = (size_t)UINT32_MAX + 1 };
        expect("transfer with len > UINT32_MAX rejected",
               bitters_spi_transfer(&spi, &big, 1), -EINVAL);
    }

    /* set_speed / set_wordsize are struct updates, applied at next transfer */
    expect("set_speed(1000000)",  bitters_spi_set_speed(&spi, 1000000), 0);
    expect("  speed stored",      (int)spi.speed, 1000000);
    expect("set_speed(same)",     bitters_spi_set_speed(&spi, 1000000), 0);
    expect("set_wordsize(16)",    bitters_spi_set_wordsize(&spi, 16), 0);
    expect("  word size stored",  (int)spi.word, 16);

    /* error paths that never reach a real bus */
    bitters_spi_t nodev = BITTERS_SPI_INITIALIZER(99, 0);
    bitters_spi_cfg_t cfg = { .mode = 0, .speed = 1000000, .word = 8 };
    expect("enable(/dev/spidev99.0)", bitters_spi_enable(&nodev, &cfg), -ENOENT);
    expect("  fd left closed after a failed enable", nodev.fd, -1);

    bitters_spi_t huge = BITTERS_SPI_INITIALIZER(999999, 999999);
    expect("enable() with a path that would truncate",
           bitters_spi_enable(&huge, &cfg), -ENOMEM);

    expect("disable() of a never-enabled interface", bitters_spi_disable(&spi), 0);

    printf("spi arguments: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
