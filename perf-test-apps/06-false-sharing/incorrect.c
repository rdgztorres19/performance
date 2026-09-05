/*
 * INCORRECT: per-thread counters packed adjacent, so unrelated threads share one cache line.
 * Resume: "Avoid False Sharing and Cache Line Contention"
 *
 * Each thread writes only its OWN counter, so there is no logical sharing and
 * no lock is needed. But coherency is tracked per 64-byte cache line, not per
 * variable: eight 8-byte counters land in a single line. Under MESI, a core
 * must hold that line in Modified state to write it, which invalidates every
 * other core's copy. So each thread's increment steals the line from the others
 * and their next increment must fetch it back. The line ping-pongs between
 * cores over the interconnect, turning what should be an L1-resident register
 * increment into a cross-core round trip. This is false sharing: the data is
 * private, the cache line is not.
 *
 * Measure: perf stat -e cache-misses,cache-references ./incorrect
 *   The coherency traffic shows up as misses on a working set of only 32 bytes,
 *   which is otherwise absurd. On Intel the MESI events (e.g.
 *   mem_load_l3_hit_retired.xsnp_hitm) show the HITM snoops directly: another
 *   core had the line Modified and had to hand it over.
 *
 * The size of the gap is hardware-dependent. It is largest when the contending
 * threads sit on cores that do not share a cache level, because every transfer
 * then crosses the interconnect. On a chip where all the threads land in one
 * cluster behind a shared L2 (Apple Silicon P-cores, for instance) the
 * ping-pong stays on-die and the measured ratio is smaller -- around 2x here
 * rather than the 5-10x typical of a multi-socket or cross-cluster system.
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 4
#define ITERATIONS  (400ULL * 1000 * 1000)

/*
 * Packed adjacent: 4 counters * 8 bytes = 32 bytes, well inside one 64-byte
 * line. The _Alignas is essential to the demo, not cosmetic: it forces the
 * array to start on a line boundary so all four counters provably land in a
 * SINGLE line. Without it the array could straddle two lines, splitting the
 * threads into two separately-contending groups and diluting the effect.
 */
static _Alignas(CACHE_LINE) volatile unsigned long long counters[NUM_THREADS];

_Static_assert(sizeof(counters) <= CACHE_LINE,
               "all counters must fit in one cache line for this demo");

static void *worker(void *arg)
{
    const int id = *(const int *)arg;
    for (unsigned long long i = 0; i < ITERATIONS; i++) {
        /* Private to this thread. Only the cache line is shared. */
        counters[id]++;
    }
    return NULL;
}

int main(void)
{
    printf("INCORRECT: %d threads, counters packed in ONE cache line (false sharing)\n",
           NUM_THREADS);
    fflush(stdout);

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
    for (int i = 0; i < NUM_THREADS; i++) total += counters[i];
    BENCH_CONSUME(total);

    printf("Total increments: %llu\n", total);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
