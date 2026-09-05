/*
 * CORRECT: same branch over the same data, but sorted first so the branch is predictable.
 * Resume: "Branch Prediction"
 *
 * The data, the comparison, the additions and the number of taken branches are
 * identical to incorrect.c. Only the ORDER changes. Sorted input turns the
 * condition into one long run of false followed by one long run of true, which
 * the branch predictor's history tables learn immediately: it mispredicts twice
 * per pass (once entering the loop, once at the single transition point)
 * instead of ~2 million times. The pipeline stays full, speculative work is
 * never squashed, and the loop runs at the pipeline's issue width.
 *
 * The sort happens OUTSIDE the timed region: this demo measures the cost of
 * unpredictability, not the cost of sorting. In real code the payoff only
 * exists when the data is already ordered or is reused across many passes.
 *
 * Measure: perf stat -e branches,branch-misses ./correct
 */
#include "../_c_common/bench.h"

#define N (1u << 22) /* identical working set to incorrect.c */

static unsigned rng_state = 12345u;

static unsigned rng_next(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

int main(void)
{
    printf("CORRECT: branchy filter over SORTED data (branch becomes predictable)\n");
    fflush(stdout);

    bench_install_signals();

    unsigned char *data = malloc(N);
    if (!data) { perror("malloc"); return 1; }
    for (unsigned i = 0; i < N; i++) data[i] = (unsigned char)(rng_next() & 0xFF);

    /*
     * Counting sort, outside the timed region. Same 4M bytes and the same
     * multiset of values as incorrect.c, just arranged in ascending order.
     */
    size_t hist[256] = {0};
    for (unsigned i = 0; i < N; i++) hist[data[i]]++;
    unsigned pos = 0;
    for (int v = 0; v < 256; v++)
        for (size_t k = 0; k < hist[v]; k++) data[pos++] = (unsigned char)v;

    const double duration = bench_duration_sec();
    const double t0 = bench_now();
    long long laps = 0;
    unsigned long long sum = 0;

    while (!bench_stop && bench_now() - t0 < duration) {
        for (unsigned i = 0; i < N; i++) {
            unsigned char v = data[i];
            /*
             * Predictable now: false for every index below the 128 boundary,
             * true for every index above it.
             *
             * The branchless alternative removes the branch instead of making
             * it predictable, and so works on unsorted data too. For a plain
             * accumulate it is written as a mask:
             *     sum += v & -(unsigned long long)(v >= 128);
             * `(v >= 128)` yields 0 or 1; negating it gives an all-zero or
             * all-ones mask, so the add always executes and contributes either
             * v or 0. No control-flow decision remains for the predictor to get
             * wrong, so there is nothing to flush. Compilers reach the same
             * shape on their own via a conditional move (cmov on x86-64, csel
             * on arm64) -- which is precisely why the body here had to be made
             * expensive enough that the compiler declines to do so.
             *
             * That trade-off is the whole point: branchless always evaluates
             * both sides. It wins when the branch is unpredictable and the body
             * is cheap, and loses when the body is costly (as here) or when the
             * branch predicts well anyway (as it does after sorting).
             *
             * The taken path below is a 3-step dependent multiply chain rather
             * than a plain `sum += v`: a cheap body gets if-converted into a
             * conditional select, which would erase the branch this demo
             * exists to measure. Three steps is past the point where the
             * compiler will speculate unconditionally, so a real conditional
             * branch survives. See incorrect.c for the measured tuning.
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
