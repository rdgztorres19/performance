/*
 * INCORRECT: each pipeline stage memcpy's the payload into a fresh buffer first.
 * Resume: "Use Zero-Copy Patterns"
 *
 * A 4-stage pipeline over a 2 MB payload moves 8 MB per iteration that the work
 * itself never needed. Each memcpy is a read plus a write, so the DRAM traffic is
 * 2x the copied bytes: ~16 MB of bandwidth per iteration burned on relocating
 * data that was already addressable. The payload is far larger than L2, so every
 * copy streams through the cache and evicts whatever the previous stage left
 * behind - the destination lines also incur read-for-ownership fills before they
 * are overwritten. On top of that, each stage mallocs and frees 2 MB, which libc
 * services with mmap/munmap, so every stage pays fresh soft page faults too.
 *
 * Measure: perf stat -e cache-misses,page-faults,instructions ./incorrect
 *          Bandwidth: STAGES copies/iter, each a 2 MB read + 2 MB write.
 */
#include "../_c_common/bench.h"

#define PAYLOAD  (2u * 1024u * 1024u)   /* 2 MB: well beyond L2, bandwidth-bound */
#define STAGES   4u
#define N_ITERS  2000u                  /* identical work count in correct.c */

/*
 * One pipeline stage. It owns a private copy of the payload, sums a strided
 * sample of it, and hands its copy to the next stage.
 */
static uint64_t stage_process(const unsigned char *src, size_t len, unsigned stage)
{
    unsigned char *mine = malloc(len);
    if (!mine) { perror("malloc"); exit(1); }

    memcpy(mine, src, len);              /* the copy this version cannot avoid */

    uint64_t sum = 0;
    for (size_t i = 0; i < len; i += CACHE_LINE)
        sum += (uint64_t)mine[i] + stage;

    free(mine);
    return sum;
}

int main(void)
{
    printf("INCORRECT: %u-stage pipeline, memcpy %u MB per stage (%u MB copied/iter)\n",
           STAGES, PAYLOAD / (1024u * 1024u), STAGES * PAYLOAD / (1024u * 1024u));
    fflush(stdout);

    unsigned char *payload = malloc(PAYLOAD);
    if (!payload) { perror("malloc"); return 1; }
    for (uint32_t i = 0; i < PAYLOAD; i++)
        payload[i] = (unsigned char)(i * 31u + 7u);

    double t0 = bench_now();

    uint64_t checksum = 0;
    for (uint32_t it = 0; it < N_ITERS; it++) {
        /*
         * Chain the stages: each one copies what the previous stage produced, so
         * the payload is duplicated STAGES times per iteration.
         */
        unsigned char *cur = malloc(PAYLOAD);
        if (!cur) { perror("malloc"); return 1; }
        memcpy(cur, payload, PAYLOAD);

        for (unsigned s = 0; s < STAGES; s++)
            checksum += stage_process(cur, PAYLOAD, s);

        free(cur);
    }

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(checksum);

    double copied_mb = (double)N_ITERS * (STAGES + 1u) * PAYLOAD / (1024.0 * 1024.0);
    printf("checksum=%llu  copied=%.0f MB  bandwidth=%.2f MB/s (read+write = 2x)\n",
           (unsigned long long)checksum, copied_mb, copied_mb / elapsed);
    printf("Done in %.3fs\n", elapsed);

    free(payload);
    return 0;
}
