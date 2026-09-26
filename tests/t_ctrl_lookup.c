/* The controller name resolving to a device: a chip label, a list of
 * alternatives tried in order, and a name that is nothing at all. Guards
 * against the Pi 5 defect -- the header bank not being gpiochip0, so a
 * fixed device name drives the wrong controller -- by proving the label
 * route on the mockup chip, whose label the test asks the kernel for.
 * Also pins down that a label and the device name it stands for share one
 * controller, and that the scan behind the lookup leaks no descriptor. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>
#include "bitters.h"
#include "bitters/gpio.h"

static int bad = 0;

static int chip_fds(void) {             /* /dev/gpiochip descriptors held */
    DIR *d = opendir("/proc/self/fd"); struct dirent *e;
    char p[sizeof "/proc/self/fd/" + sizeof e->d_name], t[256]; int n = 0;
    if (!d) return -1;
    while ((e = readdir(d))) {
        snprintf(p, sizeof p, "/proc/self/fd/%s", e->d_name);
        ssize_t l = readlink(p, t, sizeof t - 1);
        if (l < 0) continue;
        t[l] = 0;
        if (strstr(t, "/dev/gpiochip")) n++;
    }
    closedir(d); return n;
}
static void expect(const char *what, int got, int want) {
    printf("  %-46s got %d, want %d %s\n", what, got, want,
           got == want ? "" : " <-- MISMATCH");
    if (got != want) bad = 1;
}
int main(void) {
    const char *chip = getenv("BH_CHIP");
    if (!chip || !*chip) { printf("need BH_CHIP\n"); return 77; }

    /* the label BH_CHIP carries, from the kernel rather than assumed:
     * gpio-mockup calls its chips gpio-mockup-A, gpio-mockup-B, ... */
    char path[64], label[GPIO_MAX_NAME_SIZE + 1], spec[128];
    struct gpiochip_info info;
    snprintf(path, sizeof path, "/dev/%s", chip);
    int fd = open(path, O_RDONLY);
    if (fd < 0 || ioctl(fd, GPIO_GET_CHIPINFO_IOCTL, &info) < 0) {
        printf("cannot read the chip info of %s\n", path); return 1;
    }
    close(fd);
    snprintf(label, sizeof label, "%.*s", (int)sizeof info.label, info.label);
    printf("  %s is labelled \"%s\"\n", chip, label);
    if (!*label) { printf("chip has no label: cannot test\n"); return 1; }

    if (bitters_gpio_init() < 0) return 1;
    bitters_gpio_pin_t by_label, by_name, by_alt, bogus;
    bitters_gpio_cfg_t cfg = { .label = "bh-lookup", .dir = BITTERS_GPIO_DIR_INPUT };
    int before = chip_fds();

    /* ---- the label names the chip ---- */
    BITTERS_GPIO_PIN_INIT(&by_label, label, 0);
    expect("enable through the chip label", bitters_gpio_pin_enable(&by_label, &cfg), 0);
    expect("one controller opened", chip_fds() - before, 1);

    /* ---- the device name and the label are the same controller ---- */
    BITTERS_GPIO_PIN_INIT(&by_name, chip, 1);
    expect("enable through the device name", bitters_gpio_pin_enable(&by_name, &cfg), 0);
    expect("label and device name share the controller", chip_fds() - before, 1);

    /* ---- alternatives: the first that exists wins ---- */
    snprintf(spec, sizeof spec, "no-such-chip|no-such-label||%s", label);
    BITTERS_GPIO_PIN_INIT(&by_alt, spec, 2);
    expect("enable through the first alternative that exists", bitters_gpio_pin_enable(&by_alt, &cfg), 0);
    expect("still one controller", chip_fds() - before, 1);

    /* ---- a name that is nothing ---- */
    BITTERS_GPIO_PIN_INIT(&bogus, "no-such-chip|no-such-label", 3);
    expect("a name that is nothing fails with -ENOENT", bitters_gpio_pin_enable(&bogus, &cfg), -ENOENT);
    expect("and opened nothing (scan leaked no fd)", chip_fds() - before, 1);

    bitters_gpio_pin_disable(&by_label);
    bitters_gpio_pin_disable(&by_name);
    bitters_gpio_pin_disable(&by_alt);
    expect("everything released", chip_fds() - before, 0);

    printf("controller lookup: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
