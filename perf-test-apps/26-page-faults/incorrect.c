/*
 * INCORRECT: touches 1 GB of fresh pages inside the timed loop, taking a fault each time.
 * Resume: "Avoid Page Faults: Keep Working Set in Physical Memory"
 *
 * mmap only reserves virtual address space: no physical frame is attached until a
 * page is first touched. That first store traps into the kernel as a minor fault,
 * which must find a free frame, zero it (the kernel may never leak another
 * process's data), install a PTE, and return -- thousands of cycles for a write
 * that would otherwise cost a few nanoseconds. The timed region here pays that
 * kernel round trip once per page, so it measures fault handling, not memory speed.
 *
 * Measure: perf stat -e page-faults,minor-faults ./incorrect   (or /usr/bin/time -v ./incorrect)
 */
#include "../_c_common/bench.h"
#include <sys/mman.h>
#include <unistd.h>

#define REGION_BYTES (1024ull * 1024ull * 1024ull)   /* 1 GB of address space */

int main(void)
{
    size_t page = (size_t)sysconf(_SC_PAGESIZE);   /* 4 KB on x86-64, 16 KB on arm64 */
    size_t pages = REGION_BYTES / page;

    printf("INCORRECT: first touch of %llu MB inside the timed loop (%zu pages of %zu B)\n",
           REGION_BYTES / (1024ull * 1024ull), pages, page);
    fflush(stdout);

    /*
     * MAP_ANONYMOUS gives a lazily-backed zero mapping: every page below is
     * untouched, so each store in the timed loop is guaranteed to fault.
     */
    char *region = mmap(NULL, REGION_BYTES, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) { perror("mmap"); return 1; }

    double t0 = bench_now();

    /* One store per page: the minimum work that still forces a fault per page. */
    uint64_t touched = 0;
    for (size_t off = 0; off < REGION_BYTES; off += page) {
        region[off] = 1;      /* minor page fault: traps to the kernel every time */
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
