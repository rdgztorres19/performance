/*
 * INCORRECT: branching on unsorted random data, so the branch predictor guesses wrong ~50% of the time.
 * Resume: "Branch Prediction"
 *
 * The CPU is deeply pipelined: it speculatively fetches and executes ~15-20
 * stages past a conditional branch before the comparison result is known. With
 * uniformly random values the `v >= 128` outcome carries maximum entropy, so
 * the predictor's history tables cannot learn a pattern and it is wrong roughly
 * half the time. Every miss squashes the whole speculative window and refills
 * the front end, costing on the order of 15-20 cycles of pure stall. The memory
 * access pattern is a perfectly prefetchable linear scan, so essentially the
 * entire runtime difference versus correct.c is pipeline flush cost.
 *
 * Measure: perf stat -e branches,branch-misses ./incorrect
 */
#include "../_c_common/bench.h"

#define N (1u << 22) /* 4M bytes: fits comfortably in L3, so memory is not the bottleneck */

static unsigned rng_state = 12345u;

/* xorshift: cheap and deterministic, so both versions see the identical data. */
static unsigned rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

int main(void)
{
    printf("INCORRECT: branchy filter over UNSORTED random data\n");
    fflush(stdout);

    bench_install_signals();

    unsigned char *data = malloc(N);
    if (!data) { perror("malloc"); return 1; }
    for (unsigned i = 0; i < N; i++) data[i] = (unsigned char)(rng_next() & 0xFF);

    const double duration = bench_duration_sec();
    const double t0 = bench_now();
    long long laps = 0;
    unsigned long long sum = 0;

    while (!bench_stop && bench_now() - t0 < duration) {
        for (unsigned i = 0; i < N; i++) {
            unsigned char v = data[i];
            /*
             * Unpredictable: the predictor cannot do better than a coin flip.
             *
             * The taken path is a 3-step dependent multiply chain rather than
             * a plain `sum += v`. That is deliberate and load-bearing: with a
             * cheap body the compiler if-converts, emitting a conditional
             * select (csel on arm64, cmov on x86-64) that computes both
             * outcomes and picks one -- erasing the very branch this demo
             * exists to measure. Measured here, a 1- or 2-step body yields a
             * correct/incorrect ratio of 1.00 for exactly that reason. Three
             * steps is past the point where the compiler thinks unconditional
             * speculation is worth it, so a real conditional branch survives
             * and the predictor has to guess it. Much longer chains regress the
             * ratio too, as multiply latency swamps the ~15-cycle flush.
             */
            if (v >= 128) {
                for (int k = 0; k < 3; k++)
                    sum = (sum + v) * 1099511628211ULL;
            }
        }
        BENCH_CONSUME(sum);
        laps++;
    }

    BENCH_CONSUME(sum);
    free(data);

    printf("Stopped: %lld laps\n", laps);
    return 0;
}
