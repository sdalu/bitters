#ifndef __BITTERS__SPI__H
#define __BITTERS__SPI__H

#include <linux/spi/spidev.h>

#define BITTERS_SPI_INITIALIZER(_id, _ce)				\
    {									\
       .id           = (_id),						\
       .ce	     = (_ce),						\
       .fd           = -1,						\
    }


#define BITTERS_SPI_TRANSFERT_MSB		0
#define BITTERS_SPI_TRANSFERT_LSB		1

#define BITTERS_SPI_WORDSIZE(x)			(x)

#define BITTERS_SPI_MODE_0			SPI_MODE_0
#define BITTERS_SPI_MODE_1			SPI_MODE_1
#define BITTERS_SPI_MODE_2			SPI_MODE_2
#define BITTERS_SPI_MODE_3			SPI_MODE_3


typedef struct bitters_spi {
    int id;		/* SPI device id  		*/
    int ce;		/* Chip Enable id 		*/
    /* private */
    int fd;		/* File descriptor on device	*/
    uint32_t speed;	/* Bus speed in Hz		*/
    uint8_t  word;	/* Size of SPI word: 8, 16	*/
} bitters_spi_t;


typedef struct bitters_spi_cfg {
    uint8_t  mode;	/* Bus mode: SPI_MODE_{0,1,2,3} */
    uint32_t speed;	/* Bus speed in Hz		*/
    uint8_t  word;	/* Size of SPI word: 8, 16	*/ 
    uint8_t  transfert; /* Transfert mode: LSB or MSB	*/
} bitters_spi_cfg_t;

struct bitters_spi_transfert {
    uint8_t *tx;	/* RX buffer or NULL 		*/
    uint8_t *rx;	/* TX buffer ot NULL 		*/
    size_t   len;	/* buffer size			*/
};


int bitters_spi_init(void);
int bitters_spi_enable(bitters_spi_t *pin, bitters_spi_cfg_t *cfg);
int bitters_spi_set_speed(bitters_spi_t *spi, uint32_t speed);
int bitters_spi_transfert(bitters_spi_t *spi,
	const struct bitters_spi_transfert *xfr, unsigned int count);

#endif
