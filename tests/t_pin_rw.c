/* bitters_gpio_pin_read() / bitters_gpio_pin_write(), which nothing else
 * in the suite exercises. Uses a gpio-mockup chip: an input line is driven
 * from debugfs and read back through the API, an output line is written
 * through the API and read back from debugfs. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "bitters.h"
#include "bitters/gpio.h"

static int bad = 0;

static void drive(int line, int v) {
    char c[192];
    snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/%d",
             v, getenv("BH_CHIP"), line);
    if (system(c)) { }
}
static int observe(int line) {          /* what the mockup chip actually holds */
    char p[192], buf[16]; FILE *f; int v = -1;
    snprintf(p, sizeof p, "/sys/kernel/debug/gpio-mockup/%s/%d", getenv("BH_CHIP"), line);
    if ((f = fopen(p, "r")) == NULL) return -1;
    if (fgets(buf, sizeof buf, f)) v = atoi(buf);
    fclose(f); return v;
}
static void expect(const char *what, int got, int want) {
    printf("  %-38s got %d, want %d %s\n", what, got, want,
           got == want ? "" : " <-- MISMATCH");
    if (got != want) bad = 1;
}
int main(void) {
    bitters_gpio_pin_t in, out;
    bitters_gpio_cfg_t cin  = { .label = "bh-in",  .dir = BITTERS_GPIO_DIR_INPUT };
    bitters_gpio_cfg_t cout = { .label = "bh-out", .dir = BITTERS_GPIO_DIR_OUTPUT,
                                .defval = 1 };
    int v;
    const char *chip = getenv("BH_CHIP");
    if (!chip || !*chip) { printf("need BH_CHIP\n"); return 77; }
    if (bitters_gpio_init() < 0) return 1;

    /* ---- input: drive the line, read it through the API ---- */
    drive(0, 0); usleep(50000);
    BITTERS_GPIO_PIN_INIT(&in, chip, 0);
    if (bitters_gpio_pin_enable(&in, &cin) < 0) { printf("enable input failed\n"); return 1; }
    v = -1; expect("pin_read() of a line driven low",  bitters_gpio_pin_read(&in, &v) == 0 ? v : -1, 0);
    drive(0, 1); usleep(50000);
    v = -1; expect("pin_read() of a line driven high", bitters_gpio_pin_read(&in, &v) == 0 ? v : -1, 1);
    /* the value pointer is documented as optional */
    expect("pin_read(pin, NULL) return code", bitters_gpio_pin_read(&in, NULL), 0);

    /* ---- output: write through the API, observe the line ---- */
    BITTERS_GPIO_PIN_INIT(&out, chip, 1);
    if (bitters_gpio_pin_enable(&out, &cout) < 0) { printf("enable output failed\n"); return 1; }
    expect("defval=1 applied at enable", observe(1), 1);
    expect("pin_write(0) return code", bitters_gpio_pin_write(&out, 0), 0);
    expect("line after pin_write(0)", observe(1), 0);
    expect("pin_write(1) return code", bitters_gpio_pin_write(&out, 1), 0);
    expect("line after pin_write(1)", observe(1), 1);
    /* any non-zero must mean high, not just 1 */
    expect("pin_write(42) treated as high", bitters_gpio_pin_write(&out, 42), 0);
    expect("line after pin_write(42)", observe(1), 1);
    /* read back an output line through the API */
    v = -1; expect("pin_read() of our own output", bitters_gpio_pin_read(&out, &v) == 0 ? v : -1, 1);

    bitters_gpio_pin_disable(&in);
    bitters_gpio_pin_disable(&out);
    printf("pin read/write: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
