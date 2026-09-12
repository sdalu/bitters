/* F6 safety net: the pins lock must not deadlock against controller
 * teardown. Exercises, concurrently:
 *   - callbacks that reconfigure a sibling pin (pins lock from irq thread)
 *   - main thread enable/disable churn on the same pins
 *   - full release of every pin, so refcount hits 0 and the destroying
 *     thread joins the irq thread while a callback may want the pins lock
 * exit 0 = survived, 43 = deadlock (watchdog), 139 = crash. */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"
static bitters_gpio_pin_t A, B;
static bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                                  .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
static volatile int stop = 0;
static volatile unsigned cbs = 0, cycles = 0;
static void cb_b(bitters_gpio_pin_t *p, void *a) { (void)p; (void)a; cbs++; }
static void cb_a(bitters_gpio_pin_t *p, void *a) {   /* reconfigure sibling */
    (void)p; (void)a; cbs++;
    bitters_gpio_pin_disable(&B);
    if (bitters_gpio_pin_enable(&B, &cfg) == 0)
        bitters_gpio_irq_callback(&B, cb_b, NULL);
}
static void *edges(void *x) {
    (void)x; char c[160]; int v = 0;
    while (!stop) {
        for (int l = 0; l < 2; l++) {
            snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/%d",
                     v, getenv("BH_CHIP"), l);
            if (system(c)) { }
        }
        v ^= 1; usleep(15000);
    }
    return NULL;
}
static void *watchdog(void *x) {
    (void)x; sleep(30);
    printf("WATCHDOG: no progress -> DEADLOCK (cycles=%u cbs=%u)\n", cycles, cbs);
    fflush(stdout); _exit(43);
}
int main(void) {
    pthread_t te, tw;
    if (bitters_gpio_init() < 0) return 1;
    pthread_create(&tw, NULL, watchdog, NULL);
    pthread_create(&te, NULL, edges, NULL);
    for (int i = 0; i < 60; i++) {
        BITTERS_GPIO_PIN_INIT(&A, getenv("BH_CHIP"), 0);
        BITTERS_GPIO_PIN_INIT(&B, getenv("BH_CHIP"), 1);
        bitters_gpio_pin_enable(&A, &cfg);
        bitters_gpio_pin_enable(&B, &cfg);
        bitters_gpio_irq_callback(&A, cb_a, NULL);
        bitters_gpio_irq_callback(&B, cb_b, NULL);
        usleep(40000);
        /* release everything: refcount -> 0 -> destroy -> join irq thread,
         * possibly while that thread is inside cb_a wanting the pins lock */
        bitters_gpio_pin_disable(&A);
        bitters_gpio_pin_disable(&B);
        cycles++;
    }
    stop = 1; pthread_join(te, NULL);
    printf("survived %u enable/teardown cycles, %u callbacks\n", cycles, cbs);
    return 0;
}
