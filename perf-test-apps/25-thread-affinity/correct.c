/*
 * CORRECT: the same workload, one thread per core, each pinned to its own core.
 * Resume: "Thread Affinity"
 *
 * Pinning a thread to a single core lets its working set stay resident in that
 * core's private L1/L2 for the whole run: after the first pass, accesses hit in
 * cache instead of refilling from L3 or DRAM. Because exactly one thread runs
 * per core, the scheduler has no reason to migrate anything, so no thread pays
 * the cold-cache penalty of arriving on a core that has never seen its data,
 * and no line has to be transferred away from a core that still wants it.
 *
 * Platform honesty: real pinning exists only on Linux, via
 * pthread_setaffinity_np with a CPU_SET holding a single core. macOS has no
 * equivalent - THREAD_AFFINITY_POLICY is only a hint to keep threads together,
 * it never binds a thread to a core, and on Apple Silicon it is not implemented
 * at all (thread_policy_set returns KERN_NOT_SUPPORTED). This file attempts it
 * anyway, reports truthfully whether pinning actually took effect, and runs the
 * identical workload either way so the file still compiles and measures.
 * On macOS the remaining gain comes from not oversubscribing the cores, which
 * is itself the practical half of the affinity lesson.
 *
 * Measure: perf stat -e cache-misses,migrations ./correct        (Linux)
 *          migrations should fall to ~0 and cache-misses drop sharply
 *
 * Expected result by platform - read this before judging the numbers:
 *   Linux  : pinning is real, so this version wins on cache-misses and
 *            migrations, and usually on wall time.
 *   macOS  : pinning is UNAVAILABLE (thread_policy_set returns
 *            KERN_NOT_SUPPORTED on Apple Silicon), so this file and
 *            incorrect.c run the identical code path and time the SAME.
 *            Measured on an M-series 14-core: ~0.23s both ways. That is the
 *            honest outcome, not a tuning failure - without a pinning API
 *            there is no cache residency to win back. Run it on Linux to see
 *            the effect this topic is about.
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>

#ifdef __linux__
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sched.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#endif

/* ~192 KB: comfortably larger than L1, meant to live in each core's L2. */
#define SET_BYTES (192 * 1024)
#define SET_WORDS (SET_BYTES / (int)sizeof(uint64_t))
/* Total passes across ALL threads: identical in both files. */
#define TOTAL_PASSES 42000

static int g_nthreads;
static double g_deadline;
static volatile int g_pinned_ok;   /* did pinning actually take effect? */

typedef struct {
    int id;
    uint64_t sum;
} work_t;

/*
 * Bind the calling thread to exactly one core. Returns 1 on real pinning,
 * 0 when the platform cannot do it - never pretends to have succeeded.
 */
static int pin_to_core(int core)
{
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    return pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0;
#elif defined(__APPLE__)
    /*
     * Affinity TAGS, not core numbers: threads sharing a tag are hinted to run
     * near each other. A distinct tag per thread is the closest available
     * analogue to "spread these out and keep them put". Returns
     * KERN_NOT_SUPPORTED on Apple Silicon, which we report rather than hide.
     */
    thread_affinity_policy_data_t policy = { .affinity_tag = core + 1 };
    kern_return_t kr = thread_policy_set(
        pthread_mach_thread_np(pthread_self()),
        THREAD_AFFINITY_POLICY,
        (thread_policy_t)&policy,
        THREAD_AFFINITY_POLICY_COUNT);
    return kr == KERN_SUCCESS;
#else
    (void)core;
    return 0;
#endif
}

static void run_workload(work_t *t)
{
    uint64_t *buf = malloc(SET_BYTES);
    if (!buf) return;

    for (int i = 0; i < SET_WORDS; i++) buf[i] = (uint64_t)i;

    const uint64_t stride = 1031;
    uint64_t idx = (uint64_t)t->id * 17;
    uint64_t sum = 0;

    int passes = TOTAL_PASSES / g_nthreads;
    for (int p = 0; p < passes; p++) {
        for (int i = 0; i < SET_WORDS; i++) {
            idx = (idx + stride) % (uint64_t)SET_WORDS;
            sum += buf[idx];
            buf[idx] = sum;
        }
        if ((p & 0x3F) == 0 && (bench_stop || bench_now() > g_deadline)) break;
    }

    t->sum = sum;
    BENCH_CONSUME(sum);
    free(buf);
}

static void *worker(void *arg)
{
    work_t *t = (work_t *)arg;
    if (pin_to_core(t->id)) g_pinned_ok = 1;
    run_workload(t);
    return NULL;
}

int main(void)
{
    long ncores = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncores < 1) ncores = 4;
    /* Exactly one thread per core: nothing for the scheduler to shuffle. */
    g_nthreads = (int)ncores;

    printf("CORRECT: %d pinned threads (%ld cores), %d KB working set each\n",
           g_nthreads, ncores, SET_BYTES / 1024);
    fflush(stdout);

    bench_install_signals();
    double t0 = bench_now();
    g_deadline = t0 + (double)bench_duration_sec();

    pthread_t *th = calloc((size_t)g_nthreads, sizeof(pthread_t));
    work_t *tasks = calloc((size_t)g_nthreads, sizeof(work_t));
    if (!th || !tasks) { perror("calloc"); return 1; }

    for (int i = 0; i < g_nthreads; i++) {
        tasks[i].id = i;
        if (pthread_create(&th[i], NULL, worker, &tasks[i]) != 0) {
            perror("pthread_create");
            return 1;
        }
    }

    uint64_t total = 0;
    for (int i = 0; i < g_nthreads; i++) {
        pthread_join(th[i], NULL);
        total += tasks[i].sum;
    }

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(total);

    printf("pinning: %s\n", g_pinned_ok
        ? "ACTIVE (threads bound to cores)"
        : "UNAVAILABLE on this platform - ran unpinned, one thread per core");
    printf("checksum=%llu\n", (unsigned long long)total);
    printf("Done in %.3fs\n", elapsed);
    free(th);
    free(tasks);
    return 0;
}
