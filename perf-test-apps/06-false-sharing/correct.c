/*
 * CORRECT: each per-thread counter padded to its own cache line, eliminating false sharing.
 * Resume: "Avoid False Sharing and Cache Line Contention"
 *
 * Same threads, same counters, same total increments. The only change is
 * layout: each counter is _Alignas(CACHE_LINE) and padded to a full 64 bytes,
 * so no two threads' counters can land in the same line. Each core now holds
 * its own line in Modified state permanently and never has to give it up. The
 * increment stays an L1 cache hit and zero coherency traffic crosses the
 * interconnect.
 *
 * The cost is memory: 64 bytes per counter instead of 8. That is the standard
 * trade for any hot, frequently written per-thread or per-core slot.
 *
 * Measure: perf stat -e cache-misses,cache-references ./correct
 *   Compare against ./incorrect: the miss count collapses because the MESI
 *   invalidate/re-fetch ping-pong is gone, not because less work is done.
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4
#define ITERATIONS  (400ULL * 1000 * 1000)

/*
 * The padding is what makes this correct. _Alignas alone starts each element on
 * a line boundary; sizing the struct to a full line keeps the NEXT element from
 * sharing the remainder of this one.
 */
typedef struct {
    _Alignas(CACHE_LINE) volatile unsigned long long value;
    char pad[CACHE_LINE - sizeof(unsigned long long)];
} padded_counter_t;

static padded_counter_t counters[NUM_THREADS];

static void *worker(void *arg)
{
    const int id = *(const int *)arg;
    for (unsigned long long i = 0; i < ITERATIONS; i++) {
        counters[id].value++;
    }
    return NULL;
}

int main(void)
{
    printf("CORRECT: %d threads, each counter on its OWN cache line (padded)\n",
           NUM_THREADS);
    fflush(stdout);

    /* If this ever fails the padding is wrong and the demo is meaningless. */
    if (sizeof(padded_counter_t) != CACHE_LINE) {
        fprintf(stderr, "padding broken: sizeof=%zu\n", sizeof(padded_counter_t));
        return 1;
    }

    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];

    const double t0 = bench_now();

    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker, &ids[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    for (int i = 0; i < NUM_THREADS; i++) pthread_join(threads[i], NULL);

    const double elapsed = bench_now() - t0;

    unsigned long long total = 0;
    for (int i = 0; i < NUM_THREADS; i++) total += counters[i].value;
    BENCH_CONSUME(total);

    printf("Total increments: %llu\n", total);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
