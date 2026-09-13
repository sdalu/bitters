/*
 * Copyright (c) 2019-2020,2024,2026
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <linux/types.h>
#include <linux/spi/spidev.h>

#include "bitters.h"
#include "bitters/gpio.h"
#include "bitters/spi.h"



/*== Log & Assert helpers ==============================================*/

#if defined(BITTERS_SPI_WITH_LOG)
#define BITTERS_SPI_LOG(x, ...)						\
    BITTERS_LOG("spi: " x, ##__VA_ARGS__)
#else
#define BITTERS_SPI_LOG(x, ...)
#endif

#if defined(BITTERS_SPI_WITH_ASSERT)
#define BITTERS_SPI_ASSERT(x)						\
    BITTERS_ASSERT(x)
#else
#define BITTERS_SPI_ASSERT(x)
#endif

#define BITTERS_SPI_ASSERT_CE(x)					\
    BITTERS_SPI_ASSERT(((x) >= -1) && ((x) < 100))

#define BITTERS_SPI_ASSERT_DEVID(x)					\
    BITTERS_SPI_ASSERT(((x) >= 0) && ((x) < 100))

#define BITTERS_SPI_ASSERT_BITS_PER_WORD(x)				\
    BITTERS_SPI_ASSERT(((x) == 8) || ((x) == 16))



#if !defined(BITTERS_SILENCE_WARNING            ) &&			\
    !defined(BITTERS_SILENCE_SPI_BUFSIZE_WARNING)
/* The spidev buffer a single transfer has to fit in: what the kernel was
 * told, and one page when it was told nothing (spidev's own default).
 * Read once; 0 when it cannot be established. */
static size_t
_bitters_spi_bufsiz(void)
{
    /* Atomic like the one-shot flag below, and for the same reason. Two
     * threads arriving together read /sys twice and store the same
     * answer, which is why a plain relaxed load and store is enough. */
    static size_t cached = 0;
    size_t bufsiz = __atomic_load_n(&cached, __ATOMIC_RELAXED);
    if (bufsiz != 0)
	return bufsiz;

    FILE *fp = fopen("/sys/module/spidev/parameters/bufsiz", "r");
    if (fp != NULL) {
	unsigned long value;
	if (fscanf(fp, "%lu", &value) == 1)
	    bufsiz = (size_t)value;
	fclose(fp);
    }
    if (bufsiz == 0) {
	long page = sysconf(_SC_PAGESIZE);
	if (page > 0)
	    bufsiz = (size_t)page;
    }
    __atomic_store_n(&cached, bufsiz, __ATOMIC_RELAXED);
    return bufsiz;
}

/* Explain a failed transfer that the spidev buffer limit explains, and
 * say nothing otherwise.
 *
 * This used to be said by bitters_spi_init(), to every program, before it
 * had transferred anything: a limit most of them never come near, on a
 * path where nothing had gone wrong yet. One consumer silenced every
 * warning the library has to be rid of it. Here it costs a stat of
 * /sys on the failure path only, and when it does appear it is about the
 * transfer that just failed. */
static void
_bitters_spi_warn_about_bufsize(size_t len)
{
    static int once   = 0;
    size_t     bufsiz = _bitters_spi_bufsiz();

    if ((bufsiz == 0) || (len <= bufsiz))                          return;
    if (__atomic_exchange_n(&once, 1, __ATOMIC_RELAXED) ||
	(getenv("BITTERS_SILENCE_WARNING"            ) != NULL) ||
	(getenv("BITTERS_SILENCE_SPI_BUFSIZE_WARNING") != NULL)) return;

    fprintf(stderr,
	"\n"
	"bitters: spi: a %zu byte segment does not fit the %zu byte spidev\n"
	"       | buffer, which is what this transfer failed on. Either\n"
	"       |   * raise it: spidev.bufsiz=<bytes> on the kernel command line\n"
	"       |   * or split the transfer into segments that fit\n"
	"\n", len, bufsiz);
}

#  define BITTERS_SPI_WARN_ABOUT_BUFSIZE(len)				\
    _bitters_spi_warn_about_bufsize(len)
#else
#  define BITTERS_SPI_WARN_ABOUT_BUFSIZE(len)	((void)(len))
#endif

/*== Exported function =================================================*/

int
bitters_spi_init(void)
{
    return 0;
}



int
bitters_spi_enable(bitters_spi_t *spi, bitters_spi_cfg_t *cfg)
{
    int rc = -EINVAL;

    BITTERS_SPI_ASSERT_DEVID(spi->id);
    BITTERS_SPI_ASSERT_CE(spi->ce);

    /* Already enabled */
    if (spi->fd >= 0)
	return 0;

    /* Enable
     */
    int ce = 0;
    if (spi->ce >= 0)
	ce = spi->ce;

    char path[18]; /* Enough room for: /dev/spidev00.00 */
    rc = snprintf(path, sizeof(path), "/dev/spidev%d.%d", spi->id, ce);
    if ((rc < 0) || ((size_t)rc >= sizeof(path))) {
	rc = -ENOMEM;
	BITTERS_SPI_LOG("failed to build path for spidev");
	goto failed;
    }

    spi->fd = open(path, O_RDWR);
    if (spi->fd < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("can't open spi device %s (%s)",
			path, strerror(errno));
	goto failed;
    }

    rc = ioctl(spi->fd, SPI_IOC_WR_MODE, &cfg->mode);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("failed to set spi mode (%s)", strerror(errno));
	goto failed;
    }

    rc = ioctl(spi->fd, SPI_IOC_WR_LSB_FIRST, &cfg->transfer);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("failed to set spi transfer (%s)", strerror(errno));
	goto failed;
    }

    rc = ioctl(spi->fd, SPI_IOC_WR_BITS_PER_WORD, &cfg->word);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("failed to set spi word size (%s)", strerror(errno));
	goto failed;
    }

    rc = ioctl(spi->fd, SPI_IOC_WR_MAX_SPEED_HZ, &cfg->speed);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("failed to set speed (%s)", strerror(errno));
	goto failed;
    }

    spi->word     = cfg->word;
    spi->speed    = cfg->speed;
    spi->transfer = cfg->transfer;

    BITTERS_SPI_LOG("SPI device %d enabled (using: %s)", spi->id, path);

    return 0;

 failed:
    if (spi->fd >= 0) {
	close(spi->fd);
	spi->fd = -1;
    }
    return rc;
}


int
bitters_spi_disable(bitters_spi_t *spi)
{
    /* Already disabled */
    if (spi->fd < 0)
	return 0;

    /* Disable
     * On Linux the descriptor is released even when close() fails
     * (e.g. EINTR), so the device is marked disabled regardless:
     * keeping the stale fd would make a later enable() no-op on a
     * dead descriptor, and a retried disable() a double close */
    int rc = 0;
    if (close(spi->fd) < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("failed to close spi-%d device (%s)",
			spi->id, strerror(errno));
    }

    spi->fd = -1;
    return rc;
}



int
bitters_spi_set_speed(bitters_spi_t *spi, uint32_t speed)
{
    if (spi->speed == speed)
	return 0;

    BITTERS_SPI_LOG("changing speed %d -> %d", spi->speed, speed);
    spi->speed = speed;

    return 0;
}

int
bitters_spi_set_wordsize(bitters_spi_t *spi, uint8_t word)
{
    if (spi->word == word)
	return 0;

    BITTERS_SPI_LOG("changing word size %d -> %d", spi->word, word);
    spi->word = word;

    return 0;
}


int
bitters_spi_transfer(bitters_spi_t *spi,
	const struct bitters_spi_transfer *xfr, unsigned int count)
{
    /* Nothing to transfer (also avoids a zero-length VLA below) */
    if (count == 0)
	return 0;

    /* SPI_IOC_MESSAGE(count) encodes the size in 14 bits. The bound is
     * checked by division (not through SPI_MSGSIZE, whose count *
     * sizeof multiplication can wrap on 32-bit size_t and slip a huge
     * count past the check into the VLA below). */
    if (count >= (1 << 14) / sizeof(struct spi_ioc_transfer))
	return -EINVAL;

    struct spi_ioc_transfer tr[count];
    size_t longest = 0;
    for (unsigned int i = 0 ; i < count ; i++) {
	/* Kernel spi_ioc_transfer.len is a 32-bit field.
	 * Compiled only where size_t is wider than that: on a 32-bit
	 * target the comparison can never be true, and a compiler that
	 * says so (gcc -Wtype-limits) is right */
#if SIZE_MAX > UINT32_MAX
	if (xfr[i].len > (size_t)UINT32_MAX) {
	    BITTERS_SPI_LOG("chunk %u too large (%zu)", i, xfr[i].len);
	    return -EINVAL;
	}
#endif
	memset(&tr[i], 0, sizeof(struct spi_ioc_transfer));
	tr[i].tx_buf        = (unsigned long) xfr[i].tx;
	tr[i].rx_buf        = (unsigned long) xfr[i].rx;
	tr[i].len           = xfr[i].len;
	tr[i].bits_per_word = spi->word;
	tr[i].speed_hz      = spi->speed;
	if (xfr[i].len > longest)
	    longest = xfr[i].len;
    }

    int rc = ioctl(spi->fd, SPI_IOC_MESSAGE(count), &tr);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("can't send spi message (count=%d) (%s)",
		count, strerror(errno));
	/* A segment the spidev buffer cannot hold is a failure the caller
	 * can act on, and one this layer cannot paper over: say so, once,
	 * and only when that is what the transfer ran into */
	BITTERS_SPI_WARN_ABOUT_BUFSIZE(longest);
    } else {
	BITTERS_SPI_LOG("transfered done (count=%d)", count);
    }

    return rc;
}
