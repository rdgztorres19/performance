/*
 * INCORRECT: Array of Structs - summing one field drags 64 bytes per 4 useful bytes.
 * Resume: "Use Cache-Friendly Memory Layouts to Improve Performance"
 *
 * Particle is 64 bytes, so consecutive x fields sit 64 bytes apart. The cache
 * cannot fetch less than a line, so each x load pulls in a whole line whose other
 * 60 bytes (y, z, mass, id, padding) are never read and are evicted untouched.
 * The effective bandwidth is therefore 1/16th of what the bus actually delivers,
 * and the working set that must stream through the cache is 16x larger than the
 * data the loop truly needs. The stride is regular, so the prefetcher still works
 * -- this is purely wasted cache-line payload, not a prefetch failure.
 *
 * Measure: perf stat -e cache-misses,L1-dcache-load-misses ./incorrect
 */
#include "../_c_common/bench.h"

#define N_PARTICLES (16u * 1024u * 1024u)  /* 16 Mi * 64 B = 1 GB of structs */
#define PASSES      3

struct Particle {
    float x, y, z;
    float mass;
    int   id;
    char  pad[44];      /* rounds the struct to exactly one 64-byte cache line */
};

int main(void)
{
    printf("INCORRECT: AoS, %u particles x %d passes (struct=%zu B, field=%zu B)\n",
           N_PARTICLES, PASSES, sizeof(struct Particle), sizeof(float));
    fflush(stdout);

    struct Particle *p = malloc((size_t)N_PARTICLES * sizeof(struct Particle));
    if (!p) { perror("malloc"); return 1; }

    /* Written outside the timed region, so every page is already resident. */
    for (uint32_t i = 0; i < N_PARTICLES; i++) {
        p[i].x = (float)(i & 0x3FF);
        p[i].y = 1.0f; p[i].z = 2.0f; p[i].mass = 3.0f;
        p[i].id = (int)i;
        memset(p[i].pad, 0, sizeof(p[i].pad));
    }

    double t0 = bench_now();

    /*
     * Four float accumulators, not one double: a single accumulator serialises
     * the loop on FP-add latency (~4 cycles each) and caps BOTH versions at a
     * throughput far below memory speed, hiding the layout effect entirely.
     * Independent partial sums let the vector unit issue loads at full rate, so
     * what is actually being compared is the memory layout.
     */
    float a0 = 0, a1 = 0, a2 = 0, a3 = 0;
    for (int pass = 0; pass < PASSES; pass++)
        for (uint32_t i = 0; i < N_PARTICLES; i += 4) {
            /* 64-byte stride: each load uses 4 bytes of a line and wastes 60. */
            a0 += p[i + 0].x; a1 += p[i + 1].x;
            a2 += p[i + 2].x; a3 += p[i + 3].x;
        }
    float sum = a0 + a1 + a2 + a3;

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(sum);

    printf("sum=%.1f  useful=%.0f MB/s\n", (double)sum,
           (double)N_PARTICLES * sizeof(float) * PASSES / elapsed / (1024.0 * 1024.0));
    printf("Done in %.3fs\n", elapsed);

    free(p);
    return 0;
}
