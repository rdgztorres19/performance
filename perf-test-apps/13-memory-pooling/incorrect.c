/*
 * INCORRECT: malloc a fresh 4 KB buffer for every operation, then free it.
 * Resume: "Use Memory Pooling"
 *
 * Each op allocates a buffer, fills a 1 KB record into it, uses it, and frees it.
 * The allocator work per buffer - size-class lookup, free-list unlink, header
 * write, then re-link and coalesce on free - costs more than the record itself.
 * Because DEPTH buffers are held live at once, the allocator cannot just hand
 * back the one chunk it recycled last time: it walks its bins and returns
 * scattered addresses, so each buffer is a cold region whose lines must be pulled
 * in from L2/DRAM and whose page may need a soft fault on first touch. The memory
 * is never reused warm - every op starts on freshly-minted, cache-cold bytes.
 *
 * Measure: perf stat -e cache-misses,page-faults,minor-faults ./incorrect
 */
#include "../_c_common/bench.h"

#define BUF_SIZE  4096u     /* rented buffer size */
#define USED_LEN  1024u     /* bytes actually written per op: a 1 KB record */
#define DEPTH     32u       /* buffers held live at once, as in a real pipeline */
#define N_OPS     20000u    /* pipeline passes per lap, identical in correct.c */

/*
 * Called through volatile function pointers so clang cannot apply its builtin
 * malloc/free elision and delete the very allocator traffic this demo measures.
 */
static void *(*volatile alloc_fn)(size_t) = malloc;
static void (*volatile free_fn)(void *) = free;

int main(void)
{
    printf("INCORRECT: malloc+free a %u-byte buffer per op, %u live, %u ops/lap\n",
           BUF_SIZE, DEPTH, N_OPS * DEPTH);
    fflush(stdout);

    unsigned char *live[DEPTH];

    bench_install_signals();
    double duration = (double)bench_duration_sec();
    double t0 = bench_now();

    long long laps = 0;
    uint64_t checksum = 0;

    while (!bench_stop && bench_now() - t0 < duration) {
        for (uint32_t op = 0; op < N_OPS; op++) {
            /* Rent phase: DEPTH buffers outstanding at the same time. */
            for (uint32_t d = 0; d < DEPTH; d++) {
                live[d] = alloc_fn(BUF_SIZE);
                if (!live[d]) { perror("malloc"); return 1; }

                /* Fill work, byte for byte the same as correct.c. */
                for (uint32_t i = 0; i < USED_LEN; i += CACHE_LINE)
                    live[d][i] = (unsigned char)(i + op);
            }

            /* Use-and-release phase. */
            for (uint32_t d = 0; d < DEPTH; d++) {
                for (uint32_t i = 0; i < USED_LEN; i += CACHE_LINE)
                    checksum += live[d][i];
                free_fn(live[d]);
            }
        }
        laps++;
        BENCH_CONSUME(checksum);
    }

    BENCH_CONSUME(checksum);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("Stopped: %lld laps\n", laps);
    return 0;
}
