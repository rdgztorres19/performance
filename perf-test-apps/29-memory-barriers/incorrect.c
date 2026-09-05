/*
 * INCORRECT: publish/subscribe with plain volatile ints and no ordering.
 * Resume: "Use Memory Barriers for Correct Lock-Free Programming"
 *
 * The writer does "data = seq; ready = seq;". volatile stops the COMPILER from
 * caching those in registers, but it emits no barrier, so nothing stops the
 * compiler from reordering the two stores or the CPU from making them visible
 * out of order. On a weakly-ordered core the two lines sit in the store buffer
 * and drain independently, so "ready" can land in a remote core's view before
 * "data" does. The reader, with no acquire barrier, may also let its load of
 * data be satisfied from a stale line it already held. Either way it observes
 * ready == N while data is still an older value: a torn publish. There is no
 * happens-before edge, so this is a plain data race, not merely a slow path.
 *
 * data and ready are deliberately on SEPARATE cache lines. Sharing one line
 * makes them travel together and hides the bug - which is exactly why this
 * class of race is so unreliable to reproduce by accident.
 *
 * Platform note: on x86-64 this rarely or never fails. TSO already guarantees
 * store->store and load->load ordering in hardware, so the bug stays latent and
 * the code looks correct in testing. On arm64 (Apple Silicon, AWS Graviton) it
 * fails readily, which is why "it passed on my x86 laptop" is not evidence of
 * correctness. Anomalies below are counted, not timed.
 *
 * Measure: correctness, not speed. Compare anomaly counts:
 *          ./incorrect   (expect > 0 on arm64)   vs   ./correct (always 0)
 *
 * Measured in this repo: 15,259 anomalies running natively on Apple Silicon,
 * but 0 inside the Docker image on the same machine, where the container had
 * fewer cores and the two threads were rarely on different physical cores at
 * the same instant. A zero here is NOT proof the code is correct: it means the
 * race did not happen to be observed in that run. Give it real parallelism
 * (CPU_LIMIT=8 or higher) before drawing any conclusion.
 */
#include "../_c_common/bench.h"
#include <pthread.h>

#define ROUNDS 5000000

/*
 * Each flag gets its own cache line so the two stores can become visible
 * independently. 128 covers the 128-byte pairing on Apple Silicon.
 */
static volatile int g_data  __attribute__((aligned(128)));
static char g_pad[128];
static volatile int g_ready __attribute__((aligned(128)));

static long long g_anomalies;
static long long g_checks;
static double g_deadline;

static void *reader(void *arg)
{
    (void)arg;
    int last = 0;
    while (last < ROUNDS) {
        /*
         * Load ready, then data. With no acquire barrier nothing orders these
         * two loads, and nothing forces data to be at least as fresh as ready.
         */
        int rd = g_ready;
        int dt = g_data;

        /*
         * The writer always stores data BEFORE ready, so in a correctly
         * ordered world data >= ready at every instant. Seeing ready ahead of
         * data means the publish was observed torn.
         */
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
    printf("INCORRECT: %d publish rounds, plain volatile, no barriers\n", ROUNDS);
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
        /* The publish: payload first, then the flag. No release barrier. */
        g_data = r;
        g_ready = r;

        if ((r & 0xFFFFF) == 0 && (bench_stop || bench_now() > g_deadline)) {
            g_ready = ROUNDS;  /* release the reader */
            break;
        }
    }

    pthread_join(th, NULL);
    double elapsed = bench_now() - t0;
    BENCH_CONSUME(g_pad[0]);

    printf("anomalies=%lld out of %lld observations "
           "(reader saw ready ahead of data)\n", g_anomalies, g_checks);
    printf("Done in %.3fs\n", elapsed);
    return 0;
}
