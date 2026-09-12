/* Two controllers released from each other's interrupt callback must not
 * deadlock. An irq thread that joins another irq thread can be joined by
 * it in turn. Needs TWO mockup chips: BH_CHIP and BH_CHIP2.
 * exit 0 = survived, 44 = deadlock (watchdog), 139 = crash. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <pthread.h>
#include "bitters.h"
#include "bitters/gpio.h"

static bitters_gpio_pin_t A, B;          /* A on chip 1, B on chip 2 */
static bitters_gpio_cfg_t cfg = { .dir = BITTERS_GPIO_DIR_INPUT,
                                  .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
static volatile int done_a = 0, done_b = 0;

static void drive(const char *chip, int line, int v) {
    char c[192];
    snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/%d", v, chip, line);
    if (system(c)) { }
}
/* Each callback releases the *other* chip's only pin, so each irq thread
 * tries to tear down the controller the other irq thread is running. */
static void cb_a(bitters_gpio_pin_t *p, void *x) {
    (void)p; (void)x;
    if (done_a) return;
    done_a = 1;
    usleep(60000);                       /* let the sibling callback start */
    bitters_gpio_pin_disable(&B);        /* last pin of chip 2 */
}
static void cb_b(bitters_gpio_pin_t *p, void *x) {
    (void)p; (void)x;
    if (done_b) return;
    done_b = 1;
    usleep(60000);
    bitters_gpio_pin_disable(&A);        /* last pin of chip 1 */
}
/* A deadlocked teardown never reaches _bitters_gpio_ctrl_free(), so the
 * chip descriptor stays open: that is the observable. The main thread
 * itself never blocks here, which is why the watchdog alone misses it. */
static int chipfds(void) {
    DIR *d = opendir("/proc/self/fd"); struct dirent *e; int n = 0;
    char p[300], t[256];
    while ((e = readdir(d))) {
        snprintf(p, sizeof p, "/proc/self/fd/%s", e->d_name);
        ssize_t r = readlink(p, t, sizeof t - 1); if (r < 0) continue; t[r] = 0;
        if (strstr(t, "/dev/gpiochip")) n++;
    }
    closedir(d); return n;
}
static void *watchdog(void *x) {
    (void)x; sleep(20);
    printf("WATCHDOG: two irq threads joining each other -> DEADLOCK\n");
    fflush(stdout); _exit(44);
}
int main(void) {
    pthread_t wd;
    const char *c1 = getenv("BH_CHIP"), *c2 = getenv("BH_CHIP2");
    /* getenv() yields "" for a set-but-empty variable, which the suite
     * passes when only one mockup chip exists: treat it as absent. */
    if (!c1 || !*c1 || !c2 || !*c2) {
        printf("need BH_CHIP and BH_CHIP2 (two mockup chips)\n"); return 77; }
    pthread_create(&wd, NULL, watchdog, NULL);
    /* Put both lines in a known state BEFORE registering: a previous run
     * may have left them high, and the reset edge would otherwise fire a
     * callback during setup and tear one chip down on its own. */
    drive(c1, 0, 0); drive(c2, 0, 0); usleep(200000);

    BITTERS_GPIO_PIN_INIT(&A, c1, 0);
    BITTERS_GPIO_PIN_INIT(&B, c2, 0);
    if (bitters_gpio_init() < 0) return 1;
    if (bitters_gpio_pin_enable(&A, &cfg) < 0) { printf("enable A failed\n"); return 1; }
    if (bitters_gpio_pin_enable(&B, &cfg) < 0) { printf("enable B failed\n"); return 1; }
    bitters_gpio_irq_callback(&A, cb_a, NULL);
    bitters_gpio_irq_callback(&B, cb_b, NULL);
    usleep(200000);
    drive(c1, 0, 1); drive(c2, 0, 1);   /* fire both callbacks together */
    usleep(2000000);
    int leaked = chipfds();
    printf("after cross teardown: %d gpiochip fd(s) still open (want 0) -- %s\n",
           leaked, leaked ? "DEADLOCKED, controllers never released" : "both released");
    return leaked ? 44 : 0;
}
