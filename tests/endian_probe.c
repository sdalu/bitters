#include <stdint.h>
#include <stddef.h>
#include "bitters/i2c.h"
struct bitters_i2c_transfer probe_read  = { .read  = 1 };   /* want dir 0x02 */
struct bitters_i2c_transfer probe_write = { .write = 1 };   /* want dir 0x01 */
