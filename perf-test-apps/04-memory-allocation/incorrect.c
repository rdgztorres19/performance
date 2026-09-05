/*
 * INCORRECT: malloc/free one small object per item, millions of times.
 * Resume: "Use Memory Pooling"
 *
 * Every malloc walks the allocator's size-class free lists, splits or pops a
 * chunk, and writes a header next to the payload; every free re-links that chunk
 * and may coalesce with its neighbours. That is dozens of dependent loads and
 * stores of pure bookkeeping per 32-byte object, and on glibc/macOS libc it also
 * touches an arena lock. Worse, the returned pointers are scattered across the
 * heap: consecutive objects land on unrelated cache lines, so the hardware
 * prefetcher sees no stride and each object costs a fresh demand miss instead of
 * riding along in a line already in L1.
 *
 * Measure: perf stat -e cache-misses,dTLB-load-misses,instructions ./incorrect
 *          allocator calls: 2 per object (1 malloc + 1 free) = 8,000,000 per lap.
 */
#include "../_c_common/bench.h"

/*
 * Called through volatile function pointers on purpose. Clang treats malloc/free
 * as builtins and will delete a malloc/free pair whose memory does not escape,
 * which silently turns this benchmark into an empty loop that "beats" the arena.
 * Going through a volatile pointer makes the calls opaque, so the allocator work
 * we are here to measure actually happens.
 */
static void *(*volatile alloc_fn)(size_t) = malloc;
static void (*volatile free_fn)(void *) = free;

#define N_OBJECTS 4000000u   /* logical objects per lap, identical in correct.c */

/* 32 bytes: small enough that allocator bookkeeping dominates the real work. */
typedef struct {
    uint64_t id;
    uint64_t value;
    uint64_t tag;
    uint64_t pad;
} Item;

int main(void)
{
    printf("INCORRECT: malloc+free per object, %u objects/lap (%u allocator calls/lap)\n",
           N_OBJECTS, N_OBJECTS * 2u);
    fflush(stdout);

    bench_install_signals();
    double duration = (double)bench_duration_sec();
    double t0 = bench_now();

    long long laps = 0;
    uint64_t checksum = 0;

    while (!bench_stop && bench_now() - t0 < duration) {
        for (uint32_t i = 0; i < N_OBJECTS; i++) {
            Item *it = alloc_fn(sizeof(Item));  /* free-list walk + header write */
            if (!it) { perror("malloc"); return 1; }

            /* Same per-object work as correct.c: write fields, then read one. */
            it->id = i;
            it->value = (uint64_t)i * 2654435761ull;
            it->tag = it->id ^ it->value;
            it->pad = 0;
            checksum += it->tag;

            free_fn(it);                        /* re-link + possible coalesce */
        }
        laps++;
        BENCH_CONSUME(checksum);
    }

    BENCH_CONSUME(checksum);
    printf("checksum=%llu\n", (unsigned long long)checksum);
    printf("Stopped: %lld laps\n", laps);
    return 0;
}
