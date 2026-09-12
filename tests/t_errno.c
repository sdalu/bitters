/* Item 3: _bitters_gpio_ctrl_create() cleans up (close/free) between the
 * real failure and the caller reading errno at gpio.c:479.
 * "null" -> /dev/null opens fine, then GPIO_GET_CHIPINFO fails with
 * ENOTTY, so the failure path runs with fd >= 0 and close() is called. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bitters.h"
#include "bitters/gpio.h"
int main(void) {
    bitters_gpio_pin_t P;
    bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT };
    BITTERS_GPIO_PIN_INIT(&P, "null", 0);
    if (bitters_gpio_init() < 0) return 1;
    int rc = bitters_gpio_pin_enable(&P, &cfg);
    printf("enable(/dev/null) = %d (%s)  expected -%d (%s)\n",
           rc, strerror(-rc), ENOTTY, strerror(ENOTTY));
    return (rc == -ENOTTY) ? 0 : 9;
}
