/*
 * Copyright (c) 2019 
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



/*== Exported function =================================================*/

int
bitters_spi_init(void)
{
    return 0;
}



int
bitters_spi_enable(bitters_spi_t *spi, bitters_spi_cfg_t *cfg)
{
    int rc = -1;

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
    if ((rc < 0) || (rc >= sizeof(path))) {
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

    uint8_t mode = cfg->mode;
    if (cfg->transfert == BITTERS_SPI_TRANSFERT_LSB)
	mode |= SPI_LSB_FIRST;
    rc = ioctl(spi->fd, SPI_IOC_WR_MODE, &mode);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("failed to set spi mode/transfert (%s)",
			strerror(errno));
	goto failed;
    }

    rc = ioctl(spi->fd, SPI_IOC_WR_BITS_PER_WORD, &cfg->word);
    if (rc < 0) { // XXX: ^^^ seems to have no effect ?! ^^^
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

    spi->word  = cfg->word;
    spi->speed = cfg->speed;
    
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
     */
    if (close(spi->fd) < 0) {
	int rc = -errno;
	BITTERS_SPI_LOG("failed to close spi-%d device (%s)",
			spi->id, strerror(errno));
	return rc;
    }

    spi->fd = -1;
    return 0;
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
bitters_spi_transfert(bitters_spi_t *spi,
	const struct bitters_spi_transfert *xfr, unsigned int count)
{
    struct spi_ioc_transfer tr[count];
    for (unsigned int i = 0 ; i < count ; i++) {
	memset(&tr[i], 0, sizeof(struct spi_ioc_transfer));
	tr[i].tx_buf        = (unsigned long) xfr[i].tx;
	tr[i].rx_buf        = (unsigned long) xfr[i].rx;
	tr[i].len           = xfr[i].len;
	tr[i].bits_per_word = spi->word;
	tr[i].speed_hz      = spi->speed;
    }
    
    int rc = ioctl(spi->fd, SPI_IOC_MESSAGE(count), &tr);
    if (rc < 0) {
	rc = -errno;
	BITTERS_SPI_LOG("can't send spi message (count=%d) (%s)",
		count, strerror(errno));
    } else {
	BITTERS_SPI_LOG("transfered done (count=%d)", count);
    }

    return rc;
}
