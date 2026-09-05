/*
 * CORRECT: each stage receives a pointer + length into the ORIGINAL buffer.
 * Resume: "Use Zero-Copy Patterns"
 *
 * A pointer and a length are 16 bytes in registers; handing them to a stage costs
 * nothing regardless of how big the payload is. The 8 MB of per-iteration memcpy
 * traffic (16 MB of DRAM read+write) simply does not happen, and neither do the
 * per-stage mmap/munmap and their soft page faults. Because all four stages now
 * read the SAME physical lines back to back, stage 2 finds in L2/L3 what stage 1
 * just pulled in, so later stages run near cache speed instead of re-streaming a
 * private copy from DRAM. Same four stages, same strided sum, same checksum as
 * incorrect.c: only the ownership model changed.
 *
 * Measure: perf stat -e cache-misses,page-faults,instructions ./correct
 *          Bandwidth: 0 copies. The payload is read in place, never written.
 */
#include "../_c_common/bench.h"

#define PAYLOAD  (2u * 1024u * 1024u)   /* identical payload to incorrect.c */
#define STAGES   4u
#define N_ITERS  2000u                  /* identical work count */

/*
 * The same stage, but const-borrowing its input. No allocation, no copy: the
 * caller keeps ownership and the stage only reads through the pointer.
 */
static uint64_t stage_process(const unsigned char *data, size_t len, unsigned stage)
{
    uint64_t sum = 0;
    for (size_t i = 0; i < len; i += CACHE_LINE)
        sum += (uint64_t)data[i] + stage;
    return sum;
}

int main(void)
{
    printf("CORRECT: %u-stage pipeline over %u MB, pointer+len only (0 MB copied/iter)\n",
           STAGES, PAYLOAD / (1024u * 1024u));
    fflush(stdout);

    unsigned char *payload = malloc(PAYLOAD);
    if (!payload) { perror("malloc"); return 1; }
    for (uint32_t i = 0; i < PAYLOAD; i++)
        payload[i] = (unsigned char)(i * 31u + 7u);

    double t0 = bench_now();

    uint64_t checksum = 0;
    for (uint32_t it = 0; it < N_ITERS; it++)
        for (unsigned s = 0; s < STAGES; s++)
            checksum += stage_process(payload, PAYLOAD, s);

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(checksum);

    printf("checksum=%llu  copied=0 MB  bandwidth=0 MB/s (nothing relocated)\n",
           (unsigned long long)checksum);
    printf("Done in %.3fs\n", elapsed);

    free(payload);
    return 0;
}
