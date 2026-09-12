/* 400 enable/register/unregister/disable cycles: descriptor and O_NONBLOCK
 * state must return to baseline every time. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "bitters.h"
#include "bitters/gpio.h"
static int fdcount(void){
    DIR *d = opendir("/proc/self/fd"); struct dirent *e; int n=0; char p[256],t[256];
    while ((e = readdir(d))) { snprintf(p,sizeof p,"/proc/self/fd/%s",e->d_name);
        ssize_t r=readlink(p,t,sizeof t-1); if(r<0)continue; t[r]=0;
        if (strstr(t,"/dev/gpiochip")||strstr(t,"gpio-line")||strstr(t,"gpio-event")) n++; }
    closedir(d); return n;
}
static void cb(bitters_gpio_pin_t *p, void *a){ (void)p;(void)a; }
int main(void){
    bitters_gpio_pin_t P; int nb_set=0, nb_clr=0;
    bitters_gpio_cfg_t cfg = { .dir=BITTERS_GPIO_DIR_INPUT,
                               .interrupt=BITTERS_GPIO_INTERRUPT_BOTH_EDGE };
    if (bitters_gpio_init() < 0) return 1;
    int base = fdcount();
    for (int i=0;i<400;i++){
        BITTERS_GPIO_PIN_INIT(&P, getenv("BH_CHIP"), 0);
        if (bitters_gpio_pin_enable(&P,&cfg) < 0) { printf("enable failed at %d\n",i); return 1; }
        bitters_gpio_irq_callback(&P, cb, NULL);
        if (fcntl(P.fd,F_GETFL,0) & O_NONBLOCK) nb_set++;
        bitters_gpio_irq_callback(&P, NULL, NULL);
        if (!(fcntl(P.fd,F_GETFL,0) & O_NONBLOCK)) nb_clr++;
        bitters_gpio_pin_disable(&P);
    }
    int end = fdcount();
    printf("400 cycles: gpio fds %d -> %d %s | O_NONBLOCK set %d/400, cleared %d/400\n",
           base, end, end>base?"LEAK":"clean", nb_set, nb_clr);
    return (end>base || nb_set!=400 || nb_clr!=400) ? 12 : 0;
}
