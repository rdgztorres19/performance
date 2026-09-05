/*
 * CORRECT: rent/return the same 4 KB buffers from a pre-allocated free list.
 * Resume: "Use Memory Pooling"
 *
 * Rent is "pop the head, follow one next pointer"; return is "push onto the
 * head". Two or three instructions each, with no size-class lookup, no chunk
 * header and no coalescing, so the allocator vanishes from the profile. The link
 * lives INSIDE the free buffer's first 8 bytes, so the pool needs no side table
 * and costs zero extra memory.
 *
 * The second win, and on warm runs the larger one, is cache residency: the pool
 * is a small fixed set of buffers recycled forever, so the lines a previous op
 * touched are still in L1/L2 and their translations still in the TLB when the
 * next op rents the same buffer back. incorrect.c gets cache-cold memory every
 * time. Same buffer size, same fill loop, same checksum.
 *
 * Measure: perf stat -e cache-misses,page-faults,minor-faults ./correct
 */
#include "../_c_common/bench.h"

#define BUF_SIZE  4096u     /* identical to incorrect.c */
#define USED_LEN  1024u
#define DEPTH     32u
#define N_OPS     20000u    /* identical logical work to incorrect.c */
#define POOL_SIZE DEPTH     /* exactly as many buffers as can be live at once */

/*
 * Intrusive singly-linked free list: while a buffer is free its first 8 bytes
 * hold the pointer to the next free buffer. Once rented, those bytes are payload
 * again - which is why the fill loop below is free to overwrite them.
 */
typedef struct FreeNode { struct FreeNode *next; } FreeNode;

static FreeNode *pool_head;

static inline unsigned char *pool_rent(void)
{
    FreeNode *n = pool_head;
    if (!n) return NULL;
    pool_head = n->next;
    return (unsigned char *)n;
}

static inline void pool_return(unsigned char *buf)
{
    FreeNode *n = (FreeNode *)buf;
    n->next = pool_head;
    pool_head = n;
}

int main(void)
{
    printf("CORRECT: %u-buffer pool of %u bytes, rent/return, %u ops/lap\n",
           POOL_SIZE, BUF_SIZE, N_OPS * DEPTH);
    fflush(stdout);

    /* One allocation for the whole pool: 128 KB, so it stays resident in L2. */
    unsigned char *backing = malloc((size_t)POOL_SIZE * BUF_SIZE);
    if (!backing) { perror("malloc"); return 1; }

    /* Pre-fault and link every buffer in before the timed region starts. */
    memset(backing, 0, (size_t)POOL_SIZE * BUF_SIZE);
    for (uint32_t i = 0; i < POOL_SIZE; i++)
        pool_return(backing + (size_t)i * BUF_SIZE);

    unsigned char *live[DEPTH];

    bench_install_signals();
    double duration = (double)bench_duration_sec();
    double t0 = bench_now();

    long long laps = 0;
    uint64_t checksum = 0;

    while (!bench_stop && bench_now() - t0 < duration) {
        for (uint32_t op = 0; op < N_OPS; op++) {
            for (uint32_t d = 0; d < DEPTH; d++) {
                live[d] = pool_rent();
                if (!live[d]) { fprintf(stderr, "pool exhausted\n"); return 1; }

                for (uint32_t i = 0; i < USED_LEN; i += CACHE_LINE)
                    live[d][i] = (unsigned char)(i + op);
            }

            for (uint32_t d = 0; d < DEPTH; d++) {
                for (uint32_t i = 0; i < USED_LEN; i += CACHE_LINE)
                    checksum += live[d][i];
                pool_return(live[d]);
            }
        }
        laps++;
        BENCH_CONSUME(checksum);
    }

    BENCH_CONSUME(checksum);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("Stopped: %lld laps\n", laps);

    free(backing);
    return 0;
}
