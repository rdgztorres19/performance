/*
 * CORRECT: each thread accumulates privately, then merges once under the mutex.
 * Resume: "Lock / Mutex Contention"
 *
 * The hot loop touches only a per-thread counter that is padded onto its own
 * cache line, so it stays in that core's L1 in Modified state for the whole run
 * and no coherency message ever leaves the core. The mutex is taken exactly
 * NTHREADS times instead of NTHREADS * INCREMENTS_PER_THREAD times, so there is
 * effectively no contention and no futex parking at all. Same logical work,
 * same final total, but the synchronization cost is amortized to nothing.
 *
 * Note the middle ground: atomic_fetch_add(&counter, 1) removes the blocking
 * and the syscalls, but all cores still fight over one cache line, so it stays
 * bounded by coherency round-trips - faster than the mutex, far slower than
 * this. Privatize first, then reduce; go atomic only when you truly cannot.
 *
 * Measure: perf stat -e context-switches,cache-misses ./correct
 *          pidstat -w -p <pid> 1      (context switches stay near idle)
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>

#define NTHREADS 8
#define INCREMENTS_PER_THREAD 20000000LL

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static long long g_counter = 0;

/*
 * Padded so two threads' counters can never land on the same 64-byte line.
 * Without the padding this would be false sharing and would perform like the
 * contended version even though there is no lock.
 */
typedef struct {
    long long local;
    char pad[CACHE_LINE - sizeof(long long)];
} slot_t;

static slot_t g_slots[NTHREADS] __attribute__((aligned(CACHE_LINE)));

/* Shared stop time so a pathologically slow machine can never hang the run. */
static double g_deadline;

static void *worker(void *arg)
{
    slot_t *slot = (slot_t *)arg;

    /*
     * Register-resident accumulator: no memory traffic in the hot loop.
     * BENCH_CONSUME sits INSIDE the loop because -O2 folds a bare "local++"
     * loop into a single add (verify with cc -S), which would make this look
     * ~200x faster than it is. Consuming each step forces the increments to
     * really execute, so both versions do the same per-iteration work and only
     * the synchronization differs.
     */
    long long local = 0;
    for (long long i = 0; i < INCREMENTS_PER_THREAD; i++) {
        if ((i & 0xFFFFF) == 0 && (bench_stop || bench_now() > g_deadline)) break;
        local++;
        BENCH_CONSUME(local);
    }
    slot->local = local;

    /* One lock acquisition per thread for the whole run. */
    pthread_mutex_lock(&g_lock);
    g_counter += local;
    pthread_mutex_unlock(&g_lock);
    return NULL;
}

int main(void)
{
    printf("CORRECT: %d threads, private counters + one merge each, "
           "%lld increments each\n", NTHREADS, INCREMENTS_PER_THREAD);
    fflush(stdout);

    bench_install_signals();
    g_deadline = bench_now() + (double)bench_duration_sec();

    pthread_t th[NTHREADS];
    double t0 = bench_now();

    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_create(&th[i], NULL, worker, &g_slots[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }
    for (int i = 0; i < NTHREADS; i++) pthread_join(th[i], NULL);

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(g_counter);

    long long expected = (long long)NTHREADS * INCREMENTS_PER_THREAD;
    printf("counter=%lld (expected %lld) %.1f M inc/s\n",
           g_counter, expected, (double)expected / elapsed / 1e6);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
