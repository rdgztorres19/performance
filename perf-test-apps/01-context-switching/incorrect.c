/*
 * INCORRECT: ~500 CPU-bound threads on a machine with a few cores, respawned every round.
 * Resume: "Reduce Context Switching"
 *
 * 500 runnable threads share N cores, so the kernel must time-slice them. Each
 * involuntary preemption costs far more than the register save/restore: the
 * incoming thread's working set is no longer in L1/L2, its branch predictor
 * history is cold, and the TLB holds the outgoing thread's translations. Every
 * thread also carries a kernel task_struct and its own stack (typically
 * 512KB-8MB of address space), and creating one is a syscall that allocates and
 * maps that stack.
 *
 * Work is pulled from a shared atomic counter, so this program and correct.c
 * complete the EXACT same number of laps. Only the thread count differs, so the
 * time difference is thread lifecycle and scheduling overhead, not extra work.
 *
 * Note on what actually dominates: on a modern scheduler, pure CPU-bound
 * oversubscription alone is nearly free -- each thread gets a long time slice,
 * so switches are rare and their cost amortizes. Measured here, 500 vs 14
 * threads that simply run to completion differ by under 3%. The cost that does
 * show up, and the one this demo isolates, is thread CREATION: a workload that
 * spawns a fresh batch of threads per unit of work pays for 500 stack
 * allocations per round instead of reusing a handful of threads.
 *
 * Measure: vmstat 1        (watch the "cs" column: context switches/sec)
 *          pidstat -w -p <pid> 1   (cswch/s = voluntary, nvcswch/s = preempted)
 *          perf stat -e context-switches,cpu-migrations ./incorrect
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

#define NUM_THREADS 500

/* Must match correct.c exactly, or the comparison is meaningless. */
#define LAP_WORK   2000       /* inner iterations per lap */
#define TOTAL_LAPS 400000ULL  /* identical total work in both versions */
#define ROUNDS     200        /* batches; each spawns a fresh set of threads */

static atomic_ullong laps_remaining;
static atomic_ullong laps_done;

/*
 * Workers pull laps from a shared counter rather than being handed a fixed
 * slice up front. That is what keeps the total work identical across both
 * versions no matter how many threads are running.
 */
static void *worker(void *arg)
{
    (void)arg;
    unsigned long long h = 0x9E3779B97F4A7C15ULL;
    unsigned long long mine = 0;

    for (;;) {
        /*
         * fetch_sub returns the PREVIOUS value, so 0 means the counter was
         * already drained. A huge value means it wrapped below zero, which is
         * also "drained" -- both cases stop the worker, so it always exits.
         */
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
    const long cores = sysconf(_SC_NPROCESSORS_ONLN);

    printf("INCORRECT: %d threads on %ld cores (oversubscribed %.0fx), respawned each round\n",
           NUM_THREADS, cores, (double)NUM_THREADS / (double)cores);
    fflush(stdout);

    bench_install_signals();

    static pthread_t threads[NUM_THREADS];
    atomic_init(&laps_done, 0);

    const double t0 = bench_now();
    unsigned long long spawned = 0;

    for (int r = 0; r < ROUNDS && !bench_stop; r++) {
        atomic_store(&laps_remaining, TOTAL_LAPS / ROUNDS);

        int started = 0;
        for (int i = 0; i < NUM_THREADS; i++) {
            if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
                /* Hit the thread limit: proceed with what we have. */
                break;
            }
            started++;
        }
        spawned += (unsigned long long)started;

        /*
         * Workers exit as soon as the counter drains, so these joins are
         * bounded by the round's work and cannot hang. On SIGINT the loop above
         * stops starting new rounds; the current round still drains and joins.
         */
        for (int i = 0; i < started; i++) pthread_join(threads[i], NULL);
    }

    const double elapsed = bench_now() - t0;
    const unsigned long long laps = atomic_load(&laps_done);
    BENCH_CONSUME(laps);

    printf("Threads spawned: %llu, laps: %llu\n", spawned, laps);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
