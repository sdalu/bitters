/* Finding 6: two threads enabling the SAME pin both see pin->ctrl==NULL and
 * both bump refcount -> controller (chip fd + irq thread) never released,
 * and one line fd is overwritten and leaked.
 * Counts real leaked descriptors; also flags the data race under TSan. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"
#define ITERS 40
static bitters_gpio_pin_t P;
static bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                                  .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
static pthread_barrier_t bar;
static void *racer(void *x) {
    (void)x;
    for (int i = 0; i < ITERS; i++) {
        pthread_barrier_wait(&bar);
        bitters_gpio_pin_enable(&P, &cfg);
        pthread_barrier_wait(&bar);
    }
    return NULL;
}
static void count_fds(int *chip, int *line) {
    DIR *d = opendir("/proc/self/fd"); struct dirent *e;
    char p[256], t[256]; *chip = 0; *line = 0;
    while ((e = readdir(d))) {
        snprintf(p, sizeof p, "/proc/self/fd/%s", e->d_name);
        ssize_t n = readlink(p, t, sizeof t - 1); if (n < 0) continue; t[n] = 0;
        if (strstr(t, "/dev/gpiochip")) (*chip)++;
        else if (strstr(t, "gpio-line") || strstr(t, "gpio-event")) (*line)++;
    }
    closedir(d);
}
int main(void) {
    pthread_t t; int c0, l0, c1, l1;
    if (bitters_gpio_init() < 0) return 1;
    count_fds(&c0, &l0);
    pthread_barrier_init(&bar, NULL, 2);
    pthread_create(&t, NULL, racer, NULL);
    for (int i = 0; i < ITERS; i++) {
        BITTERS_GPIO_PIN_INIT(&P, getenv("BH_CHIP"), 0);
        pthread_barrier_wait(&bar);
        bitters_gpio_pin_enable(&P, &cfg);
        pthread_barrier_wait(&bar);
        bitters_gpio_pin_disable(&P);      /* one disable, as the API expects */
    }
    pthread_join(t, NULL);
    count_fds(&c1, &l1);
    printf("after %d enable-races + %d disables:\n", ITERS, ITERS);
    printf("  /dev/gpiochip fds still open : %d (was %d)  <- controller leak\n", c1, c0);
    printf("  gpio line fds still open     : %d (was %d)  <- leaked line fds\n", l1, l0);
    return (c1 > c0 || l1 > l0) ? 7 : 0;
}
