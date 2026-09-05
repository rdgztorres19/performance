/*
 * CORRECT: pre-faults the same 1 GB before timing, so the loop takes no faults.
 * Resume: "Avoid Page Faults: Keep Working Set in Physical Memory"
 *
 * The physical frames still have to be allocated and zeroed -- that cost is not
 * magic, it is simply moved out of the measured path and paid once at startup,
 * where a warm-up belongs. By the time the timed loop runs, every page already has
 * a valid PTE, so each store is an ordinary cache/DRAM write that stays entirely in
 * user mode with no kernel entry. This is what pre-faulting buys a latency-sensitive
 * service: the same total work, but none of it inside the hot path.
 *
 * Measure: perf stat -e page-faults,minor-faults ./correct   (or /usr/bin/time -v ./correct)
 */
#include "../_c_common/bench.h"
#include <sys/mman.h>
#include <unistd.h>

#define REGION_BYTES (1024ull * 1024ull * 1024ull)   /* same 1 GB as incorrect.c */

int main(void)
{
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    size_t pages = REGION_BYTES / page;

    printf("CORRECT: %llu MB pre-faulted before timing (%zu pages of %zu B)\n",
           REGION_BYTES / (1024ull * 1024ull), pages, page);
    fflush(stdout);

    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
#ifdef __linux__
    /* MAP_POPULATE makes the kernel install every PTE up front, in one call. */
    flags |= MAP_POPULATE;
#endif

    char *region = mmap(NULL, REGION_BYTES, PROT_READ | PROT_WRITE, flags, -1, 0);
    if (region == MAP_FAILED) { perror("mmap"); return 1; }

    /*
     * Portable pre-fault, and on Linux a safety net in case MAP_POPULATE was
     * ignored (it is advisory). Touching each page here takes all the minor
     * faults OUTSIDE the timed region -- this loop is the whole technique.
     */
    for (size_t off = 0; off < REGION_BYTES; off += page)
        region[off] = 1;

    double t0 = bench_now();

    /* Identical loop to incorrect.c, but every PTE is already valid: no faults. */
    uint64_t touched = 0;
    for (size_t off = 0; off < REGION_BYTES; off += page) {
        region[off] = 1;      /* plain user-mode store, no kernel entry */
        touched++;
    }

    double elapsed = bench_now() - t0;
    BENCH_CONSUME(touched);

    printf("pages touched=%llu  %.0f ns/page\n",
           (unsigned long long)touched, elapsed * 1e9 / (double)pages);
    printf("Done in %.3fs\n", elapsed);

    munmap(region, REGION_BYTES);
    return 0;
}
