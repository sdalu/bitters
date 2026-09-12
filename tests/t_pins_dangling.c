/* Finding 4: after pin_disable(), the controller table must not keep a
 * pointer to the released pin. White-box: includes gpio.c for the private
 * struct. exit 0 = clean, 8 = dangling. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../src/gpio.c"   /* white-box: needs the private controller struct */
static void cb(bitters_gpio_pin_t *p, void *a) { (void)p; (void)a; }
int main(void) {
    bitters_gpio_pin_t P, Q;
    bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                               .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    const char *chip = getenv("BH_CHIP");
    BITTERS_GPIO_PIN_INIT(&P, chip, 0);
    BITTERS_GPIO_PIN_INIT(&Q, chip, 1);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&P, &cfg) < 0) return 1;
    if (bitters_gpio_pin_enable(&Q, &cfg) < 0) return 1;   /* keeps ctrl alive */
    if (bitters_gpio_irq_callback(&P, cb, NULL) < 0) return 1;
    struct bitters_gpio_ctrl *c = P.ctrl;
    bitters_gpio_irq_callback(&P, NULL, NULL);   /* documented unregister */
    bitters_gpio_pin_disable(&P);                /* pin released */
    printf("ctrl->pins[0] after disable = %s (expect NULL)\n",
           c->pins[0] == NULL ? "NULL" : "&P  <-- DANGLING");
    int bad = (c->pins[0] != NULL);
    bitters_gpio_pin_disable(&Q);
    return bad ? 8 : 0;
}
