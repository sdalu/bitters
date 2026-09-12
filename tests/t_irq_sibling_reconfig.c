/* Finding 2: a callback that reconfigures a sibling pin must not leave the
 * irq thread blocked in read() on a stale revents, holding irq_lock.
 * exit 42 = hung (bug present); exit 0 = ok. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"
static bitters_gpio_pin_t A, B;
static bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                                  .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
static volatile int a_hits = 0, b_hits = 0, done_reconf = 0;
static void drive(int line, int v) {
    char c[160];
    snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/%d", v, getenv("BH_CHIP"), line);
    if (system(c)) { }
}
static void cb_b(bitters_gpio_pin_t *p, void *a) { (void)p; (void)a; b_hits++; }
static void cb_a(bitters_gpio_pin_t *p, void *a) {
    (void)p; (void)a; a_hits++;
    if (a_hits == 1) { usleep(400000); }                  /* let both lines queue */
    else if (a_hits == 2 && !done_reconf) {               /* reconfigure sibling B */
        done_reconf = 1;
        bitters_gpio_pin_disable(&B);
        bitters_gpio_pin_enable(&B, &cfg);
        bitters_gpio_irq_callback(&B, cb_b, NULL);
    }
}
static void *watchdog(void *a) {
    (void)a; sleep(12);
    printf("WATCHDOG: irq_lock still held after 12s -> blocked in read()\n");
    fflush(stdout); _exit(42);
}
int main(void) {
    pthread_t wd; pthread_create(&wd, NULL, watchdog, NULL);
    BITTERS_GPIO_PIN_INIT(&A, getenv("BH_CHIP"), 0);
    BITTERS_GPIO_PIN_INIT(&B, getenv("BH_CHIP"), 1);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&A, &cfg) < 0) return 1;
    if (bitters_gpio_pin_enable(&B, &cfg) < 0) return 1;
    bitters_gpio_irq_callback(&A, cb_a, NULL);
    bitters_gpio_irq_callback(&B, cb_b, NULL);
    drive(0,0); drive(1,0); usleep(300000);   /* known-good start state */
    drive(0,1);                                /* edge on A -> cb_a #1 holds */
    usleep(100000);
    drive(0,0); drive(1,1);                    /* queue 2nd A edge + 1st B edge */
    usleep(800000);
    int rc = bitters_gpio_irq_callback(&A, cb_a, NULL);  /* needs irq_lock */
    printf("reached end: irq_callback=%d a_hits=%d b_hits=%d\n", rc, a_hits, b_hits);
    return 0;
}
