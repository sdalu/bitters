/*
 * Copyright (c) 2019-2020,2024
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
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#include "bitters.h"
#include "bitters/i2c.h"



/*== Log & Assert helpers ==============================================*/

#if defined(BITTERS_I2C_WITH_LOG)
#define BITTERS_I2C_LOG(x, ...)						\
    BITTERS_LOG("i2c: " x, ##__VA_ARGS__)
#else
#define BITTERS_I2C_LOG(x, ...)
#endif

#if defined(BITTERS_I2C_WITH_ASSERT)
#define BITTERS_I2C_ASSERT(x)						\
    BITTERS_ASSERT(x)
#else
#define BITTERS_I2C_ASSERT(x)
#endif

#define BITTERS_I2C_ASSERT_DEVID(x)					\
    BITTERS_I2C_ASSERT(((x) >= 0) && ((x) < 256))




/*== Internal functions ================================================*/

static void
_bitters_i2c_warn_about_hardware_config(void) {
    static int once = 0;
    if (once++                                          ||
	(getenv("BITTERS_SILENCE_WARNING"    ) != NULL) ||
	(getenv("BITTERS_SILENCE_RPI_WARNING") != NULL)) return;

    fprintf(stderr,
	"\n"
	"bitters: It is not possible to configure I2C speed dynamically,\n"
	"       | you can perform the following at boot time\n"
        "       |   * config.txt (add: i2c_arm_baudrate=xxx)\n"
	"       |   * modprobe for i2c driver (set: baudrate=xxx)\n"
	"       |   * device-tree with clock-frequency (see: brcm,bcm2835-i2c.txt)\n"
	"\n");
}

#if !defined(BITTERS_SILENCE_WARNING    ) &&				\
    !defined(BITTERS_SILENCE_RPI_WARNING)
#  define BITTERS_I2C_WARN_ABOUT_HARDWARE_CONFIG()			\
    _bitters_i2c_warn_about_hardware_config()
#else
#  define BITTERS_I2C_WARN_ABOUT_HARDWARE_CONFIG()
#endif



/*== Exported function =================================================*/

int
bitters_i2c_init(void)
{
    return 0;
}



int
bitters_i2c_enable(bitters_i2c_t *i2c, bitters_i2c_cfg_t *cfg)
{
    int rc = -EINVAL;

    BITTERS_I2C_ASSERT_DEVID(i2c->id);

    /* Already enabled */
    if (i2c->fd >= 0)
	return 0;

    /* Enable
     */
    char path[13]; /* Enough room for: /dev/i2c-000 */
    rc = snprintf(path, sizeof(path), "/dev/i2c-%d", i2c->id);
    if ((rc < 0) || (rc >= sizeof(path))) {
	rc = -ENOMEM;
	BITTERS_I2C_LOG("failed to build path for i2cdev");
	goto failed;
    }

    i2c->fd = open(path, O_RDWR);
    if (i2c->fd < 0) {
	rc = -errno;
	BITTERS_I2C_LOG("can't open i2c device %s (%s)",
			path, strerror(errno));
	goto failed;
    }

    /* Check functionalities
     * We need I2C_FUNC_I2C as we will do plain I2C transaction
     */
    rc = ioctl(i2c->fd, I2C_FUNCS, &i2c->funcs);
    if (rc < 0) {
	rc = -errno;
	BITTERS_I2C_LOG("can't get information about i2c device %s (%s)",
			path, strerror(errno));
	goto failed;
    }
    BITTERS_I2C_LOG("i2c device %s has functionnalities: 0x%lx",
		    path, i2c->funcs);

    if (! (i2c->funcs & I2C_FUNC_I2C)) {
	rc = -EOPNOTSUPP;
	BITTERS_I2C_LOG("i2c-%d is missing I2C_FUNC_I2C functionality",
			i2c->id);
	goto failed;
    }

    /* Set speed
     */
    if (cfg->speed != 0) {
	BITTERS_I2C_WARN_ABOUT_HARDWARE_CONFIG();
	BITTERS_I2C_LOG("ignoring i2c speed");
    }


    BITTERS_I2C_LOG("I2C device %d enabled (using: %s)", i2c->id, path);

    return 0;

 failed:
    if (i2c->fd >= 0) {
	close(i2c->fd);
	i2c->fd = -1;
    }
    return rc;
}


int
bitters_i2c_disable(bitters_i2c_t *i2c)
{
    /* Already disabled */
    if (i2c->fd < 0)
	return 0;

    /* Disable
     */
    if (close(i2c->fd) < 0) {
	int rc = -errno;
	BITTERS_I2C_LOG("failed to close i2c-%d device (%s)",
			i2c->id, strerror(errno));
	return rc;
    }

    i2c->fd = -1;
    return 0;
}



int
bitters_i2c_set_speed(bitters_i2c_t *i2c, uint32_t speed)
{
    BITTERS_I2C_WARN_ABOUT_HARDWARE_CONFIG();
    return -ENOSYS;
}


int
bitters_i2c_transfer(bitters_i2c_t *i2c, bitters_i2c_addr_t addr,
	const struct bitters_i2c_transfer *xfr, unsigned int count)
{
    struct i2c_msg msg[count];
    struct i2c_rdwr_ioctl_data msgset = {
       .msgs  = msg,
       .nmsgs = count,
    };

    uint16_t i2c_addr  = addr & BITTERS_I2C_ADDR_MSK;
    uint16_t i2c_flags = 0;

    BITTERS_I2C_LOG("transferring %d msg to 0x%02x", msgset.nmsgs, i2c_addr);

    if (addr & BITTERS_I2C_ADDR_10)
	i2c_flags |= I2C_M_TEN;

    for (unsigned int i = 0 ; i < count ; i++) {
	/* Kernel i2c_msg.len is a 16-bit field */
	if (xfr[i].len > UINT16_MAX) {
	    BITTERS_I2C_LOG("chunk %u too large (%zu > %u)",
			    i, xfr[i].len, UINT16_MAX);
	    return -EINVAL;
	}
	memset(&msg[i], 0, sizeof(struct i2c_msg));
	msg[i].addr  = i2c_addr;
	msg[i].flags = i2c_flags;
	switch (xfr[i].dir) {
	case BITTERS_I2C_TRANSFER_WRITE:
	    break;
	case BITTERS_I2C_TRANSFER_READ:
	    msg[i].flags |= I2C_M_RD;
	    break;
	default:
	    return -EINVAL;
	}
	msg[i].len   = xfr[i].len;
	msg[i].buf   = xfr[i].buf;

	BITTERS_I2C_LOG("msg[%u].%c @ %p [len=%zu]",
			i,
			(msg[i].flags & I2C_M_RD) ? 'r' : 'w',
			(void *)xfr[i].buf, xfr[i].len);
    }

    int rc = ioctl(i2c->fd, I2C_RDWR, &msgset);
    if (rc < 0) {
	BITTERS_I2C_LOG("transfer failed (%s)", strerror(errno));
	return -errno;
    }

    return 0;
}
