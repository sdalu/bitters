/*
 * Copyright (c) 2019-2020,2024
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BITTERS__SPI__H
#define __BITTERS__SPI__H

/**
 * @file  spi.c
 * @brief SPI interface
 *
 * @addtogroup Bitters
 * @{
 */


#include <stddef.h>
#include <stdint.h>
#include <linux/spi/spidev.h>

/*== Constants =========================================================*/

/**
 * Most Significant Bit first
 */
#define BITTERS_SPI_TRANSFER_MSB		0
/**
 * Least Significant Bit first
 */
#define BITTERS_SPI_TRANSFER_LSB		1

/**
 * SPI word size.
 * Usual value: 8, 16.
 */
#define BITTERS_SPI_WORDSIZE(x)			(x)

/**
 * SPI Mode 0
 */
#define BITTERS_SPI_MODE_0			SPI_MODE_0
/**
 * SPI Mode 1 (CPHA)
 */
#define BITTERS_SPI_MODE_1			SPI_MODE_1
/**
 * SPI Mode 2 (CPOL)
 */
#define BITTERS_SPI_MODE_2			SPI_MODE_2
/**
 * SPI Mode 3 (CPOL + CPHA)
 */
#define BITTERS_SPI_MODE_3			SPI_MODE_3


/*== Macros ============================================================*/

/**
 * Initialize an SPI interface.
 * Ex: bitters_spi_t spi0 = BITTERS_SPI_INITIALIZER(id, ce);
 */

#define BITTERS_SPI_INITIALIZER(_id, _ce)				\
    {									\
       .id           = (_id),						\
       .ce	     = (_ce),						\
       .fd           = -1,						\
    }


/**
 * SPI definition.
 */
typedef struct bitters_spi {
    int id;		/**< SPI device id  		*/
    int ce;		/**< Chip Enable id 		*/
    /* private */
    int fd;		/* File descriptor on device	*/
    uint32_t speed;	/* Bus speed in Hz		*/
    uint8_t  word;	/* Size of SPI word: 8, 16	*/
    uint8_t  transfer;  /* Transfer mode: LSB or MSB	*/
} bitters_spi_t;


/**
 * SPI configuration.
 */
typedef struct bitters_spi_cfg {
    uint8_t  mode;	/**< Bus mode: SPI_MODE_{0,1,2,3} */
    uint32_t speed;	/**< Bus speed in Hz		*/
    uint8_t  word;	/**< Size of SPI word: 8, 16	*/
    uint8_t  transfer;  /**< Transfer mode: LSB or MSB	*/
} bitters_spi_cfg_t;


/**
 * SPI transfer chunk.
 */
struct bitters_spi_transfer {
    uint8_t *tx;	/**< RX buffer or NULL 		*/
    uint8_t *rx;	/**< TX buffer ot NULL 		*/
    size_t   len;	/**< buffer size		*/
};


int bitters_spi_init(void);

/**
 * Enable SPI interface with selected configuration.
 *
 * @param spi		SPI interface
 * @param cfg		SPI configuration
 * @return < 0 in case of error (-errno)
 */
int bitters_spi_enable(bitters_spi_t *spi, bitters_spi_cfg_t *cfg);

/**
 * Disable SPI interface
 *
 * @param spi		SPI interface
 * @return < 0 in case of error (-errno)
 */
int bitters_spi_disable(bitters_spi_t *spi);

/**
 * Change speed of SPI bus.
 *
 * @param spi		SPI interface
 * @param speed		bus speed in Hz
 * @return < 0 in case of error (-errno)
 */
int bitters_spi_set_speed(bitters_spi_t *spi, uint32_t speed);

/**
 * Change word size of SPI bus.
 *
 * @param spi		SPI interface
 * @param word		word size
 * @return < 0 in case of error (-errno)
 */
int bitters_spi_set_wordsize(bitters_spi_t *spi, uint8_t word);

/**
 * Change speed of SPI bus.
 *
 * @param spi		SPI interface
 * @param xfr		chunk to be transfered
 * @param count		number of transfered chunk
 * @return < 0 in case of error (-errno)
 */
int bitters_spi_transfer(bitters_spi_t *spi,
	const struct bitters_spi_transfer *xfr, unsigned int count);

/** @} */

#endif
