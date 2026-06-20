/*
 * Copyright (c) 2019-2020
 * Stephane D'Alu, Inria Chroma / Inria Agora, INSA Lyon, CITI Lab.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __BITTERS__I2C__H
#define __BITTERS__I2C__H

/**
 * @file  i2c.c
 * @brief I2C interface
 *
 * @addtogroup Bitters
 * @{
 */


#include <stddef.h>
#include <stdint.h>

/*== Macros ============================================================*/

/**
 * Initialize an I2C interface.
 * Ex: bitters_i2c_t i2c0 = BITTERS_I2C_INITIALIZER(id);
 */

#define BITTERS_I2C_INITIALIZER(_id)					\
    {									\
       .id           = (_id),						\
       .fd           = -1,						\
    }


/**
 * I2C definition.
 */
typedef struct bitters_i2c {
    int id;		/**< I2C device id  		*/
    /* private */
    int fd;		/* File descriptor on device	*/
    unsigned long funcs;/* I2C functionalities		*/
} bitters_i2c_t;


/**
 * I2C configuration.
 */
typedef struct bitters_i2c_cfg {
    uint32_t speed;	/**< Bus speed in Hz		*/
} bitters_i2c_cfg_t;


/**
 * I2C address.
 * @note if it is a 10-bit address, the flag BITTERS_I2C_ADDR_10 must
 *       be or-ed with it.
 */
typedef uint16_t bitters_i2c_addr_t;
#define BITTERS_I2C_ADDR_8	0x0000	/**< I2C  8-bit address		*/
#define BITTERS_I2C_ADDR_10	0x1000  /**< I2C 10-bit address flag	*/
#define BITTERS_I2C_ADDR_MSK	0x0fff	/**< mask isolating the address	*/


/**
 * I2C transfer chunk.
 *
 * Set @c dir to either BITTERS_I2C_TRANSFER_READ or
 * BITTERS_I2C_TRANSFER_WRITE to select the direction of the transfer.
 * The @c read / @c write bitfields are an alternate, endian-safe view of
 * @c dir and are not meant to be set together.
 */
struct bitters_i2c_transfer {
    uint8_t *buf;		/**< data buffer (read into or written from) */
    size_t   len;		/**< buffer size in bytes	*/
    union {
      uint8_t  dir;		/**< direction (read or write)	*/
      struct {
#if   defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	uint8_t read :1;	/**< read direction		*/
	uint8_t write:1;	/**< write direction		*/
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	uint8_t write:1;	/**< write direction		*/
	uint8_t read :1;	/**< read direction		*/
#elif defined(__BYTE_ORDER__)
#  error __BYTE_ORDER__ value is not supported
#else
#  error __BYTE_ORDER__ is not defined by compiler
#endif
      };
    };
};
#define BITTERS_I2C_TRANSFER_READ	0x02	/**< read from the device */
#define BITTERS_I2C_TRANSFER_WRITE	0x01	/**< write to the device	 */


/**
 * Initialize the I2C subsystem.
 * Normally called automatically by bitters_init(); call it directly only
 * if you use the I2C API without the rest of the library.
 *
 * @return < 0 in case of error (-errno)
 */
int bitters_i2c_init(void);

/**
 * Enable I2C interface with selected configuration.
 *
 * @param i2c		I2C interface
 * @param cfg		I2C configuration
 * @return < 0 in case of error (-errno)
 */
int bitters_i2c_enable(bitters_i2c_t *i2c, bitters_i2c_cfg_t *cfg);

/**
 * Disable I2C interface
 *
 * @param i2c		I2C interface
 * @return < 0 in case of error (-errno)
 */
int bitters_i2c_disable(bitters_i2c_t *i2c);

/**
 * Change speed of I2C bus.
 * @note Not supported on Linux: the bus speed is a property of the
 *       hardware platform and must be configured at boot time. This
 *       function always fails with -ENOSYS.
 *
 * @param i2c		I2C interface
 * @param speed		bus speed in Hz
 * @return -ENOSYS	always (see note)
 */
int bitters_i2c_set_speed(bitters_i2c_t *i2c, uint32_t speed);

/**
 * Perform an I2C transfer.
 *
 * @param i2c		I2C interface
 * @param addr		I2C address
 *			(or-ed with BITTERS_I2C_ADDR_10 if necessary)
 * @param xfr		chunks to be transferred
 * @param count		number of transferred chunks
 * @return < 0 in case of error (-errno)
 */
int bitters_i2c_transfer(bitters_i2c_t *i2c, bitters_i2c_addr_t addr,
	const struct bitters_i2c_transfer *xfr, unsigned int count);


/** @} */

#endif
