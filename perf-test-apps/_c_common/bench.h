/*
 * Shared helpers for the C demos.
 *
 * Kept header-only so each demo is a single self-contained translation unit
 * and can be compiled with one cc invocation.
 */
#ifndef PERF_BENCH_H
#define PERF_BENCH_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>

/*
 * Helpers below are marked unused: a one-shot demo uses only the timing helpers
 * and a looping demo only the duration/signal ones, so -Wall -Wextra would
 * otherwise warn about whichever half a given demo does not call.
 */
#define BENCH_UNUSED __attribute__((unused))

/* Set by SIGINT/SIGTERM so long-running demos can stop cleanly. */
BENCH_UNUSED static volatile sig_atomic_t bench_stop = 0;

BENCH_UNUSED static void bench_on_signal(int sig) { (void)sig; bench_stop = 1; }

BENCH_UNUSED static void bench_install_signals(void)
{
    signal(SIGINT, bench_on_signal);
    signal(SIGTERM, bench_on_signal);
}

/* Monotonic seconds. CLOCK_MONOTONIC is immune to wall-clock adjustments. */
BENCH_UNUSED static double bench_now(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

/* Duration for the looping demos, overridable from the environment. */
BENCH_UNUSED static int bench_duration_sec(void)
{
    const char *s = getenv("DURATION_SEC");
    if (!s || !*s) return 300;
    int v = atoi(s);
    return v > 0 ? v : 300;
}

/*
 * Keep a value "used" so the optimizer cannot delete the work that produced it.
 * Without this, -O2 happily removes an entire benchmark loop.
 */
#define BENCH_CONSUME(x)                                   \
    do {                                                   \
        __asm__ __volatile__("" :: "r"(x) : "memory");     \
    } while (0)

/* Cache line size on x86-64 and arm64. */
#define CACHE_LINE 64

#endif /* PERF_BENCH_H */
