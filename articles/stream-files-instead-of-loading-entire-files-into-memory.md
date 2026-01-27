# Stream Files Instead of Loading Entire Files into Memory

**Loading entire files into memory (`File.ReadAllText()`, `File.ReadAllBytes()`) requires allocating RAM equal to the file size, which can cause `OutOfMemoryException` for large files and increase GC pressure. Streaming files (reading in chunks) processes files of any size with constant memory usage, enabling processing of files larger than available RAM.**

---

## Executive Summary (TL;DR)

Loading entire files into memory requires allocating RAM equal to the file size. For a 1 GB file, you need 1 GB of free RAM. This causes `OutOfMemoryException` when files exceed available memory, increases GC pressure (large allocations trigger full GC), and wastes memory (you might only need to process a small portion). Streaming files (using `StreamReader`, `FileStream`, or `System.IO.Pipelines`) reads data in small chunks (e.g., 4–64 KB buffers), processing incrementally with constant memory usage. This enables processing files of any size (even larger than RAM), reduces GC pressure (smaller allocations), and improves memory efficiency. The trade-off is slightly higher complexity (need to manage buffers and state) and potentially slower for small files (<1 MB) due to overhead. Use streaming for large files (>100 MB), memory-constrained environments, or when you only need to process a portion of the file. Avoid streaming for small files (<1 MB) where the overhead isn't worth it, or when you need random access to the entire file. Typical improvements: Can process files 10×–1000× larger than available RAM, 50%–90% lower memory usage, reduced GC pauses.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you load an entire file into memory?**

```csharp
// ❌ Bad: Load entire file into memory
var content = File.ReadAllText("large-file.txt");  // What happens here?
ProcessContent(content);
```

**What happens:**
1. **`File.ReadAllText()` allocates a string** equal to the file size
2. **Reads entire file from disk** into that string (synchronous I/O)
3. **File data is now in RAM** (duplicated: disk cache + your string)
4. **GC pressure**: Large string allocation can trigger full GC
5. **Memory usage**: RAM usage = file size (e.g., 1 GB file = 1 GB RAM)

**Real-world example: Processing a 10 GB log file**

Imagine a log analysis tool that processes a 10 GB log file:

```csharp
// ❌ Bad: Load entire file
public void AnalyzeLogs(string logPath)
{
    var logs = File.ReadAllLines(logPath);  // 10 GB file → 10 GB RAM
    foreach (var log in logs)
    {
        AnalyzeLog(log);
    }
}
```

**What happens:**
- **Memory allocation**: Tries to allocate 10 GB for the string array
- **OutOfMemoryException**: If available RAM < 10 GB, the application crashes
- **GC pressure**: Even if it fits, allocating 10 GB triggers full GC (100–1000 ms pause)
- **Slow startup**: Must read entire 10 GB before processing starts

**Why this fails:**
- Most systems don't have 10 GB of free RAM
- Even if they do, allocating 10 GB wastes memory (you might only need to process a small portion)
- Full GC pauses make the application unresponsive

### Key Terms Explained (Start Here!)

**What is streaming?** Reading/writing data incrementally in small chunks (buffers) rather than loading everything into memory at once. Example: Reading a 1 GB file in 64 KB chunks uses only 64 KB of RAM, not 1 GB.

**What is a buffer?** A small chunk of memory (typically 4–64 KB) used to hold data temporarily during I/O operations. Streams read data into buffers, process it, then read the next chunk.

**What is `File.ReadAllText()`?** A .NET method that reads an entire file into a single string. It allocates a string equal to the file size, reads all data synchronously, and returns it. For large files, this can cause `OutOfMemoryException`.

**What is `StreamReader`?** A .NET class that reads text files incrementally (line-by-line or character-by-character). It uses an internal buffer (default: 1 KB) and reads from disk as needed, using constant memory regardless of file size.

**What is `System.IO.Pipelines`?** A high-performance streaming API in .NET that uses producer-consumer pattern with backpressure. It's optimized for zero-copy scenarios and async I/O, providing better performance than `StreamReader` for high-throughput scenarios.

**What is backpressure?** A mechanism where the consumer (reader) signals the producer (writer) to slow down when the consumer can't keep up. `System.IO.Pipelines` implements backpressure automatically to prevent memory buildup.

**What is GC pressure?** The workload on the garbage collector. Large allocations (e.g., 1 GB string) trigger full GC collections, which pause all threads for 100–1000 ms. Streaming reduces GC pressure by using smaller, short-lived allocations.

**What is sequential access?** Reading data from start to end in order. Streaming is optimized for sequential access. Random access (jumping to arbitrary positions) is slower with streaming.

### Common Misconceptions

**"Streaming is always slower than loading everything"**
- **The truth**: For large files (>100 MB), streaming is often faster because it starts processing immediately (no wait for entire file to load) and avoids GC pauses. For small files (<1 MB), loading everything can be faster due to fewer syscalls.

**"I need the entire file in memory to process it"**
- **The truth**: Most file processing (parsing, filtering, transformation) can be done incrementally. You rarely need the entire file in memory at once. Example: Counting lines in a file only requires processing one line at a time.

**"Streaming is too complex"**
- **The truth**: Basic streaming with `StreamReader` is simple (3–5 lines of code). `System.IO.Pipelines` is more complex but only needed for high-performance scenarios.

**"Memory is cheap, so loading large files is fine"**
- **The truth**: Even with plenty of RAM, loading large files causes GC pressure (full GC pauses), wastes memory (you might only need a small portion), and increases risk of OOM in production (memory fragmentation, other processes).

---

## How It Works

### Understanding Streaming vs Loading Entire File

**How `File.ReadAllText()` works (load entire file):**

```csharp
var content = File.ReadAllText("large-file.txt");
```

1. **Allocate string**: Allocate a string buffer equal to file size (e.g., 1 GB)
2. **Read entire file**: Read all 1 GB from disk into the string (synchronous, blocks until complete)
3. **Return string**: Return the entire string
4. **Memory usage**: 1 GB RAM (plus disk cache)

**Timeline:**
- **0 ms**: Start reading
- **1000 ms**: Still reading (file is 1 GB, disk is 100 MB/s)
- **10000 ms**: Finished reading, return string
- **10000 ms**: Start processing (only now!)

**How `StreamReader` works (streaming):**

```csharp
using var reader = new StreamReader("large-file.txt");
string line;
while ((line = reader.ReadLine()) != null)
{
    ProcessLine(line);
}
```

1. **Open file**: Open file handle (no data read yet)
2. **Read buffer**: Read first chunk (e.g., 4 KB) into internal buffer
3. **Process line**: Extract first line from buffer, process it
4. **Read next chunk**: If buffer is empty, read next 4 KB from disk
5. **Repeat**: Continue until end of file

**Timeline:**
- **0 ms**: Start reading first chunk
- **0.04 ms**: First chunk read (4 KB / 100 MB/s)
- **0.04 ms**: Start processing first line (immediately!)
- **0.08 ms**: Process second line (from same buffer)
- **...**: Continue processing while reading in background

**Key insight**: Streaming starts processing immediately (after first chunk), while loading entire file must wait for the entire file to be read.

### Technical Details: How Streaming Works in .NET

**What happens when you use `StreamReader`:**

```csharp
using var reader = new StreamReader("large-file.txt");
string line = reader.ReadLine();  // What happens here?
```

1. **`StreamReader` constructor**:
   - Opens file handle (`FileStream`)
   - Allocates internal buffer (default: 1 KB for `StreamReader`, 4 KB for underlying `FileStream`)
   - **Memory usage: ~5 KB** (buffer + object overhead)

2. **`ReadLine()` call**:
   - Checks if buffer has a complete line
   - If not, reads more data from disk into buffer (async if `ReadLineAsync()`)
   - Extracts line from buffer (creates new string)
   - Returns line
   - **Memory usage: ~5 KB + line size** (typically <1 KB per line)

3. **Next `ReadLine()` call**:
   - Reuses same buffer (no new allocation)
   - Reads next chunk if needed
   - **Memory usage: constant** (~5 KB)

**Memory usage comparison:**

| Approach | Memory Usage (1 GB file) |
|----------|--------------------------|
| **`File.ReadAllText()`** | 1 GB (entire file) |
| **`StreamReader`** | ~5 KB (buffer only) |
| **Improvement** | **200,000× less memory** |

**What happens when you use `System.IO.Pipelines`:**

```csharp
var pipe = new Pipe();
var writer = pipe.Writer;
var reader = pipe.Reader;

// Producer: Read from file, write to pipe
await FillPipeAsync(filePath, writer);

// Consumer: Read from pipe, process data
await ReadPipeAsync(reader);
```

1. **Pipe creation**:
   - Creates a buffer pool (reusable buffers)
   - Sets up producer-consumer communication
   - **Memory usage: ~64 KB** (initial buffer pool)

2. **Producer (`FillPipeAsync`)**:
   - Reads from file in chunks (e.g., 4 KB)
   - Writes to pipe buffer
   - If consumer is slow, backpressure pauses producer (prevents memory buildup)

3. **Consumer (`ReadPipeAsync`)**:
   - Reads from pipe buffer
   - Processes data
   - Returns buffer to pool when done (zero-copy, no allocations)

**Why `System.IO.Pipelines` is faster:**
- **Zero-copy**: Buffers are reused, not allocated per read
- **Backpressure**: Automatically prevents memory buildup
- **Async-optimized**: Better async/await performance than `StreamReader`
- **Buffer pooling**: Reuses buffers instead of allocating new ones

---

## Why This Becomes a Bottleneck

Loading entire files becomes a bottleneck because:

**Memory exhaustion**: Files larger than available RAM cause `OutOfMemoryException`. Example: 10 GB file on a system with 8 GB RAM = crash.

**GC pressure**: Large allocations trigger full GC collections. Example: Allocating 1 GB string triggers full GC (100–1000 ms pause), making the application unresponsive.

**Wasted memory**: You might only need to process a small portion of the file, but loading everything wastes memory. Example: Searching for a specific line in a 10 GB file only needs that line, not the entire file.

**Slow startup**: Must read entire file before processing starts. Example: Processing a 10 GB file: 100 seconds to load + 10 seconds to process = 110 seconds total. Streaming: 0 seconds to start + 110 seconds processing = 110 seconds total, but processing starts immediately.

**Memory fragmentation**: Large allocations can fragment the heap, making future allocations slower and increasing risk of OOM.

**Inefficient for partial processing**: If you only need the first 1 MB of a 10 GB file, loading everything wastes 9.999 GB of memory and I/O time.

---

## Advantages

**Can process files of any size**: Streaming uses constant memory (buffer size), so you can process files larger than available RAM. Example: Process a 100 GB file on a system with 8 GB RAM.

**Lower memory usage**: Uses only buffer size (4–64 KB) instead of entire file size. Example: 1 GB file: 1 GB → 64 KB = 16,000× less memory.

**Reduced GC pressure**: Smaller allocations reduce GC frequency and pause time. Example: 1 GB allocation triggers full GC (1000 ms pause), 64 KB allocations trigger minor GC (10 ms pause).

**Faster startup**: Processing starts immediately after first chunk, not after entire file is loaded. Example: 10 GB file: 100 seconds to load + 10 seconds to process = 110 seconds (load-first) vs 0 seconds to start + 110 seconds processing = 110 seconds (streaming, but starts immediately).

**Better for partial processing**: Can stop early if you find what you need. Example: Searching for a specific line can stop after finding it, not after reading the entire file.

**Enables processing of files larger than RAM**: Critical for big data, log analysis, ETL pipelines where files can be terabytes.

---

## Disadvantages and Trade-offs

**Slightly higher complexity**: Managing buffers and state is more complex than `File.ReadAllText()`. Requires understanding of streams, buffers, and async I/O.

**Potentially slower for small files**: For files <1 MB, the overhead of buffer management and multiple syscalls can be slower than loading everything. Example: 100 KB file: `ReadAllText()` = 1 ms, `StreamReader` = 1.5 ms (50% slower).

**No random access**: Streaming is sequential. Can't jump to arbitrary positions without reading from the beginning. Example: Can't read line 1,000,000 without reading lines 1–999,999 first.

**State management**: Need to track position, handle partial reads, manage buffer state. More error-prone than simple `ReadAllText()`.

**Less convenient for simple cases**: For small files where you need the entire content, `ReadAllText()` is simpler and often faster.

---

## When to Use This Approach

Use streaming when:

- **Large files** (>100 MB). Example: Log files, database dumps, CSV files, JSON files. Streaming is essential for files larger than available RAM.
- **Memory is constrained** (containers, embedded systems, mobile). Example: Docker containers with 512 MB RAM limit, processing 1 GB files.
- **Processing can be incremental** (line-by-line, record-by-record). Example: Parsing logs, filtering data, transforming CSV rows.
- **Files are larger than available RAM**. Example: Processing 10 GB files on 8 GB RAM systems.
- **You only need a portion of the file**. Example: Searching for a specific line, processing first N records, early termination.
- **High-throughput scenarios** (ETL, data pipelines). Example: Processing millions of records where memory efficiency matters.

---

## When Not to Use It

Avoid streaming when:

- **Small files** (<1 MB). Example: Configuration files, small JSON responses. The overhead isn't worth it, and `ReadAllText()` is simpler and often faster.
- **You need random access** (jump to arbitrary positions). Example: Database index files, binary formats with offset tables. Use memory-mapped files or load into memory.
- **You need the entire file in memory anyway**. Example: Image processing that requires the entire image, cryptographic operations on entire file.
- **Simplicity is more important than performance**. Example: Scripts, one-off tools where development speed matters more than memory efficiency.
- **File is already in page cache**. Example: Frequently accessed small files that are already cached by the OS. Loading everything might be faster due to cache hits.

---

## Performance Impact

Typical improvements when streaming instead of loading entire files:

- **Memory usage**: **100×–10,000× lower**. Example: 1 GB file: 1 GB → 64 KB = 16,000× less memory.
- **Can process files larger than RAM**: Enables processing files 10×–1000× larger than available RAM. Example: Process 100 GB file on 8 GB RAM system.
- **GC pause reduction**: 50%–90% lower GC pause time. Example: 1 GB allocation triggers 1000 ms full GC, 64 KB allocations trigger 10 ms minor GC (100× faster).
- **Startup time**: Processing starts immediately (0 ms) vs after entire file loads (seconds to minutes). Example: 10 GB file: 100 seconds to load before processing starts vs immediate start with streaming.

**Important**: For small files (<1 MB), streaming can be 10%–50% slower due to overhead. For large files (>100 MB), streaming is often faster due to immediate processing start and reduced GC pauses.

---

## Common Mistakes

**Using `File.ReadAllText()` for large files**: This causes `OutOfMemoryException` for files larger than available RAM. Always use streaming for files >100 MB.

**Not disposing streams**: Leaking `StreamReader` or `FileStream` keeps file handles open, exhausting file descriptors. Always use `using` statements.

**Reading entire file into memory when you only need a portion**: Example: Loading 10 GB file to search for one line. Use streaming and stop after finding the line.

**Using synchronous I/O in async methods**: `StreamReader.ReadLine()` blocks the thread. Use `ReadLineAsync()` in async methods.

**Not handling partial reads correctly**: Assuming a single `Read()` call reads all data. Always check return value and loop until all data is read.

**Using small buffers for high-throughput scenarios**: Default `StreamReader` buffer (1 KB) is too small for high-throughput. Use larger buffers (64 KB) or `System.IO.Pipelines`.

**Not using `System.IO.Pipelines` for high-performance scenarios**: For high-throughput (>100 MB/s), `System.IO.Pipelines` provides better performance than `StreamReader`.

---

## Example Scenarios

### Scenario 1: Processing large log files

**Problem**: A log analysis tool processes 10 GB log files. Loading entire file causes `OutOfMemoryException` on systems with <10 GB RAM.

**Bad approach** (load entire file):

```csharp
// ❌ Bad: Load entire file into memory
public void AnalyzeLogs(string logPath)
{
    var logs = File.ReadAllLines(logPath);  // 10 GB → OutOfMemoryException!
    foreach (var log in logs)
    {
        if (IsErrorLog(log))
        {
            AnalyzeError(log);
        }
    }
}
```

**Good approach** (stream line-by-line):

```csharp
// ✅ Good: Stream file line-by-line
public void AnalyzeLogs(string logPath)
{
    using var reader = new StreamReader(logPath);
    string line;
    while ((line = reader.ReadLine()) != null)
    {
        if (IsErrorLog(line))
        {
            AnalyzeError(line);
        }
    }
    // Memory usage: ~5 KB (buffer) instead of 10 GB
}
```

**Results**:
- **Bad**: `OutOfMemoryException` on systems with <10 GB RAM
- **Good**: Works on any system, uses ~5 KB memory
- **Improvement**: Can process files of any size

---

### Scenario 2: Searching for specific content (early termination)

**Problem**: Search for a specific line in a 10 GB log file. Loading entire file wastes memory and I/O time.

**Bad approach** (load entire file):

```csharp
// ❌ Bad: Load entire file, then search
public string FindLogEntry(string logPath, string searchTerm)
{
    var logs = File.ReadAllLines(logPath);  // 10 GB loaded, 100 seconds
    return logs.FirstOrDefault(log => log.Contains(searchTerm));
    // Wasted: Loaded 10 GB but might only need first 1 MB
}
```

**Good approach** (stream with early termination):

```csharp
// ✅ Good: Stream and stop when found
public string FindLogEntry(string logPath, string searchTerm)
{
    using var reader = new StreamReader(logPath);
    string line;
    while ((line = reader.ReadLine()) != null)
    {
        if (line.Contains(searchTerm))
        {
            return line;  // Stop early, don't read rest of file
        }
    }
    return null;
    // Memory usage: ~5 KB, stops after finding match (might only read 1 MB)
}
```

**Results**:
- **Bad**: 100 seconds to load 10 GB, then search (wastes 9.999 GB if match is early)
- **Good**: Stops after finding match (might only read 1 MB = 0.01 seconds)
- **Improvement**: 10,000× faster if match is early in file

---

### Scenario 3: High-throughput CSV processing (use Pipelines)

**Problem**: ETL pipeline processes 100 GB CSV files at 500 MB/s. Need maximum throughput with minimal memory.

**Bad approach** (load entire file):

```csharp
// ❌ Bad: Load entire file
public void ProcessCsv(string csvPath)
{
    var lines = File.ReadAllLines(csvPath);  // 100 GB → OutOfMemoryException
    foreach (var line in lines)
    {
        ProcessCsvRow(line);
    }
}
```

**Good approach** (use `System.IO.Pipelines` for high performance):

```csharp
// ✅ Good: Use System.IO.Pipelines for high throughput
using System.IO.Pipelines;
using System.Text;

public async Task ProcessCsvAsync(string csvPath)
{
    var pipe = new Pipe();
    var fillTask = FillPipeAsync(csvPath, pipe.Writer);
    var readTask = ReadPipeAsync(pipe.Reader);
    
    await Task.WhenAll(fillTask, readTask);
}

private async Task FillPipeAsync(string filePath, PipeWriter writer)
{
    using var file = File.OpenRead(filePath);
    
    while (true)
    {
        var memory = writer.GetMemory(64 * 1024);  // 64 KB buffer
        int bytesRead = await file.ReadAsync(memory);
        
        if (bytesRead == 0)
            break;
            
        writer.Advance(bytesRead);
        var result = await writer.FlushAsync();
        
        if (result.IsCompleted)
            break;
    }
    
    await writer.CompleteAsync();
}

private async Task ReadPipeAsync(PipeReader reader)
{
    while (true)
    {
        var result = await reader.ReadAsync();
        var buffer = result.Buffer;
        
        // Process CSV lines from buffer
        var position = ProcessBuffer(buffer);
        
        reader.AdvanceTo(position);
        
        if (result.IsCompleted)
            break;
    }
    
    await reader.CompleteAsync();
}

private SequencePosition ProcessBuffer(ReadOnlySequence<byte> buffer)
{
    var reader = new SequenceReader<byte>(buffer);
    
    while (reader.TryReadTo(out ReadOnlySpan<byte> line, (byte)'\n'))
    {
        var lineStr = Encoding.UTF8.GetString(line);
        ProcessCsvRow(lineStr);
    }
    
    return reader.Position;
}
```

**Results**:
- **Bad**: `OutOfMemoryException` for 100 GB file
- **Good**: Processes 100 GB file with ~64 KB memory, high throughput (500 MB/s)
- **Improvement**: Can process files of any size, zero-copy with buffer pooling

---

## Summary and Key Takeaways

Loading entire files into memory (`File.ReadAllText()`, `File.ReadAllBytes()`) requires RAM equal to file size, causing `OutOfMemoryException` for large files and increasing GC pressure. Streaming files (using `StreamReader`, `FileStream`, or `System.IO.Pipelines`) reads data in small chunks (4–64 KB buffers), processing incrementally with constant memory usage. This enables processing files of any size (even larger than RAM), reduces memory usage by 100×–10,000×, and reduces GC pauses by 50%–90%. The trade-off is slightly higher complexity and potentially slower for small files (<1 MB) due to overhead. Use streaming for large files (>100 MB), memory-constrained environments, or when you only need to process a portion of the file. Avoid streaming for small files (<1 MB) where overhead isn't worth it, or when you need random access to the entire file. For high-throughput scenarios (>100 MB/s), use `System.IO.Pipelines` instead of `StreamReader` for better performance. Always dispose streams properly (`using` statements) to avoid file descriptor leaks. The "right" answer depends on file size: <1 MB = load everything, >100 MB = stream, 1–100 MB = measure and choose based on memory constraints.

---

<!-- Tags: Performance, Optimization, Storage & I/O, File I/O, Memory Management, .NET Performance, C# Performance, Garbage Collection, Throughput Optimization, System Design, Architecture, Profiling, Benchmarking, Measurement -->
