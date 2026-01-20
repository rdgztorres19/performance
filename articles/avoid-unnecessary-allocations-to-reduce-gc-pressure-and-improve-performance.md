# Avoid Unnecessary Allocations to Reduce GC Pressure and Improve Performance

**Minimize heap allocations in your code, especially in hot paths, to reduce garbage collection overhead, improve performance by 10-50%, and create more predictable latency profiles.**

---

## Executive Summary (TL;DR)

Every time you create an object with `new` in C# (like `new List<int>()` or `new byte[1000]`), memory is allocated on the heap, which the garbage collector must track and eventually free. Avoiding unnecessary allocations, especially in frequently executed code paths (hot paths), reduces GC pressure, decreases GC pause frequency, and improves performance by 10-50% in allocation-heavy code. The trade-off is potentially more complex code—you may need to reuse objects, use memory pools, prefer value types (`struct`) over reference types (`class`), or use stack allocation for small temporary buffers. Use allocation avoidance techniques in hot paths, performance-critical code, high-throughput systems, and when profiling shows many allocations. Avoid over-optimizing in non-critical code or when code simplicity is more important than performance.

---

## Problem Context

### Understanding the Basic Problem

**What are allocations?** An allocation happens when you create a new object on the heap using `new` in C#. For example, `new List<int>()`, `new byte[1000]`, or `new MyClass()` all allocate memory on the heap. The garbage collector (GC) tracks all allocated objects and eventually frees them when they're no longer used.

**The problem with allocations**: Each allocation has costs:
- **Allocation overhead**: Finding available memory, updating allocation metadata, initializing the object
- **GC tracking**: The garbage collector must track every allocated object
- **GC collections**: More allocations mean more frequent garbage collections
- **GC pauses**: When GC runs, it may pause your application threads (in some GC modes)
- **Memory pressure**: Frequent allocations increase memory usage and can cause fragmentation

**Real-world example**: Imagine a function that processes network packets. For each packet, it creates a temporary buffer:

```csharp
// This function is called 10,000 times per second
public void ProcessPacket(byte[] packet)
{
    var buffer = new byte[256]; // Allocation every call
    // Process packet into buffer
    TransformPacket(packet, buffer);
    // Buffer becomes garbage, GC must collect it
}
```

At 10,000 packets per second, this creates 10,000 allocations per second. The garbage collector must track and collect all these objects, causing frequent GC runs and performance degradation.

### Key Terms Explained (Start Here!)

Before diving deeper, let's understand the basic building blocks:

**What is the heap?** The heap is a region of memory where objects live longer than function calls. When you use `new` in C# (like `new byte[100]`), the object is allocated on the heap. The heap is managed by the garbage collector.

**What is the garbage collector (GC)?** The GC is a system that automatically manages heap memory. It identifies objects that are no longer referenced and frees their memory. GC has overhead: it must scan memory, track object lifetimes, and may pause your application to clean up.

**What is GC pressure?** The amount of work the garbage collector must do. More allocations = more GC pressure = more frequent GC pauses = worse performance.

**What is GC pause / Stop-the-world pause?** When GC runs, application threads may be paused so GC can safely analyze memory. During a pause, your application isn't processing requests, causing latency spikes.

**What is a hot path?** Code that executes frequently—like code inside loops, frequently called functions, or performance-critical sections. Optimizing hot paths provides the biggest performance gains.

**What is heap allocation?** Creating objects on the heap using `new` in C#. Heap allocations are tracked by the GC and eventually collected.

**What is stack allocation?** Allocating memory on the call stack (for local variables). Stack-allocated memory is automatically freed when the function returns—no GC needed. Use `stackalloc` in C# for small temporary buffers.

**What is memory pooling?** Reusing pre-allocated objects instead of creating new ones. Instead of `new byte[1000]` each time, you rent from a pool and return when done. This reduces allocations.

**What is allocation overhead?** The cost of creating an object—searching for free memory, updating allocation metadata, initializing the object. Even fast allocations have overhead (10-100 nanoseconds).

### Why Naive Solutions Fail

**Allocating in hot paths**: Creating objects inside loops or frequently called functions. This multiplies allocation overhead and GC pressure by the number of iterations or calls.

**Not reusing objects**: Creating new objects when existing objects could be reused (like `StringBuilder`, buffers, or collections). This creates unnecessary allocations.

**Using reference types for small objects**: Using `class` for small objects when `struct` (value type) would work. Value types don't cause heap allocations (stored on stack or inline).

**Not considering object lifetime**: Creating objects that live longer than necessary, or creating objects in hot paths when they could be created once and reused.

---

## How It Works

### Understanding Heap Allocation

**How heap allocation works** (traditional `new`):
1. Runtime searches for available memory in the managed heap
2. If found, marks memory as allocated and returns pointer
3. If not found, triggers garbage collection to free memory
4. If still not found after GC, expands heap (allocates more memory from OS)
5. Returns pointer to allocated memory
6. Initializes the object (calls constructor if applicable)
7. Registers the object with the garbage collector

**Cost of allocation**: Even fast allocations have overhead:
- Searching free memory: ~10-100 nanoseconds
- Updating allocation metadata: ~10-50 nanoseconds
- Object initialization: Depends on constructor
- GC registration: Minimal but adds up
- If GC triggered: Pause time (microseconds to milliseconds)

**Example**: Creating a `byte[1000]` array:
- Allocation: ~50-200 nanoseconds
- If called 1 million times: 50-200 milliseconds just in allocation overhead
- Plus GC must track and collect all 1 million objects

### Understanding Garbage Collection Overhead

**How GC tracks objects**: Every allocated object is registered with the GC. The GC maintains data structures to track which objects are live (still in use) and which are dead (can be freed).

**GC collection process**:
1. GC identifies live objects (still referenced)
2. GC marks dead objects (no longer referenced)
3. GC frees memory for dead objects
4. GC may compact memory (move objects to reduce fragmentation)
5. Application threads may pause during this process

**Cost of GC**: GC collections have overhead:
- **Scan time**: GC must scan all objects to find dead ones
- **Pause time**: Application threads may pause (stop-the-world pause)
- **Compaction**: Moving objects and updating references (if compaction occurs)
- **Frequency**: More allocations = more frequent GC runs

**Example**: With 10,000 allocations per second:
- GC runs every few seconds
- Each GC run pauses application for 5-50ms
- Users experience latency spikes regularly
- Reducing allocations reduces GC frequency and pauses

### Understanding Allocation Alternatives

**Stack allocation** (`stackalloc`):
- Allocates memory on the call stack instead of the heap
- Automatically freed when function returns (no GC needed)
- Fast allocation (1-5 CPU cycles, just moving a pointer)
- Limited size (< 1KB typically, stack size is limited)
- Only for temporary buffers used during function call

**Memory pooling**:
- Pre-allocates a collection of objects
- You "rent" an object when needed
- You "return" it when done (for reuse)
- Objects are reused instead of created/destroyed
- Reduces allocations dramatically (50-90% reduction typical)

**Value types** (`struct`):
- Stored directly (on stack or inline in arrays/objects)
- Don't create heap allocations
- No GC tracking needed
- Use for small objects that don't need reference semantics

---

## Why This Becomes a Bottleneck

### GC Pause Accumulation

**The problem**: Frequent allocations cause frequent GC runs. Each GC run may pause application threads. While individual pauses might be short (milliseconds), frequency matters.

**Real-world impact**: In a high-throughput server processing 10,000 requests per second, if each request allocates a 1KB buffer:
- 10,000 allocations per second = 10MB allocated per second
- GC runs every few seconds
- Each GC run pauses threads for 5-50ms
- Over time, GC pauses accumulate and become noticeable

**Example**: Processing 1 million items, each allocating a 1KB buffer:
- Without allocation avoidance: 1 million allocations → GC runs 100-1000 times → 100-1000 pauses
- With allocation avoidance: 100 allocations (reusing pools) → GC runs 1-10 times → 1-10 pauses

**Impact**: GC pauses accumulate. 1000 pauses of 10ms = 10 seconds total pause time. Reducing pauses 100x improves latency significantly.

### Allocation Overhead Accumulation

**The problem**: Each allocation has overhead (searching memory, updating metadata). When allocating millions of objects, overhead accumulates.

**Cost**: Even fast allocations (10-100 nanoseconds) add up. 1 million allocations × 100 nanoseconds = 100 milliseconds just in allocation overhead.

**Real-world impact**: In a hot path called 10 million times per second:
- 10 million allocations × 50 nanoseconds = 500 milliseconds per second
- 50% of CPU time spent on allocation overhead!

**Example**: A function that creates a `List<int>` every call:
- Allocation overhead: ~100 nanoseconds per call
- Called 1 million times: 100ms total allocation overhead
- Plus GC must track and collect 1 million lists

**Impact**: Allocation overhead becomes significant in hot paths. Reducing allocations reduces this overhead, freeing CPU cycles for actual work.

### Memory Bandwidth Saturation

**The problem**: Frequent allocations use memory bandwidth. When many allocations occur simultaneously, they compete for memory bandwidth, creating a bottleneck.

**Why this matters**: Memory bandwidth is limited. Allocation-heavy code can saturate memory bandwidth, slowing down all memory operations.

**Impact**: In multi-threaded applications, frequent allocations can cause memory bandwidth contention, slowing down all threads.

---

## When to Use This Approach

**Hot paths**: Code that executes frequently (loops, frequently called functions, performance-critical sections). Optimizing hot paths provides the biggest gains.

**High-throughput systems**: Applications processing many requests/operations per second. Reducing allocations improves scalability and performance.

**Performance-critical code**: Code where every cycle counts—games, real-time systems, high-performance computing. Allocation overhead matters in these scenarios.

**When profiling shows many allocations**: Profiling tools (like PerfView, dotMemory) show high allocation rates in hot paths. This indicates allocation avoidance would help.

**Memory-constrained systems**: Systems with limited memory. Reducing allocations reduces memory usage and GC pressure.

**Latency-sensitive applications**: Applications where latency spikes (from GC pauses) hurt user experience. Reducing allocations reduces GC pauses.

**Why these scenarios**: In all these cases, allocations directly impact performance. Reducing them provides measurable benefits (better performance, fewer GC pauses, more consistent latency).

---

## Common Mistakes

**Allocating in loops**: Creating objects inside loops. This multiplies allocations by the number of loop iterations. Move allocations outside loops when possible.

**Not reusing objects**: Creating new objects when existing objects could be reused. Use object pooling or class-level fields for reusable objects.

**Using reference types for small objects**: Using `class` for small objects when `struct` would work. Value types don't cause heap allocations.

**Ignoring profiling data**: Not measuring allocation rates before optimizing. Optimize based on data, not assumptions.

**Premature optimization**: Spending too much time avoiding allocations in non-critical code. Profile first, optimize bottlenecks.

**Not clearing object state before reuse**: When reusing objects (via pooling), forgetting to clear or reset state. This causes bugs.

**Over-optimizing**: Trying to avoid all allocations, even when they don't matter. Focus on hot paths where allocations accumulate.

**Why these are mistakes**: They either create unnecessary allocations, waste effort on non-bottlenecks, or cause bugs. Understand where allocations matter and optimize there.

---

## Optimization Techniques

### Technique 1: Move Allocations Outside Loops

**When**: You have allocations inside loops that execute frequently.

**The problem**:
```csharp
// ❌ Allocation inside loop
public void ProcessItems(List<Item> items)
{
    foreach (var item in items)
    {
        var processor = new ItemProcessor(); // Allocation every iteration
        processor.Process(item);
    }
}
```

**Problems**:
- Creates allocation for every loop iteration
- At 10,000 iterations = 10,000 allocations
- GC must track and collect all objects
- High allocation overhead and GC pressure

**The solution**:
```csharp
// ✅ Allocation outside loop
public void ProcessItems(List<Item> items)
{
    var processor = new ItemProcessor(); // One allocation
    foreach (var item in items)
    {
        processor.Process(item); // Reuse same object
    }
}

// ✅ Better: Use struct if processor is stateless
public struct ItemProcessor
{
    public void Process(Item item) { /* ... */ }
}

public void ProcessItems(List<Item> items)
{
    var processor = new ItemProcessor(); // Struct, no heap allocation
    foreach (var item in items)
    {
        processor.Process(item);
    }
}
```

**Why it works**: Moving allocations outside loops creates objects once instead of many times. Reusing the same object eliminates repeated allocation overhead. Using `struct` eliminates heap allocation entirely if the processor doesn't need to be shared.

**Performance**: 90-99% reduction in allocations for loops. 30-80% performance improvement in allocation-heavy loops.

### Technique 2: Reuse Objects with Class-Level Fields

**When**: You have objects that can be reused across method calls.

**The problem**:
```csharp
// ❌ Creating new objects every call
public class DataProcessor
{
    public string ProcessData(int value)
    {
        var sb = new StringBuilder(); // Allocation every call
        sb.Append("Value: ");
        sb.Append(value);
        return sb.ToString();
    }
}
```

**Problems**:
- Creates `StringBuilder` for every method call
- If called frequently, creates many allocations
- GC must track and collect all objects

**The solution**:
```csharp
// ✅ Reuse object at class level
public class DataProcessor
{
    private readonly StringBuilder _sb = new StringBuilder(); // One allocation
    
    public string ProcessData(int value)
    {
        _sb.Clear(); // Reset for reuse
        _sb.Append("Value: ");
        _sb.Append(value);
        return _sb.ToString();
    }
}
```

**Why it works**: Creating the object once at class level and reusing it eliminates repeated allocations. You must clear/reset the object before reuse to avoid bugs.

**Performance**: Eliminates allocations from hot paths. 20-50% improvement when `StringBuilder` or similar objects are created frequently.

### Technique 3: Use Memory Pooling for Temporary Buffers

**When**: You frequently need temporary arrays or buffers.

**The problem**:
```csharp
// ❌ Allocating buffer every call
public void ProcessData(byte[] input)
{
    var buffer = new byte[4096]; // Allocation every call
    // Use buffer
    ProcessBuffer(input, buffer);
}
```

**Problems**:
- Creates buffer allocation for every call
- Frequent calls = many allocations
- GC must track and collect all buffers

**The solution**:
```csharp
// ✅ Use ArrayPool for temporary buffers
using System.Buffers;

public class DataProcessor
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData(byte[] input)
    {
        var buffer = _pool.Rent(4096); // Rent from pool
        try
        {
            // Use buffer (only use buffer[0..4096])
            ProcessBuffer(input, buffer, 4096);
        }
        finally
        {
            _pool.Return(buffer); // Return to pool for reuse
        }
    }
}
```

**Why it works**: `ArrayPool` maintains a collection of pre-allocated arrays. Renting reuses existing arrays, eliminating allocations. Returning arrays to the pool makes them available for future rentals.

**Performance**: 50-90% reduction in allocations. 20-40% improvement in GC performance.

### Technique 4: Prefer Value Types (struct) for Small Objects

**When**: You have small objects that don't need reference semantics.

**The problem**:
```csharp
// ❌ Reference type causes heap allocation
public class Point
{
    public int X;
    public int Y;
}

public void ProcessPoints(List<Point> points)
{
    foreach (var point in points)
    {
        var normalized = new Point { X = point.X / 10, Y = point.Y / 10 }; // Heap allocation
        ProcessPoint(normalized);
    }
}
```

**Problems**:
- `Point` is a class, so `new Point()` allocates on heap
- In loops, creates many heap allocations
- GC must track and collect all objects

**The solution**:
```csharp
// ✅ Value type eliminates heap allocation
public struct Point
{
    public int X;
    public int Y;
}

public void ProcessPoints(List<Point> points)
{
    foreach (var point in points)
    {
        var normalized = new Point { X = point.X / 10, Y = point.Y / 10 }; // No heap allocation
        ProcessPoint(normalized);
    }
}
```

**Why it works**: Value types (`struct`) are stored directly (on stack or inline in arrays). They don't create heap allocations, so they don't require GC tracking. Use value types for small objects when you don't need reference semantics.

**Performance**: Eliminates heap allocations entirely for small objects. 25-50% improvement when many small objects are created.

### Technique 5: Use Stack Allocation for Small Temporary Buffers

**When**: You need small temporary buffers (< 1KB) that are only used during a function call.

**The problem**:
```csharp
// ❌ Heap allocation for temporary buffer
public void ProcessData(byte[] data)
{
    var buffer = new byte[256]; // Heap allocation
    // Use buffer
    ProcessBuffer(data, buffer);
    // Buffer becomes garbage, GC must collect it
}
```

**Problems**:
- Creates heap allocation for temporary buffer
- Buffer is only used during function call
- GC must track and collect buffer
- Wasteful for short-lived buffers

**The solution**:
```csharp
// ✅ Stack allocation for temporary buffer
public void ProcessData(byte[] data)
{
    Span<byte> buffer = stackalloc byte[256]; // Stack allocation
    // Use buffer
    ProcessBuffer(data, buffer);
    // Buffer automatically freed when function returns (no GC!)
}
```

**Why it works**: `stackalloc` allocates memory on the call stack instead of the heap. Stack memory is automatically freed when the function returns—no GC needed. This is perfect for small temporary buffers.

**Performance**: Eliminates allocation overhead and GC tracking. 20-40% improvement for frequently called functions with temporary buffers.

### Technique 6: Avoid String Concatenation in Hot Paths

**When**: You're building strings frequently in hot paths.

**The problem**:
```csharp
// ❌ String concatenation creates allocations
public string BuildMessage(string[] parts)
{
    string result = "";
    foreach (var part in parts)
    {
        result += part; // Creates new string each time (allocation!)
    }
    return result;
}
```

**Problems**:
- String concatenation (`+`) creates new string objects
- Each concatenation is an allocation
- In loops, creates many allocations
- GC must track and collect all intermediate strings

**The solution**:
```csharp
// ✅ Use StringBuilder (reuse or pool)
public class MessageBuilder
{
    private readonly StringBuilder _sb = new StringBuilder(); // Reuse
    
    public string BuildMessage(string[] parts)
    {
        _sb.Clear(); // Reset for reuse
        foreach (var part in parts)
        {
            _sb.Append(part); // No allocation, appends to existing buffer
        }
        return _sb.ToString(); // One allocation (final string)
    }
}

// ✅ Or use string interpolation (more efficient than +)
public string BuildMessage(int value, string name)
{
    return $"Value: {value}, Name: {name}"; // More efficient than concatenation
}
```

**Why it works**: `StringBuilder` reuses internal buffers, avoiding allocations during string building. Only the final string allocation occurs. String interpolation is also more efficient than concatenation.

**Performance**: 90-99% reduction in allocations for string building. 20-60% improvement in string-heavy hot paths.

### Technique 7: Avoid LINQ Allocations When Possible

**When**: You're using LINQ in hot paths where allocations matter.

**The problem**:
```csharp
// ❌ LINQ creates intermediate allocations
public void ProcessItems(List<Item> items)
{
    var filtered = items.Where(x => x.IsValid).ToList(); // Allocation
    var mapped = filtered.Select(x => x.Value * 2).ToList(); // Another allocation
    foreach (var value in mapped)
    {
        ProcessValue(value);
    }
}
```

**Problems**:
- LINQ operations create intermediate collections
- `ToList()` allocates a new list
- Multiple LINQ operations = multiple allocations
- GC must track and collect all collections

**The solution**:
```csharp
// ✅ Process directly without LINQ allocations
public void ProcessItems(List<Item> items)
{
    foreach (var item in items)
    {
        if (item.IsValid) // Filter inline
        {
            int value = item.Value * 2; // Map inline
            ProcessValue(value);
        }
    }
}

// ✅ If LINQ is needed, avoid ToList() when possible
public void ProcessItems(List<Item> items)
{
    // No ToList() - process enumerable directly
    foreach (var value in items
        .Where(x => x.IsValid)
        .Select(x => x.Value * 2))
    {
        ProcessValue(value); // Enumerable executes lazily, no intermediate list
    }
}
```

**Why it works**: Processing directly avoids intermediate collection allocations. LINQ enumerables execute lazily—they don't create collections until you iterate. Only `ToList()` or similar methods force immediate allocation.

**Performance**: 50-90% reduction in allocations when avoiding LINQ allocations. 15-40% improvement in LINQ-heavy hot paths.

---

## Example Scenarios

### Scenario 1: Processing Items in a Loop

**Problem**: Processing a list of items, creating objects inside the loop.

**Current code (slow)**:
```csharp
// ❌ Allocation inside loop
public void ProcessItems(List<Item> items)
{
    foreach (var item in items)
    {
        var processor = new ItemProcessor(); // Allocation every iteration
        processor.Process(item);
    }
}

// If called with 10,000 items:
// - 10,000 allocations
// - GC must track and collect all processors
// - High allocation overhead and GC pressure
```

**Problems**:
- Creates allocation for every loop iteration
- 10,000 items = 10,000 allocations
- High allocation overhead (10,000 × 100ns = 1ms)
- High GC pressure (GC must track and collect 10,000 objects)

**Improved code (faster)**:
```csharp
// ✅ Allocation outside loop
public void ProcessItems(List<Item> items)
{
    var processor = new ItemProcessor(); // One allocation
    foreach (var item in items)
    {
        processor.Process(item); // Reuse same object
    }
}

// ✅ Best: Use struct if processor is stateless
public struct ItemProcessor
{
    public void Process(Item item) { /* ... */ }
}

public void ProcessItems(List<Item> items)
{
    var processor = new ItemProcessor(); // Struct, no heap allocation
    foreach (var item in items)
    {
        processor.Process(item);
    }
}
```

**Results**:
- **Allocations**: Reduced from 10,000 to 1 (or 0 with struct)
- **Allocation overhead**: Reduced from 1ms to 0.1ms (or 0 with struct)
- **GC pressure**: Reduced dramatically (1 object or 0 objects to track)
- **Performance**: 30-80% improvement

### Scenario 2: Building Strings in Hot Paths

**Problem**: Building strings frequently using concatenation.

**Current code (slow)**:
```csharp
// ❌ String concatenation creates allocations
public string BuildLogMessage(string level, string message, int userId)
{
    return "[" + level + "] " + message + " User: " + userId.ToString();
    // Each + creates intermediate string allocations
}

// Called 10,000 times per second:
// - Multiple allocations per call
// - Many intermediate string objects created
// - High GC pressure
```

**Problems**:
- Each string concatenation (`+`) creates new string objects
- Multiple concatenations = multiple allocations
- 10,000 calls = tens of thousands of allocations
- GC must track and collect all intermediate strings

**Improved code (faster)**:
```csharp
// ✅ Use string interpolation (more efficient)
public string BuildLogMessage(string level, string message, int userId)
{
    return $"[{level}] {message} User: {userId}";
    // Single allocation for final string
}

// ✅ Better: Reuse StringBuilder for hot paths
public class Logger
{
    private readonly StringBuilder _sb = new StringBuilder(); // Reuse
    
    public string BuildLogMessage(string level, string message, int userId)
    {
        _sb.Clear(); // Reset for reuse
        _sb.Append('[');
        _sb.Append(level);
        _sb.Append("] ");
        _sb.Append(message);
        _sb.Append(" User: ");
        _sb.Append(userId);
        return _sb.ToString(); // One allocation (final string)
    }
}
```

**Results**:
- **Allocations**: Reduced dramatically (one final string instead of many intermediate strings)
- **GC pressure**: Reduced (fewer objects to track)
- **Performance**: 20-60% improvement in string-heavy hot paths
- **Throughput**: Higher (less allocation overhead)

### Scenario 3: Temporary Buffers in Network Processing

**Problem**: Processing network packets, creating temporary buffers for each packet.

**Current code (slow)**:
```csharp
// ❌ Heap allocation for each packet
public void ProcessPacket(byte[] packet)
{
    var buffer = new byte[256]; // Heap allocation
    // Process packet into buffer
    TransformPacket(packet, buffer);
    SendProcessedData(buffer);
    // Buffer becomes garbage, GC must collect it
}

// Processing 10,000 packets/second:
// - 10,000 allocations per second
// - 2.56MB allocated per second
// - GC runs frequently
```

**Problems**:
- Creates heap allocation for every packet
- 10,000 packets/second = 10,000 allocations/second
- GC must track and collect all buffers
- High GC pressure causes performance degradation

**Improved code (faster)**:
```csharp
// ✅ Stack allocation for temporary buffer
public void ProcessPacket(byte[] packet)
{
    Span<byte> buffer = stackalloc byte[256]; // Stack allocation
    // Process packet into buffer
    TransformPacket(packet, buffer);
    SendProcessedData(buffer);
    // Buffer automatically freed (no GC!)
}

// ✅ Alternative: Use ArrayPool for larger buffers
using System.Buffers;

public class PacketProcessor
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessPacket(byte[] packet)
    {
        var buffer = _pool.Rent(256); // Rent from pool
        try
        {
            TransformPacket(packet, buffer, 256);
            SendProcessedData(buffer, 256);
        }
        finally
        {
            _pool.Return(buffer); // Return to pool
        }
    }
}
```

**Results**:
- **Allocations**: Eliminated (stack allocation) or reduced by 90%+ (pooling)
- **GC pressure**: Reduced dramatically (no objects or pooled objects)
- **Performance**: 20-40% improvement in packet processing
- **Latency**: More consistent (fewer GC pauses)

---

## Summary and Key Takeaways

Avoiding unnecessary allocations, especially in hot paths, reduces garbage collection overhead, decreases GC pause frequency, and improves performance by 10-50% in allocation-heavy code. The trade-off is potentially more complex code—you may need to reuse objects, use memory pools, prefer value types, or use stack allocation. Use allocation avoidance techniques in hot paths, performance-critical code, high-throughput systems, and when profiling shows many allocations.

<!-- Tags: Memory Management, Garbage Collection, Performance, Optimization, .NET Performance, C# Performance, System Design -->
