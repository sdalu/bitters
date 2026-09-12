/* The documented poll-it-yourself path: a pin with interrupts enabled but
 * NO callback registered must keep blocking semantics in bitters_gpio_irq_wait()
 * and in bitters_gpio_irq_fill_pollfd(). This is what the O_NONBLOCK toggle
 * added for callback pins must not leak into.
 * exit 0 = ok, 46 = returned early (non-blocking leaked), 47 = wrong result. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"

static bitters_gpio_pin_t P;
static volatile int returned = 0, result = 0;

static void drive(int v) {
    char c[192];
    snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/0", v, getenv("BH_CHIP"));
    if (system(c)) { }
}
static void *waiter(void *x) {
    (void)x;
    result   = bitters_gpio_irq_wait(&P);   /* must BLOCK until an edge */
    returned = 1;
    return NULL;
}
int main(void) {
    pthread_t t;
    bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                               .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    drive(0); usleep(200000);
    BITTERS_GPIO_PIN_INIT(&P, getenv("BH_CHIP"), 0);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&P, &cfg) < 0) { printf("enable failed\n"); return 1; }

    /* no bitters_gpio_irq_callback() here -- this is the polling API */
    int fl = fcntl(P.fd, F_GETFL, 0);
    printf("  descriptor is %s (want blocking)\n",
           (fl & O_NONBLOCK) ? "NON-BLOCKING" : "blocking");
    if (fl & O_NONBLOCK) return 46;

    struct pollfd pfd;
    if (bitters_gpio_irq_fill_pollfd(&P, &pfd) < 0) { printf("fill_pollfd failed\n"); return 1; }
    if (pfd.fd != P.fd) { printf("fill_pollfd gave the wrong fd\n"); return 47; }
    if (poll(&pfd, 1, 300) != 0) { printf("poll ready with no edge\n"); return 47; }

    pthread_create(&t, NULL, waiter, NULL);
    usleep(500000);
    if (returned) { printf("  irq_wait() returned before any edge -> non-blocking leaked\n"); return 46; }
    drive(1);                                   /* now give it an edge */
    usleep(500000);
    if (!returned) { printf("  irq_wait() did not wake on the edge\n"); return 47; }
    pthread_join(t, NULL);
    printf("  irq_wait() blocked, then returned %d (%s)\n",
           result, result > 0 ? "event id" : strerror(-result));
    bitters_gpio_pin_disable(&P);
    return result > 0 ? 0 : 47;
}
