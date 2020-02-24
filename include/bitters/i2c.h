/*
 * Copyright (c) 2019
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
#define BITTERS_I2C_ADDR_8	0x0000	/**< I2C  8-bit address */
#define BITTERS_I2C_ADDR_10	0x1000  /**< I2C 10-bit address */
#define BITTERS_I2C_ADDR_MSK	0x0fff


/**
 * I2C transfert chunk.
 */
struct bitters_i2c_transfert {
    uint8_t *buf;		/**< buffer or NULL 		*/
    size_t   len;		/**< buffer size		*/
    uint8_t  direction;		/**< direction (read or write)	*/
};
#define BITTERS_I2C_TRANSFERT_READ	0
#define BITTERS_I2C_TRANSFERT_WRITE	1


int bitters_i2c_init(void);

/**
 * Enable I2C interface with selected configuration.
 *
 * @param i2c		I2C interface
 * @param cfg		I2C configuration
 * @return < 0 in case of error
 */
int bitters_i2c_enable(bitters_i2c_t *i2c, bitters_i2c_cfg_t *cfg);

/**
 * Disable I2C interface
 *
 * @param i2c		I2C interface
 * @return < 0 in case of error
 */
int bitters_i2c_disable(bitters_i2c_t *i2c);

/**
 * Change speed of I2C bus.
 *
 * @param i2c		I2C interface
 * @param speed		bus speed in Hz
 * @return < 0 in case of error
 */
int bitters_i2c_set_speed(bitters_i2c_t *i2c, uint32_t speed);

/**
 * Change speed of I2C bus.
 *
 * @param i2c		I2C interface
 * @param addr		I2C addressed
 *			(or-ed with BITTERS_I2C_ADDR_10 if necessary)
 * @param xfr		chunk to be transfered
 * @param count		number of transfered chunk
 * @return < 0 in case of error
 */
int bitters_i2c_transfert(bitters_i2c_t *i2c, bitters_i2c_addr_t addr,
	const struct bitters_i2c_transfert *xfr, unsigned int count);


/** @} */

#endif
