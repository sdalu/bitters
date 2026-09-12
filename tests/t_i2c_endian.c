/* The read/write bitfields must be an alternate view of `dir`, matching
 * BITTERS_I2C_TRANSFER_{READ,WRITE}, on every ABI. Run natively; for
 * other endiannesses see `make endian` which inspects cross codegen. */
#include <stdio.h>
#include <string.h>
#include "bitters/i2c.h"
int main(void) {
    struct bitters_i2c_transfer x;
    int bad = 0;
    memset(&x, 0, sizeof x); x.read = 1;
    if (x.dir != BITTERS_I2C_TRANSFER_READ) {
        printf(".read=1  -> dir=0x%02x, want 0x%02x  MISMATCH\n",
               x.dir, BITTERS_I2C_TRANSFER_READ); bad = 1;
    }
    memset(&x, 0, sizeof x); x.write = 1;
    if (x.dir != BITTERS_I2C_TRANSFER_WRITE) {
        printf(".write=1 -> dir=0x%02x, want 0x%02x  MISMATCH\n",
               x.dir, BITTERS_I2C_TRANSFER_WRITE); bad = 1;
    }
    printf("i2c dir/bitfield view: %s\n", bad ? "BROKEN" : "consistent");
    return bad;
}
