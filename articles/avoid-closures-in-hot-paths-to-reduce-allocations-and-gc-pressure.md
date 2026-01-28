# Avoid Closures in Hot Paths to Reduce Allocations and GC Pressure

**Closures capture variables from the outer scope, creating heap-allocated objects (display classes) to maintain state. In hot paths (frequently executed code), these allocations accumulate, increasing GC pressure and reducing performance. Avoiding closures (by passing parameters explicitly, using static methods, or refactoring to avoid captured variables) eliminates these allocations, improving performance by 10%–30% in code with many closures. The trade-off: avoiding closures can make code less convenient (must pass parameters explicitly) and may require refactoring. Use closure-free code in hot paths, frequently executed lambdas, or performance-critical code. Avoid optimizing closures in cold paths where the overhead is negligible.**

---

## Executive Summary (TL;DR)

Closures capture variables from the outer scope, creating heap-allocated objects (display classes) to maintain state. Each closure that captures variables allocates a display class object on the heap. In hot paths (frequently executed code), these allocations accumulate: 1 million lambda calls with closures = 1 million allocations = increased GC pressure. Avoiding closures (by passing parameters explicitly, using static methods, or refactoring to avoid captured variables) eliminates these allocations, improving performance by 10%–30% in code with many closures. Use closure-free code in hot paths, frequently executed lambdas, or performance-critical code. Avoid optimizing closures in cold paths where the overhead is negligible. The trade-off: avoiding closures can make code less convenient (must pass parameters explicitly) and may require refactoring. Typical improvements: 10%–30% faster in code with many closures, reduced GC pressure, fewer allocations. Common mistakes: using closures in hot paths, not realizing closures cause allocations, assuming closures are free.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you use a closure in a hot path?**

Imagine a hot path that processes 1 million items using a lambda with a closure:

```csharp
// ❌ Bad: Closure captures variable
public void ProcessItems(List<Item> items, int threshold)
{
    // This lambda captures 'threshold' from outer scope
    var filtered = items.Where(x => x.Value > threshold).ToList();
    // What happens: Each lambda call creates a closure object
    // 1 million items = 1 million lambda calls = 1 million closure allocations
}
```

**What happens:**
- **Lambda expression**: `x => x.Value > threshold` captures `threshold` from outer scope
- **Closure creation**: Compiler creates a display class to hold captured variables
- **Allocation**: Each lambda invocation may allocate a closure object (depending on compiler optimizations)
- **GC pressure**: Many allocations increase GC frequency and pause time
- **Performance impact**: Allocations and GC overhead reduce performance

**Why this is slow in hot paths:**
- **Allocations**: Closures create heap-allocated objects (display classes)
- **GC pressure**: Many allocations trigger more frequent GC collections
- **Memory overhead**: Closure objects consume memory
- **CPU overhead**: GC collections pause threads, reducing throughput

**Without closure:**

```csharp
// ✅ Good: No closure, pass parameter explicitly
public void ProcessItems(List<Item> items, int threshold)
{
    var filtered = new List<Item>();
    foreach (var item in items)
    {
        if (item.Value > threshold)  // Direct comparison, no closure
            filtered.Add(item);
    }
    // What happens: No closure allocations, no display class
    // 1 million items = 0 allocations for closure
}
```

**What happens:**
- **Direct comparison**: `item.Value > threshold` uses parameter directly
- **No closure**: No display class needed, no allocations
- **No GC pressure**: No allocations = no GC overhead
- **Better performance**: No allocation overhead

**Improvement: 10%–30% faster** by eliminating closure allocations and reducing GC pressure.

### Key Terms Explained (Start Here!)

**What is a closure?** A function that captures variables from its outer (enclosing) scope. In C#, lambdas and anonymous methods can capture variables, creating closures. Example: `x => x.Value > threshold` captures `threshold` from the outer scope.

**What is a captured variable?** A variable from the outer scope that is used inside a lambda or anonymous method. Example: `threshold` in `x => x.Value > threshold` is a captured variable.

**What is a display class?** A compiler-generated class that holds captured variables. When a lambda captures variables, the compiler creates a display class to store those variables. Example: Lambda capturing `threshold` creates a display class with a field for `threshold`.

**What is a hot path?** Code that is executed frequently (e.g., in loops, request handlers, frequently called methods). Hot paths benefit most from avoiding closures because the overhead accumulates. Example: Processing 1 million items in a loop = hot path.

**What is a cold path?** Code that is executed infrequently (e.g., error handling, initialization, rarely called methods). Cold paths don't benefit much from avoiding closures because the overhead is negligible. Example: Error logging that happens once per hour = cold path.

**What is an allocation?** Creating a new object on the heap. Allocations trigger GC when the heap is full. Example: `new DisplayClass()` = allocation.

**What is GC pressure?** The workload on the garbage collector. Many allocations increase GC pressure, causing more frequent GC collections and longer pause times. Example: 1 million allocations = high GC pressure = frequent GC = performance degradation.

**What is a lambda expression?** An anonymous function that can capture variables from the outer scope. Example: `x => x.Value > threshold` is a lambda expression.

**What is a delegate?** A type that represents a method. Lambdas are compiled to delegates. Example: `Func<Item, bool>` is a delegate type.

**What is a static method?** A method that doesn't capture instance or local variables. Static methods can't create closures, so they don't cause allocations. Example: `private static bool IsAboveThreshold(Item item, int threshold)` is a static method.

### Common Misconceptions

**"Closures are free"**
- **The truth**: Closures create heap-allocated objects (display classes) to hold captured variables. Each closure that captures variables may allocate an object, causing GC pressure.

**"Closures are always slow"**
- **The truth**: Closures are fine in cold paths (infrequently executed code). The overhead is negligible when executed rarely. Closures become a problem in hot paths (frequently executed code) where allocations accumulate.

**"All lambdas create closures"**
- **The truth**: Only lambdas that capture variables create closures. Lambdas that don't capture variables (e.g., `x => x.Value > 10`) don't create closures.

**"Avoiding closures always improves performance"**
- **The truth**: Avoiding closures helps in hot paths, but the improvement is modest (10%–30%). In cold paths, the overhead is negligible, so avoiding closures provides minimal benefit.

**"Closures are the same as passing parameters"**
- **The truth**: Closures capture variables, creating heap-allocated objects. Passing parameters directly uses the stack, which is faster and doesn't cause allocations.

---

## How It Works

### Understanding How Closures Work in .NET

**How a closure captures variables:**

```csharp
public void ProcessItems(List<Item> items, int threshold)
{
    // Lambda captures 'threshold' from outer scope
    var filtered = items.Where(x => x.Value > threshold).ToList();
}
```

**What the compiler generates (simplified):**

```csharp
// Compiler generates a display class
private sealed class DisplayClass
{
    public int threshold;  // Captured variable
    
    public bool Method(Item x)
    {
        return x.Value > this.threshold;  // Uses captured variable
    }
}

public void ProcessItems(List<Item> items, int threshold)
{
    // Create display class instance (allocation!)
    var displayClass = new DisplayClass { threshold = threshold };
    
    // Lambda becomes a method on display class
    var filtered = items.Where(displayClass.Method).ToList();
}
```

**What happens:**
1. **Compiler creates display class**: A class to hold captured variables (`threshold`)
2. **Allocation**: `new DisplayClass()` allocates an object on the heap
3. **Lambda becomes method**: The lambda becomes a method on the display class
4. **GC pressure**: The allocation increases GC pressure

**How avoiding closures works:**

```csharp
public void ProcessItems(List<Item> items, int threshold)
{
    var filtered = new List<Item>();
    foreach (var item in items)
    {
        if (item.Value > threshold)  // Direct parameter use, no closure
            filtered.Add(item);
    }
}
```

**What happens:**
1. **No display class**: No captured variables, so no display class needed
2. **No allocation**: No object allocation
3. **Direct parameter use**: `threshold` is used directly from the stack
4. **No GC pressure**: No allocations = no GC overhead

**Key insight**: Closures create heap-allocated objects to hold captured variables. Avoiding closures eliminates these allocations, reducing GC pressure.

### Technical Details: When Closures Cause Allocations

**Closure with captured variable (causes allocation):**

```csharp
int threshold = 10;
var filtered = items.Where(x => x.Value > threshold).ToList();
// Captures 'threshold' → creates display class → allocation
```

**What happens:**
- **Captured variable**: `threshold` is captured
- **Display class**: Compiler creates display class
- **Allocation**: `new DisplayClass()` allocates object
- **GC pressure**: Allocation triggers GC when heap is full

**Lambda without captured variables (no allocation):**

```csharp
var filtered = items.Where(x => x.Value > 10).ToList();
// No captured variables → no display class → no allocation
```

**What happens:**
- **No captured variables**: Lambda doesn't capture any variables
- **No display class**: Compiler doesn't create display class
- **No allocation**: No object allocation
- **No GC pressure**: No allocations = no GC overhead

**Multiple captured variables:**

```csharp
int threshold = 10;
string category = "A";
var filtered = items.Where(x => x.Value > threshold && x.Category == category).ToList();
// Captures 'threshold' and 'category' → creates display class with both → allocation
```

**What happens:**
- **Multiple captured variables**: `threshold` and `category` are captured
- **Display class**: Compiler creates display class with both fields
- **Allocation**: `new DisplayClass()` allocates object (same as single variable)
- **GC pressure**: Same allocation overhead (one object holds all captured variables)

**Key insight**: Any captured variable creates a display class and allocation. The number of captured variables doesn't matter (one object holds all).

---

## Why This Becomes a Bottleneck

Closures become a bottleneck in hot paths because:

**Accumulating allocations**: In hot paths, closures create many allocations. Example: Processing 1 million items with a closure = 1 million allocations (depending on compiler optimizations) = high GC pressure.

**GC pressure**: Many allocations trigger more frequent GC collections, causing pause times. Example: 1 million allocations = frequent GC = 100–1000 ms pause times = reduced throughput.

**Memory overhead**: Closure objects consume memory. Example: 1 million closure objects × 24 bytes (object overhead) = 24 MB of memory just for closures.

**CPU overhead**: GC collections pause threads, wasting CPU cycles. Example: GC pause of 100 ms = 100 ms of CPU time wasted, reducing throughput.

**Cache pollution**: Many small allocations can pollute the CPU cache, slowing down subsequent operations. Example: Allocating many closure objects evicts useful data from cache.

---

## Advantages

**Reduced allocations**: Avoiding closures eliminates display class allocations, reducing the number of heap allocations. Example: 1 million lambda calls without closures = 0 allocations vs 1 million allocations with closures.

**Lower GC pressure**: Fewer allocations mean less GC pressure, reducing GC frequency and pause times. Example: 50% fewer allocations = 50% less GC pressure = fewer GC pauses.

**Better performance**: Eliminating allocations improves performance by 10%–30% in code with many closures. Example: Hot path with closures = 100 ms, without closures = 70–90 ms (10%–30% faster).

**Less memory usage**: No closure objects means less memory usage. Example: 1 million closure objects = 24 MB saved.

**Better cache utilization**: Fewer allocations mean better CPU cache utilization, improving performance.

---

## Disadvantages and Trade-offs

**Less convenient code**: Avoiding closures may require passing parameters explicitly or refactoring code. Example: Must pass `threshold` as parameter instead of capturing it.

**More verbose**: Closure-free code can be more verbose than using lambdas with closures. Example: `foreach` loop is more verbose than `Where()` with lambda.

**Requires refactoring**: Avoiding closures may require significant code changes. Example: Refactoring LINQ queries to `foreach` loops.

**Not always beneficial**: In cold paths, avoiding closures provides minimal benefit. Example: Code executed once per hour doesn't benefit from avoiding closures.

**May reduce code readability**: Some developers find lambdas with closures more readable than explicit loops. Example: `items.Where(x => x.Value > threshold)` vs `foreach` loop.

---

## When to Use This Approach

Avoid closures when:

- **Hot paths** (frequently executed code). Example: Request handlers, loops processing many items, frequently called methods. Closures in hot paths cause accumulating allocations.

- **Performance-critical code** (code where performance matters). Example: Data processing pipelines, high-throughput systems, real-time systems. Avoiding closures improves performance.

- **Many closures** (code with many lambda expressions that capture variables). Example: Multiple LINQ queries with closures, event handlers with closures. Many closures = many allocations.

- **GC pressure is a problem** (profiling shows high GC pressure). Example: GC pauses are affecting performance. Avoiding closures reduces GC pressure.

**Recommended approach:**
- **Hot paths**: Avoid closures, use explicit parameters or static methods
- **Cold paths**: Closures are fine, overhead is negligible
- **LINQ queries**: Consider refactoring to `foreach` loops in hot paths
- **Event handlers**: Avoid capturing variables if possible, pass parameters explicitly

---

## When Not to Use It

Don't avoid closures when:

- **Cold paths** (infrequently executed code). Example: Error handling, initialization, rarely called methods. The overhead is negligible.

- **Code readability is more important** (maintainability over performance). Example: Prototypes, non-critical code, code where performance doesn't matter.

- **Closures don't capture variables** (lambdas that don't capture variables don't cause allocations). Example: `x => x.Value > 10` doesn't capture variables, so no allocation.

- **Single execution** (code executed once or rarely). Example: Application startup, one-time operations. The overhead is negligible.

- **Performance impact is negligible** (profiling shows closures aren't a bottleneck). Example: Code where closures account for <1% of execution time.

---

## Performance Impact

Typical improvements when avoiding closures:

- **Performance**: **10%–30% faster** in code with many closures. Example: Hot path processing 1 million items: 100 ms (with closures) → 70–90 ms (without closures) = 10%–30% faster.

- **Allocations**: **50%–100% fewer allocations** (depending on how many closures are avoided). Example: 1 million lambda calls with closures = 1 million allocations, without closures = 0 allocations.

- **GC pressure**: **50%–90% lower GC pressure**. Example: 50% fewer allocations = 50% less GC pressure = fewer GC pauses.

**Important**: The improvement depends on how many closures are in hot paths. If closures are only in cold paths, the improvement is negligible (<1%). If closures are in hot paths, the improvement is significant (10%–30%).

---

## Common Mistakes

**Using closures in hot paths**: Using lambdas with captured variables in frequently executed code. Example: `items.Where(x => x.Value > threshold)` in a hot path. Avoid closures in hot paths.

**Not realizing closures cause allocations**: Assuming closures are free and don't cause allocations. Example: Using closures everywhere without considering the performance impact. Closures create allocations.

**Assuming all lambdas create closures**: Thinking that all lambdas cause allocations. Example: Avoiding all lambdas, even those that don't capture variables. Only lambdas that capture variables create closures.

**Over-optimizing cold paths**: Avoiding closures in code that's rarely executed. Example: Refactoring error handling to avoid closures. The overhead is negligible in cold paths.

**Not measuring**: Assuming avoiding closures improves performance without measuring. Example: Refactoring code without benchmarking. Always measure to verify improvement.

**Sacrificing readability unnecessarily**: Making code less readable to avoid closures when performance doesn't matter. Example: Refactoring readable LINQ queries to verbose `foreach` loops in non-critical code.

---

## Example Scenarios

### Scenario 1: Hot path with LINQ and closure

**Problem**: A hot path processes 1 million items using LINQ with a closure, causing many allocations.

**Bad approach** (closure captures variable):

```csharp
// ❌ Bad: Closure captures 'threshold'
public List<Item> FilterItems(List<Item> items, int threshold)
{
    // Lambda captures 'threshold' → creates display class → allocation
    return items.Where(x => x.Value > threshold).ToList();
    // 1 million items = 1 million lambda calls = potential allocations
}
```

**Good approach** (no closure, explicit parameter):

```csharp
// ✅ Good: No closure, explicit comparison
public List<Item> FilterItems(List<Item> items, int threshold)
{
    var filtered = new List<Item>();
    foreach (var item in items)
    {
        if (item.Value > threshold)  // Direct comparison, no closure
            filtered.Add(item);
    }
    return filtered;
    // 1 million items = 0 closure allocations
}
```

**Better approach** (static method to avoid closure):

```csharp
// ✅ Better: Static method, no closure possible
private static bool IsAboveThreshold(Item item, int threshold)
{
    return item.Value > threshold;
}

public List<Item> FilterItems(List<Item> items, int threshold)
{
    var filtered = new List<Item>();
    foreach (var item in items)
    {
        if (IsAboveThreshold(item, threshold))  // Static method, no closure
            filtered.Add(item);
    }
    return filtered;
}
```

**Results**:
- **Bad**: Potential allocations from closures, GC pressure, 10%–30% slower
- **Good**: No closure allocations, lower GC pressure, 10%–30% faster
- **Improvement**: 10%–30% faster, reduced GC pressure

---

### Scenario 2: Event handler with closure

**Problem**: Event handler captures variables, creating allocations on each event.

**Bad approach** (closure captures variables):

```csharp
// ❌ Bad: Closure captures 'userId' and 'logger'
public void SubscribeToEvents(EventBus bus, int userId, ILogger logger)
{
    bus.OnEvent += (sender, e) =>
    {
        // Lambda captures 'userId' and 'logger' → creates display class → allocation
        logger.Log($"User {userId} received event: {e.Type}");
    };
    // Each event = potential allocation from closure
}
```

**Good approach** (no closure, capture in class field):

```csharp
// ✅ Good: No closure, use class fields
public class EventSubscriber
{
    private readonly int _userId;
    private readonly ILogger _logger;
    
    public EventSubscriber(int userId, ILogger logger)
    {
        _userId = userId;
        _logger = logger;
    }
    
    public void SubscribeToEvents(EventBus bus)
    {
        bus.OnEvent += OnEventReceived;  // Method reference, no closure
    }
    
    private void OnEventReceived(object sender, Event e)
    {
        _logger.Log($"User {_userId} received event: {e.Type}");  // Use fields, no closure
    }
}
```

**Results**:
- **Bad**: Allocation per event from closure, GC pressure
- **Good**: No closure allocations, no GC pressure
- **Improvement**: Eliminated allocations, reduced GC pressure

---

### Scenario 3: Multiple LINQ queries with closures

**Problem**: Hot path uses multiple LINQ queries with closures, causing many allocations.

**Bad approach** (multiple closures):

```csharp
// ❌ Bad: Multiple closures capturing variables
public List<Item> ProcessItems(List<Item> items, int minValue, int maxValue, string category)
{
    // Each LINQ query captures variables → multiple allocations
    var filtered = items
        .Where(x => x.Value > minValue)  // Closure 1: captures 'minValue'
        .Where(x => x.Value < maxValue)  // Closure 2: captures 'maxValue'
        .Where(x => x.Category == category)  // Closure 3: captures 'category'
        .ToList();
    // 3 closures = 3 potential allocations per item
    return filtered;
}
```

**Good approach** (single loop, no closures):

```csharp
// ✅ Good: Single loop, no closures
public List<Item> ProcessItems(List<Item> items, int minValue, int maxValue, string category)
{
    var filtered = new List<Item>();
    foreach (var item in items)
    {
        if (item.Value > minValue && 
            item.Value < maxValue && 
            item.Category == category)  // Direct comparisons, no closures
        {
            filtered.Add(item);
        }
    }
    return filtered;
    // 0 closures = 0 allocations
}
```

**Results**:
- **Bad**: 3 closures per item = many allocations, GC pressure, slower
- **Good**: 0 closures = 0 allocations, no GC pressure, faster
- **Improvement**: 10%–30% faster, significantly reduced allocations

---

## Summary and Key Takeaways

Closures capture variables from the outer scope, creating heap-allocated objects (display classes) to maintain state. In hot paths (frequently executed code), these allocations accumulate, increasing GC pressure and reducing performance. Avoiding closures (by passing parameters explicitly, using static methods, or refactoring to avoid captured variables) eliminates these allocations, improving performance by 10%–30% in code with many closures. Use closure-free code in hot paths, frequently executed lambdas, or performance-critical code. Avoid optimizing closures in cold paths where the overhead is negligible. The trade-off: avoiding closures can make code less convenient (must pass parameters explicitly) and may require refactoring. Typical improvements: 10%–30% faster in code with many closures, reduced GC pressure, fewer allocations. Common mistakes: using closures in hot paths, not realizing closures cause allocations, assuming closures are free, over-optimizing cold paths. Always measure to verify improvement. Only lambdas that capture variables create closures—lambdas without captured variables don't cause allocations.

---

<!-- Tags: Performance, Optimization, Memory Management, Garbage Collection, .NET Performance, C# Performance, Latency Optimization, System Design, Architecture, Profiling, Benchmarking, Measurement -->
