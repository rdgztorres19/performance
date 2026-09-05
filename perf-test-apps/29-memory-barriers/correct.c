/*
 * CORRECT: the same publish/subscribe, ordered with release/acquire atomics.
 * Resume: "Use Memory Barriers for Correct Lock-Free Programming"
 *
 * atomic_store_explicit(&ready, seq, memory_order_release) guarantees that
 * every write the writer performed BEFORE it - here the store to data - is
 * visible to any thread that then reads ready with memory_order_acquire. The
 * release side prevents earlier stores from sinking past the flag; the acquire
 * side prevents later loads from being hoisted above it. Together they build a
 * happens-before edge from writer to reader, so observing ready == N implies
 * data == N. On arm64 this lowers to stlr / ldar; on x86-64 it costs no extra
 * instruction at all, because TSO already provides the ordering - the barrier
 * is what makes that guarantee portable instead of accidental.
 *
 * The flags stay on separate cache lines exactly as in incorrect.c, so the only
 * difference between the two files is the ordering, not the layout. The anomaly
 * count here is structurally zero, not merely small.
 *
 * Measure: correctness, not speed. anomalies must be 0 on every platform.
 *          ./correct     vs    ./incorrect
 */
#include "../_c_common/bench.h"
#include <pthread.h>
#include <stdatomic.h>

#define ROUNDS 5000000

static _Atomic int g_data  __attribute__((aligned(128)));
static char g_pad[128];
static _Atomic int g_ready __attribute__((aligned(128)));

static long long g_anomalies;
static long long g_checks;
static double g_deadline;

static void *reader(void *arg)
{
    (void)arg;
    int last = 0;
    while (last < ROUNDS) {
        /*
         * Acquire load: everything the writer did before its release store to
         * ready is guaranteed visible to us after this returns.
         */
        int rd = atomic_load_explicit(&g_ready, memory_order_acquire);

        /*
         * relaxed is sufficient for the payload: the acquire above already
         * ordered this load after the flag, which is the whole guarantee.
         */
        int dt = atomic_load_explicit(&g_data, memory_order_relaxed);

        if (rd > dt) g_anomalies++;

        g_checks++;
        last = rd;

        /* Bounded so a missed publish can never hang the run. */
        if ((g_checks & 0xFFFFF) == 0 &&
            (bench_stop || bench_now() > g_deadline)) break;
    }
    BENCH_CONSUME(last);
    return NULL;
}

int main(void)
{
    printf("CORRECT: %d publish rounds, release/acquire ordering\n", ROUNDS);
    fflush(stdout);

    bench_install_signals();
    double t0 = bench_now();
    g_deadline = t0 + (double)bench_duration_sec();

    pthread_t th;
    if (pthread_create(&th, NULL, reader, NULL) != 0) {
        perror("pthread_create");
        return 1;
    }

    for (int r = 1; r <= ROUNDS; r++) {
        /* Payload first, relaxed: the release store below is what orders it. */
        atomic_store_explicit(&g_data, r, memory_order_relaxed);
        /* The publish. Release: no earlier store may sink past this point. */
        atomic_store_explicit(&g_ready, r, memory_order_release);

        if ((r & 0xFFFFF) == 0 && (bench_stop || bench_now() > g_deadline)) {
            atomic_store_explicit(&g_ready, ROUNDS, memory_order_release);
            break;
        }
    }

    pthread_join(th, NULL);
    double elapsed = bench_now() - t0;
    BENCH_CONSUME(g_pad[0]);

    printf("anomalies=%lld out of %lld observations "
           "(release/acquire makes this structurally 0)\n",
           g_anomalies, g_checks);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
