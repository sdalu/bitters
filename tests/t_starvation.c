/* A busy low-numbered line must not starve a higher-numbered one.
 * Line 0 is driven as fast as possible; line 5 slowly. Both have
 * callbacks. exit 0 = line 5 serviced, 13 = starved. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"
static bitters_gpio_pin_t A, B;
static bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                                  .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
static volatile int stop = 0;
static volatile unsigned hit_a = 0, hit_b = 0;
static void cb_a(bitters_gpio_pin_t *p, void *x){ (void)p;(void)x; hit_a++; }
static void cb_b(bitters_gpio_pin_t *p, void *x){ (void)p;(void)x; hit_b++; }
static int openline(int l){
    char p[160];
    snprintf(p,sizeof p,"/sys/kernel/debug/gpio-mockup/%s/%d", getenv("BH_CHIP"), l);
    return open(p, O_WRONLY);
}
static void *fast(void *x){            /* hammer line 0 */
    (void)x; int fd = openline(0); int v = 0;
    while (!stop) { char c = v ? '1' : '0'; v ^= 1;
                    if (write(fd,&c,1) < 0) {} }
    close(fd); return NULL;
}
static void *slow(void *x){            /* tickle line 5 */
    (void)x; int fd = openline(5); int v = 0;
    while (!stop) { char c = v ? '1' : '0'; v ^= 1;
                    if (write(fd,&c,1) < 0) {} usleep(100000); }
    close(fd); return NULL;
}
int main(void){
    pthread_t tf, ts;
    BITTERS_GPIO_PIN_INIT(&A, getenv("BH_CHIP"), 0);
    BITTERS_GPIO_PIN_INIT(&B, getenv("BH_CHIP"), 5);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&A,&cfg) < 0 || bitters_gpio_pin_enable(&B,&cfg) < 0) return 1;
    bitters_gpio_irq_callback(&A, cb_a, NULL);
    bitters_gpio_irq_callback(&B, cb_b, NULL);
    pthread_create(&tf,NULL,fast,NULL); pthread_create(&ts,NULL,slow,NULL);
    sleep(4);
    stop = 1; pthread_join(tf,NULL); pthread_join(ts,NULL);
    printf("line0 callbacks=%-8u  line5 callbacks=%-4u  %s\n",
           hit_a, hit_b, hit_b ? "line5 serviced" : "line5 STARVED");
    bitters_gpio_pin_disable(&A); bitters_gpio_pin_disable(&B);
    return hit_b ? 0 : 13;
}
