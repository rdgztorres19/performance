/*
 * CORRECT: Struct of Arrays - x lives in its own array, so every fetched byte is used.
 * Resume: "Use Cache-Friendly Memory Layouts to Improve Performance"
 *
 * Splitting the hot field into a dedicated float array makes consecutive x values
 * 4 bytes apart instead of 64. One 64-byte cache line now carries 16 values the
 * loop will actually add, so the same sum touches 16x fewer lines and moves 16x
 * fewer bytes across the memory bus. The contiguous unit-stride layout is also
 * what the vector unit wants: the compiler can load 4-8 floats per instruction,
 * which it cannot do when the values are scattered 64 bytes apart.
 *
 * Measure: perf stat -e cache-misses,L1-dcache-load-misses ./correct
 */
#include "../_c_common/bench.h"

#define N_PARTICLES (16u * 1024u * 1024u)  /* same particle count as incorrect.c */
#define PASSES      3                      /* same number of additions */

int main(void)
{
    printf("CORRECT: SoA, %u particles x %d passes (x array is %zu B/element)\n",
           N_PARTICLES, PASSES, sizeof(float));
    fflush(stdout);

    /*
     * The cold fields still exist -- SoA splits the layout, it does not drop data.
     * They are simply in their own arrays, so the x loop never pays to fetch them.
     */
    float *x    = malloc((size_t)N_PARTICLES * sizeof(float));
    float *y    = malloc((size_t)N_PARTICLES * sizeof(float));
    float *z    = malloc((size_t)N_PARTICLES * sizeof(float));
    float *mass = malloc((size_t)N_PARTICLES * sizeof(float));
    int   *id   = malloc((size_t)N_PARTICLES * sizeof(int));
    if (!x || !y || !z || !mass || !id) { perror("malloc"); return 1; }

    /* Identical values to incorrect.c, so the sums must match exactly. */
    for (uint32_t i = 0; i < N_PARTICLES; i++) {
        x[i] = (float)(i & 0x3FF);
        y[i] = 1.0f; z[i] = 2.0f; mass[i] = 3.0f;
        id[i] = (int)i;
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
            /* 4-byte stride: these four loads share one cache line. */
            a0 += x[i + 0]; a1 += x[i + 1];
            a2 += x[i + 2]; a3 += x[i + 3];
        }
    float sum = a0 + a1 + a2 + a3;

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(sum);

    printf("sum=%.1f  useful=%.0f MB/s\n", (double)sum,
           (double)N_PARTICLES * sizeof(float) * PASSES / elapsed / (1024.0 * 1024.0));
    printf("Done in %.3fs\n", elapsed);

    free(id); free(mass); free(z); free(y); free(x);
    return 0;
}
