/* Regression guard: ordinary edge delivery must keep working. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bitters.h"
#include "bitters/gpio.h"
static volatile int hits = 0;
static void cb(bitters_gpio_pin_t *p, void *a) { (void)p; (void)a; hits++; }
static void drive(int line, int v) {
    char c[160];
    snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/%d", v, getenv("BH_CHIP"), line);
    if (system(c)) { }
}
int main(void) {
    bitters_gpio_pin_t A;
    bitters_gpio_cfg_t cfg = { .label = "bh-A", .dir = BITTERS_GPIO_DIR_INPUT,
                               .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    BITTERS_GPIO_PIN_INIT(&A, getenv("BH_CHIP"), 0);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&A, &cfg) < 0) return 1;
    if (bitters_gpio_irq_callback(&A, cb, NULL) < 0) return 1;
    drive(0, 0); usleep(300000); hits = 0;
    for (int i = 0; i < 3; i++) { drive(0,1); usleep(150000); drive(0,0); usleep(150000); }
    usleep(300000);
    printf("edges delivered = %d (expect 6)\n", hits);
    bitters_gpio_pin_disable(&A);
    return hits == 6 ? 0 : 1;
}
