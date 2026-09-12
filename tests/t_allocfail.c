/* Enable a pin with allocation N forced to fail; report the error and
 * whether any descriptor leaked. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include "bitters.h"
#include "bitters/gpio.h"
static int fdcount(void){
    DIR *d = opendir("/proc/self/fd"); struct dirent *e; int n = 0;
    char p[256], t[256];
    while ((e = readdir(d))) {
        snprintf(p,sizeof p,"/proc/self/fd/%s", e->d_name);
        ssize_t r = readlink(p,t,sizeof t-1); if (r<0) continue; t[r]=0;
        if (strstr(t,"/dev/gpiochip") || strstr(t,"gpio-line") || strstr(t,"gpio-event")) n++;
    }
    closedir(d); return n;
}
int main(void){
    bitters_gpio_pin_t P;
    bitters_gpio_cfg_t cfg = { .dir=BITTERS_GPIO_DIR_INPUT,
                               .interrupt=BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    BITTERS_GPIO_PIN_INIT(&P, getenv("BH_CHIP"), 0);
    if (bitters_gpio_init() < 0) return 1;
    int before = fdcount();
    int rc = bitters_gpio_pin_enable(&P, &cfg);
    if (rc == 0) bitters_gpio_pin_disable(&P);
    int after = fdcount();
    printf("enable=%-4d (%-24s) gpio fds before=%d after=%d %s\n",
           rc, rc ? strerror(-rc) : "ok", before, after,
           (after > before) ? "LEAK" : "clean");
    return (after > before) ? 11 : 0;
}
