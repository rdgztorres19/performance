# Avoid False Sharing and Cache Line Contention
*(Clear, Practical, From Zero)*

Design memory layouts so that data written by different threads does **not** share cache lines.  
This avoids unnecessary cache invalidations and allows real parallel execution.

---

## Executive Summary (TL;DR)

False sharing happens when **multiple threads write to different variables that live inside the same CPU cache line** (usually 64 bytes).

Even though the variables are logically independent, the CPU cache works with **cache lines**, not variables.  
When one thread writes, other cores must invalidate their copy of the entire cache line.  
The cache line then **bounces between cores**, creating hidden serialization.

This causes:
- Poor scalability
- High CPU usage with low throughput
- Performance degradation (10–50% or worse)

False sharing **does NOT break correctness**, only performance — which makes it hard to notice.

**Solution:** ensure that data frequently written by different threads lives in **different cache lines**, using:
- Per-thread data (preferred)
- Padding and alignment (when necessary)

Only apply after **profiling confirms cache contention**.

---

## 1. What Is a Cache Line? (From Zero)

The CPU does **not** load individual variables into cache.

Instead, it loads memory in **fixed-size blocks** called **cache lines**.

- Typical size: **64 bytes** (x86-64)
- Some ARM CPUs: **128 bytes**
- Minimum unit moved between:
  - RAM ↔ cache
  - Core ↔ core

> You cannot partially load or partially invalidate a cache line.  
> It is always **all or nothing**.

---

## 2. Simple Analogy (Very Important)

Think of memory like this:

- **RAM** = a large filing cabinet
- **CPU cache** = your desk
- **Cache line** = a folder

Even if you only need **one page**, you must bring **the entire folder** to your desk.

The CPU works exactly the same way.

---

## 3. How Variables End Up Sharing Cache Lines

```csharp
class Counters {
    public long A; // 8 bytes
    public long B; // 8 bytes
}
```

**Memory layout (simplified):**

```
| A (8B) | B (8B) | other data ... |
|<---------- 64 bytes (one cache line) ---------->|
```

Even though A and B are:
- Different variables
- Used by different threads

They live in the same cache line.

---

## 4. The Golden Rule (Explains Everything)

**If two threads write to anything inside the same cache line, they compete.**

It does **NOT** matter:
- That variables are different
- That there are no locks
- That the code is correct

This is false sharing.

---

## 5. Why the CPU Forces This (Cache Coherency)

Each CPU core has its own cache.

The CPU must guarantee:

> "All cores see a consistent view of memory."

To do this, it uses cache coherency protocols (e.g., MESI).

**Core rule:**
- Only one core can modify a cache line at a time
- Other cores must invalidate their copy before writing

Correct behavior — but expensive.

---

## 6. False Sharing Explained as a Timeline (Cronograma)

### Scenario Setup

- **Core 0** → Thread 0 → writes to variable `A`
- **Core 1** → Thread 1 → writes to variable `B`
- `A` and `B` share the same cache line

### Why Different Cores Have the Same Cache Line

When Thread 0 on Core 0 first reads `A`, the CPU loads the entire 64-byte cache line containing `A` (and `B`) into Core 0's L1 cache.

Later, when Thread 1 on Core 1 reads `B`, the CPU loads the same cache line (now containing both `A` and `B`) into Core 1's L1 cache.

**Both cores now have identical copies of the same cache line in their local caches.**

### Detailed Timeline

```
Time    Core 0 (Thread 0)              Core 1 (Thread 1)              Cache Line State
───────────────────────────────────────────────────────────────────────────────────────
T0      Reads A                        -                               Core 0: Exclusive (E)
                                                                        Cache line loaded from RAM
                                                                        Cost: ~100-300 cycles

T1      -                              Reads B                         Core 0: Shared (S)
                                                                        Core 1: Shared (S)
                                                                        Both have read-only copies
                                                                        Cost: ~40-100 cycles (transfer from Core 0)

T2      Writes to A                    -                               Core 0: Modified (M)
                                                                        Core 1: Invalid (I) ← INVALIDATION!
                                                                        
                                                                        Core 0 sends "invalidate" message to Core 1
                                                                        Cost: ~10-20 cycles (inter-core communication)

T3      (continues working)            Wants to write B                Core 1 detects cache line is Invalid
                                                                        Core 1 must request cache line from Core 0
                                                                        
                                                                        Request sent to Core 0: "I need this cache line"
                                                                        Cost: ~10-20 cycles (request)

T4      Receives request               (waiting...)                    Core 0 must write back to memory (if Modified)
                                                                        Core 0 sends cache line to Core 1
                                                                        Cost: ~40-100 cycles (write-back + transfer)

T5      -                              Receives cache line             Core 0: Shared (S)
                                                                        Core 1: Modified (M)
                                                                        Now Core 1 has exclusive ownership
                                                                        Cost: ~10 cycles (state update)

T6      (wants A again)                Writes to B                     Core 1: Modified (M)
                                                                        Core 0: Invalid (I) ← INVALIDATION AGAIN!
                                                                        
                                                                        Process repeats...
                                                                        Cost: ~40-100 cycles per cycle
```

### Who Controls the Cache Line Updates?

**The CPU's cache coherency protocol (MESI) controls everything automatically:**

1. **Hardware-level**: No software involvement required—it happens in CPU hardware
2. **Cache controller**: Each core has a cache controller that manages MESI states
3. **Interconnect**: Cores communicate through the CPU interconnect (bus or mesh)
4. **Snooping**: Cores "snoop" on each other's cache transactions to maintain coherency

### Cost Breakdown

**Per false sharing cycle:**
- Invalidation message: ~10-20 cycles
- Cache line request: ~10-20 cycles  
- Write-back to memory (if needed): ~10-30 cycles
- Cache line transfer: ~40-100 cycles
- State updates: ~5-10 cycles

**Total per cycle: ~75-180 CPU cycles**

If Thread 0 writes to `A` 1 million times per second, and Thread 1 writes to `B` 1 million times per second:
- **Potential cache line transfers: 2 million per second**
- **Wasted cycles: ~150-360 million cycles per second**
- **On a 3GHz CPU: 5-12% of total CPU time wasted on false sharing overhead**

### Why This Creates Serialization

Even though Thread 0 and Thread 1 are running on different cores (true parallelism), they **cannot write simultaneously** because:

1. Only one core can have the cache line in Modified state
2. The other core must wait for the transfer to complete
3. This creates **implicit serialization** at the hardware level

**Result**: What looks like parallel execution is actually serial execution with expensive synchronization.

### Visual Representation

```
Normal Parallel Execution (no false sharing):
──────────────────────────────────────────
Core 0: [Write A][Write A][Write A][Write A]...  ← Continuous
Core 1: [Write B][Write B][Write B][Write B]...  ← Continuous
        ↑ Both working simultaneously

With False Sharing:
──────────────────────────────────────────
Core 0: [Write A][----WAIT----][Write A][----WAIT----]...
Core 1: [----WAIT----][Write B][----WAIT----][Write B]...
        ↑ Taking turns (serialized!)
        ↑ Wasted cycles during WAIT
```

This hidden serialization is why performance degrades even though your code looks perfectly parallel.

---

### Common Misconceptions

**"Separate variables mean separate memory locations"**
- The CPU caches data in 64-byte chunks called cache lines. Two variables declared separately can end up on the same cache line if they're close in memory. Think of it like apartment buildings: even though you live in apartment 101 and your neighbor in 102, you share the same building (cache line).

**"Lock-free code is automatically fast"**
- Lock-free data structures avoid blocking but can suffer from false sharing when multiple threads update adjacent fields. Atomic operations still trigger cache invalidations.

**"High CPU usage means good parallelization"**
- False sharing can cause high CPU usage while destroying actual parallelism. CPUs spend time waiting for cache lines to transfer between cores, not doing useful work.

**"The compiler/runtime will optimize this away"**
- Compilers don't automatically pad structures to prevent false sharing. They optimize for single-threaded performance, not multi-threaded cache behavior.

### Why Naive Solutions Fail

**Adding locks**: Makes it worse by serializing execution completely. The problem is cache contention, not lack of synchronization.

**Increasing thread count**: More threads mean more cache invalidations (O(n²) growth). The problem compounds.

**Logical separation**: Different variables can still share cache lines if they're physically close in memory. CPU sees physical layout, not code structure.

**Algorithm optimization alone**: Doesn't help if threads are fighting over cache lines at the hardware level.

## Why This Becomes a Bottleneck

**Cache Line Ping-Pong**: Cache lines constantly bounce between cores. Each transfer consumes bandwidth and creates latency. Impact: 10-50% performance degradation.

**Serialization Despite Parallelism**: Threads appear to run in parallel but wait for each other at the cache level. Impact: Applications that should scale linearly instead plateau or degrade.

**Memory Bus Saturation**: Cache line transfers consume bandwidth, competing with actual data access. Impact: System-wide performance degradation.

**NUMA Amplification**: In NUMA systems, remote cache transfers are 2-3x slower. Impact: 50-100% additional latency.

**Scalability Collapse**: Performance degrades with more threads instead of improving. Impact: Applications fail to utilize available CPU cores.

## When to Use This Approach

**High-performance multi-threaded applications**: Many threads, high throughput requirements, processing large volumes of data.

**Frequently-updated shared state**: Per-thread counters, statistics structures, lock-free data structures with adjacent fields.

**Profiling indicates cache contention**: High cache miss rates, poor scalability, cache line transfers detected by tools.

**High-thread-count systems**: 8+ cores where false sharing effects are amplified. Especially important in NUMA systems.

**Latency-sensitive parallel workloads**: Real-time systems, financial trading, game engines, media processing.

**Lock-free algorithms**: Lock-free queues, stacks, hash tables. These are particularly susceptible to false sharing.

## When Not to Use It

**Single-threaded applications**: No parallelism means no false sharing.

**Read-only shared data**: False sharing only occurs with writes. Multiple threads reading is fine (Shared state).

**Infrequently accessed data**: Padding overhead isn't justified by occasional access.

**Small data structures**: Structures that naturally span multiple cache lines might not need explicit padding.

**Cloud/containerized environments**: Hardware topology is abstracted, cache line sizes may vary.

**Development/prototyping**: Premature optimization distracts from correctness. Profile first.

**Memory-constrained systems**: Embedded systems, mobile devices. Padding overhead might be unacceptable.

**When profiling shows no issue**: Don't optimize what isn't broken. Use tools to confirm before adding padding.

---

## How to Avoid False Sharing (General Principles)

### Strategy 1: Per-Thread Data (Preferred)

**Best approach**: Give each thread its own copy of data. No sharing = no false sharing.

**When to use**: 
- Per-thread counters, statistics, or accumulators
- Thread-local state that's aggregated later

**Benefits**:
- No padding overhead
- No false sharing (each thread has separate memory)
- Cleaner, simpler code

**Trade-off**: Must aggregate results when needed (but this is usually infrequent).

**Example pattern**:
- Each thread maintains its own counter/state
- Periodically (or at end), aggregate across all threads
- Much cheaper than constant cache line contention

### Strategy 2: Padding and Alignment

**When per-thread data isn't feasible**: Use padding to separate shared data into different cache lines.

**Key principles**:
- Ensure frequently-written variables start at cache line boundaries
- Pad each variable to at least cache line size (64 or 128 bytes)
- Use compiler directives to enforce alignment

**Benefits**:
- Works when data must be shared
- Predictable memory layout

**Trade-offs**:
- Increased memory usage (4-8x)
- More complex code
- Platform-specific (cache line sizes vary)

### Strategy 3: Separate Data Structures

**Design approach**: Design data structures so hot fields written by different threads are naturally separated.

**Principles**:
- Place head/tail pointers in separate cache lines
- Separate producer/consumer fields
- Group data by access pattern (hot vs. cold)

**Benefits**:
- Natural separation, less artificial padding
- Better overall data structure design

### Strategy 4: Reduce Write Frequency

**Optimization**: Reduce how often threads write to shared data.

**Techniques**:
- Batch updates (write every N operations instead of every operation)
- Use local accumulators, then periodically update shared state
- Prefer read-heavy patterns

**Benefits**:
- Less false sharing even if data shares cache lines
- Better cache efficiency overall

**When to use**: When you can't avoid sharing but can reduce write frequency.

### Strategy 5: Cache Line Size Awareness

**Know your platform**:
- x86-64: 64 bytes
- Some ARM: 128 bytes
- Test on target hardware

**Implementation**:
- Use constants for cache line size
- Consider padding to 128 bytes for cross-platform safety
- Document assumptions

### General Checklist

1. **Profile first**: Use `perf c2c` or VTune to identify false sharing
2. **Choose strategy**: Prefer per-thread data when possible
3. **Measure impact**: Verify improvements after changes
4. **Document decisions**: Explain why padding/alignment exists
5. **Test on target**: Different platforms have different cache line sizes

---

## How to Avoid False Sharing in C#

### Method 1: ThreadLocal<T> (Best for Per-Thread Data)

**Use when**: Each thread needs its own accumulator, counter, or state.

```csharp
using System.Threading;

public class RequestCounter {
    // Each thread gets its own counter - no sharing!
    private readonly ThreadLocal<long> _counter = new ThreadLocal<long>(() => 0);
    
    public void Increment() {
        _counter.Value++;  // Thread-local, no false sharing
    }
    
    public long GetTotal() {
        // Aggregate across all threads when needed
        long total = 0;
        // Note: ThreadLocal doesn't provide easy enumeration
        // You might need to track threads manually or use a different approach
        return total;
    }
}

// Better: Use ThreadLocal with explicit thread tracking
public class ThreadSafeCounter {
    private readonly ThreadLocal<long> _counter = new ThreadLocal<long>(() => 0);
    private readonly ConcurrentDictionary<int, long> _threadCounters = new();
    
    public void Increment() {
        _counter.Value++;
        _threadCounters[Thread.CurrentThread.ManagedThreadId] = _counter.Value;
    }
    
    public long GetTotal() {
        return _threadCounters.Values.Sum();
    }
}
```

**Why it works**: Each thread accesses completely separate memory locations. No cache line sharing possible.

**When to use**: Counters, statistics, accumulators that need per-thread isolation.

### Method 2: StructLayout with Padding (For Shared Data)

**Use when**: Data must be shared but needs cache line separation.

```csharp
using System.Runtime.InteropServices;

// Option 1: Explicit size with padding
[StructLayout(LayoutKind.Explicit, Size = 128)]  // Pad to 128 bytes (safe for 64 and 128-byte cache lines)
public struct PaddedCounter {
    [FieldOffset(0)]
    public long Value;
    // Rest is automatic padding to 128 bytes
}

public class ThreadSafeCounters {
    private readonly PaddedCounter[] _counters;
    
    public ThreadSafeCounters(int threadCount) {
        _counters = new PaddedCounter[threadCount];
    }
    
    public void Increment(int threadId) {
        Interlocked.Increment(ref _counters[threadId].Value);
    }
}

// Option 2: Manual padding fields
public class ManualPaddedCounter {
    private long _counter;
    
    // Pad to ensure next instance starts at new cache line
    // Cache line is 64 bytes, long is 8 bytes
    // Need 7 more longs (56 bytes) to reach 64 bytes total
    private long _padding1, _padding2, _padding3, _padding4,
                 _padding5, _padding6, _padding7;
    
    public long Value {
        get => _counter;
        set => _counter = value;
    }
}
```

**Why it works**: Forces each `PaddedCounter` to occupy a full cache line (128 bytes), ensuring separate cache lines.

**When to use**: Arrays of per-thread data that must be indexed by thread ID.

### Method 3: Separate Cache Lines for Lock-Free Structures

**Use when**: Building lock-free queues, stacks, or other concurrent structures.

```csharp
using System.Runtime.InteropServices;

[StructLayout(LayoutKind.Explicit, Size = 128)]
public class LockFreeQueue<T> where T : class {
    private readonly T[] _buffer;
    private readonly int _capacity;
    
    [FieldOffset(0)]
    private volatile int _head;  // Consumer writes here - first cache line
    
    // Padding to next cache line (64 bytes)
    [FieldOffset(64)]
    private volatile int _tail;  // Producer writes here - second cache line
    
    public LockFreeQueue(int capacity) {
        _capacity = capacity;
        _buffer = new T[capacity];
        _head = 0;
        _tail = 0;
    }
    
    public bool TryEnqueue(T item) {
        int currentTail = _tail;
        int nextTail = (currentTail + 1) % _capacity;
        
        if (nextTail == _head) {
            return false; // Queue full
        }
        
        _buffer[currentTail] = item;
        _tail = nextTail;  // Producer writes to separate cache line
        return true;
    }
    
    public bool TryDequeue(out T item) {
        int currentHead = _head;
        
        if (currentHead == _tail) {
            item = default(T);
            return false; // Queue empty
        }
        
        item = _buffer[currentHead];
        _buffer[currentHead] = null;
        _head = (currentHead + 1) % _capacity;  // Consumer writes to separate cache line
        return true;
    }
}
```

**Why it works**: Producer (`_tail`) and consumer (`_head`) write to different cache lines, eliminating contention.

### Method 4: Separate Arrays for Different Threads

**Use when**: You need indexed access but can separate by thread.

```csharp
// ❌ Bad: All counters in one array - false sharing
public class BadCounters {
    private readonly long[] _counters = new long[Environment.ProcessorCount];
    
    public void Increment(int threadId) {
        _counters[threadId]++;  // False sharing if elements share cache lines
    }
}

// ✅ Good: Use padded structures
public class GoodCounters {
    private readonly PaddedCounter[] _counters;
    
    public GoodCounters() {
        int threadCount = Environment.ProcessorCount;
        _counters = new PaddedCounter[threadCount];
    }
    
    public void Increment(int threadId) {
        Interlocked.Increment(ref _counters[threadId].Value);
    }
}

// Using the PaddedCounter struct from Method 2
[StructLayout(LayoutKind.Explicit, Size = 128)]
public struct PaddedCounter {
    [FieldOffset(0)]
    public long Value;
}
```

### Method 5: Reduce Write Frequency (Batching)

**Use when**: You can't avoid sharing but can reduce write frequency.

```csharp
public class BatchedCounter {
    // Thread-local accumulator
    private readonly ThreadLocal<long> _localCounter = new ThreadLocal<long>(() => 0);
    
    // Shared counter, updated less frequently
    private long _sharedCounter;
    private readonly object _lock = new object();
    private const int BATCH_SIZE = 1000;
    
    public void Increment() {
        _localCounter.Value++;
        
        // Only update shared counter every BATCH_SIZE increments
        if (_localCounter.Value % BATCH_SIZE == 0) {
            lock (_lock) {
                _sharedCounter += BATCH_SIZE;
            }
        }
    }
    
    public long GetTotal() {
        long total = _sharedCounter;
        // Add any remaining in thread-local counters
        // (simplified - in practice, you'd need to track all threads)
        return total;
    }
}
```

**Why it works**: Reduces writes to shared data by 1000x (from every increment to every 1000 increments).

### Method 6: Using Memory-Mapped or Aligned Allocation (Advanced)

**Use when**: You need precise control over memory layout.

```csharp
using System.Runtime.InteropServices;

// Note: This requires unsafe code and platform-specific implementation
public unsafe class AlignedCounter {
    private long* _counter;
    
    public AlignedCounter() {
        // Allocate aligned to cache line boundary (64 bytes)
        // This is platform-specific and requires P/Invoke or native allocation
        _counter = (long*)AlignedAlloc(64, sizeof(long));
    }
    
    private void* AlignedAlloc(ulong alignment, ulong size) {
        // Platform-specific implementation needed:
        // - Windows: _aligned_malloc
        // - Linux: posix_memalign or aligned_alloc
        // - Use DllImport or NativeMemory.AlignedAlloc (modern .NET)
        throw new NotImplementedException("Platform-specific implementation");
    }
    
    // Modern .NET alternative (if available)
    public void ModernApproach() {
        // .NET 6+ has NativeMemory.AlignedAlloc
        // IntPtr ptr = NativeMemory.AlignedAlloc((nuint)sizeof(long), 64);
    }
}
```

**Why it works**: Ensures memory starts exactly at a cache line boundary.

**When to use**: When you need guaranteed alignment and can't use `StructLayout`.

### C# Best Practices Summary

1. **Prefer ThreadLocal<T>**: Simplest and most effective for per-thread data
2. **Use StructLayout for arrays**: When you need indexed access to per-thread data
3. **Separate producer/consumer fields**: For lock-free structures, ensure 64+ bytes separation
4. **Batch updates**: Reduce write frequency when you can't avoid sharing
5. **Use constants**: Define `CACHE_LINE_SIZE = 64` or `128` as a constant
6. **Verify with tools**: Use profiling tools to confirm false sharing is fixed
7. **Document**: Add comments explaining why padding exists

### Common C# Pitfalls

**Pitfall 1: Assuming array elements are separate**
```csharp
// ❌ Bad: Array of longs - elements might share cache lines
long[] counters = new long[8];  // 8 * 8 = 64 bytes - all in one cache line!

// ✅ Good: Use padded structures
PaddedCounter[] counters = new PaddedCounter[8];  // Each is 128 bytes - separate cache lines
```

**Pitfall 2: Not using StructLayout**
```csharp
// ❌ Bad: Compiler might reorder fields
public struct Counter {
    public long Value;
    public long Padding1, Padding2, ...;  // Might not work!
}

// ✅ Good: Explicit layout
[StructLayout(LayoutKind.Explicit, Size = 128)]
public struct Counter {
    [FieldOffset(0)]
    public long Value;
}
```

**Pitfall 3: Forgetting about object headers**
```csharp
// In C#, objects have headers (overhead)
// PaddedCounter struct is fine, but arrays of objects might have additional overhead
// Prefer structs over classes for per-thread data
```

### Performance Impact in C#

Typical improvements when fixing false sharing in C#:
- **Per-thread counters**: 30-50% throughput improvement
- **Lock-free queues**: 40-60% latency reduction
- **Thread pool statistics**: 25-40% overhead reduction
- **Scalability**: Can often restore linear scaling up to 16-32 threads

---

<!-- Tags: Hardware & Operating System, CPU Optimization, Performance, Optimization, Concurrency, Threading, Memory Management, Cache Strategies, Scalability, .NET Performance, C# Performance -->

