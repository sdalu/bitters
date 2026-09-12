/* bitters_delay_*() must not be cut short by an ordinary signal, while
 * leaving the synchronous fault signals deliverable (blocking those is
 * undefined if one is raised, and would cost the app its own handler).
 * Needs no privilege and no GPIO. */
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include "bitters/delay.h"

static volatile sig_atomic_t hits = 0;
static void onsig(int s) { (void)s; hits++; }

static double ms_since(struct timespec a) {
    struct timespec b; clock_gettime(CLOCK_MONOTONIC, &b);
    return (b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6;
}
int main(void) {
    struct sigaction sa = { .sa_handler = onsig };
    struct timespec t0;
    int bad = 0;

    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);

    /* an ordinary signal must not shorten the delay */
    struct itimerval it = { .it_value = { .tv_sec = 0, .tv_usec = 100000 } };
    setitimer(ITIMER_REAL, &it, NULL);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    bitters_delay_msec(400);
    double took = ms_since(t0);
    printf("  delay_msec(400) with SIGALRM at 100ms: took %.0f ms, handler ran %d time(s)\n",
           took, (int)hits);
    if (took < 380) { printf("  -> delay was cut short\n"); bad = 1; }
    if (hits == 0)  { printf("  -> signal was lost, not merely deferred\n"); bad = 1; }

    /* the caller's mask must come back exactly as it was: block one
     * signal, leave another unblocked, and check both across a delay */
    sigset_t want, before, after;
    sigemptyset(&want); sigaddset(&want, SIGUSR1);
    pthread_sigmask(SIG_BLOCK, &want, NULL);
    pthread_sigmask(SIG_SETMASK, NULL, &before);
    bitters_delay_usec(1000);
    pthread_sigmask(SIG_SETMASK, NULL, &after);
    int ok_blocked   =  sigismember(&after, SIGUSR1);
    int ok_unblocked = !sigismember(&after, SIGUSR2);
    printf("  mask across a delay: SIGUSR1 still blocked=%s, SIGUSR2 still unblocked=%s\n",
           ok_blocked ? "yes" : "NO", ok_unblocked ? "yes" : "NO");
    if (!ok_blocked || !ok_unblocked) bad = 1;
    (void)before;

    printf("delay: %s\n", bad ? "FAILED" : "ok");
    return bad;
}
