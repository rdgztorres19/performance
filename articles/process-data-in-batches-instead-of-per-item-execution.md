# Process Data in Batches Instead of Per-Item Execution

**Group multiple items together and process them as batches to reduce function call overhead, improve cache locality, and enable vectorization and parallelization optimizations.**

---

## Executive Summary (TL;DR)

Batch processing groups multiple items together and processes them in a single operation, rather than processing each item individually with separate function calls. This reduces function call overhead, improves CPU cache efficiency by keeping related data together, enables compiler optimizations like vectorization, and dramatically improves throughput for I/O and database operations (often 2-10x, sometimes 10-100x for I/O). The trade-off is increased latency for individual items (they wait for the batch to fill), higher memory usage to hold batches, and more complex code. Use batching for large-scale data processing, I/O operations, database operations, and when per-item overhead is significant. Avoid batching when latency is critical, when working with small datasets, or when items must be processed immediately.

---

## Problem Context

**What is batch processing?** Instead of processing items one at a time (per-item execution), batch processing groups multiple items together and processes them in a single operation. For example, instead of inserting 1000 database records with 1000 separate INSERT statements, you insert all 1000 records in one batch operation.

**The problem with per-item processing**: Each item requires:
- A separate function call (overhead: saving registers, stack management, parameter passing)
- Separate memory access (poor cache locality)
- Separate I/O operation (network round-trips, disk seeks)
- Separate database query (connection overhead, query parsing, result handling)

**Real-world example**: Processing 100,000 items individually might require 100,000 function calls, 100,000 database round-trips, and 100,000 cache misses. This creates massive overhead that dominates execution time, not the actual processing work.

### Key Terms Explained

**Function call overhead**: The cost of invoking a function: saving CPU registers, managing the call stack, passing parameters, and jumping to the function code. Modern CPUs minimize this, but it's still measurable, especially when called millions of times.

**Cache locality**: The principle that accessing data that's close together in memory is faster. CPUs cache memory in blocks (cache lines), so accessing nearby data means it's likely already in cache.

**Vectorization**: A CPU optimization where the processor performs the same operation on multiple data items simultaneously using SIMD (Single Instruction, Multiple Data) instructions. Instead of processing items one by one, the CPU processes 4, 8, or 16 items at once.

**SIMD (Single Instruction, Multiple Data)**: CPU instructions that operate on multiple data elements simultaneously. Modern CPUs can add 8 integers at once instead of 8 separate additions.

### Common Misconceptions

**"Batching only helps for I/O operations"**
- **Reality**: Batching helps for CPU-bound work too. It reduces function call overhead, improves cache locality, and enables vectorization. The benefits are universal.

**"Batching always improves performance"**
- **Reality**: Batching trades latency for throughput. For small datasets or latency-critical operations, per-item processing might be better. Batching also uses more memory.

---

## How It Works

### Function Call Overhead Reduction

**What happens in a function call**:
1. Save CPU registers (current state)
2. Push parameters onto the stack
3. Jump to function code
4. Execute function
5. Return value
6. Restore registers
7. Jump back to caller

**Cost**: Even optimized, this takes several CPU cycles. Modern CPUs minimize this, but it's still measurable.

**Example: Single-item function calls**

```csharp
// ❌ Bad: Single-item function calls
foreach (var x in data)
{
    ProcessItem(x);
}

void ProcessItem(int x)
{
    result += x * 2;
}
```

**What happens internally**:
- Method prologue/epilogue per item (saving/restoring registers, stack setup)
- Stack setup/teardown for each call
- Poor instruction cache locality (jumping between caller and function code repeatedly)
- CPU spends time in function call overhead, not in the actual computation

**Cost**: High call overhead relative to useful work. For simple operations like `x * 2`, the overhead can be 50-80% of total time.

**Example: Batched function call**

```csharp
// ✅ Good: Batched function call
ProcessBatch(data);

void ProcessBatch(int[] batch)
{
    for (int i = 0; i < batch.Length; i++)
    {
        result += batch[i] * 2;
    }
}
```

**What improves**:
- One function call instead of N calls
- Tight loop (CPU stays in hot code path)
- Better instruction cache reuse (CPU executes sequential instructions)
- CPU spends time in computation, not in call overhead

**Performance**: Overhead drops from 50-80% to <5%. The CPU stays in the hot loop, executing efficiently.

**Why it matters**: If each function call has 10 nanoseconds of overhead, processing 1 million items individually costs 10 milliseconds just in overhead. With batching (1000 items per batch), overhead is only 0.01 milliseconds—1000x reduction.

### Cache Locality Improvement (L1 / L2 Cache)

**What is cache locality?** Accessing data that's close together in memory is faster because CPUs cache memory in blocks (cache lines, typically 64 bytes). When you access an item, nearby items are likely already loaded into cache.

**Example: Pointer-heavy, per-item access**

```csharp
// ❌ Bad: Pointer-heavy, per-item access
foreach (var node in linkedList)
{
    sum += node.Value;
}
```

**What happens internally**:
- Each node likely on a different cache line (nodes scattered in memory)
- Frequent cache misses (data not in cache, must load from slower main memory)
- Memory latency dominates (waiting for memory loads)
- CPU prefetcher can't help (random access pattern)

**Cost**: Poor spatial locality. Cache miss rate might be 30-50%, meaning 30-50% of memory accesses wait 100-300 cycles for data from main memory.

**Example: Batched contiguous memory**

```csharp
// ✅ Good: Batched contiguous memory
for (int i = 0; i < array.Length; i++)
{
    sum += array[i];
}
```

**What improves**:
- Sequential memory access (items are adjacent in memory)
- Cache line prefetching (CPU loads upcoming cache lines automatically)
- Very low cache miss rate (often <5%)
- CPU prefetcher shines (predictable pattern enables hardware prefetching)

**Performance**: Cache miss rate drops from 30-50% to <5%. Memory access becomes 10-30x faster because data is already in cache.

**Why it helps**: Cache misses cost ~100-300 CPU cycles. Cache hits cost ~1-10 cycles. Improving cache hit rate by 10% can improve performance by 5-15%. With sequential access, improvements are often 20-50%.

### Vectorization Enablement (SIMD)

**What is vectorization?** Modern CPUs can perform the same operation on multiple data items simultaneously using SIMD (Single Instruction, Multiple Data) instructions. For example, adding 8 integers at once instead of 8 separate additions.

**Example: Scalar, per-item math**

```csharp
// ❌ Bad: Scalar, per-item math
for (int i = 0; i < a.Length; i++)
{
    c[i] = a[i] + b[i];
}
```

**What happens internally**:
- One add operation per iteration
- CPU uses scalar ALU (one operation at a time)
- SIMD units idle (hardware capable of parallel operations, but not used)
- Compiler can't vectorize (no clear pattern in per-item processing)

**Cost**: Wasted hardware capability. Modern CPUs have SIMD units that can process 4-16 elements simultaneously, but they sit idle.

**Example: Batched SIMD via System.Numerics**

```csharp
// ✅ Good: Batched SIMD via System.Numerics
using System.Numerics;

int i = 0;
for (; i <= a.Length - Vector<int>.Count; i += Vector<int>.Count)
{
    var va = new Vector<int>(a, i);
    var vb = new Vector<int>(b, i);
    (va + vb).CopyTo(c, i);
}

// Handle remainder with scalar operations
for (; i < a.Length; i++)
{
    c[i] = a[i] + b[i];
}
```

**What improves**:
- 4-16 elements processed per CPU instruction (depending on CPU: 4 ints on 128-bit, 8 ints on 256-bit, 16 ints on 512-bit)
- SIMD units fully utilized (hardware parallelism activated)
- True data-parallel execution (multiple elements processed simultaneously)

**Performance**: 4-16x speedup for vectorizable operations (arithmetic, comparisons, bitwise). Instead of processing 1 item per cycle, process 8-16 items per cycle.

**Why it helps**: Modern CPUs have wide SIMD units (AVX-512 can process 16 integers at once). Batching enables compilers to use these units, dramatically improving throughput for arithmetic-heavy operations.

### I/O Operation Batching

#### OS / I/O Syscalls (User → Kernel Transitions)

**What are syscalls?** System calls are requests from user-space applications to the operating system kernel. Each syscall involves a context switch from user mode to kernel mode, which has overhead.

**Example: Single-item file I/O**

```csharp
// ❌ Bad: Single-item I/O
foreach (var line in lines)
{
    File.AppendAllText("data.log", line + "\n");
}
```

**What happens internally** (each iteration):
- `open()` syscall (open file, kernel mode transition)
- `write()` syscall (write data, kernel mode transition)
- `close()` syscall (close file, kernel mode transition)
- One syscall per item
- User → kernel → user context switch every time
- File system metadata touched repeatedly (inode updates, directory updates)

**Cost**: Dominated by syscalls & context switches. Each syscall has overhead:
- Context switch: ~1-10 microseconds
- Kernel processing: ~1-5 microseconds
- File system metadata updates: ~1-10 microseconds

For 1000 items: 3000 syscalls × 5 microseconds = 15 milliseconds just in syscall overhead.

**Example: Batched file I/O**

```csharp
// ✅ Good: Batched I/O
var sb = new StringBuilder();

foreach (var line in lines)
{
    sb.AppendLine(line);
}

File.AppendAllText("data.log", sb.ToString());
```

**What improves**:
- Single `write()` syscall (all data written at once)
- Kernel page cache used efficiently (OS can optimize large writes)
- Minimal context switches (one user→kernel→user transition instead of thousands)
- File system metadata updated once

**Performance**: OS overhead amortized. Instead of 3000 syscalls, just 1-3 syscalls. **1000x reduction in syscall overhead**.

#### Network I/O Batching

**Example: Single send per message**

```csharp
// ❌ Bad: Single send
foreach (var msg in messages)
{
    socket.Send(msg);
}
```

**Internals**:
- Syscall per `Send()` (`send()` system call)
- TCP overhead per packet (headers, acknowledgments)
- Network stack processing per packet
- Kernel→user context switch per send

**Cost**: Each send has network stack overhead, even for small messages. TCP header overhead, acknowledgment overhead, and syscall overhead accumulate.

**Example: Batched send**

```csharp
// ✅ Good: Batched send
var buffers = messages.Select(m => new ArraySegment<byte>(m)).ToList();
socket.Send(buffers);  // Scatter-gather I/O
```

**Internals**:
- `sendmsg()` syscall with scatter-gather I/O (multiple buffers in one syscall)
- Kernel batches packets efficiently
- Network stack optimized (can combine small packets, optimize TCP flow)
- Single context switch for multiple messages

**Performance**: Network stack optimized. Kernel can batch packets, reduce overhead, and improve TCP efficiency. **10-50x improvement** depending on message size and network latency.

#### Database Operations

**Database operations**: Similar benefits. Each query has overhead:
- Network round-trip to database
- Query parsing
- Query planning
- Result serialization

**Batch database operations**: One query for 1000 items instead of 1000 queries. Overhead is paid once, not 1000 times. **20-100x improvement** for database operations.

### Synchronization / Lock Overhead Reduction

**What is synchronization overhead?** When multiple threads access shared data, they must coordinate using locks or atomic operations. Each lock acquisition/release has overhead, and frequent locking creates contention.

**Example: Lock per item**

```csharp
// ❌ Bad: Lock per item
foreach (var x in data)
{
    lock (_lock)
    {
        shared += x;
    }
}
```

**What happens internally**:
- Lock acquire/release per iteration
- Cache line bouncing between cores (lock variable shared across cores, causing cache coherency traffic)
- Potential thread contention (multiple threads waiting for the same lock)
- Context switching when threads block on lock

**Cost**: Synchronization dominates runtime. For 1000 items with 10 threads:
- 1000 lock acquisitions (each has overhead: atomic operations, cache coherency)
- Cache line ping-pong (lock variable bounces between CPU cores)
- Thread contention (threads wait for lock, reducing parallelism)

Synchronization overhead might be 50-80% of total time.

**Example: Lock once per batch**

```csharp
// ✅ Good: Lock once per batch
int local = 0;

foreach (var x in data)
{
    local += x;  // No lock needed - local variable
}

lock (_lock)
{
    shared += local;  // Single lock acquisition
}
```

**What improves**:
- Single lock acquisition (overhead paid once per batch, not per item)
- Minimal cache coherency traffic (lock acquired once, not thousands of times)
- No thread contention during computation (threads work independently on local data)
- Better CPU utilization (threads don't block waiting for locks)

**Performance**: Massive scalability gain. Synchronization overhead drops from 50-80% to <5%. Threads can work in parallel without contention, then synchronize once to update shared state.

**Why it helps**: Lock overhead is proportional to lock frequency. Reducing locks from N (per item) to N/batchSize (per batch) reduces overhead proportionally. For 1000 items in batches of 100, lock overhead drops 10x.

### Parallelization Opportunities

**Per-item processing**: Hard to parallelize efficiently because:
- Small units of work (overhead dominates)
- Synchronization overhead per item (as shown above)
- Load balancing challenges (small work units are hard to distribute evenly)

**Batch processing**: Larger units of work enable efficient parallelization:
- Batch size balances overhead vs. parallelism
- Less synchronization (one operation per batch, not per item)
- Better load balancing (larger work units distribute more evenly)

**Why it helps**: Instead of coordinating 1000 small tasks, coordinate 10 larger tasks. Less overhead, better CPU utilization.

### Allocation / GC Pressure Reduction

**What is GC pressure?** The .NET garbage collector (GC) manages memory automatically. Frequent allocations trigger GC collections, which pause application execution to reclaim memory. High allocation rates cause frequent GC pauses, hurting performance.

**Example: Allocate per item**

```csharp
// ❌ Bad: Allocate per item
foreach (var x in data)
{
    var obj = new TempObject(x);  // Allocation per item
    Process(obj);
}
```

**What happens internally**:
- Thousands of allocations (one object per item)
- GC Gen0 pressure (new objects go to generation 0)
- Frequent GC collections (GC runs when Gen0 fills up)
- GC pauses (application threads stop during collection)

**Cost**: GC becomes bottleneck. For 100,000 items:
- 100,000 allocations
- GC might run 10-50 times (depending on object size)
- Each GC pause: 1-10 milliseconds
- Total GC time: 10-500 milliseconds just in GC pauses

**Example: Batched reuse**

```csharp
// ✅ Good: Batched reuse
var buffer = new TempObject[data.Length];  // Single allocation

for (int i = 0; i < data.Length; i++)
{
    buffer[i] = new TempObject(data[i]);  // Still allocations, but predictable
}

ProcessBatch(buffer);
```

**What improves**:
- Fewer allocations (can reuse objects, or allocate in predictable patterns)
- Predictable memory layout (array allocation is contiguous, better for GC)
- Lower GC frequency (fewer allocations mean GC runs less often)
- Stable memory behavior (predictable allocation patterns help GC optimize)

**Performance**: GC pauses reduced significantly. Instead of 10-50 GC runs, might have 1-5 runs. GC time drops from 10-500ms to 1-50ms.

**Better approach: Object pooling**

```csharp
// ✅ Best: Object pooling (reuse objects)
var pool = new ObjectPool<TempObject>();

foreach (var x in data)
{
    var obj = pool.Get();  // Reuse from pool
    obj.Initialize(x);
    Process(obj);
    pool.Return(obj);  // Return to pool
}
```

**Why it helps**: Object pooling eliminates allocations entirely for temporary objects. GC pressure drops to near zero for these objects. This is especially important in high-throughput scenarios.

---

## Why This Becomes a Bottleneck

### Function Call Overhead Accumulation

**The problem**: When processing millions of items, function call overhead accumulates. Even if each call has minimal overhead (nanoseconds), millions of calls add up to significant time (seconds or minutes).

**Example**: Processing 10 million items with 20 nanoseconds overhead per call = 200 milliseconds just in overhead. The actual processing might only take 100 milliseconds, so overhead is 2x the actual work time.

**Impact**: Function call overhead becomes the dominant cost, not the actual processing. Optimizing processing code doesn't help if overhead dominates.

### Poor Cache Locality

**The problem**: Per-item processing often results in random or scattered memory access patterns. Items might be in different memory locations, causing cache misses.

**What is a cache miss?** When data isn't in CPU cache and must be loaded from slower main memory. This takes 100-300 CPU cycles instead of 1-10 cycles for cache hits.

**Impact**: High cache miss rates (30-50%) can slow down processing by 2-5x. Cache misses become a bottleneck.

### I/O Overhead Dominance

**The problem**: For I/O-bound operations (database, network, file), the overhead of each I/O operation dominates. Network round-trips, database query overhead, and disk seeks are much slower than the actual data processing.

**Example**: Reading 1000 small files individually. Each file read requires:
- Opening the file (system call overhead)
- Seeking to the right location (disk seek)
- Reading data (actual work)
- Closing the file (system call overhead)

For small files, overhead dominates. Total time might be 1000ms, but actual reading time is only 50ms. 95% of time is overhead.

**Impact**: I/O overhead becomes the bottleneck. Actual processing is fast, but overhead makes the whole operation slow.

### Lack of Optimization Opportunities

**The problem**: Per-item processing prevents compiler and CPU optimizations:
- Can't vectorize (no array processing pattern)
- Can't prefetch (random access)
- Can't parallelize efficiently (work units too small)

**Impact**: CPU and compiler can't help optimize. You're fighting against the hardware instead of working with it.

### Synchronization Overhead

**The problem**: In multi-threaded per-item processing, threads must coordinate frequently (synchronize). Each synchronization has overhead (acquiring locks, checking conditions).

**Example**: 1000 threads processing 1 item each. Each thread might need to:
- Acquire a lock (overhead)
- Update shared state (overhead)
- Release lock (overhead)

Total synchronization overhead might be 50% of total time.

**Impact**: Synchronization overhead dominates. Even with parallelism, overhead prevents efficient scaling.

---

## When to Use This Approach

**Large-scale data processing**: When processing thousands, millions, or billions of items. The overhead reduction compounds with scale.

**I/O-bound operations**: Database operations, network I/O, file I/O. Batching dramatically reduces I/O overhead (10-100x improvements common).

**CPU-bound operations with overhead**: When function call overhead or cache misses dominate. Batch processing reduces overhead and improves cache behavior.

**Throughput is more important than latency**: When processing speed matters more than individual item response time (batch processing, ETL pipelines, analytics).

**When per-item overhead is significant**: If processing each item individually has measurable overhead (function calls, I/O, synchronization), batching helps.

**Parallel processing scenarios**: When parallelizing work, batches provide better work units than individual items (less synchronization, better load balancing).

**Why these scenarios**: In all these cases, the benefits (reduced overhead, improved throughput) outweigh the costs (latency, memory, complexity). The scale or overhead makes batching worthwhile.

---

## Common Mistakes

**Batch size too large**: Choosing batch sizes that cause memory pressure or excessive latency. Start smaller (100-1000 items) and tune based on profiling.

**Batch size too small**: Batch sizes so small that overhead remains significant. Batch management overhead should be much less than per-item overhead.

**Ignoring partial batches**: Not handling the last batch (which might be smaller). Must handle edge cases.

**Not considering memory**: Large batch sizes can cause out-of-memory errors. Monitor memory usage.

**Batching latency-critical operations**: Using batching for operations that require immediate processing. This adds unacceptable latency.

**Assuming batching always helps**: Not profiling to confirm batching improves performance. Sometimes overhead is already minimal, batching doesn't help.

**Not tuning batch size**: Using arbitrary batch sizes without profiling. Optimal size depends on data size, processing time, and system resources.

**Why these are mistakes**: They waste effort, add complexity without benefits, or cause problems (memory issues, latency). Always profile and tune batch sizes based on actual measurements.

---

## Optimization Techniques

### Technique 1: Fixed Batch Size

**When**: Processing known-size datasets or when batch fill rate is predictable.

```csharp
// ✅ Good: Fixed batch size
public async Task ProcessItems(List<Item> items, int batchSize = 100)
{
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize).ToList();
        await ProcessBatchAsync(batch);
    }
}
```

**Why it works**: Simple, predictable, easy to tune. Good starting point.

**Trade-off**: Might not be optimal if item processing times vary significantly.

### Technique 2: Time-Based Batching

**When**: Items arrive continuously and you want to limit latency.

```csharp
// ✅ Good: Time-based batching (process batch every N ms)
public class TimeBasedBatcher<T>
{
    private readonly List<T> _buffer = new List<T>();
    private readonly TimeSpan _interval;
    private readonly Func<List<T>, Task> _processor;
    
    public async Task AddItemAsync(T item)
    {
        lock (_buffer)
        {
            _buffer.Add(item);
        }
        
        // Process if buffer is large enough
        if (_buffer.Count >= _maxBatchSize)
        {
            await ProcessBatch();
        }
    }
    
    // Background task processes batches on interval
    private async Task ProcessBatch()
    {
        List<T> batch;
        lock (_buffer)
        {
            batch = _buffer.ToList();
            _buffer.Clear();
        }
        
        if (batch.Count > 0)
        {
            await _processor(batch);
        }
    }
}
```

**Why it works**: Limits latency (items processed within time window) while still batching.

**Trade-off**: More complex, requires background processing.

### Technique 3: Adaptive Batch Sizing

**When**: Optimal batch size varies based on conditions (data size, system load).

```csharp
// ✅ Good: Adaptive batch size based on item size
public async Task ProcessItems(List<Item> items)
{
    int batchSize = CalculateOptimalBatchSize(items);
    
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize).ToList();
        await ProcessBatchAsync(batch);
        
        // Adjust based on performance
        batchSize = AdjustBatchSize(batchSize, processingTime);
    }
}
```

**Why it works**: Adapts to conditions, can optimize dynamically.

**Trade-off**: Complex, requires monitoring and adjustment logic.

### Technique 4: Parallel Batch Processing

**When**: Batches can be processed independently in parallel.

```csharp
// ✅ Good: Process batches in parallel
public async Task ProcessItems(List<Item> items, int batchSize = 100)
{
    var batches = items
        .Select((item, index) => new { item, index })
        .GroupBy(x => x.index / batchSize)
        .Select(g => g.Select(x => x.item).ToList())
        .ToList();
    
    // Process all batches in parallel
    await Task.WhenAll(batches.Select(batch => ProcessBatchAsync(batch)));
}
```

**Why it works**: Combines batching benefits with parallelism. Reduces overhead and utilizes multiple CPU cores.

**Trade-off**: Requires more memory (all batches in memory), might overwhelm downstream systems.

### Technique 5: Database Batch Operations

**When**: Inserting, updating, or querying database records.

```csharp
// ✅ Good: Batch database inserts
public async Task InsertItemsBatch(List<Item> items, int batchSize = 1000)
{
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize);
        
        // Use parameterized batch query
        await connection.ExecuteAsync(
            "INSERT INTO Items (Id, Name, Value) VALUES (@Id, @Name, @Value)",
            batch);
    }
}

// ✅ Better: Use bulk insert APIs when available
public async Task BulkInsertItems(List<Item> items)
{
    using var bulkCopy = new SqlBulkCopy(connection);
    bulkCopy.DestinationTableName = "Items";
    await bulkCopy.WriteToServerAsync(ToDataTable(items));
}
```

**Why it works**: Reduces database round-trips dramatically. One query for many items instead of many queries.

**Performance**: 20-100x improvement for database operations.

### Technique 6: SIMD Vectorization for Arithmetic Operations

**When**: Performing arithmetic operations (add, multiply, compare) on arrays of numeric data.

```csharp
// ❌ Bad: Scalar operations
for (int i = 0; i < a.Length; i++)
{
    c[i] = a[i] + b[i];  // One add per iteration
}

// ✅ Good: SIMD vectorization
using System.Numerics;

int i = 0;
// Process in chunks using SIMD
for (; i <= a.Length - Vector<int>.Count; i += Vector<int>.Count)
{
    var va = new Vector<int>(a, i);  // Load 8-16 integers at once
    var vb = new Vector<int>(b, i);
    (va + vb).CopyTo(c, i);  // Add 8-16 integers simultaneously
}

// Handle remainder with scalar operations
for (; i < a.Length; i++)
{
    c[i] = a[i] + b[i];
}
```

**Why it works**: `Vector<T>` uses CPU SIMD instructions (SSE, AVX, AVX-512) to process multiple elements simultaneously. Instead of 1 operation per cycle, process 4-16 operations per cycle.

**Performance**: 4-16x speedup for arithmetic operations, depending on CPU capabilities:
- SSE (128-bit): 4 integers at once
- AVX (256-bit): 8 integers at once
- AVX-512 (512-bit): 16 integers at once

**When to use**: Large arrays with simple arithmetic operations (add, subtract, multiply, compare). The compiler can also auto-vectorize simple loops, but explicit `Vector<T>` gives more control.

---

## Example Scenarios

### Scenario 1: Database Batch Inserts

**Problem**: Inserting 100,000 records one by one is slow (100,000 database round-trips).

**Solution**: Batch inserts (1000 records per batch = 100 round-trips).

```csharp
// ❌ Bad: Individual inserts
public async Task InsertItemsBad(List<Item> items)
{
    foreach (var item in items)
    {
        await connection.ExecuteAsync(
            "INSERT INTO Items (Id, Name) VALUES (@Id, @Name)",
            item);  // 100,000 database round-trips!
    }
}

// ✅ Good: Batch inserts
public async Task InsertItemsGood(List<Item> items, int batchSize = 1000)
{
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize);
        await connection.ExecuteAsync(
            "INSERT INTO Items (Id, Name) VALUES (@Id, @Name)",
            batch);  // Only 100 round-trips
    }
}
```

**Performance**: 50-100x improvement. From minutes to seconds.

### Scenario 2: Image Processing

**Problem**: Processing images one by one has function call overhead and poor cache locality.

**Solution**: Process images in batches to enable vectorization and improve cache behavior.

```csharp
// ❌ Bad: Per-image processing
public void ProcessImagesBad(List<Image> images)
{
    foreach (var image in images)
    {
        ProcessImage(image);  // Function call overhead per image
    }
}

// ✅ Good: Batch processing
public void ProcessImagesGood(List<Image> images, int batchSize = 10)
{
    for (int i = 0; i < images.Count; i += batchSize)
    {
        var batch = images.Skip(i).Take(batchSize).ToList();
        ProcessBatch(batch);  // Better cache locality, can vectorize
    }
}
```

**Performance**: 3-5x improvement from cache locality and vectorization.

### Scenario 3: Network API Calls

**Problem**: Making individual API calls has network round-trip overhead.

**Solution**: Batch API calls or use batch endpoints.

```csharp
// ❌ Bad: Individual API calls
public async Task ProcessItemsBad(List<Item> items)
{
    foreach (var item in items)
    {
        await apiClient.ProcessItemAsync(item);  // Network round-trip per item
    }
}

// ✅ Good: Batch API calls
public async Task ProcessItemsGood(List<Item> items, int batchSize = 100)
{
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize).ToList();
        await apiClient.ProcessBatchAsync(batch);  // One round-trip per batch
    }
}
```

**Performance**: 10-50x improvement depending on network latency.

---

## Summary and Key Takeaways

Batch processing groups multiple items together and processes them in single operations, reducing function call overhead, improving cache locality, enabling vectorization, and dramatically improving I/O throughput. The trade-off is increased latency for individual items, higher memory usage, and more complex code.

**Core Principle**: Process multiple items together to amortize overhead and enable optimizations. Overhead reduction compounds with scale.

**Main Trade-off**: Latency vs. throughput. Batching improves throughput (more items processed per second) but increases individual item latency (items wait for batch).

<!-- Tags: Performance, Optimization, Throughput Optimization, Database Optimization, File I/O, Networking, .NET Performance, C# Performance, Algorithms, System Design, Scalability -->
