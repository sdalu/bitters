/* Finding 6: after two threads race on one pin, using that pin must not
 * crash. exit 0 = no crash, 139 = SIGSEGV. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"
static bitters_gpio_pin_t P;
static bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                                  .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
static pthread_barrier_t bar;
static void cb(bitters_gpio_pin_t *p, void *a) { (void)p; (void)a; }
static void *racer(void *x) { (void)x; pthread_barrier_wait(&bar);
    bitters_gpio_pin_enable(&P, &cfg); return NULL; }
int main(void) {
    pthread_t t;
    if (bitters_gpio_init() < 0) return 1;
    pthread_barrier_init(&bar, NULL, 2);
    BITTERS_GPIO_PIN_INIT(&P, getenv("BH_CHIP"), 0);
    pthread_create(&t, NULL, racer, NULL);
    pthread_barrier_wait(&bar);
    bitters_gpio_pin_enable(&P, &cfg);
    pthread_join(t, NULL);
    printf("state after race: fd=%d ctrl=%s flags=0x%02x  invariant=%s\n",
           P.fd, P.ctrl ? "set" : "NULL", P.flags,
           ((P.ctrl != NULL) == (P.fd >= 0)) ? "holds" : "BROKEN");
    fflush(stdout);
    int rc = bitters_gpio_irq_callback(&P, cb, NULL);
    printf("irq_callback() returned %d (no crash)\n", rc);
    return 0;
}
