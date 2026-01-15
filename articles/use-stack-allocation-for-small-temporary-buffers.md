# Use Stack Allocation for Small Temporary Buffers

**Allocate small temporary arrays and buffers on the stack instead of the heap to avoid garbage collection overhead, improve performance, and reduce memory allocations in hot paths.**

---

## Executive Summary (TL;DR)

Stack allocation uses `stackalloc` in C# to create small temporary arrays directly on the call stack instead of the heap. This eliminates garbage collection overhead, reduces memory allocations, and improves performance by 10-50% in hot paths that frequently allocate small buffers. Stack-allocated memory is automatically freed when the function returns (no GC needed), making it ideal for small temporary arrays (< 1KB typically). The trade-off is limited size (typically 1-8MB per thread), stack-only lifetime (can't return stack-allocated buffers from functions), and requires using `Span<T>` for type safety. Use stack allocation for small temporary buffers in performance-critical code paths, especially when you know the size at compile time. Avoid it for large buffers, data that needs to outlive the function, or when the size is unknown at runtime.

---

## Problem Context

### Understanding the Basic Problem

**What is the stack?** The stack is a region of memory that stores local variables and function call information. Think of it like a stack of plates—you add plates to the top (push) and remove from the top (pop). When you call a function, its local variables are "pushed" onto the stack. When the function returns, those variables are automatically "popped" off. The stack is managed automatically by the runtime—you don't need to free memory manually.

**What is the heap?** The heap is a larger region of memory where objects live longer. When you create an object with `new` in C#, it's allocated on the heap. The heap is managed by the garbage collector (GC), which automatically finds and frees memory that's no longer used. This is convenient but has overhead—the GC must track objects, decide when to collect them, and actually free the memory.

**The problem with heap allocation for small temporary buffers**: When you allocate small arrays or buffers on the heap (like `new byte[100]`), you're asking the garbage collector to:
1. Find available memory
2. Allocate the object
3. Track the object for garbage collection
4. Later, when the object is no longer used, the GC must identify it as garbage and free it

For small temporary buffers that only exist during a function call, this is wasteful. The buffer is used briefly and then discarded, but the GC must track and collect it.

**Real-world example**: Imagine a function that processes network packets. For each packet, it needs a small 256-byte buffer to temporarily store processed data:

```csharp
// ❌ Using heap allocation (the common way)
public void ProcessPacket(byte[] packet)
{
    var tempBuffer = new byte[256]; // Allocated on heap
    // Process data into tempBuffer
    ProcessData(packet, tempBuffer);
    // Function returns, tempBuffer becomes garbage
    // GC must eventually collect it
}
```

If this function processes 10,000 packets per second, you're creating 10,000 heap allocations per second, all of which the GC must track and collect. This creates unnecessary GC pressure.

**Why this matters**: Garbage collection isn't free. The GC must:
- Stop your application (in some GC modes) to collect garbage
- Scan memory to find unused objects
- Move objects around (in compacting GC)
- Update references

For small temporary buffers that are only used briefly, stack allocation eliminates this overhead entirely.

### Key Terms Explained (Start Here!)

Before diving deeper, let's understand the basic building blocks:

**What is stack allocation?** Stack allocation places data directly on the call stack (the memory region that stores function call information and local variables). Stack-allocated memory is automatically freed when the function returns—no garbage collector needed.

**What is heap allocation?** Heap allocation places data in the heap (a larger, managed memory region). Heap-allocated objects are managed by the garbage collector, which automatically frees them when they're no longer needed. This is what happens with `new` in C#.

**What is the garbage collector (GC)?** The GC is a system that automatically manages heap memory. It identifies objects that are no longer referenced and frees their memory. GC has overhead: it must scan memory, track object lifetimes, and pause your application (in some modes) to clean up.

**What is `stackalloc`?** A C# keyword that allocates memory on the stack instead of the heap. Used with `Span<T>` for type safety: `Span<byte> buffer = stackalloc byte[256];`

**What is `Span<T>`?** A .NET type that represents a reference to a contiguous region of memory. `Span<T>` can point to stack-allocated memory, heap-allocated arrays, or unmanaged memory. It's type-safe and bounds-checked.

**What is a hot path?** Code that executes frequently—like code inside loops, frequently called functions, or performance-critical sections. Optimizing hot paths provides the biggest performance gains.

**What is GC pressure?** The amount of work the garbage collector must do. More allocations = more GC pressure = more frequent GC pauses = worse performance.

**What is stack overflow?** When the stack runs out of space (typically 1-8MB per thread). This crashes your program. Stack overflow happens when you allocate too much on the stack, have deep recursion, or use very large stack-allocated buffers.

### Common Misconceptions

**"Stack allocation is only for advanced/low-level code"**
- **The truth**: Modern C# makes stack allocation accessible with `Span<T>`. You don't need unsafe code for basic stack allocation. It's a useful tool for any developer writing performance-sensitive code.

**"Stack allocation is dangerous"**
- **The truth**: Stack allocation is safe when used correctly. Using `Span<T>` with `stackalloc` provides type safety and bounds checking. The main risks are stack overflow (avoided by limiting size) and returning stack-allocated memory from functions (prevented by compiler/runtime).

**"Heap allocation is always fine—the GC handles it"**
- **The truth**: While the GC is convenient, it has overhead. For small temporary buffers used briefly, stack allocation is faster and has zero GC overhead. The GC is great for longer-lived objects, but overkill for temporary buffers.

**"Stack allocation only matters in high-performance code"**
- **The truth**: Stack allocation helps in any code that frequently allocates small temporary buffers. Even in "regular" applications, reducing GC pressure improves performance and reduces latency spikes from GC pauses.

**"All allocations should use the stack"**
- **The truth**: Stack allocation has limits (size, lifetime). Use it for small, temporary buffers. Use heap allocation for larger objects, objects that outlive the function, or when size is unknown at compile time.

### Why Naive Solutions Fail

**Allocating small buffers on the heap**: Creating small arrays with `new` for temporary use. This works but creates unnecessary GC pressure. The GC must track and collect these short-lived objects, adding overhead.

**Using large stack-allocated buffers**: Trying to allocate large arrays on the stack (e.g., `stackalloc byte[1_000_000]`). This causes stack overflow because the stack has limited size (typically 1-8MB per thread).

**Returning stack-allocated memory**: Trying to return a stack-allocated buffer from a function. Stack memory is invalid after the function returns, so this would cause crashes. Stack-allocated memory must stay within the function scope.

**Not understanding stack size limits**: Assuming you can allocate as much as you want on the stack. The stack is small (1-8MB typically). Large allocations belong on the heap.

---

## How It Works

### Understanding Stack vs Heap Allocation

**How heap allocation works** (traditional `new`):
1. Runtime searches for available memory in the heap
2. Allocates memory for the object
3. Initializes the object (calls constructor if applicable)
4. Registers the object with the garbage collector
5. Returns a reference to the object
6. Later, when the object is no longer referenced, the GC identifies it as garbage
7. GC frees the memory (may require pausing the application)

**How stack allocation works** (`stackalloc`):
1. Compiler/runtime adjusts the stack pointer (just moves a pointer—very fast!)
2. Memory is immediately available (no search, no GC registration)
3. Object is used during the function
4. When the function returns, stack pointer moves back (memory is automatically freed—no GC needed!)

**Key difference**: Stack allocation is just moving a pointer. Heap allocation involves memory management, GC tracking, and eventual collection. For small temporary buffers, stack allocation is much faster.

### Technical Details: Stack Allocation Mechanics

**What happens at runtime**:
- **Stack pointer**: The CPU maintains a register (stack pointer) that points to the "top" of the stack
- **Allocation**: `stackalloc` simply moves the stack pointer forward (subtracts from the pointer value)
- **Deallocation**: When the function returns, the stack pointer moves back (adds to the pointer value)
- **No GC involvement**: The garbage collector never sees stack-allocated memory

**Performance characteristics**:
- **Allocation speed**: O(1) - just moving a pointer (typically 1-5 CPU cycles)
- **Deallocation speed**: O(1) - automatic when function returns (zero cost)
- **GC overhead**: Zero (GC doesn't track stack memory)
- **Cache locality**: Excellent (stack memory is accessed frequently, stays in CPU cache)

**Size limits**:
- **Per-thread stack size**: Typically 1-8MB (configurable, but limited)
- **Safe stackalloc size**: < 1KB typically (to avoid stack overflow)
- **Maximum practical size**: Depends on remaining stack space (function call depth matters)

### Using stackalloc in C#

**Basic syntax**:
```csharp
// Allocate 256 bytes on the stack
Span<byte> buffer = stackalloc byte[256];

// Allocate 100 integers on the stack
Span<int> numbers = stackalloc int[100];
```

**Why `Span<T>` is required**: `stackalloc` returns a pointer to unmanaged memory. `Span<T>` provides a type-safe, bounds-checked wrapper around this pointer, making stack allocation safe and easy to use.

**What `Span<T>` provides**:
- Type safety (can't accidentally treat bytes as integers)
- Bounds checking (prevents buffer overflows)
- No unsafe code needed (in modern C#)
- Consistent API (works with heap arrays, stack arrays, and unmanaged memory)

**Example - Simple stack allocation**:
```csharp
public int SumSmallArray(int[] source)
{
    // Allocate small buffer on stack (not heap)
    Span<int> temp = stackalloc int[100];
    
    // Copy data (if needed) or process directly
    for (int i = 0; i < Math.Min(source.Length, 100); i++)
    {
        temp[i] = source[i] * 2;
    }
    
    // Process temp buffer
    int sum = 0;
    for (int i = 0; i < temp.Length; i++)
    {
        sum += temp[i];
    }
    
    return sum;
    // temp is automatically freed when function returns (no GC!)
}
```

### Stack Allocation vs Heap Allocation: The Trade-offs

**Stack allocation advantages**:
- **Speed**: Extremely fast allocation (just moving a pointer)
- **No GC overhead**: Garbage collector never sees stack memory
- **Automatic cleanup**: Memory is freed when function returns (zero cost)
- **Cache efficiency**: Stack memory is accessed frequently, stays in CPU cache
- **Predictable performance**: No GC pauses

**Stack allocation disadvantages**:
- **Size limits**: Stack is small (1-8MB typically), so only for small buffers
- **Lifetime limits**: Memory is only valid during the function call (can't return it)
- **Stack overflow risk**: Allocating too much causes crashes
- **Thread-local**: Each thread has its own stack, can't share stack memory between threads

**Heap allocation advantages**:
- **Large size**: Can allocate very large objects (limited by available memory)
- **Flexible lifetime**: Objects can outlive the function that created them
- **Shareable**: Multiple threads can reference the same heap object
- **Dynamic size**: Can allocate based on runtime values

**Heap allocation disadvantages**:
- **Slower allocation**: Must find available memory, register with GC
- **GC overhead**: Garbage collector must track and collect objects
- **GC pauses**: GC may pause your application to collect garbage
- **Cache pressure**: Heap objects may be scattered in memory, worse cache locality

**The key insight**: Use stack allocation for small, temporary buffers. Use heap allocation for larger objects or objects that need to outlive the function.

---

## Advantages

**Eliminates GC overhead**: Stack-allocated memory is never tracked by the garbage collector. This reduces GC pressure, leading to fewer GC collections and more consistent performance.

**Faster allocation**: Stack allocation is just moving a pointer (1-5 CPU cycles), compared to heap allocation (100-500 CPU cycles). This is 20-100x faster for allocation.

**Automatic cleanup**: Stack memory is automatically freed when the function returns. No manual memory management, no GC involvement—just automatic cleanup with zero cost.

**Better cache performance**: Stack memory has excellent cache locality. Stack-allocated buffers are more likely to be in CPU cache, reducing cache misses and improving performance.

**Predictable performance**: No GC pauses affect stack-allocated memory. This leads to more consistent, predictable performance in hot paths.

**Reduced memory fragmentation**: Stack allocation doesn't fragment memory (unlike heap allocation, which can cause fragmentation over time).

---

## Disadvantages and Trade-offs

**Size limitations**: Stack size is limited (typically 1-8MB per thread). You can only allocate small buffers on the stack (< 1KB typically). Large buffers must use the heap.

**Lifetime restrictions**: Stack-allocated memory is only valid during the function call. You cannot return stack-allocated buffers from functions—they become invalid when the function returns.

**Stack overflow risk**: Allocating too much on the stack causes stack overflow (crashes your program). Must be careful with allocation size and stack depth.

**Thread-local only**: Each thread has its own stack. Stack-allocated memory cannot be shared between threads (unlike heap objects, which can be shared).

**Requires known size**: `stackalloc` requires the size to be known at compile time (or at least a constant expression). Cannot dynamically allocate based on runtime values (easily).

**Learning curve**: Understanding when to use stack allocation, size limits, and lifetime restrictions requires knowledge. Must understand stack vs heap trade-offs.

---

## When to Use This Approach

**Small temporary buffers**: When you need small buffers (< 1KB typically) that are only used during a function call. Perfect for temporary storage, intermediate calculations, or format conversions.

**Hot paths with frequent allocations**: When performance-critical code frequently allocates small buffers. Stack allocation eliminates allocation overhead and GC pressure in these paths.

**Known size at compile time**: When the buffer size is known at compile time (or is a small constant). `stackalloc` works best with compile-time constants.

**String formatting/building**: When building small strings (using `stackalloc char[]` with `Span<char>`). Avoids heap allocations for temporary string buffers.

**Performance-critical code**: When every cycle matters—real-time systems, high-throughput servers, game engines, or any code where performance is critical.

**Reducing GC pressure**: When you want to reduce garbage collection overhead by eliminating short-lived allocations.

**Why these scenarios**: In all these cases, stack allocation provides measurable benefits (faster allocation, less GC pressure, better performance) with acceptable trade-offs (size limits, lifetime restrictions).

---

## Common Mistakes

**Allocating too much on the stack**: Using `stackalloc` with large sizes (e.g., `stackalloc byte[100_000]`). This causes stack overflow. Keep allocations small (< 1KB typically).

**Returning stack-allocated memory**: Trying to return a `Span<T>` created with `stackalloc` from a function. Stack memory is invalid after the function returns—this causes crashes.

**Using stackalloc in recursive functions**: Allocating on the stack in recursive functions. Each recursive call uses stack space—combining recursion with stack allocation can easily cause stack overflow.

**Not understanding size limits**: Assuming you can allocate as much as you want. Stack is limited (1-8MB typically). Respect size limits.

**Using unsafe code unnecessarily**: Using unsafe pointers with `stackalloc` when `Span<T>` would work. Modern C# provides `Span<T>` for type-safe stack allocation—prefer it over unsafe code.

**Premature optimization**: Using stack allocation before profiling shows it's needed. Measure first—only optimize when profiling shows allocations are a bottleneck.

**Why these are mistakes**: They cause crashes (stack overflow), bugs (returning invalid memory), or waste effort (premature optimization). Understand stack allocation limitations and use it appropriately.

---

## Optimization Techniques

### Technique 1: Replace Small Heap Allocations with stackalloc

**When**: You have small temporary buffers (< 1KB) that are only used during a function call.

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

**Why it works**: `stackalloc` allocates memory on the stack instead of the heap. The stack is automatically managed—memory is freed when the function returns, with zero GC overhead.

**Performance**: Eliminates heap allocation and GC tracking. 20-100x faster allocation, zero GC overhead.

### Technique 2: String Formatting with stackalloc

**When**: Building small strings frequently (formatting numbers, creating small text).

**The problem**:
```csharp
// ❌ Heap allocation for string building
public string FormatValue(int value)
{
    var buffer = new char[32]; // Heap allocation
    if (value.TryFormat(buffer, out int written))
    {
        return new string(buffer, 0, written); // Another allocation
    }
    return value.ToString();
}
```

**The solution**:
```csharp
// ✅ Stack allocation for string building
public string FormatValue(int value)
{
    Span<char> buffer = stackalloc char[32]; // Stack allocation
    if (value.TryFormat(buffer, out int written))
    {
        return new string(buffer.Slice(0, written)); // Only one allocation (the string)
    }
    return value.ToString();
}
```

**Why it works**: The formatting buffer is allocated on the stack (no heap allocation). Only the final string is allocated on the heap. This reduces allocations in hot paths.

**Performance**: Eliminates temporary buffer allocation. Only the final string is allocated on the heap.

### Technique 3: Parsing Buffers with stackalloc

**When**: Parsing data with small temporary buffers (network protocols, file formats, etc.).

**The problem**:
```csharp
// ❌ Heap allocation for parsing buffer
public void ParsePacket(byte[] packet)
{
    var header = new byte[16]; // Heap allocation
    Array.Copy(packet, 0, header, 0, 16);
    ProcessHeader(header);
}
```

**The solution**:
```csharp
// ✅ Stack allocation for parsing buffer
public void ParsePacket(byte[] packet)
{
    Span<byte> header = stackalloc byte[16]; // Stack allocation
    packet.AsSpan(0, 16).CopyTo(header);
    ProcessHeader(header);
}
```

**Why it works**: The parsing buffer is allocated on the stack, eliminating heap allocation. For frequently parsed data, this reduces GC pressure significantly.

**Performance**: Eliminates allocation overhead in parsing hot paths. Faster parsing, less GC pressure.

### Technique 4: Know Your Limits

**Important**: Stack allocation has limits. Understanding and respecting these limits prevents stack overflow.

**Safe practices**:
- Keep allocations small (< 1KB typically)
- Avoid stack allocation in recursive functions
- Consider stack depth (deep call stacks have less available stack space)
- Test with realistic stack depth (your production call stack)

**Example of respecting limits**:
```csharp
// ✅ Safe: Small allocation
Span<byte> buffer = stackalloc byte[256]; // OK

// ❌ Dangerous: Large allocation
Span<byte> buffer = stackalloc byte[100_000]; // May cause stack overflow!

// ✅ Better for large buffers: Use heap allocation
byte[] buffer = new byte[100_000]; // Safe for large buffers
```

**Why this matters**: Stack overflow crashes your program. Respecting size limits prevents crashes and makes stack allocation safe.

---

## Example Scenarios

### Scenario 1: Network Packet Processing

**Problem**: Processing network packets requires a small temporary buffer for each packet. Using heap allocation creates GC pressure.

**Current code (slow)**:
```csharp
// ❌ Heap allocation for each packet
public void ProcessPacket(byte[] packet)
{
    var tempBuffer = new byte[256]; // Heap allocation
    // Process packet into tempBuffer
    TransformPacket(packet, tempBuffer);
    SendProcessedData(tempBuffer);
    // tempBuffer becomes garbage, GC must collect it
}
```

**Problems**:
- Creates heap allocation for every packet
- At 10,000 packets/second = 10,000 allocations/second
- GC must track and collect all these objects
- GC pressure causes performance degradation

**Improved code (faster)**:
```csharp
// ✅ Stack allocation for each packet
public void ProcessPacket(byte[] packet)
{
    Span<byte> tempBuffer = stackalloc byte[256]; // Stack allocation
    // Process packet into tempBuffer
    TransformPacket(packet, tempBuffer);
    SendProcessedData(tempBuffer);
    // tempBuffer automatically freed (no GC!)
}
```

**Results**:
- **Allocations**: Eliminated (no heap allocations)
- **GC pressure**: Reduced significantly (no objects to track)
- **Performance**: 20-40% improvement in packet processing
- **Latency**: More consistent (fewer GC pauses)

### Scenario 2: String Formatting in Hot Path

**Problem**: Formatting numbers frequently creates temporary buffers. Heap allocation for these buffers adds overhead.

**Current code (slow)**:
```csharp
// ❌ Heap allocation for formatting
public string FormatNumber(int value)
{
    var buffer = new char[32]; // Heap allocation
    if (value.TryFormat(buffer, out int written))
    {
        return new string(buffer, 0, written);
    }
    return value.ToString();
}
```

**Problems**:
- Creates heap allocation for formatting buffer
- Frequent calls = many allocations
- GC pressure from short-lived buffers

**Improved code (faster)**:
```csharp
// ✅ Stack allocation for formatting
public string FormatNumber(int value)
{
    Span<char> buffer = stackalloc char[32]; // Stack allocation
    if (value.TryFormat(buffer, out int written))
    {
        return new string(buffer.Slice(0, written)); // Only string allocation
    }
    return value.ToString();
}
```

**Results**:
- **Allocations**: Reduced (only final string is allocated)
- **GC pressure**: Reduced (no temporary buffer allocation)
- **Performance**: 15-30% improvement in formatting hot paths
- **Throughput**: Higher (less allocation overhead)

### Scenario 3: Parsing Small Data Structures

**Problem**: Parsing small data structures (like headers) requires temporary buffers. Heap allocation adds overhead.

**Current code (slow)**:
```csharp
// ❌ Heap allocation for parsing
public void ParseHeader(byte[] data)
{
    var header = new byte[16]; // Heap allocation
    Array.Copy(data, 0, header, 0, 16);
    ProcessHeader(header);
}
```

**Problems**:
- Heap allocation for each header parsed
- Overhead from allocation and GC tracking
- GC pressure from many small allocations

**Improved code (faster)**:
```csharp
// ✅ Stack allocation for parsing
public void ParseHeader(byte[] data)
{
    Span<byte> header = stackalloc byte[16]; // Stack allocation
    data.AsSpan(0, 16).CopyTo(header);
    ProcessHeader(header);
}
```

**Results**:
- **Allocations**: Eliminated (no heap allocation)
- **GC pressure**: Reduced (no objects to track)
- **Performance**: 10-25% improvement in parsing hot paths
- **Latency**: More consistent (fewer GC pauses)

---

## Summary and Key Takeaways

Stack allocation uses `stackalloc` to create small temporary buffers on the stack instead of the heap. This eliminates garbage collection overhead, reduces allocations, and improves performance by 10-50% in hot paths with frequent small allocations. The trade-off is limited size (< 1KB typically), stack-only lifetime (can't return from functions), and requires understanding stack limits to avoid stack overflow.
