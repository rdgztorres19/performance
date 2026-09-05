/*
 * CORRECT: one arena allocated up front, objects carved out by bumping a pointer.
 * Resume: "Use Memory Pooling"
 *
 * Allocation collapses to "read offset, add 32, store offset" - a couple of
 * instructions with no free lists, no chunk headers and no lock. Deallocation of
 * the whole lap is a single store resetting the offset to zero, so the allocator
 * call count per lap drops from 8,000,000 to 1. The layout matters as much as the
 * speed: objects come out strictly consecutive, so two 32-byte Items share one
 * 64-byte cache line and the stride prefetcher streams the arena ahead of the
 * loop. Same object count, same field writes, same checksum as incorrect.c.
 *
 * Measure: perf stat -e cache-misses,dTLB-load-misses,instructions ./correct
 *          allocator calls: 1 malloc for the whole run (arena reset, never freed).
 */
#include "../_c_common/bench.h"

#define N_OBJECTS 4000000u   /* identical logical work to incorrect.c */

typedef struct {
    uint64_t id;
    uint64_t value;
    uint64_t tag;
    uint64_t pad;
} Item;

typedef struct {
    unsigned char *base;
    size_t used;
    size_t cap;
} Arena;

/*
 * Bump allocation. Inlined by -O2 into an add and a compare, which is the entire
 * point: the fast path never leaves the CPU's store buffer.
 */
static inline void *arena_alloc(Arena *a, size_t n)
{
    /* Keep 8-byte alignment; sizeof(Item) is already a multiple, so this is free. */
    size_t off = (a->used + 7u) & ~(size_t)7u;
    if (off + n > a->cap) return NULL;
    a->used = off + n;
    return a->base + off;
}

static inline void arena_reset(Arena *a) { a->used = 0; }

int main(void)
{
    printf("CORRECT: bump-pointer arena, %u objects/lap (1 allocator call total)\n",
           N_OBJECTS);
    fflush(stdout);

    Arena arena;
    arena.cap = (size_t)N_OBJECTS * sizeof(Item);
    arena.base = malloc(arena.cap);
    arena.used = 0;
    if (!arena.base) { perror("malloc"); return 1; }

    /*
     * Pre-fault the arena outside the timed region. incorrect.c reuses the same
     * few hot heap pages lap after lap, so charging first-touch page faults to
     * this version only would make the comparison unfair.
     */
    memset(arena.base, 0, arena.cap);

    bench_install_signals();
    double duration = (double)bench_duration_sec();
    double t0 = bench_now();

    long long laps = 0;
    uint64_t checksum = 0;

    while (!bench_stop && bench_now() - t0 < duration) {
        arena_reset(&arena);               /* "frees" 4M objects with one store */

        for (uint32_t i = 0; i < N_OBJECTS; i++) {
            Item *it = arena_alloc(&arena, sizeof(Item));
            if (!it) { fprintf(stderr, "arena exhausted\n"); return 1; }

            it->id = i;
            it->value = (uint64_t)i * 2654435761ull;
            it->tag = it->id ^ it->value;
            it->pad = 0;
            checksum += it->tag;
        }
        laps++;
        BENCH_CONSUME(checksum);
    }

    BENCH_CONSUME(checksum);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("Stopped: %lld laps\n", laps);

    free(arena.base);
    return 0;
}
