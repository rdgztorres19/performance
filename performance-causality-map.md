# Performance Causality Map by Domain

Cause-effect map organized by **CPU**, **CPU cache**, **RAM**, **Disk I/O**, and **Network**. Each item has a **Concept** (definition), **Why** (argument), and **From** (source in the resume).

---

## 1. CPU

*Pipeline, cores, branches, scheduling, threads, lock contention.*

### Inside the CPU

**Branch misprediction → Pipeline flush & wasted cycles**
- **Concept:** *Branch misprediction* = the CPU guessed the outcome of a conditional (if/else, loop) to keep the pipeline full, but the guess was wrong. The *pipeline* is the sequence of stages (fetch, decode, execute, memory, write-back, etc.); each stage holds one *instruction* (machine code, not “lines of code”—one source line can be many instructions). There are as many instructions in flight as stages—typically **10–20**. “Full” = every stage has an instruction so every cycle one finishes and one starts (max throughput); the CPU guesses branches because otherwise it would stall fetch until the branch is resolved and stages would sit empty.
- **What’s in the pipeline:** A mix: instructions from *before* the branch (in flight) and from *after* the branch *speculatively* fetched from the guessed path. The pipeline does *not* contain only wrong-path logic; only the *speculative* part (instructions after the branch from the wrong target) is wrong—often 10–15 stages already fetched/decoded from that path.
- **Discarded & flush:** When the branch result is known, if the guess was wrong, every *speculatively* fetched wrong-path instruction is invalid. The CPU **discards** them: they are **flushed**—marked “do not commit,” so they never write to registers or memory—and fetch is **repointed** to the correct branch target. The *same* physical pipeline is used (no new pipeline): wrong-path entries are invalidated and the pipeline **refills naturally** as fetch pulls from the correct PC (we don’t “erase then fill” manually). Cost = cycles spent on wrong-path fetch/decode/execute plus refill → often **10–20+ cycles** per misprediction.
- **Why:** The CPU must guess before the condition is resolved (e.g. it may depend on a slow load). Wrong guess → discard + refill. In tight loops with unpredictable branches, this can waste a large fraction of time (e.g. 10–20%).
- **From:** Branch Prediction.

**Unpredictable branches in hot loop → More mispredictions**
- **Concept:** *Predictable* means the hardware’s history-based predictor (e.g. “this branch was taken the last 4 times”) can guess correctly most of the time. *Unpredictable* means the outcome varies in a way the predictor can’t exploit (e.g. random, or 50/50).
- **Why:** Predictors use local or global history of branch outcomes. If the branch has no stable pattern (e.g. data-dependent with random-looking data), the predictor will be wrong about half the time. Each wrong guess triggers a full pipeline flush. So in a hot loop, you pay the misprediction penalty on a large fraction of iterations, which can easily waste 10–20% of CPU time even though the “useful” work per iteration is small.
- **From:** Branch Prediction.

**Many slow cores (vs fewer fast) → More serialization at locks & bottlenecks**
- **Concept:** *Amdahl’s law* says total speedup is limited by the sequential fraction of the program. *Critical section* is the code protected by a lock; only one thread can run it at a time, so it’s effectively sequential.
- **Why:** Sequential parts and lock critical sections run at the speed of a single core. On slow cores, that single-core segment takes longer in wall-clock time, so threads hold locks longer and more threads end up waiting. In addition, if you have more threads than (slow) cores, the scheduler preempts more often, causing more context switches and more cache eviction. So you pay both: longer serial segments and more overhead from switching—both hurt throughput and latency.
- **From:** Prefer Fewer Fast CPU Cores.

### CPU and scheduling

**More runnable threads than cores → More context switches**
- **Concept:** *Context switch* is when the OS stops one thread and runs another on the same core: it saves the first thread’s state (registers, PC, etc.) and restores the second’s. *Runnable* means the thread could run if given a core.
- **Why:** The scheduler gives each runnable thread a time slice so that all make progress. If there are N runnable threads and C cores, then at least N−C threads are never running at any instant; the scheduler keeps rotating who runs. Every rotation is a context switch. So the number of switches per unit time grows with the number of runnable threads. Each switch has a fixed cost (save/restore state, scheduler logic, and often TLB/cache side effects), so total overhead grows and useful work per core goes down.
- **From:** Reduce Context Switching, Prefer Fewer Fast Cores.

**Blocking I/O (sync) → Context switches**
- **Concept:** *Blocking I/O* means the thread calls read/write and the OS puts the thread to sleep until the I/O completes; the thread does nothing and consumes no CPU while waiting.
- **Why:** When the thread blocks, it is no longer runnable. The scheduler must pick another thread to run on that core (one context switch). When the I/O completes, the blocked thread becomes runnable again; when the scheduler eventually runs it, that’s another context switch. So each blocking I/O operation causes at least two context switches. If the hot path does many blocking I/O calls, you get many switches, plus the cache on that core is now full of the “other” thread’s data when your thread resumes—so you also pay cache misses.
- **From:** Reduce Context Switching, Use Asynchronous I/O.

**Lock contention → Context switches**
- **Concept:** *Lock contention* is when several threads compete for the same lock; only one holds it at a time, so the others must wait.
- **Why:** When a thread tries to acquire a lock that is already held, it typically blocks in the kernel (e.g. on a futex/semaphore). Blocking means the thread is descheduled (context switch out). When the lock is released, the OS wakes one waiter; that thread becomes runnable and will be scheduled again (context switch in). So each time a thread blocks on a lock, you get at least two context switches. Under high contention, many threads block and wake repeatedly, so context-switch and cache-eviction overhead can dominate.
- **From:** Reduce Context Switching.

**Busy-wait (spin loop) → Wasted CPU, no yield**
- **Concept:** *Busy-wait* is a loop that repeatedly checks a condition (e.g. a flag) without giving up the core; the thread never calls a blocking primitive that would let the scheduler run someone else.
- **Why:** The thread keeps consuming 100% of its time slice checking the condition. It doesn’t yield, so other threads on the same core get less (or no) time unless the scheduler preempts it. If preempted, you still get a context switch and cache eviction; if not, you’re just burning cycles. So busy-wait doesn’t reduce context switches; it either wastes CPU in the spin or causes the same switch cost as blocking, plus it can starve other threads. Proper synchronization (e.g. event, semaphore) lets the thread sleep until the condition is true, freeing the core for useful work.
- **From:** Avoid Busy-Wait Loops.

### CPU and control flow

**Exceptions for control flow → Very high cost (100×–10000×)**
- **Concept:** *Control flow* is how execution moves (if/else, loops, returns). Using *exceptions for control flow* means using throw/catch to signal “value not found” or “parse failed” instead of returning a value or an error code.
- **Why:** Throwing an exception is not a simple jump. The runtime must allocate an exception object, capture the stack trace, and unwind the call stack until it finds a matching catch, running any finally blocks along the way. That’s heap allocation, many pointer traversals, and possibly I/O for symbol resolution. A normal return or a TryParse that returns false is a handful of instructions. So using exceptions on the hot path (e.g. inside a loop for “key not in map”) can make that path 100× to 10,000× slower than using return values or Try-patterns, and it also increases GC pressure.
- **From:** Avoid Exceptions for Control Flow.

**Compression when CPU-bound → Worse latency/throughput**
- **Concept:** *CPU-bound* means the limiting factor is CPU time, not I/O or network. *Compression* trades CPU cycles for fewer bytes to send or store.
- **Why:** Compression and decompression cost CPU. If the bottleneck is already CPU (e.g. high request rate, cores saturated), adding compression increases the work per request. You spend more time compressing and decompressing than you save on I/O or network transfer. So end-to-end latency can go up and throughput can go down. Compression pays off when the bottleneck is I/O or bandwidth and you have spare CPU; then the reduction in bytes transferred more than compensates for the extra CPU.
- **From:** Balance Compression vs I/O Cost.

### CPU ↔ other domains

- **Context switches** (CPU/scheduling) → working set evicted → **cache misses** (CPU cache).
- **Lock contention** → threads block → more **context switches** → more **cache misses**.
- **Many threads** → more stacks and scheduler work → more **RAM** and **context switches**.

---

## 2. CPU cache

*L1/L2/L3, cache lines, locality, prefetching, false sharing.*

### Cache misses

**Context switches → More cache misses**
- **Concept:** *CPU cache* is small, very fast memory on the chip (L1/L2/L3) that holds copies of recently used data from RAM. A *cache miss* is when the CPU needs data that isn’t in cache and must fetch it from RAM (or a lower-level cache), which takes many more cycles.
- **Why:** Each core’s cache holds the working set of whatever thread last ran on it. When the OS preempts that thread and runs another, the new thread’s memory accesses fill the cache and evict the previous thread’s data. When the first thread runs again (possibly on the same or another core), its data may no longer be in cache, so it suffers a burst of cache misses (often 100–300 cycles each). So context switching doesn’t only cost the switch itself; it also causes a “cold” restart for the thread’s cache footprint, which can cost much more than the switch.
- **From:** Reduce Context Switching, Thread Affinity.

**Thread migration (no affinity) → Cache cold on new core → More misses**
- **Concept:** *Thread migration* is when the scheduler moves a thread from one core to another. *Affinity* is pinning a thread to a core (or set of cores) so it doesn’t migrate.
- **Why:** The thread’s working set was hot in the old core’s L1/L2. When the thread is scheduled on a different core, that core’s caches don’t contain the thread’s data—they contain whatever ran there last. So the thread effectively starts cold: every access to its data is a cache miss until the new core has fetched it. That can mean hundreds of misses and a long stall before the thread is “warm” again. With affinity, the thread tends to run on the same core, so its data tends to stay in that core’s cache.
- **From:** Thread Affinity.

**Random / pointer-chasing access → Prefetcher can’t help → More misses**
- **Concept:** *Hardware prefetcher* watches access patterns and fetches data into cache before the program requests it. It works well with *sequential* or *stride* access (e.g. array in order, or every 8th element). *Pointer-chasing* is following pointers (e.g. linked list, tree), so the next address is not predictable from the current one.
- **Why:** Prefetchers use simple rules: “they just read address A, next they’ll read A+64” or “they’re reading with stride 8”. With random or pointer-based access, the next address depends on the *content* of memory (the pointer value), which the prefetcher doesn’t know in advance. So it can’t issue useful prefetches. Every access is effectively a surprise to the cache, so you get a cache miss (or a hit only if the line was already there from a previous access). That’s why linked-list traversal is much slower than array traversal for the same number of elements: the prefetcher helps the array, not the list.
- **From:** Optimize Memory Access Patterns.

**Poor memory layout (e.g. wrong AoS/SoA) → More cache lines loaded & bandwidth waste**
- **Concept:** *AoS* (array of structs) stores full records contiguously (e.g. [id,name,val], [id,name,val], …). *SoA* (struct of arrays) stores each field in its own array (e.g. all ids, then all names, then all values). The right choice depends on access pattern: one field only vs several fields together.
- **Why:** If you iterate over a single field (e.g. sum of `value`), AoS loads entire structs into cache—you use one field and waste the rest of each cache line. SoA loads only the value array, so each cache line is full of useful data. Conversely, if you always use several fields together (e.g. id and value), AoS keeps them in the same line (one load); with SoA you need two separate loads (two lines). So the wrong layout either loads too much (wasting cache and bandwidth) or does too many loads (again wasting bandwidth and increasing latency).
- **From:** Use Cache-Friendly Memory Layouts.

**Unaligned or scattered hot data → More cache lines & pollution**
- **Concept:** *Alignment* means data is placed at addresses that match its size (e.g. 8-byte value at address multiple of 8). *Hot data* is frequently accessed; *cold data* is rarely accessed. *Cache pollution* is when useful lines are evicted to make room for data that won’t be used again soon.
- **Why:** If a struct is unaligned or hot and cold fields are mixed, a single logical access can span two cache lines (so two loads) or pull in a full line of cold data. That wastes cache space and memory bandwidth, and can evict other hot data (pollution). Keeping hot fields together and aligned (e.g. to cache-line boundaries for heavily shared data) reduces the number of lines touched and keeps the working set smaller and hotter.
- **From:** Use Cache-Friendly Memory Layouts.

### Cache coherence and sharing

**False sharing → Cache line bouncing → Implicit serialization**
- **Concept:** *Cache line* is the unit of transfer between cache and RAM (usually 64 bytes). *False sharing* is when two threads write to *different* variables that happen to lie in the same cache line. *Coherency* (e.g. MESI) ensures all cores see a consistent view of memory by tracking and invalidating lines when they are written.
- **Why:** Coherency works per line, not per variable. When thread A writes to variable X in line L, the protocol must invalidate line L in every other core’s cache, because the protocol doesn’t know that thread B only cares about variable Y in the same line. So thread B’s next write to Y will need to fetch the line again; then A’s next write to X invalidates it for B again. The line “bounces” between cores. That causes extra bus traffic and, effectively, serialization: the two threads can’t update “their” variable in parallel because the hardware serializes updates to the whole line. Fix: put per-thread data on different cache lines (padding or separate arrays).
- **From:** Avoid False Sharing and Cache Line Contention.

**Lock-free updates on adjacent fields → False sharing**
- **Concept:** *Lock-free* code uses atomic operations (e.g. CAS) instead of locks. It doesn’t block, but it still accesses memory; if two threads update adjacent fields (e.g. two counters), they may sit in the same 64-byte line.
- **Why:** Atomic writes still participate in cache coherency. Writing one variable in a line marks the line modified and invalidates it in other caches. So even without locks, two threads updating different but adjacent fields (e.g. per-thread counters in an array) can cause the same line to bounce. Lock-free only removes blocking; it doesn’t remove false sharing. You still need to separate frequently written per-thread data by at least a cache line (or use padding) to get real parallelism.
- **From:** Avoid False Sharing.

### Cache ↔ other domains

- **Cache misses** → load waits for **RAM** → **CPU pipeline stalls**.
- **Sequential access** (good for cache) also enables **prefetching** and better **disk I/O** patterns when data comes from file.
- **Many allocations** (RAM) → more addresses touched → **cache pollution** and eviction of useful data.

---

## 3. RAM (memory)

*Virtual memory, pages, heap, GC, allocations, fragmentation, pooling.*

### Virtual memory & pages

**First touch of allocated memory → Page fault (minor or major)**
- **Concept:** *Virtual memory* gives each process its own address space; the OS maps virtual addresses to physical RAM (or swap) via *page tables*. A *page* is a fixed block (e.g. 4 KB). *Lazy allocation* means the OS doesn’t commit physical pages until the program first accesses the address. A *page fault* is the CPU trap when an access hits an unmapped or swapped page.
- **Why:** When you allocate (e.g. `new byte[1<<20]`), the OS often only reserves address space; it doesn’t assign physical pages yet. The first time you touch each page, the CPU faults, the kernel allocates a physical page, updates the page table, and resumes. So the first touch of each page has a one-time cost (minor fault if the page was never mapped; major fault if it had to be brought back from swap). For large allocations, that’s many faults on first use; if you need predictable latency, you can “warm” the memory by touching each page once after allocating.
- **From:** Avoid Page Faults.

**Working set larger than physical RAM → Major page faults (swap)**
- **Concept:** *Working set* is the set of memory pages the process is actively using. *Swap* (or page file) is disk space used as an extension of RAM; the OS moves less-used pages to swap to free physical RAM for active pages.
- **Why:** If the working set is larger than physical RAM, the OS must constantly swap: it evicts some pages to disk and brings others in when they’re accessed. Accessing a page that was swapped out causes a *major* page fault: the kernel must read the page from disk (milliseconds) and then resume. So execution is repeatedly stalled by disk I/O, and throughput/latency collapse. The fix is to reduce working set (e.g. process in chunks, stream, or use more RAM) so that the hot set fits in physical memory.
- **From:** Avoid Page Faults.

**Page faults → Context switch to kernel & possible disk I/O**
- **Concept:** Handling a page fault is kernel work: the CPU traps into the kernel, the fault handler runs (may read from disk), then returns to user mode.
- **Why:** Every page fault is a mode switch (user→kernel) and possibly a blocking disk read. So you pay CPU overhead (trap, handler, return) and, for major faults, disk latency. In a tight loop over memory that isn’t all resident, you can get many faults per second, so the process spends a significant fraction of time in the kernel or waiting on disk instead of doing useful work.
- **From:** Avoid Page Faults.

### Heap, allocations & GC

**Many small allocations in hot path → GC pressure**
- **Concept:** *GC pressure* is the load on the garbage collector: how much memory is allocated, how many objects are live, and how often the GC must run to reclaim memory.
- **Why:** Every allocation (e.g. `new T()`) is tracked by the allocator and the GC. A high allocation rate means more work for the allocator (finding free space, updating metadata) and more objects for the GC to trace and collect. Short-lived objects tend to die in Gen0, so Gen0 collections become frequent. Each collection has a cost (scanning, possibly compacting); if you allocate heavily on the hot path, that cost is paid often and can show up as latency spikes or a noticeable fraction of CPU time.
- **From:** Use Memory Pooling, Avoid Unnecessary Allocations.

**High GC pressure → More/longer GC pauses (stop-the-world)**
- **Concept:** *Stop-the-world* means the GC pauses application threads so it can walk and update the heap safely. *Pause* is the time during which your code doesn’t run.
- **Why:** To collect or compact, the GC must see a consistent view of the heap (which references point where). The simplest way is to stop all mutator threads, do the work, then resume. So more pressure (more allocations, more live data) can mean more frequent collections and/or larger heaps to scan and move, so longer pauses. Those pauses are visible as latency spikes (e.g. p99) and can reduce throughput because the app isn’t running during the pause.
- **From:** Use Memory Pooling, Avoid Heap Fragmentation.

**Heap fragmentation → More GC work & allocation failures**
- **Concept:** *Fragmentation* is when free memory is split into many small gaps between live objects. You might have enough *total* free memory but no single gap large enough for a new allocation. *Compaction* is when the GC moves objects to consolidate free space.
- **Why:** Over time, allocating and freeing objects of mixed sizes leaves holes. The allocator looks for a contiguous block of the requested size; if none exists, allocation fails (OutOfMemoryException) even though the sum of free blocks is large. The GC can compact (move objects and update references) to create large free regions, but compaction is expensive (copy, update all pointers, and often a full pause). So fragmentation increases GC work and pause time, and in the worst case causes allocation failure. Avoiding mixing small and large allocations, and using pooling for frequent allocations, reduces fragmentation.
- **From:** Avoid Heap Fragmentation.

**Closures in hot path → Display class allocations → GC pressure**
- **Concept:** A *closure* is a lambda or anonymous function that “captures” variables from the outer scope (e.g. `threshold` in `x => x > threshold`). The compiler implements this by generating a *display class*: a heap-allocated object that holds the captured variables.
- **Why:** Every time the closure is created (e.g. in a loop), the compiler may allocate a new display class instance. So a hot loop that creates a closure per iteration (e.g. `items.Where(x => x > threshold)`) can allocate as many objects as iterations. That drives allocation rate and GC pressure just like any other allocation storm. If the closure is created once outside the loop and reused, there’s only one allocation; if the lambda doesn’t capture anything, the compiler may use a single static delegate and no allocation.
- **From:** Avoid Closures in Hot Paths.

**Loading entire large file into memory → High RAM use & OOM / GC risk**
- **Concept:** *Load entire file* means reading the whole file into one or a few large buffers (e.g. `File.ReadAllBytes`, `File.ReadAllLines`) instead of streaming in chunks.
- **Why:** The heap must hold at least as much data as the file size (often more, if you parse into objects). For a file larger than available memory, you risk OutOfMemoryException. Even when it fits, one huge allocation (or many from ReadAllLines) can trigger a full GC, fragment the heap, and increase pause time. Streaming (read in fixed-size chunks, process, discard) keeps memory usage bounded and avoids these spikes.
- **From:** Stream Files Instead of Loading Entire Files.

### Pooling & layout

**Memory/array pooling → Less allocator & GC work**
- **Concept:** *Pooling* means reusing a fixed set of buffers or objects instead of allocating new ones each time. You “rent” from the pool and “return” when done.
- **Why:** Reuse means fewer allocations, so the allocator does less work and fewer objects are live. The GC has less to trace and collect, so collections are less frequent and/or shorter. For hot paths that need temporary buffers or short-lived objects, pooling can cut allocation rate dramatically (e.g. 50–90%) and reduce GC pauses and CPU time.
- **From:** Use Memory Pooling.

**Stack allocation for small buffers → No heap or GC**
- **Concept:** *Stack allocation* (e.g. `stackalloc`, `Span` over stack memory) allocates in the current method’s stack frame. When the method returns, the stack is unwound and that memory is automatically “freed” without the GC.
- **Why:** The allocator and GC never see this memory. There’s no heap allocation, no tracking, and no collection. So for small, short-lived buffers (e.g. a few hundred bytes) that don’t escape the method, stack allocation removes all allocation and GC cost. The only limit is stack size (e.g. 1 MB per thread); too large or recursive use can cause stack overflow.
- **From:** Use Stack Allocation for Small Temporary Buffers.

### Correctness (RAM visibility)

**Lock-free code without proper barriers → Wrong ordering & races**
- **Concept:** *Memory barriers* (or fences) restrict how the CPU and compiler can reorder memory operations. *Acquire* ensures reads after the barrier see writes that happened before the barrier; *release* ensures writes before the barrier are visible to others before writes after. Lock-free code often relies on one thread “publishing” a value and another “reading” it; without the right barriers, the reader can see the publish flag set but still see stale data.
- **Why:** CPUs reorder loads and stores for performance. So the order in which writes become visible to other cores can differ from program order. If thread A writes `data = 42` then `ready = true`, thread B might see `ready == true` but still read `data == 0` if there’s no acquire/release semantics. That’s a data race and wrong results. Barriers (or operations that imply them, like volatile read/write or Interlocked) enforce the needed ordering so that “see ready” implies “see all prior writes including data.”
- **From:** Use Memory Barriers for Correct Lock-Free Programming.

### RAM ↔ other domains

- **Page faults** (RAM/virtual memory) cause **kernel entry** and possibly **disk I/O** (swap) → **CPU** stalls.
- **Many allocations** → more **GC** (RAM) and **cache** (CPU cache) pollution.
- **Fragmentation** (RAM) → more **GC** work and compaction → more **CPU** and possibly more **disk**-like behavior (moving memory).

---

## 4. Disk I/O

*Sequential vs random, syscalls, buffers, fsync, files, streaming.*

### Access pattern

**Random I/O → Low throughput (HDD and SSD)**
- **Concept:** *Random I/O* is when read/write requests go to many different offsets in the file (or device) in no particular order. *Sequential I/O* is when requests go in order (e.g. byte 0, then 4K, then 8K).
- **Why:** On HDDs, each random access typically requires a seek (move the head) and rotation to the right sector—often 5–15 ms per seek. So random I/O is dominated by seek time and you get very low MB/s. On SSDs there’s no mechanical seek, but each request still has per-command overhead (FTL mapping, queueing); many small random requests don’t saturate the device’s bandwidth. Sequential I/O lets the OS and device do read-ahead and large transfers, so you get much higher throughput. So whenever you can (e.g. sort offsets, batch, or design for sequential access), you avoid the random-I/O penalty.
- **From:** Prefer Sequential I/O Over Random I/O.

**Sequential read with large buffer → Read-ahead & high throughput**
- **Concept:** *Read-ahead* is when the OS or device prefetches the next blocks while the application is still processing the current ones. *Buffer size* is how much data you request per read call.
- **Why:** Sequential access is predictable: the next read is likely at the next offset. The OS and device use that to prefetch ahead, so when your next read() arrives, the data may already be in the page cache or device cache. Larger buffers (e.g. 64–256 KB) mean fewer read syscalls for the same amount of data and better alignment with device block sizes, so you amortize per-request overhead and get closer to the device’s peak bandwidth.
- **From:** Prefer Sequential I/O, Choose Correct I/O Chunk Sizes.

### Syscalls & batching

**Many small reads/writes → Many syscalls**
- **Concept:** A *syscall* is a call from user code into the kernel (e.g. read, write). Each call has a fixed cost: user→kernel mode switch, parameter handling, and return. The *buffer* in read(buffer, size) is the destination/source for the data; *size* is how many bytes you transfer per call.
- **Why:** The cost of a syscall doesn’t depend much on how many bytes you transfer—it’s the mode switch and kernel path. So if you read 1 GB with 1 KB buffers, you do a million syscalls; with 64 KB buffers, you do about 16 thousand. The same data costs 60× more syscall overhead with small buffers. That overhead shows up as CPU time and can limit throughput even when the device could deliver more. So choosing a buffer size that matches your access pattern (and device block size) reduces syscalls and improves throughput.
- **From:** Choose Correct I/O Chunk Sizes, Use Buffered Streams.

**No buffering (e.g. raw stream, tiny buffer) → Syscall per small op**
- **Concept:** *Buffering* means accumulating many small logical reads or writes in memory and doing one or a few large syscalls when the buffer is full (or flushed).
- **Why:** If every logical write (e.g. one line of text) goes straight to the kernel, each one is a separate syscall and possibly a separate device request. Buffered streams (e.g. StreamWriter with a 4–64 KB buffer) collect many small writes in user space and only call write() when the buffer is full or flushed. So you trade a bit of latency (data sits in the buffer until flush) for far fewer syscalls and much higher throughput when you have many small operations.
- **From:** Use Buffered Streams, Use Write Batching.

**Byte-by-byte or per-item file I/O → Terrible throughput**
- **Concept:** *Per-item I/O* means one read or write call per logical item (e.g. one line, one record) instead of batching many items into one or few calls.
- **Why:** Each call has fixed overhead (syscall, often a device command). So N items mean N times that overhead. Moreover, tiny transfers don’t give the OS or device a clear sequential pattern, so read-ahead and batching at the device level don’t help. The result is that most of the time is spent in overhead, not in moving data. Batching (e.g. read 64 KB, process many items from that buffer; or buffer many writes and flush in chunks) amortizes the overhead over many items and restores throughput.
- **From:** Process Data in Batches.

### Durability & sync

**Frequent fsync → Throughput limited by sync latency**
- **Concept:** *fsync* (or Flush(true)) tells the OS to flush all dirty data for that file to stable storage and not return until the data is on disk. It’s used to guarantee *durability* (data survives a crash). *Sync latency* is the time one fsync takes (e.g. 1–10 ms HDD, 0.1–1 ms SSD).
- **Why:** Until fsync returns, your thread is blocked (or the async equivalent waits). So if you fsync after every write, the maximum number of durable writes per second is about 1 / fsync_latency (e.g. 100–1000 per second on HDD). That’s true regardless of how fast the device can do sequential writes; you’re serializing on the sync. To get higher durable throughput, you must batch: write many logical updates, then fsync once (e.g. group commit). That way throughput is limited by device bandwidth, not by sync latency.
- **From:** Avoid Frequent fsync Calls.

**Batching writes then one fsync → Much higher write throughput**
- **Concept:** *Batching* here means accumulating multiple logical writes in memory (or in a log buffer) and calling fsync once for the whole batch so that all of them become durable together.
- **Why:** One fsync flushes everything dirty for that file. So 1000 logical writes that are already in the OS page cache can be made durable with one fsync. You pay the sync latency once per batch instead of once per write. Throughput then depends on how fast you can write data into the cache and how often you fsync, not on doing one sync per small write. This is the same idea as group commit in databases: amortize the expensive sync over many transactions.
- **From:** Avoid Frequent fsync Calls, Use Write Batching.

### Files & metadata

**Many small files → Metadata overhead & inode exhaustion**
- **Concept:** The *filesystem* stores each file’s data and *metadata* (name, size, permissions, timestamps, pointers to data blocks). On Unix-style FSs, metadata lives in an *inode*; the FS has a fixed number of inodes. Creating a file allocates an inode and updates directory structures.
- **Why:** Every file creation and open involves reading/updating metadata (directories, inode allocation, block allocation). So 10 million small files mean millions of metadata operations—directory lookups, inode reads, etc. That’s slow (especially on HDDs, where metadata is scattered) and can exhaust the inode count even when there’s free disk space. Consolidating into fewer, larger files (e.g. one log per hour instead of per message) cuts metadata ops and inode use and often improves both performance and operational sanity.
- **From:** Avoid Many Small Files.

**No file preallocation → File fragmentation & allocation overhead**
- **Concept:** *Preallocation* means reserving space for the file up front (e.g. fallocate, SetLength) so the FS allocates a contiguous (or large) extent once. Without it, the file grows in small chunks and the FS allocates new blocks on demand.
- **Why:** When the file grows incrementally, the FS allocates blocks wherever it has free space. Over time, the file’s blocks can be scattered across the device (file fragmentation). Reading the file sequentially then requires many seeks (HDD) or many small I/Os (SSD), and each growth step may require metadata updates (inode, allocation structures). Preallocating reserves a contiguous region (or a few large extents) so subsequent writes stay sequential and you avoid repeated small allocations and fragmentation.
- **From:** Preallocate File Space.

**Exclusive file locks → Serialization & contention**
- **Concept:** An *exclusive lock* (e.g. FileShare.None) means only one process (or handle) can have the file open at a time; others block until the lock is released.
- **Why:** Every process that needs the file must wait for the lock. So all access is serialized: only one process at a time can do work. Throughput becomes 1/N of what a single process could do, and latency grows with the number of waiters. If the goal is concurrent reads (or controlled concurrent writes), you need a sharing mode that allows it (e.g. FileShare.Read for concurrent reads) or application-level coordination instead of exclusive file locking.
- **From:** Avoid File Locks.

### Streaming vs load-all

**Streaming (chunked read) → Bounded RAM, no full-file load**
- **Concept:** *Streaming* means reading the file in fixed-size chunks (e.g. 4–64 KB), processing each chunk, and discarding it before reading the next—so only a small amount of data is in memory at any time.
- **Why:** Memory usage is roughly the chunk size (plus any processed state you keep). So you can process files much larger than RAM without OOM. You also avoid a single huge allocation that could trigger full GC or fragmentation. The trade-off is that you can’t randomly seek cheaply (you’d have to read from the start or maintain an index); for sequential processing (logs, CSV, etc.) streaming is the right fit.
- **From:** Stream Files Instead of Loading Entire Files.

### Disk I/O ↔ other domains

- **Blocking disk read** → thread blocks → **context switch** (**CPU**/scheduling).
- **Small I/O buffers** → more **syscalls** → more **CPU** (mode switches) and **kernel** work.
- **Random I/O** and **many small files** → more **CPU** and device time per byte.

---

## 5. Network

*Connections, pooling, RTT, batching, serialization, protocols.*

### Connections

**New connection per request → High latency & socket exhaustion**
- **Concept:** A *connection* (e.g. TCP) is a long-lived channel between client and server. Establishing it requires a *handshake* (TCP: SYN, SYN-ACK, ACK; TLS: certificate exchange, key agreement). *Socket exhaustion* is when the client runs out of available ephemeral ports or file descriptors because too many connections were opened and not closed.
- **Why:** Each new connection pays at least one RTT for TCP and often one or two more for TLS. So the first byte of your request is delayed by that handshake on every request if you don’t reuse connections. In addition, each connection uses a socket (and an ephemeral port on the client). Creating and closing many connections per second can exhaust the port pool or descriptor limit, leading to “cannot assign requested address” or similar. Reusing connections (pooling) avoids repeated handshakes and keeps the number of open connections bounded.
- **From:** Use Connection Pooling.

**Connection pooling (reuse) → Lower latency & no socket exhaustion**
- **Concept:** *Connection pooling* keeps a set of already-open connections to a given host (or database) and hands one to the application when it needs to send a request; when the request is done, the connection is returned to the pool instead of closed.
- **Why:** The first time you need a connection, the pool may create one (you pay the handshake once). Later requests get an existing connection, so there’s no handshake delay—latency is just request + response. The pool also limits the total number of connections (e.g. max 10 per host), so you never open thousands of short-lived connections and you avoid socket exhaustion. Throughput improves because you’re not spending time and ports on connect/disconnect.
- **From:** Use Connection Pooling.

**Keep-alive (HTTP) → Connections stay open → Pooling useful**
- **Concept:** *Keep-alive* (HTTP/1.1 persistent connections) means the server doesn’t close the TCP connection after sending the response; the client can send another request on the same connection. Without it, each request would need a new connection.
- **Why:** If the server closed the connection after every response, the client would have to open a new TCP (and TLS) connection for every request. There would be nothing to pool: every request would pay the full handshake cost. With keep-alive, the connection stays open after the response, so the client’s connection pool can hold it and reuse it for the next request. So keep-alive is what makes HTTP connection pooling possible; pooling is the application-level use of those kept-alive connections.
- **From:** Use Connection Pooling (Keep-Alive Behind the Scenes).

### Round-trips & batching

**One network request per item (no batching) → RTT dominates**
- **Concept:** *RTT* (round-trip time) is the time for a packet to go from client to server and the response to come back. Every separate request pays at least one RTT before the response can start. *Batching* means sending multiple logical operations (e.g. 100 item IDs) in one request and getting multiple results in one response.
- **Why:** If you need N items and do one request per item, you pay N round-trips (plus N times any per-request overhead). So total latency is at least N × RTT plus server time. On a 50 ms RTT link, 100 items mean at least 5 seconds of round-trip time alone. Throughput is capped at roughly 1/RTT requests per second per connection. So when RTT is significant, “one request per item” makes latency and throughput dominated by the network, not by server capacity. Batching (one request for N items) cuts round-trips to one (or a few), so latency and throughput can improve by a factor of N or close to it.
- **From:** Batch Network Requests.

**Batch request (N items in one call) → One RTT for N items**
- **Concept:** A *batch API* accepts multiple items (e.g. a list of IDs) in one request and returns multiple results (e.g. list of records) in one response. One round-trip carries all of them.
- **Why:** You pay one RTT and one set of request/response overhead for N items instead of N. So total time is roughly 1× RTT + (server time to process N items) + (transfer time for the larger payload). When RTT is large (e.g. cross-region, mobile), that’s often 5×–50× better than N separate requests. The trade-off is that you need an API that supports batching and you may add a bit of latency for the first item (waiting for the batch to be built or sent).
- **From:** Batch Network Requests.

### Serialization & payload

**JSON for high-throughput / large payloads → CPU & bandwidth**
- **Concept:** *JSON* is a text format: human-readable but verbose (repeated key names, numbers as text). *Serialization* is turning in-memory objects into bytes to send; *deserialization* is the reverse. Both cost CPU and the resulting payload size affects network transfer time.
- **Why:** JSON repeats key names for every object (e.g. `"id":1,"name":"a"` for each of 1000 items). So the payload is larger than a compact binary encoding. Larger payload means more bytes to send and receive, so more bandwidth and more transfer time. Parsing and generating JSON also cost CPU (string handling, number conversion). At high message rate or with large payloads, serialization/deserialization can become a CPU bottleneck and the extra bytes can saturate the network. Binary formats (e.g. protobuf) reduce both size and CPU and often give 3–10× improvement in throughput and latency for the same data.
- **From:** Use Binary Protocols.

**Binary protocol (e.g. protobuf) → Smaller payload & less CPU**
- **Concept:** *Binary protocol* encodes data as compact bytes (e.g. field number + value, no key strings). *Protocol Buffers* uses a schema (`.proto`) so both sides know the encoding; the encoder/decoder does simple copies and arithmetic, not string parsing.
- **Why:** Field identity is a small integer, not a string, so the payload doesn’t repeat key names. Numbers are stored in compact binary form. Encoding and decoding are mostly memory copies and simple logic, so CPU cost is much lower than JSON parse/serialize. The result is smaller messages (often 50–80% smaller) and 3–10× faster encode/decode. That reduces network time and CPU load, so you can handle higher message rate and get lower latency when the bottleneck is CPU or network.
- **From:** Use Binary Protocols.

**Compression on fast LAN / when CPU-bound → Can hurt**
- **Concept:** *Compression* (e.g. gzip) reduces bytes on the wire at the cost of CPU to compress and decompress. It’s a trade-off: save network time, spend CPU time.
- **Why:** On a fast LAN, transfer time for even a large payload may be small (e.g. milliseconds). Adding compression adds CPU work (often more on the sender and receiver than the time saved on the wire). If the system is already CPU-bound (high request rate, cores saturated), compression adds load and can increase latency and reduce throughput. Compression helps when the bottleneck is bandwidth (e.g. slow or metered link) and you have spare CPU; then the reduction in bytes more than pays for the CPU cost.
- **From:** Balance Compression vs I/O Cost.

### Network ↔ other domains

- **New connection per request** → more **CPU** (handshakes) and more **RAM** (socket buffers, TLS state).
- **Blocking network I/O** → **context switch** (**CPU**/scheduling).
- **Large payloads / JSON** → more **CPU** (serialization) and **network** bandwidth.

---

## Cross-domain summary

| Domain      | Main causes → effects |
|------------|------------------------|
| **CPU**    | Many threads > cores → context switches. Blocking I/O / locks → context switches. Branch misprediction → pipeline flush. Exceptions on hot path → huge cost. Busy-wait → wasted CPU. |
| **CPU cache** | Context switches / thread migration → cache misses. Random access → prefetcher useless → misses. False sharing → line bouncing → serialization. Poor layout → more lines loaded. |
| **RAM**    | Many allocations → GC pressure → pauses. Fragmentation → more GC, allocation failures. Closures → allocations. Page faults → kernel + possible disk. Lock-free without barriers → wrong ordering. |
| **Disk I/O** | Random I/O → low throughput. Many small ops → many syscalls. Frequent fsync → throughput capped. Many small files → metadata/inode cost. No preallocation → fragmentation. No buffering → syscall per op. |
| **Network** | New connection per request → latency + socket exhaustion. One request per item → RTT dominates. JSON at high throughput → CPU & bandwidth. |

### Chain examples across domains

1. **Blocking disk read** (Disk I/O) → thread blocks → **context switch** (CPU) → **cache eviction** (CPU cache) → **cache misses** when thread resumes.
2. **Many allocations** (RAM) → **GC pressure** → **pauses** (CPU idle) and **cache pollution** (CPU cache).
3. **No connection pooling** (Network) → new TCP+TLS per request → more **CPU** (handshakes) and **socket exhaustion** (RAM/OS).
4. **Many small writes** (Disk I/O) → many **syscalls** (CPU mode switches) and **device ops** (Disk I/O).
5. **Context switches** (CPU) → **working set evicted** (CPU cache) → **cache misses** → **memory wait** (RAM) → **pipeline stalls** (CPU).

Use this map to trace from a symptom (e.g. high CPU, many cache misses, slow disk, high latency) back to causes in the right domain (CPU, CPU cache, RAM, disk, network) and across them.
