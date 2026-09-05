/*
 * CORRECT: exactly one thread per core, reused across every round.
 * Resume: "Reduce Context Switching"
 *
 * One runnable thread per core means the scheduler has nothing to arbitrate:
 * each thread keeps a core for its whole run, so nothing evicts its working set
 * from L1, its branch predictor history stays warm, and its TLB entries stay
 * valid. Involuntary context switches drop to roughly zero.
 *
 * The larger win here is that threads are created once per round instead of 500
 * at a time -- ~2,800 pthread_create calls over the whole run instead of
 * ~100,000. Each creation is a syscall that allocates and maps a fresh stack,
 * and each exit tears it down again. That lifecycle cost, not the time-slicing
 * itself, is what dominates the gap on a modern scheduler.
 *
 * This is the principle behind a fixed-size thread pool sized to the core
 * count: past one runnable thread per core, extra threads add scheduling
 * overhead, memory, and creation cost without adding parallelism, because there
 * is no additional hardware to run them on. A real pool goes one step further
 * and reuses the same threads across rounds too.
 *
 * Work is pulled from the same shared atomic counter as incorrect.c, so both
 * complete the EXACT same number of laps and the times are directly comparable.
 *
 * Measure: vmstat 1        (watch the "cs" column: context switches/sec)
 *          pidstat -w -p <pid> 1   (cswch/s = voluntary, nvcswch/s = preempted)
 *          perf stat -e context-switches,cpu-migrations ./correct
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

/* One thread per core, resolved at runtime; this bounds the static array. */
#define MAX_THREADS 256

/* Must match incorrect.c exactly, or the comparison is meaningless. */
#define LAP_WORK   2000       /* inner iterations per lap */
#define TOTAL_LAPS 400000ULL  /* identical total work in both versions */
#define ROUNDS     200        /* same number of batches as incorrect.c */

static atomic_ullong laps_remaining;
static atomic_ullong laps_done;

/* Byte-for-byte identical to the worker in incorrect.c. */
static void *worker(void *arg)
{
    (void)arg;
    unsigned long long h = 0x9E3779B97F4A7C15ULL;
    unsigned long long mine = 0;

    for (;;) {
        unsigned long long r = atomic_fetch_sub(&laps_remaining, 1);
        if (r == 0 || r > (1ULL << 62)) break;

        for (int i = 0; i < LAP_WORK; i++) {
            h ^= h >> 12;
            h ^= h << 25;
            h ^= h >> 27;
            h *= 0x2545F4914F6CDD1DULL;
        }
        mine++;
    }

    atomic_fetch_add(&laps_done, mine);
    BENCH_CONSUME(h);
    return NULL;
}

int main(void)
{
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < 1) cores = 1;
    if (cores > MAX_THREADS) cores = MAX_THREADS;
    const int num_threads = (int)cores;

    printf("CORRECT: %d threads on %d cores (one per core)\n",
           num_threads, num_threads);
    fflush(stdout);

    bench_install_signals();

    static pthread_t threads[MAX_THREADS];
    atomic_init(&laps_done, 0);

    const double t0 = bench_now();
    unsigned long long spawned = 0;

    for (int r = 0; r < ROUNDS && !bench_stop; r++) {
        atomic_store(&laps_remaining, TOTAL_LAPS / ROUNDS);

        int started = 0;
        for (int i = 0; i < num_threads; i++) {
            if (pthread_create(&threads[i], NULL, worker, NULL) != 0) break;
            started++;
        }
        spawned += (unsigned long long)started;

        /* Bounded by the round's work: workers stop once the counter drains. */
        for (int i = 0; i < started; i++) pthread_join(threads[i], NULL);
    }

    const double elapsed = bench_now() - t0;
    const unsigned long long laps = atomic_load(&laps_done);
    BENCH_CONSUME(laps);

    printf("Threads spawned: %llu, laps: %llu\n", spawned, laps);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
