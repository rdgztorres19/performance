/*
 * INCORRECT: walks a 256 MB array in a random permutation, defeating the prefetcher.
 * Resume: "Optimize Memory Access Patterns to Enable Hardware Prefetching"
 *
 * The array is 64x larger than L3, so nothing stays resident. Each index jumps to
 * an unrelated cache line, so the hardware stride prefetcher sees no pattern and
 * issues no speculative fills: every load is a demand miss that walks L1 -> L2 ->
 * L3 -> DRAM. Each miss also lands on a different page, so the TLB thrashes on top
 * of it. The 64-byte line dragged in is used for exactly one 4-byte int before it
 * is evicted, wasting 60 of every 64 bytes of the bandwidth it cost to fetch.
 *
 * Measure: perf stat -e cache-misses,LLC-load-misses ./incorrect
 */
#include "../_c_common/bench.h"

#define N_INTS  (64u * 1024u * 1024u)   /* 64 Mi int32 = 256 MB, far beyond any L3 */
#define PASSES  4                       /* enough work to time reliably */

int main(void)
{
    printf("INCORRECT: random-order walk, %u MB x %d passes (prefetcher useless)\n",
           (unsigned)(N_INTS * sizeof(int32_t) / (1024u * 1024u)), PASSES);
    fflush(stdout);

    int32_t *data = malloc((size_t)N_INTS * sizeof(int32_t));
    uint32_t *order = malloc((size_t)N_INTS * sizeof(uint32_t));
    if (!data || !order) { perror("malloc"); return 1; }

    /*
     * Setup is OUTSIDE the timed region on purpose: we measure the access pattern,
     * not the cost of building the permutation. Writing a distinct value to every
     * element also forces the kernel to back the whole mapping with real private
     * pages, so page faults do not leak into the measurement.
     */
    uint64_t rng = 0x9E3779B97F4A7C15ull;
    for (uint32_t i = 0; i < N_INTS; i++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        data[i] = (int32_t)(rng & 0xFF);
        order[i] = i;
    }

    /* Fisher-Yates with a fixed seed: the same permutation on every run. */
    for (uint32_t i = N_INTS - 1; i > 0; i--) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        uint32_t j = (uint32_t)(rng % (i + 1));
        uint32_t t = order[i]; order[i] = order[j]; order[j] = t;
    }

    double t0 = bench_now();

    int64_t sum = 0;
    for (int p = 0; p < PASSES; p++)
        for (uint32_t i = 0; i < N_INTS; i++)
            sum += data[order[i]];  /* unpredictable address -> unavoidable miss */

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(sum);

    printf("checksum=%lld  effective=%.0f MB/s\n", (long long)sum,
           (double)N_INTS * sizeof(int32_t) * PASSES / elapsed / (1024.0 * 1024.0));
    printf("Done in %.3fs\n", elapsed);

    free(order);
    free(data);
    return 0;
}
