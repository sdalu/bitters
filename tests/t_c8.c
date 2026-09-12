/* C8: when the non-blocking toggle cannot be applied, registering a
 * callback must fail rather than silently install one on a blocking
 * descriptor. Unregistering must still succeed. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bitters.h"
#include "bitters/gpio.h"
static void cb(bitters_gpio_pin_t *p, void *a) { (void)p; (void)a; }
int main(void) {
    bitters_gpio_pin_t P;
    bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                               .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    BITTERS_GPIO_PIN_INIT(&P, getenv("BH_CHIP"), 0);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&P, &cfg) < 0) { printf("enable failed\n"); return 1; }
    int reg   = bitters_gpio_irq_callback(&P, cb, NULL);
    int unreg = bitters_gpio_irq_callback(&P, NULL, NULL);
    printf("register=%d (%s)  unregister=%d\n",
           reg, reg ? strerror(-reg) : "ok", unreg);
    bitters_gpio_pin_disable(&P);
    return (reg == 0) ? 0 : 10;    /* 0 = registration accepted, 10 = refused */
}
