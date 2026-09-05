/*
 * INCORRECT: every thread increments one shared counter under one global mutex.
 * Resume: "Lock / Mutex Contention"
 *
 * The counter lives on a single cache line and so does the mutex word. Every
 * acquisition has to pull that line into the acquiring core in Exclusive state,
 * which invalidates it in all the other cores: the line ping-pongs across the
 * interconnect once per increment. Worse, when the lock is already held the
 * loser does not just spin, it parks in the kernel (futex on Linux, ulock on
 * macOS), so each contended acquisition can cost a pair of syscalls plus a
 * voluntary context switch. Throughput collapses to whatever one core can do
 * while serialized, minus the coherency and scheduling tax.
 *
 * Measure: perf stat -e context-switches,cache-misses ./incorrect
 *          pidstat -w -p <pid> 1      (cswch/s + nvcswch/s climb hard)
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>

#define NTHREADS 8
#define INCREMENTS_PER_THREAD 20000000LL

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static long long g_counter = 0;

/* Shared stop time so a pathologically slow machine can never hang the run. */
static double g_deadline;

static void *worker(void *arg)
{
    (void)arg;
    for (long long i = 0; i < INCREMENTS_PER_THREAD; i++) {
        if ((i & 0xFFFF) == 0 && (bench_stop || bench_now() > g_deadline)) break;
        /*
         * The critical section is a single add. That is the point: the lock
         * costs orders of magnitude more than the work it protects.
         */
        pthread_mutex_lock(&g_lock);
        g_counter++;
        pthread_mutex_unlock(&g_lock);
    }
    return NULL;
}

int main(void)
{
    printf("INCORRECT: %d threads, one global mutex, %lld increments each\n",
           NTHREADS, INCREMENTS_PER_THREAD);
    fflush(stdout);

    bench_install_signals();
    g_deadline = bench_now() + (double)bench_duration_sec();

    pthread_t th[NTHREADS];
    double t0 = bench_now();

    for (int i = 0; i < NTHREADS; i++) {
        if (pthread_create(&th[i], NULL, worker, NULL) != 0) {
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
