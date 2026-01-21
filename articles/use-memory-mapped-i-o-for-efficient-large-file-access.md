# Use Memory-Mapped I/O for Efficient Large File Access

**Map files directly into your process's virtual address space to enable efficient random access, automatic OS caching, and shared memory access for large files without loading entire files into RAM.**

---

## Executive Summary (TL;DR)

Memory-mapped I/O maps files directly into your process's virtual address space, allowing you to access file data as if it were in memory. The operating system handles page loading and caching automatically, enabling efficient random access to large files without loading entire files into RAM. This improves performance by 20-50% for large file access compared to traditional file I/O, especially for random access patterns. Use memory-mapped I/O for large files that don't fit in memory, random access patterns, shared file access between processes, and database systems. The trade-off is less control over when pages are loaded (OS manages it), potential page faults if data isn't cached, and synchronization complexity for writes. Avoid memory-mapped I/O for small files, sequential-only access, or when you need precise control over I/O timing.

---

## Problem Context

### Understanding the Basic Problem

**What is memory-mapped I/O?** Memory-mapped I/O is a technique where the operating system maps a file directly into your process's virtual address space. Instead of reading file data into a buffer in your code, you access the file data directly through memory pointers, as if the file were already in memory. The OS handles loading file pages into physical RAM on demand.

**The problem with traditional file I/O**: When you read a large file using traditional methods (like `File.ReadAllBytes()` or `FileStream.Read()`), you must:
1. Allocate a buffer in memory (often the entire file size)
2. Read data from disk into the buffer (copying from disk to memory)
3. Process the data from the buffer
4. If you need random access, you must read different parts of the file separately, each requiring a separate disk read

**Real-world example**: Imagine you have a 10GB database file and you need to access records at random offsets (byte 1000, byte 5000000, byte 20000000). With traditional I/O:
- You'd need to read each section separately: `fileStream.Seek(1000); fileStream.Read(buffer, 0, 100);`
- Each `Seek` + `Read` operation requires a system call and potentially a disk seek
- For random access, this means many disk seeks (slow on traditional hard drives)
- You must manage buffers and track file positions manually

With memory-mapped I/O:
- The file is mapped into virtual memory: `var accessor = mmf.CreateViewAccessor(1000, 100);`
- You access it like memory: `var value = accessor.ReadInt32(0);`
- The OS handles loading pages on demand (page faults)
- Random access is as simple as pointer arithmetic
- Multiple processes can share the same mapped file

### Key Terms Explained (Start Here!)

Before diving deeper, let's understand the basic building blocks:

**What is virtual address space?** Every process has its own virtual address space—a large, continuous range of memory addresses (e.g., 0 to 2^64 bytes on 64-bit systems). These addresses are "virtual" because they don't directly correspond to physical RAM addresses. The operating system maps virtual addresses to physical RAM addresses (or disk) using page tables.

**What is a page?** Memory is divided into fixed-size blocks called pages (typically 4KB on x86-64 systems, 16KB on some ARM systems). When you access memory, the CPU loads entire pages, not individual bytes. Pages are the unit of memory management—the OS maps, loads, and swaps pages.

**What is a page fault?** When your program accesses a virtual address that isn't currently mapped to physical RAM, the CPU triggers a page fault interrupt. The OS handles this by loading the required page from disk (if it's a memory-mapped file) or allocating memory. Page faults are how memory-mapped files load data on demand.

**What is memory mapping?** The process of associating a range of virtual addresses with a file (or physical memory). When you memory-map a file, the OS creates entries in the page table that map virtual addresses to file offsets. Accessing those virtual addresses triggers page faults that load file data into physical RAM.

**What is a view accessor?** In .NET, a `MemoryMappedViewAccessor` provides a window into a memory-mapped file. You create a view that maps a specific range of the file (offset and length) into virtual memory. The view gives you a pointer-like interface to access file data.

**What is lazy loading?** Memory-mapped files use lazy loading—pages aren't loaded into physical RAM until you actually access them. This means mapping a 10GB file is fast (just creates page table entries), but accessing data triggers page faults that load pages on demand.

**What is shared memory?** When multiple processes memory-map the same file, they can share the same physical RAM pages (read-only sharing). Changes to the file are visible to all processes mapping it. This enables efficient inter-process communication and shared data access.

**What is copy-on-write (COW)?** When you memory-map a file with write access, the OS can use copy-on-write. Initially, all processes share the same physical pages (read-only). When a process writes to a page, the OS creates a private copy of that page for that process. This enables efficient sharing until writes occur.

### Common Misconceptions

**"Memory-mapped files load the entire file into memory"**
- **The truth**: Memory-mapped files use lazy loading—only accessed pages are loaded into physical RAM. Mapping a 10GB file creates page table entries but doesn't allocate 10GB of RAM. Pages are loaded on demand when you access them.

**"Memory-mapped I/O is always faster than traditional I/O"**
- **The truth**: Memory-mapped I/O is faster for random access patterns and large files, but for sequential-only access of small files, traditional I/O can be faster (less overhead, better OS buffering). Choose based on access patterns.

**"Memory-mapped files are only for read-only access"**
- **The truth**: Memory-mapped files support read-write access. You can modify file data through the memory-mapped view, and changes are written back to the file (with OS-managed flushing). However, write access requires synchronization for multi-process scenarios.

**"You can't use memory-mapped I/O for files larger than RAM"**
- **The truth**: Memory-mapped files work perfectly for files larger than RAM. The OS loads only accessed pages into physical RAM, swapping out unused pages if RAM is full. This enables working with files much larger than available RAM.

**"Memory-mapped I/O eliminates all disk I/O"**
- **The truth**: Memory-mapped I/O still requires disk I/O—pages must be loaded from disk on first access (page faults). However, the OS caches pages in RAM, so subsequent accesses to the same pages are fast (no disk I/O). The benefit is automatic caching and efficient random access.

### Why Naive Solutions Fail

**Loading entire files into memory**: Reading entire large files into memory (e.g., `File.ReadAllBytes()`) works for small files but fails for large files—you run out of RAM, trigger garbage collection pressure, and waste memory on unused data.

**Sequential reads for random access**: Using `FileStream.Seek()` + `Read()` for random access works but is inefficient—each access requires a system call and potentially a disk seek. For random access patterns, this means many slow disk seeks.

**Manual buffer management**: Managing your own buffers for file I/O works but is complex—you must track file positions, handle buffer boundaries, manage memory allocation, and coordinate with the OS's file cache. Memory-mapped I/O handles this automatically.

**Not considering OS caching**: Traditional I/O relies on the OS file cache, but you have no control over it. Memory-mapped I/O gives you direct access to cached pages and better integration with the OS's memory management.

---

## How It Works

### Virtual Memory and Memory Mapping

**How virtual memory works**:
1. Your process has a virtual address space (e.g., 0 to 2^64 bytes on 64-bit systems)
2. The OS maintains page tables that map virtual addresses to physical RAM addresses (or indicate "not in RAM")
3. When you access a virtual address, the CPU's Memory Management Unit (MMU) translates it to a physical address using page tables
4. If the page is in RAM: Access proceeds normally (fast, ~100 nanoseconds)
5. If the page is not in RAM: Page fault occurs (slow, OS loads page from disk)

**How memory-mapped files work**:
1. You request to map a file: `MemoryMappedFile.CreateFromFile(filePath)`
2. The OS creates page table entries that map virtual addresses to file offsets (not physical RAM yet)
3. The OS marks these pages as "not present" in the page table
4. When you access a mapped address: Page fault occurs
5. The OS page fault handler:
   - Identifies that the page is from a memory-mapped file
   - Reads the corresponding file region (4KB page) from disk
   - Allocates physical RAM page
   - Loads file data into RAM
   - Updates page table to map virtual address to physical RAM
6. Your program continues, accessing data from RAM (fast)

**Why this is efficient**: The OS handles all the complexity—page loading, caching, swapping. You just access memory addresses, and the OS ensures the right data is there. For random access, this means the OS loads only the pages you access, not the entire file.

### Page Faults and Lazy Loading

**What happens during a page fault**:
1. Your code accesses a virtual address in the mapped region: `accessor.ReadInt32(offset)`
2. CPU's MMU looks up the address in the page table
3. Page table indicates "page not in physical RAM" (page fault)
4. CPU generates page fault interrupt (switches to kernel mode)
5. OS page fault handler runs:
   - Identifies the faulting address is in a memory-mapped file
   - Calculates which file offset corresponds to this virtual address
   - Reads 4KB page from file (disk I/O, ~5-10ms for disk, ~100-500μs for SSD)
   - Allocates physical RAM page
   - Copies file data into RAM page
   - Updates page table to map virtual address to physical RAM
6. CPU retries the memory access (now succeeds, data is in RAM)

**Cost of page faults**:
- **Minor page fault** (page in RAM but not mapped): ~1,000-10,000 CPU cycles
- **Major page fault** (page must be loaded from disk): ~10,000-100,000+ CPU cycles + disk I/O latency
  - Disk: ~5-10 milliseconds (5,000,000-10,000,000 CPU cycles at 1GHz)
  - SSD: ~100-500 microseconds (100,000-500,000 CPU cycles)

**Why lazy loading matters**: Loading a 10GB file would take forever if done eagerly. With lazy loading, mapping is instant (just creates page table entries), and pages load on demand. You only pay the cost of loading pages you actually access.

### OS Caching and Page Management

**How the OS caches pages**:
1. When a page is loaded from disk, it stays in physical RAM (OS file cache)
2. Subsequent accesses to the same page are fast (no disk I/O, just memory access)
3. If physical RAM is full, the OS uses a Least Recently Used (LRU) algorithm to evict pages
4. Evicted pages are written to swap (if modified) or just discarded (if read-only)
5. If you access an evicted page again, it's loaded from disk again (page fault)

**Benefits of OS caching**:
- **Automatic**: You don't manage caching—the OS does it
- **Efficient**: OS uses all available RAM for caching
- **Shared**: Multiple processes mapping the same file share cached pages (read-only)
- **Adaptive**: OS adjusts cache based on memory pressure

**Why this beats manual buffering**: With traditional I/O, you manage your own buffers, and the OS also maintains a file cache. This can lead to double buffering (your buffer + OS cache). With memory-mapped I/O, there's only one copy in RAM (the OS cache), and you access it directly.

### Shared Memory and Multi-Process Access

**How shared memory works**:
1. Process A maps a file: Creates page table entries, pages marked "not present"
2. Process A accesses a page: Page fault, OS loads page into physical RAM
3. Process B maps the same file: Creates page table entries pointing to the same physical RAM pages (read-only sharing)
4. Process B accesses the same page: No page fault (page already in RAM), accesses shared physical page
5. Both processes see the same data (read-only)

**Copy-on-write for writes**:
1. Process A maps file with write access
2. Process B maps same file with write access
3. Initially, both share physical pages (read-only in page tables)
4. Process A writes to a page: Page fault occurs
5. OS creates a private copy of the page for Process A
6. Process A's page table updated to point to private copy
7. Process B still uses shared page (read-only)
8. Process B writes to same page: Gets its own private copy

**Why shared memory is efficient**: Multiple processes can share the same physical RAM pages, reducing memory usage. For read-only access (like shared configuration files, databases), this is very efficient.

---

## Why This Becomes a Bottleneck

### Traditional File I/O Limitations

**The problem with sequential reads for random access**:
- Each random access requires: `fileStream.Seek(offset)` + `fileStream.Read(buffer)`
- Each operation is a system call (kernel mode switch, ~1,000-10,000 cycles)
- Each seek may require a disk seek (mechanical drives: ~5-10ms, SSDs: ~100-500μs)
- For random access patterns, this means many slow disk seeks
- You must manage buffers and track file positions manually

**Real-world impact**: Accessing 1000 random records in a 10GB file:
- Traditional I/O: 1000 seeks + 1000 reads = 1000 × (5-10ms seek + read time) = 5-10 seconds
- Memory-mapped I/O: 1000 page faults (first access) + cached accesses (subsequent) = ~1-2 seconds (with OS caching)

**The problem with loading entire files**:
- Large files don't fit in RAM (e.g., 10GB file, 8GB RAM)
- Loading entire file causes: Out of memory, swap thrashing, GC pressure
- You waste memory on unused data (load 10GB, use 100MB)
- Garbage collection overhead for large allocations

### Memory-Mapped I/O Advantages

**Efficient random access**: Memory-mapped I/O enables pointer-like access to file data. Random access is as simple as `accessor.ReadInt32(offset)`—no seeks, no manual buffer management. The OS handles page loading automatically.

**Automatic OS caching**: The OS caches accessed pages in RAM. Subsequent accesses to the same pages are fast (no disk I/O). You don't manage caching—the OS does it efficiently.

**Shared memory**: Multiple processes can share the same mapped file, reducing memory usage. For read-only access, this is very efficient (one copy in RAM, shared by all processes).

**Lazy loading**: Only accessed pages are loaded into RAM. You can map files much larger than RAM, and only pay the cost of loading pages you access.

---

## Advantages

**Efficient random access**: Memory-mapped I/O enables pointer-like access to file data, making random access as simple as memory access. No manual seeks or buffer management required.

**Automatic OS caching**: The OS automatically caches accessed pages in RAM. Subsequent accesses to the same pages are fast (no disk I/O). You don't manage caching—the OS handles it efficiently.

**Shared memory**: Multiple processes can share the same mapped file, reducing memory usage. For read-only access, this is very efficient (one copy in RAM, shared by all processes).

**Lazy loading**: Only accessed pages are loaded into RAM. You can map files much larger than RAM, and only pay the cost of loading pages you access.

**Simplified code**: Memory-mapped I/O simplifies code—no manual buffer management, no file position tracking, no coordination with OS file cache. You just access memory addresses.

**Better performance for random access**: Memory-mapped I/O is 20-50% faster than traditional I/O for random access patterns, especially on large files. The OS handles page loading efficiently, and caching improves subsequent accesses.

**Measurable improvements**:
- **Random access performance**: 20-50% improvement for large files
- **Memory usage**: Only accessed pages in RAM (not entire file)
- **Code complexity**: Simpler code (no manual buffer management)
- **Multi-process sharing**: Reduced memory usage (shared pages)

**Why these benefits matter**: For large file access, especially random access patterns, memory-mapped I/O provides significant performance improvements and simplifies code. The OS handles the complexity of page loading and caching, giving you efficient file access.

---

## Disadvantages and Trade-offs

**Less control over I/O timing**: The OS controls when pages are loaded (on page faults). You can't precisely control when disk I/O occurs, which can be problematic for real-time systems or when you need predictable I/O timing.

**Potential page faults**: Accessing unmapped pages triggers page faults (disk I/O). First access to a page is slow (page fault), though subsequent accesses are fast (cached). For cold starts or sparse access patterns, this can cause latency spikes.

**Synchronization complexity for writes**: When multiple processes write to the same mapped file, you need synchronization (locks, atomic operations). Copy-on-write helps but adds complexity for write coordination.

**Platform-specific behavior**: Memory-mapped I/O behavior can vary between operating systems (Windows vs. Linux). Page sizes, caching behavior, and shared memory semantics differ, requiring platform-specific considerations.

**Not always faster**: For sequential-only access of small files, traditional I/O can be faster (less overhead, better OS buffering). Memory-mapped I/O shines for random access and large files, but may not help for sequential-only patterns.

**Memory pressure**: If physical RAM is full, the OS may swap out mapped pages, causing page faults on subsequent access. This can degrade performance if the working set doesn't fit in RAM.

**Why these matter**: Memory-mapped I/O is a powerful tool but not a silver bullet. Use it where it helps (random access, large files), but understand the trade-offs (less control, potential page faults, synchronization complexity).

---

## When to Use This Approach

**Large files that don't fit in memory**: Files larger than available RAM benefit from memory-mapped I/O's lazy loading. Only accessed pages are loaded, enabling working with files much larger than RAM.

**Random access patterns**: When you need to access file data at random offsets, memory-mapped I/O is ideal. Random access is as simple as memory access, and the OS handles page loading efficiently.

**Shared file access**: When multiple processes need to access the same file, memory-mapped I/O enables efficient sharing. Processes share physical RAM pages (read-only), reducing memory usage.

**Database systems**: Databases often use memory-mapped I/O for data files. Random access to database records benefits from memory-mapped I/O's efficiency, and shared access enables multi-process database architectures.

**Configuration files and shared data**: Files that multiple processes read (configuration, shared data) benefit from memory-mapped I/O's shared memory. One copy in RAM, shared by all processes.

**Why these scenarios**: In all these cases, memory-mapped I/O provides clear benefits—efficient random access, automatic caching, shared memory, and lazy loading. These scenarios match memory-mapped I/O's strengths.

---

## Common Mistakes

**Mapping entire large files unnecessarily**: Mapping entire 100GB files when you only need 1GB of data. This creates unnecessary page table entries and can cause issues if you accidentally access unmapped regions. Map only the regions you need.

**Not handling page faults**: Assuming memory-mapped access is always fast. First access to a page triggers a page fault (disk I/O). Account for page fault latency in performance-critical code.

**Ignoring synchronization for writes**: Multiple processes writing to the same mapped file without synchronization. This causes data corruption. Use locks or atomic operations for write coordination.

**Not considering memory pressure**: Mapping large files on memory-constrained systems. If physical RAM is full, the OS swaps out mapped pages, causing performance degradation. Ensure adequate RAM or map smaller regions.

**Using memory-mapped I/O for sequential-only access**: Using memory-mapped I/O for purely sequential file access. Traditional I/O with buffering is often faster for sequential-only patterns. Use memory-mapped I/O for random access.

**Not closing mapped files**: Forgetting to dispose of `MemoryMappedFile` and `MemoryMappedViewAccessor`. This leaks resources and can prevent file access by other processes. Always use `using` statements or explicit disposal.

**Why these are mistakes**: They either waste resources, cause performance issues, or introduce bugs. Understand memory-mapped I/O's behavior (lazy loading, page faults, sharing) and use it appropriately.

---

## Optimization Techniques

### Technique 1: Map Only Required File Regions

**When**: You only need to access specific regions of a large file.

**The problem**:
```csharp
// ❌ Mapping entire large file unnecessarily
public class BadFullMapping
{
    public void ProcessLargeFile(string filePath)
    {
        // Maps entire 100GB file, even though we only need 1GB
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        using (var accessor = mmf.CreateViewAccessor()) // Maps entire file
        {
            // Only access first 1GB, but entire file is mapped
            for (long offset = 0; offset < 1_000_000_000; offset += 1000)
            {
                var value = accessor.ReadInt32(offset);
                ProcessValue(value);
            }
        }
    }
}
```

**Problems**:
- Maps entire file (creates page table entries for entire file)
- Wastes virtual address space
- Can cause issues if you accidentally access unmapped regions
- Unnecessary overhead for large files

**The solution**:
```csharp
// ✅ Map only required regions
public class GoodRegionMapping
{
    public void ProcessLargeFile(string filePath)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        {
            // Map only the region we need (1GB starting at offset 0)
            long regionSize = 1_000_000_000; // 1GB
            using (var accessor = mmf.CreateViewAccessor(0, regionSize))
            {
                // Access only mapped region
                for (long offset = 0; offset < regionSize; offset += 1000)
                {
                    var value = accessor.ReadInt32(offset);
                    ProcessValue(value);
                }
            }
        }
    }
}
```

**Why it works**: Mapping only required regions reduces virtual address space usage and page table overhead. You only create mappings for regions you actually access, reducing resource usage.

**Performance**: Reduces virtual address space usage and page table overhead. For large files, this can reduce overhead by 90%+ if you only access a small portion.

### Technique 2: Use Multiple Views for Sparse Access

**When**: You need to access sparse regions of a large file (not contiguous).

**The problem**:
```csharp
// ❌ Mapping large region for sparse access
public class BadSparseMapping
{
    public void ProcessSparseRegions(string filePath, long[] offsets)
    {
        // Maps entire region from min to max offset (wastes virtual address space)
        long minOffset = offsets.Min();
        long maxOffset = offsets.Max();
        long regionSize = maxOffset - minOffset + 1000; // Large region
        
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        using (var accessor = mmf.CreateViewAccessor(minOffset, regionSize))
        {
            foreach (var offset in offsets)
            {
                var value = accessor.ReadInt32(offset - minOffset); // Sparse access
                ProcessValue(value);
            }
        }
    }
}
```

**Problems**:
- Maps large region for sparse access (wastes virtual address space)
- Creates page table entries for unused regions
- Unnecessary overhead

**The solution**:
```csharp
// ✅ Use multiple views for sparse regions
public class GoodSparseMapping
{
    public void ProcessSparseRegions(string filePath, long[] offsets)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        {
            // Group nearby offsets to reduce number of views
            var groupedOffsets = GroupNearbyOffsets(offsets, pageSize: 4096);
            
            foreach (var group in groupedOffsets)
            {
                long regionStart = group.Min();
                long regionEnd = group.Max();
                long regionSize = regionEnd - regionStart + 1000; // Small region
                
                // Create view for this group only
                using (var accessor = mmf.CreateViewAccessor(regionStart, regionSize))
                {
                    foreach (var offset in group)
                    {
                        var value = accessor.ReadInt32(offset - regionStart);
                        ProcessValue(value);
                    }
                }
            }
        }
    }
    
    private List<List<long>> GroupNearbyOffsets(long[] offsets, long pageSize)
    {
        // Group offsets that are within pageSize of each other
        // Implementation depends on grouping strategy
        // ...
    }
}
```

**Why it works**: Creating multiple small views for sparse regions reduces virtual address space usage. You only map regions you actually access, minimizing overhead.

**Performance**: Reduces virtual address space usage for sparse access patterns. Can reduce overhead by 50-90% compared to mapping large regions.

### Technique 3: Pre-warm Frequently Accessed Regions

**When**: You know which regions will be accessed frequently and want to avoid page faults.

**The problem**:
```csharp
// ❌ Cold access causes page faults
public class BadColdAccess
{
    public void ProcessFile(string filePath)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        using (var accessor = mmf.CreateViewAccessor())
        {
            // First access to each page triggers page fault (slow)
            for (long offset = 0; offset < fileSize; offset += 4096)
            {
                var value = accessor.ReadInt32(offset); // Page fault on first access
                ProcessValue(value);
            }
        }
    }
}
```

**Problems**:
- First access to each page triggers page fault (disk I/O)
- Cold start latency (all pages must be loaded)
- Slow initial performance

**The solution**:
```csharp
// ✅ Pre-warm frequently accessed regions
public class GoodPreWarm
{
    public void ProcessFile(string filePath)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        using (var accessor = mmf.CreateViewAccessor())
        {
            // Pre-warm: Touch pages to load them into cache
            PreWarmRegion(accessor, 0, frequentlyAccessedSize);
            
            // Now access is fast (pages already in cache)
            for (long offset = 0; offset < frequentlyAccessedSize; offset += 4096)
            {
                var value = accessor.ReadInt32(offset); // Fast (cached)
                ProcessValue(value);
            }
        }
    }
    
    private void PreWarmRegion(MemoryMappedViewAccessor accessor, long start, long size)
    {
        // Touch each page to trigger loading (sequential access is efficient)
        for (long offset = start; offset < start + size; offset += 4096)
        {
            accessor.ReadByte(offset); // Touch page to load it
        }
    }
}
```

**Why it works**: Pre-warming loads pages into the OS cache before they're needed. Subsequent accesses are fast (no page faults). Sequential pre-warming is efficient (OS prefetches subsequent pages).

**Performance**: Eliminates page fault latency for pre-warmed regions. Can improve cold start performance by 50-80% for frequently accessed regions.

### Technique 4: Use Read-Only Access When Possible

**When**: You only need to read file data, not write it.

**The problem**:
```csharp
// ❌ Using read-write access when read-only is sufficient
public class BadReadWrite
{
    public void ReadFile(string filePath)
    {
        // Read-write access (more complex, copy-on-write overhead)
        using (var mmf = MemoryMappedFile.CreateFromFile(
            filePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.ReadWrite)) // Unnecessary write access
        using (var accessor = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.ReadWrite))
        {
            // Only reading, but using read-write access
            var value = accessor.ReadInt32(0);
            ProcessValue(value);
        }
    }
}
```

**Problems**:
- Read-write access is more complex (copy-on-write overhead)
- Can't share pages efficiently (each process may need private copy on write)
- Unnecessary overhead for read-only access

**The solution**:
```csharp
// ✅ Use read-only access when possible
public class GoodReadOnly
{
    public void ReadFile(string filePath)
    {
        // Read-only access (simpler, efficient sharing)
        using (var mmf = MemoryMappedFile.CreateFromFile(
            filePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.Read)) // Read-only
        using (var accessor = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.Read))
        {
            // Read-only access is simpler and more efficient
            var value = accessor.ReadInt32(0);
            ProcessValue(value);
        }
    }
}
```

**Why it works**: Read-only access is simpler and enables efficient sharing. Multiple processes can share the same physical RAM pages (no copy-on-write overhead). This reduces memory usage and improves performance.

**Performance**: Enables efficient multi-process sharing, reducing memory usage by 50-90% when multiple processes access the same file. Simpler code and better performance.

### Technique 5: Coordinate Writes with Synchronization

**When**: Multiple processes need to write to the same memory-mapped file.

**The problem**:
```csharp
// ❌ Unsynchronized writes cause data corruption
public class BadUnsyncedWrites
{
    public void WriteToFile(string filePath, int processId)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(
            filePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.ReadWrite))
        using (var accessor = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.ReadWrite))
        {
            // No synchronization - data corruption possible
            int currentValue = accessor.ReadInt32(0);
            accessor.Write(0, currentValue + 1); // Race condition!
        }
    }
}
```

**Problems**:
- No synchronization between processes
- Race conditions (multiple processes write simultaneously)
- Data corruption
- Incorrect results

**The solution**:
```csharp
// ✅ Use synchronization for writes
public class GoodSyncedWrites
{
    private readonly Mutex _writeMutex = new Mutex(false, "SharedFileMutex");
    
    public void WriteToFile(string filePath, int processId)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(
            filePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.ReadWrite))
        using (var accessor = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.ReadWrite))
        {
            // Synchronize writes with mutex
            _writeMutex.WaitOne();
            try
            {
                int currentValue = accessor.ReadInt32(0);
                accessor.Write(0, currentValue + 1); // Synchronized write
            }
            finally
            {
                _writeMutex.ReleaseMutex();
            }
        }
    }
}
```

**Why it works**: Synchronization (mutex, locks, atomic operations) coordinates writes between processes. Only one process writes at a time, preventing race conditions and data corruption.

**Performance**: Prevents data corruption and ensures correctness. Synchronization overhead is minimal compared to the cost of data corruption bugs.

---

## Example Scenarios

### Scenario 1: Database Index File Access

**Problem**: Accessing database index file with random record lookups.

**Current code (traditional I/O)**:
```csharp
// ❌ Traditional I/O for random access
public class DatabaseIndex
{
    public int LookupRecord(string indexFilePath, long recordOffset)
    {
        using (var fileStream = new FileStream(indexFilePath, FileMode.Open, FileAccess.Read))
        {
            // Each lookup requires seek + read (system call + potential disk seek)
            fileStream.Seek(recordOffset, SeekOrigin.Begin);
            byte[] buffer = new byte[4];
            fileStream.Read(buffer, 0, 4);
            return BitConverter.ToInt32(buffer, 0);
        }
    }
    
    public void ProcessRandomLookups(string indexFilePath, long[] offsets)
    {
        // 1000 random lookups = 1000 seeks + 1000 reads
        foreach (var offset in offsets)
        {
            var record = LookupRecord(indexFilePath, offset);
            ProcessRecord(record);
        }
    }
}
```

**Problems**:
- Each lookup requires seek + read (system call overhead)
- Random access causes many disk seeks (slow on mechanical drives)
- Manual buffer management
- No OS caching benefit for random access

**Improved code (memory-mapped I/O)**:
```csharp
// ✅ Memory-mapped I/O for random access
public class DatabaseIndex
{
    private MemoryMappedFile _mmf;
    private MemoryMappedViewAccessor _accessor;
    
    public void Initialize(string indexFilePath)
    {
        _mmf = MemoryMappedFile.CreateFromFile(
            indexFilePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.Read);
        _accessor = _mmf.CreateViewAccessor();
    }
    
    public int LookupRecord(long recordOffset)
    {
        // Random access is as simple as memory access
        return _accessor.ReadInt32(recordOffset);
    }
    
    public void ProcessRandomLookups(long[] offsets)
    {
        // 1000 random lookups = 1000 memory accesses (OS handles page loading)
        foreach (var offset in offsets)
        {
            var record = LookupRecord(offset); // Fast (cached pages) or page fault (first access)
            ProcessRecord(record);
        }
    }
    
    public void Dispose()
    {
        _accessor?.Dispose();
        _mmf?.Dispose();
    }
}
```

**Results**:
- **Performance**: 30-50% improvement for random access patterns
- **Code simplicity**: Simpler code (no manual buffer management)
- **OS caching**: Automatic OS caching improves subsequent accesses
- **Memory usage**: Only accessed pages in RAM (not entire file)

### Scenario 2: Large Configuration File Shared by Multiple Processes

**Problem**: Multiple processes need to read the same large configuration file.

**Current code (traditional I/O, per-process copies)**:
```csharp
// ❌ Each process loads entire file into memory
public class ConfigReader
{
    private Dictionary<string, string> _config;
    
    public void LoadConfig(string configFilePath)
    {
        // Each process loads entire file (wastes memory)
        var fileContent = File.ReadAllText(configFilePath);
        _config = ParseConfig(fileContent);
    }
    
    public string GetValue(string key)
    {
        return _config[key];
    }
}
```

**Problems**:
- Each process loads entire file into memory (wastes memory)
- No sharing between processes (each has its own copy)
- Memory usage: N processes × file size

**Improved code (memory-mapped I/O, shared memory)**:
```csharp
// ✅ Memory-mapped I/O with shared memory
public class ConfigReader
{
    private MemoryMappedFile _mmf;
    private MemoryMappedViewAccessor _accessor;
    private Dictionary<string, string> _config;
    
    public void LoadConfig(string configFilePath)
    {
        // Map file (read-only for efficient sharing)
        _mmf = MemoryMappedFile.CreateFromFile(
            configFilePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.Read);
        _accessor = _mmf.CreateViewAccessor();
        
        // Parse config from mapped file
        var fileSize = new FileInfo(configFilePath).Length;
        byte[] buffer = new byte[fileSize];
        _accessor.ReadArray(0, buffer, 0, (int)fileSize);
        _config = ParseConfig(Encoding.UTF8.GetString(buffer));
    }
    
    public string GetValue(string key)
    {
        return _config[key];
    }
    
    public void Dispose()
    {
        _accessor?.Dispose();
        _mmf?.Dispose();
    }
}
```

**Results**:
- **Memory usage**: 50-80% reduction (shared pages between processes)
- **Performance**: Faster access (OS caching, shared pages)
- **Scalability**: Better scalability (memory usage doesn't grow linearly with process count)

### Scenario 3: Sparse Access to Large Log File

**Problem**: Processing specific entries in a large log file (sparse access pattern).

**Current code (traditional I/O, loading entire file)**:
```csharp
// ❌ Loading entire file for sparse access
public class LogProcessor
{
    public void ProcessLogEntries(string logFilePath, long[] entryOffsets)
    {
        // Load entire file (wastes memory if file is large)
        var fileContent = File.ReadAllBytes(logFilePath);
        
        foreach (var offset in entryOffsets)
        {
            // Access specific entries (sparse access)
            var entry = ExtractEntry(fileContent, offset);
            ProcessEntry(entry);
        }
    }
}
```

**Problems**:
- Loads entire file into memory (wastes memory for sparse access)
- Memory usage: Entire file size (even if only accessing 1% of entries)
- Doesn't scale for very large files

**Improved code (memory-mapped I/O, lazy loading)**:
```csharp
// ✅ Memory-mapped I/O with lazy loading
public class LogProcessor
{
    public void ProcessLogEntries(string logFilePath, long[] entryOffsets)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(logFilePath))
        {
            // Group nearby offsets to minimize views
            var groupedOffsets = GroupNearbyOffsets(entryOffsets, pageSize: 4096);
            
            foreach (var group in groupedOffsets)
            {
                long regionStart = group.Min();
                long regionEnd = group.Max();
                long regionSize = regionEnd - regionStart + 1000;
                
                // Map only this region
                using (var accessor = mmf.CreateViewAccessor(regionStart, regionSize))
                {
                    foreach (var offset in group)
                    {
                        // Access entry (page fault loads page on first access)
                        var entry = ExtractEntry(accessor, offset - regionStart);
                        ProcessEntry(entry);
                    }
                }
            }
        }
    }
}
```

**Results**:
- **Memory usage**: 80-90% reduction (only accessed pages loaded)
- **Scalability**: Works with files larger than RAM
- **Performance**: Efficient sparse access (OS handles page loading)

---

## Summary and Key Takeaways

Memory-mapped I/O maps files directly into your process's virtual address space, enabling efficient random access, automatic OS caching, and shared memory access for large files without loading entire files into RAM. This improves performance by 20-50% for large file access compared to traditional file I/O, especially for random access patterns. The trade-off is less control over when pages are loaded (OS manages it), potential page faults if data isn't cached, and synchronization complexity for writes.

<!-- Tags: File I/O, Memory Management, Storage & I/O, Performance, Optimization, .NET Performance, C# Performance, System Design, Operating System Tuning -->
