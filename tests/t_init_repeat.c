/* Finding 1: bitters_gpio_init() must be safe to call more than once.
 * Built WITHOUT -DNDEBUG, which is what a user gets by default. */
#include <stdio.h>
#include <signal.h>
#include "bitters.h"
#include "bitters/gpio.h"
static void foreign(int s) { (void)s; }
int main(int argc, char **argv) {
    (void)argv;
    int third_party = (argc > 1);
    if (third_party) {
        struct sigaction sa = { .sa_handler = foreign };
        sigaction(SIGUSR1, &sa, NULL);
        printf("[third-party SIGUSR1 handler installed first]\n");
    }
    int a = bitters_gpio_init(); printf("init #1 = %d\n", a); fflush(stdout);
    int b = bitters_gpio_init(); printf("init #2 = %d\n", b); fflush(stdout);
    int c = bitters_gpio_init(); printf("init #3 = %d\n", c); fflush(stdout);
    if (third_party) return (a == -16) ? 0 : 1;      /* -EBUSY expected */
    return (a == 0 && b == 0 && c == 0) ? 0 : 1;
}
