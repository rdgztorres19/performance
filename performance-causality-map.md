# Performance Causality Map by Domain

Cause-effect map organized by **CPU**, **CPU cache**, **RAM**, **Disk I/O**, and **Network**. Each section covers that domain’s causes, effects, and links to other domains.

---

## 1. CPU

*Pipeline, cores, branches, scheduling, threads, lock contention.*

### Inside the CPU

**Branch misprediction → Pipeline flush & wasted cycles**
- **Why:** CPU speculates on a branch; when the real outcome is known, all speculated instructions are discarded and the pipeline restarts. Deep pipelines (10–20 stages) make each misprediction cost many cycles.
- **From:** Branch Prediction.

**Unpredictable branches in hot loop → More mispredictions**
- **Why:** Hardware predicts using history. Random or 50/50 branches have no stable pattern → high misprediction rate → 10–20% of CPU time can be lost in penalties.
- **From:** Branch Prediction.

**Many slow cores (vs fewer fast) → More serialization at locks & bottlenecks**
- **Why:** Sequential parts (Amdahl) and lock critical sections run at the speed of one core. Slow cores hold locks longer → more waiting; more threads on slow cores → more context switching and cache eviction.
- **From:** Prefer Fewer Fast CPU Cores.

### CPU and scheduling

**More runnable threads than cores → More context switches**
- **Why:** Scheduler gives each thread a time slice; more threads than cores ⇒ more preemptions and context switches (save/restore state, scheduler work).
- **From:** Reduce Context Switching, Prefer Fewer Fast Cores.

**Blocking I/O (sync) → Context switches**
- **Why:** Thread blocks in kernel waiting for I/O → scheduler switches to another thread; when I/O completes, another switch back ⇒ at least two context switches per blocking I/O.
- **From:** Reduce Context Switching, Use Asynchronous I/O.

**Lock contention → Context switches**
- **Why:** Thread blocks waiting for a lock → scheduler switches away; when lock is released, waiter is woken and scheduled again ⇒ extra context switches.
- **From:** Reduce Context Switching.

**Busy-wait (spin loop) → Wasted CPU, no yield**
- **Why:** Thread keeps running and checking a condition, consuming full CPU. It never yields ⇒ other threads get less time; same or more context switches if preempted, plus wasted cycles.
- **From:** Avoid Busy-Wait Loops.

### CPU and control flow

**Exceptions for control flow → Very high cost (100×–10000×)**
- **Why:** Throw allocates object, captures stack trace, unwinds stack to catch. On the hot path this dominates over a simple return or TryParse.
- **From:** Avoid Exceptions for Control Flow.

**Compression when CPU-bound → Worse latency/throughput**
- **Why:** Compression adds CPU work. If CPU is already the bottleneck, extra work increases latency and reduces throughput.
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
- **Why:** When the OS preempts a thread, another runs on that core and uses the cache; its working set evicts the previous thread’s data. When the first thread runs again, its data may be gone → cache miss (100–300 cycles each).
- **From:** Reduce Context Switching, Thread Affinity.

**Thread migration (no affinity) → Cache cold on new core → More misses**
- **Why:** Thread’s data was hot in the old core’s cache. After migration, the new core’s cache doesn’t have it → cold start until data is fetched again.
- **From:** Thread Affinity.

**Random / pointer-chasing access → Prefetcher can’t help → More misses**
- **Why:** Prefetchers rely on sequential or stride patterns. Random or pointer-based access has no predictable pattern → no useful prefetch → more misses.
- **From:** Optimize Memory Access Patterns.

**Poor memory layout (e.g. wrong AoS/SoA) → More cache lines loaded & bandwidth waste**
- **Why:** Data for one logical item spread across memory → each field may be in a different cache line → more lines loaded and more bandwidth for the same logical access.
- **From:** Use Cache-Friendly Memory Layouts.

**Unaligned or scattered hot data → More cache lines & pollution**
- **Why:** Structure spans multiple lines or hot/cold data mixed → each access pulls in unused data → more misses and cache pollution.
- **From:** Use Cache-Friendly Memory Layouts.

### Cache coherence and sharing

**False sharing → Cache line bouncing → Implicit serialization**
- **Why:** Two threads write different variables in the same 64-byte cache line. Coherency (e.g. MESI) works per line: one core’s write invalidates the line in others → line bounces between cores → effective serialization and extra bus traffic.
- **From:** Avoid False Sharing and Cache Line Contention.

**Lock-free updates on adjacent fields → False sharing**
- **Why:** Lock-free structures often use atomics; if per-thread fields sit in the same cache line, atomic writes still trigger line invalidations and bouncing.
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
- **Why:** OS often uses lazy allocation; physical pages are committed on first access. First access triggers a page fault and mapping (or load from swap).
- **From:** Avoid Page Faults.

**Working set larger than physical RAM → Major page faults (swap)**
- **Why:** OS swaps out less-used pages. Access to a swapped page triggers a major fault → load from disk (ms of latency) → huge stall vs normal RAM access.
- **From:** Avoid Page Faults.

**Page faults → Context switch to kernel & possible disk I/O**
- **Why:** Fault is handled in kernel; that implies kernel entry and possibly reading from **disk** (swap), then resume → latency and CPU overhead.
- **From:** Avoid Page Faults.

### Heap, allocations & GC

**Many small allocations in hot path → GC pressure**
- **Why:** Each allocation is tracked; high rate ⇒ more allocator work and more objects to trace ⇒ more frequent collections (especially Gen0).
- **From:** Use Memory Pooling, Avoid Unnecessary Allocations.

**High GC pressure → More/longer GC pauses (stop-the-world)**
- **Why:** GC often pauses mutator threads to get a consistent heap view. More pressure ⇒ more or longer pauses ⇒ latency spikes and throughput drops.
- **From:** Use Memory Pooling, Avoid Heap Fragmentation.

**Heap fragmentation → More GC work & allocation failures**
- **Why:** Free memory in small, non-contiguous blocks ⇒ allocator and GC work harder; large allocations can fail (OutOfMemoryException) even with enough total free memory.
- **From:** Avoid Heap Fragmentation.

**Closures in hot path → Display class allocations → GC pressure**
- **Why:** Lambdas that capture variables become heap-allocated objects. In a hot loop ⇒ many allocations ⇒ more GC pressure and cache pollution.
- **From:** Avoid Closures in Hot Paths.

**Loading entire large file into memory → High RAM use & OOM / GC risk**
- **Why:** One or few huge allocations (e.g. ReadAllBytes/ReadAllLines) tie up heap, can trigger full GC, and can cause OOM if file is larger than available memory.
- **From:** Stream Files Instead of Loading Entire Files.

### Pooling & layout

**Memory/array pooling → Less allocator & GC work**
- **Why:** Reusing buffers/objects reduces allocation rate and number of live objects ⇒ fewer collections and shorter pauses.
- **From:** Use Memory Pooling.

**Stack allocation for small buffers → No heap or GC**
- **Why:** stackalloc and Span use the stack; when the method returns, memory is freed without GC involvement ⇒ no allocation or collection cost.
- **From:** Use Stack Allocation for Small Temporary Buffers.

### Correctness (RAM visibility)

**Lock-free code without proper barriers → Wrong ordering & races**
- **Why:** CPUs reorder memory operations. Without acquire/release or full barriers, one thread can observe another’s writes in the wrong order (e.g. “ready” before “data”) ⇒ data races.
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
- **Why:** HDD: each random op implies a seek (5–15 ms). SSD: no seek but per-request overhead and FTL work; many small random ops don’t use full bandwidth. Sequential I/O allows read-ahead and streaming ⇒ much higher MB/s.
- **From:** Prefer Sequential I/O Over Random I/O.

**Sequential read with large buffer → Read-ahead & high throughput**
- **Why:** OS and device detect sequential pattern → prefetch next blocks while you consume current ones ⇒ better use of disk bandwidth.
- **From:** Prefer Sequential I/O, Choose Correct I/O Chunk Sizes.

### Syscalls & batching

**Many small reads/writes → Many syscalls**
- **Why:** Each read/write is a syscall (user→kernel mode switch). Small buffer ⇒ more syscalls for the same data ⇒ mode-switch and kernel overhead dominate.
- **From:** Choose Correct I/O Chunk Sizes, Use Buffered Streams.

**No buffering (e.g. raw stream, tiny buffer) → Syscall per small op**
- **Why:** Every small write or read crosses to kernel. Buffered streams batch many small ops into fewer, larger syscalls.
- **From:** Use Buffered Streams, Use Write Batching.

**Byte-by-byte or per-item file I/O → Terrible throughput**
- **Why:** One syscall (and often one device op) per byte or per item ⇒ overhead dominates; no read-ahead or bulk transfer.
- **From:** Process Data in Batches.

### Durability & sync

**Frequent fsync → Throughput limited by sync latency**
- **Why:** Each fsync waits until data is on stable storage (e.g. 1–10 ms HDD, 0.1–1 ms SSD). fsync per write ⇒ max throughput ≈ 1/fsync_latency (e.g. ~100–1000 ops/sec) regardless of device peak bandwidth.
- **From:** Avoid Frequent fsync Calls.

**Batching writes then one fsync → Much higher write throughput**
- **Why:** Amortize fsync cost over many logical writes; one sync commits a whole batch ⇒ throughput limited by device bandwidth, not by sync latency.
- **From:** Avoid Frequent fsync Calls, Use Write Batching.

### Files & metadata

**Many small files → Metadata overhead & inode exhaustion**
- **Why:** Each file needs inode, directory entry, allocation metadata. Many files ⇒ many metadata ops and risk of exhausting inodes even with free space.
- **From:** Avoid Many Small Files.

**No file preallocation → File fragmentation & allocation overhead**
- **Why:** File grows in small chunks; filesystem allocates blocks on demand ⇒ non-contiguous blocks ⇒ more seeks on read/write and repeated metadata updates.
- **From:** Preallocate File Space.

**Exclusive file locks → Serialization & contention**
- **Why:** Only one process can hold the lock ⇒ all others block ⇒ throughput collapses to one-at-a-time; latency grows with queue length.
- **From:** Avoid File Locks.

### Streaming vs load-all

**Streaming (chunked read) → Bounded RAM, no full-file load**
- **Why:** Process in fixed-size chunks (e.g. 4–64 KB) ⇒ constant memory use; avoid loading entire file into **RAM** ⇒ no OOM or huge GC from one big allocation.
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
- **Why:** Each connection pays TCP handshake + TLS (+ auth). That’s one or more RTTs and CPU per request. Many short-lived connections consume ephemeral ports and can exhaust sockets.
- **From:** Use Connection Pooling.

**Connection pooling (reuse) → Lower latency & no socket exhaustion**
- **Why:** Request uses an already-open connection ⇒ no handshake for that request; pool size bounded ⇒ predictable socket usage.
- **From:** Use Connection Pooling.

**Keep-alive (HTTP) → Connections stay open → Pooling useful**
- **Why:** Without keep-alive, server closes after each response ⇒ client must open a new connection every time ⇒ nothing to pool. Keep-alive keeps TCP open so the pool can reuse connections.
- **From:** Use Connection Pooling (Keep-Alive Behind the Scenes).

### Round-trips & batching

**One network request per item (no batching) → RTT dominates**
- **Why:** Total time ≥ N × RTT plus per-request overhead. With large RTT (e.g. 50–100 ms), latency scales with N and throughput is capped by round-trips per second.
- **From:** Batch Network Requests.

**Batch request (N items in one call) → One RTT for N items**
- **Why:** One round-trip carries many items ⇒ total latency ≈ 1× RTT + server time for N items ⇒ 5×–50× better when RTT is significant.
- **From:** Batch Network Requests.

### Serialization & payload

**JSON for high-throughput / large payloads → CPU & bandwidth**
- **Why:** Text format is larger and serialize/deserialize cost is higher than compact binary. At high message rate or large payloads, **CPU** and bandwidth become bottlenecks.
- **From:** Use Binary Protocols.

**Binary protocol (e.g. protobuf) → Smaller payload & less CPU**
- **Why:** No repeated key names; compact encoding ⇒ fewer bytes and faster encode/decode than JSON ⇒ better throughput and lower **CPU** and **network** use.
- **From:** Use Binary Protocols.

**Compression on fast LAN / when CPU-bound → Can hurt**
- **Why:** Compression adds **CPU** work; on fast links or when CPU is the bottleneck, the extra CPU cost can outweigh the bandwidth saving.
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
