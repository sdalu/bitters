/* cfg.active_low: with it set, what the API reads and writes, the default
 * value at enable and the edges it reports are all logical, and the line
 * itself is the inverse. Guards against the flag being dropped, or applied
 * to one direction only -- on an output driving a relay the wrong sense is
 * the wrong state on the hardware. Uses a gpio-mockup chip, whose debugfs
 * shows the physical line. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/gpio.h>
#include "bitters.h"
#include "bitters/gpio.h"

static int bad = 0;

static void drive(int line, int v) {    /* the physical line, from debugfs */
    char c[192];
    snprintf(c, sizeof c, "echo %d > /sys/kernel/debug/gpio-mockup/%s/%d",
             v, getenv("BH_CHIP"), line);
    if (system(c)) { }
    usleep(50000);
}
static int observe(int line) {          /* what the mockup chip actually holds */
    char p[192], buf[16]; FILE *f; int v = -1;
    snprintf(p, sizeof p, "/sys/kernel/debug/gpio-mockup/%s/%d", getenv("BH_CHIP"), line);
    if ((f = fopen(p, "r")) == NULL) return -1;
    if (fgets(buf, sizeof buf, f)) v = atoi(buf);
    fclose(f); return v;
}
static int rd(bitters_gpio_pin_t *pin) {
    int v = -1;
    return bitters_gpio_pin_read(pin, &v) == 0 ? v : -1;
}
static void expect(const char *what, int got, int want) {
    printf("  %-46s got %d, want %d %s\n", what, got, want,
           got == want ? "" : " <-- MISMATCH");
    if (got != want) bad = 1;
}
int main(void) {
    bitters_gpio_pin_t in, out, irq;
    bitters_gpio_cfg_t cin  = { .label = "bh-in",  .dir = BITTERS_GPIO_DIR_INPUT,
                                .active_low = 1 };
    bitters_gpio_cfg_t cout = { .label = "bh-out", .dir = BITTERS_GPIO_DIR_OUTPUT,
                                .active_low = 1, .defval = 1 };
    bitters_gpio_cfg_t cirq = { .label = "bh-irq", .dir = BITTERS_GPIO_DIR_INPUT,
                                .active_low = 1,
                                .interrupt = BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    const char *chip = getenv("BH_CHIP");
    if (!chip || !*chip) { printf("need BH_CHIP\n"); return 77; }
    if (bitters_gpio_init() < 0) return 1;

    /* ---- input: the line's level, read through the API, is inverted ---- */
    drive(0, 1);
    BITTERS_GPIO_PIN_INIT(&in, chip, 0);
    if (bitters_gpio_pin_enable(&in, &cin) < 0) { printf("enable input failed\n"); return 1; }
    expect("line held high reads 0", rd(&in), 0);
    drive(0, 0);
    expect("line held low reads 1", rd(&in), 1);

    /* ---- output: the value written is the inverse of the line ---- */
    BITTERS_GPIO_PIN_INIT(&out, chip, 1);
    if (bitters_gpio_pin_enable(&out, &cout) < 0) { printf("enable output failed\n"); return 1; }
    expect("defval=1 drives the line low", observe(1), 0);
    expect("pin_write(0) return code", bitters_gpio_pin_write(&out, 0), 0);
    expect("line after pin_write(0) is high", observe(1), 1);
    expect("pin_write(1) return code", bitters_gpio_pin_write(&out, 1), 0);
    expect("line after pin_write(1) is low", observe(1), 0);
    expect("reading back our own output says 1", rd(&out), 1);

    /* ---- edges: reported in the logical sense ---- */
    drive(2, 0);
    BITTERS_GPIO_PIN_INIT(&irq, chip, 2);
    if (bitters_gpio_pin_enable(&irq, &cirq) < 0) { printf("enable irq failed\n"); return 1; }
    drive(2, 1);
    expect("line going high reported as a falling edge",
           bitters_gpio_irq_wait(&irq), GPIO_V2_LINE_EVENT_FALLING_EDGE);
    drive(2, 0);
    expect("line going low reported as a rising edge",
           bitters_gpio_irq_wait(&irq), GPIO_V2_LINE_EVENT_RISING_EDGE);

    bitters_gpio_pin_disable(&in);
    bitters_gpio_pin_disable(&out);
    bitters_gpio_pin_disable(&irq);
    printf("active low: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
