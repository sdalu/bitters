/* I2C argument validation and error paths. Touches no I2C bus. */
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "bitters/i2c.h"

static int bad = 0;
static void expect(const char *what, int got, int want) {
    printf("  %-46s %-5d (want %d)%s\n", what, got, want,
           got == want ? "" : "  <-- MISMATCH");
    if (got != want) bad = 1;
}
int main(void) {
    bitters_i2c_t i2c = BITTERS_I2C_INITIALIZER(0);       /* never enabled: fd = -1 */
    uint8_t buf[4] = { 0 };
    struct bitters_i2c_transfer xfr[I2C_RDWR_IOCTL_MAX_MSGS + 2];
    for (unsigned i = 0; i < sizeof xfr / sizeof xfr[0]; i++) {
        xfr[i].buf = buf; xfr[i].len = sizeof buf;
        xfr[i].dir = BITTERS_I2C_TRANSFER_WRITE;
    }
    const bitters_i2c_addr_t addr = 0x42;

    expect("transfer(count=0)", bitters_i2c_transfer(&i2c, addr, xfr, 0), 0);
    expect("transfer(count > I2C_RDWR_IOCTL_MAX_MSGS)",
           bitters_i2c_transfer(&i2c, addr, xfr, I2C_RDWR_IOCTL_MAX_MSGS + 1), -EINVAL);
    /* the maximum must pass validation and only then fail on the closed fd */
    expect("transfer(count = the maximum) reaches the ioctl",
           bitters_i2c_transfer(&i2c, addr, xfr, I2C_RDWR_IOCTL_MAX_MSGS), -EBADF);

    /* i2c_msg.len is a 16-bit field */
    struct bitters_i2c_transfer big = { .buf = buf, .len = (size_t)UINT16_MAX + 1,
                                        .dir = BITTERS_I2C_TRANSFER_WRITE };
    expect("transfer with len > UINT16_MAX rejected",
           bitters_i2c_transfer(&i2c, addr, &big, 1), -EINVAL);
    struct bitters_i2c_transfer edge = { .buf = buf, .len = UINT16_MAX,
                                         .dir = BITTERS_I2C_TRANSFER_WRITE };
    expect("len == UINT16_MAX is allowed through",
           bitters_i2c_transfer(&i2c, addr, &edge, 1), -EBADF);

    /* direction must be exactly one of the two */
    struct bitters_i2c_transfer d = { .buf = buf, .len = 1 };
    d.dir = 0;
    expect("dir = neither rejected", bitters_i2c_transfer(&i2c, addr, &d, 1), -EINVAL);
    d.dir = BITTERS_I2C_TRANSFER_READ | BITTERS_I2C_TRANSFER_WRITE;
    expect("dir = both rejected",    bitters_i2c_transfer(&i2c, addr, &d, 1), -EINVAL);
    d.dir = BITTERS_I2C_TRANSFER_READ;
    expect("dir = read reaches the ioctl", bitters_i2c_transfer(&i2c, addr, &d, 1), -EBADF);

    expect("set_speed() is unsupported on Linux", bitters_i2c_set_speed(&i2c, 100000), -ENOSYS);

    bitters_i2c_t nodev = BITTERS_I2C_INITIALIZER(99);
    bitters_i2c_cfg_t cfg = { .speed = 0 };
    expect("enable(/dev/i2c-99)", bitters_i2c_enable(&nodev, &cfg), -ENOENT);
    expect("  fd left closed after a failed enable", nodev.fd, -1);

    bitters_i2c_t huge = BITTERS_I2C_INITIALIZER(99999);
    expect("enable() with a path that would truncate",
           bitters_i2c_enable(&huge, &cfg), -ENOMEM);

    expect("disable() of a never-enabled interface", bitters_i2c_disable(&i2c), 0);

    printf("i2c arguments: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
