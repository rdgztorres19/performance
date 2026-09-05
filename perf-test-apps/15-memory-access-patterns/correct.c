/*
 * CORRECT: walks the same 256 MB array sequentially so the prefetcher streams it.
 * Resume: "Optimize Memory Access Patterns to Enable Hardware Prefetching"
 *
 * Ascending addresses are the one pattern every hardware prefetcher recognises.
 * After a couple of lines it runs ahead of the load unit and pulls the following
 * lines from DRAM before they are requested, so DRAM latency is overlapped with
 * compute instead of being paid per access. One 64-byte fill now serves 16
 * consecutive int32 reads, and one page covers thousands of accesses, so the TLB
 * is consulted once per page instead of once per element. Same array, same
 * additions, same checksum as incorrect.c: only the order of the addresses differs.
 *
 * Measure: perf stat -e cache-misses,LLC-load-misses ./correct
 */
#include "../_c_common/bench.h"

#define N_INTS  (64u * 1024u * 1024u)   /* identical working set to incorrect.c */
#define PASSES  4                       /* identical amount of logical work */

int main(void)
{
    printf("CORRECT: sequential walk, %u MB x %d passes (prefetcher streams it)\n",
           (unsigned)(N_INTS * sizeof(int32_t) / (1024u * 1024u)), PASSES);
    fflush(stdout);

    int32_t *data = malloc((size_t)N_INTS * sizeof(int32_t));
    if (!data) { perror("malloc"); return 1; }

    /* Same values and same seed as incorrect.c, so the checksums must match. */
    uint64_t rng = 0x9E3779B97F4A7C15ull;
    for (uint32_t i = 0; i < N_INTS; i++) {
        rng ^= rng << 13; rng ^= rng >> 7; rng ^= rng << 17;
        data[i] = (int32_t)(rng & 0xFF);
    }

    double t0 = bench_now();

    int64_t sum = 0;
    for (int p = 0; p < PASSES; p++)
        for (uint32_t i = 0; i < N_INTS; i++)
            sum += data[i];   /* stride 4: the prefetcher is already ahead of us */

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(sum);

    printf("checksum=%lld  effective=%.0f MB/s\n", (long long)sum,
           (double)N_INTS * sizeof(int32_t) * PASSES / elapsed / (1024.0 * 1024.0));
    printf("Done in %.3fs\n", elapsed);

    free(data);
    return 0;
}
