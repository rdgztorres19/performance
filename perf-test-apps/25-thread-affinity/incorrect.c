/*
 * INCORRECT: cache-resident threads left unpinned for the scheduler to migrate.
 * Resume: "Thread Affinity"
 *
 * Each thread walks a working set sized to sit in its core's private L1/L2. As
 * long as it stays on one core, every access after the first pass is an L1/L2
 * hit. When the scheduler migrates the thread to another core - to balance
 * load, or onto an efficiency core - the new core's private caches know nothing
 * about that working set, so the thread takes a full round of cold misses and
 * refills from L3 or DRAM. The data must also be invalidated or transferred
 * from the old core, adding coherency traffic. The work is identical to
 * correct.c; only the residency of the cache is lost.
 *
 * Thread count, working-set size and pass count are identical to correct.c.
 * The only difference is that these threads are never bound to a core, so the
 * scheduler is free to migrate them - including onto efficiency cores - and
 * each migration throws away the private-cache residency the thread built up.
 *
 * Measure: perf stat -e cache-misses,migrations ./incorrect     (Linux)
 *          both counters are markedly higher than for ./correct
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <unistd.h>

/* ~192 KB: comfortably larger than L1, meant to live in each core's L2. */
#define SET_BYTES (192 * 1024)
#define SET_WORDS (SET_BYTES / (int)sizeof(uint64_t))
/* Total passes across ALL threads: identical in both files. */
#define TOTAL_PASSES 42000

static int g_nthreads;
static double g_deadline;

typedef struct {
    int id;
    uint64_t sum;
} work_t;

/*
 * Pointer-chasing over a shuffled ring: each load depends on the previous one,
 * so the prefetcher cannot hide the misses and the measurement reflects real
 * cache residency rather than memory bandwidth.
 */
static void run_workload(work_t *t)
{
    uint64_t *buf = malloc(SET_BYTES);
    if (!buf) return;

    for (int i = 0; i < SET_WORDS; i++) buf[i] = (uint64_t)i;

    /* Stride by an odd number coprime to the size: touches every slot once. */
    const uint64_t stride = 1031;
    uint64_t idx = (uint64_t)t->id * 17;
    uint64_t sum = 0;

    int passes = TOTAL_PASSES / g_nthreads;
    for (int p = 0; p < passes; p++) {
        for (int i = 0; i < SET_WORDS; i++) {
            idx = (idx + stride) % (uint64_t)SET_WORDS;
            sum += buf[idx];
            buf[idx] = sum;          /* write too, so the line must be owned */
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
    /* No affinity call at all: the scheduler may move this thread anywhere. */
    run_workload(t);
    return NULL;
}

int main(void)
{
    long ncores = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncores < 1) ncores = 4;
    /*
     * 2x oversubscription: the scheduler must time-slice and migrate threads
     * between cores. TOTAL_PASSES is fixed, so each thread simply does half as
     * many passes - the total work is identical to correct.c.
     */
    g_nthreads = (int)ncores * 2;

    printf("INCORRECT: %d unpinned threads (%ld cores), %d KB working set each\n",
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

    printf("checksum=%llu\n", (unsigned long long)total);
    printf("Done in %.3fs\n", elapsed);
    free(th);
    free(tasks);
    return 0;
}
