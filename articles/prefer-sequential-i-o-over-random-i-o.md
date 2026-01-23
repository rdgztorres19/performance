# Prefer Sequential I/O Over Random I/O

**Design file access so reads/writes happen in large contiguous chunks, not many small jumps. This maximizes throughput and minimizes per-operation latency overhead.**

---

## Executive Summary (TL;DR)

Sequential I/O means reading or writing data in order (few large operations). Random I/O means jumping around the file (many small operations). Random I/O is slow because each operation has fixed overhead (syscalls, scheduling, device command processing) and fixed latency—especially high on HDDs due to mechanical seeks (5–15 ms per seek), but still meaningful on SSDs due to controller overhead, Flash Translation Layer (FTL) work, and queueing. If you can't make access fully sequential, you can often get most of the benefit by batching requests, sorting offsets, and coalescing nearby offsets into larger ranges. Validate improvements using throughput (MB/s) and latency percentiles (p95/p99), and be explicit about whether you're testing warm cache (data in RAM) or cold cache (data from disk). Use sequential I/O for logs, CSV/JSONL, backups, ETL, or large file transforms. Avoid forcing sequentialization for strict low-latency point reads—use a database or indexed format instead.

---

## Problem Context

### Understanding the Basic Problem

**What is I/O?** I/O (Input/Output) refers to reading from or writing to storage devices (disks, SSDs, network storage). I/O is often the slowest part of a system because storage is much slower than CPU and RAM. Understanding I/O patterns is critical for performance.

**What is sequential I/O?** Reading or writing data in order, from start to end, in large contiguous chunks. Example: reading a 10 GB log file from beginning to end. The OS and device can optimize for this pattern (read-ahead, streaming bandwidth).

**What is random I/O?** Jumping around the file, reading or writing small chunks at scattered locations. Example: reading 10,000 different 1 KB chunks from random offsets in a 10 GB file. Each jump (seek) has overhead.

**The problem with random I/O**: Each I/O operation has fixed overhead:
1. **Syscall overhead**: Calling into the OS (context switch, kernel work)
2. **Scheduling overhead**: The OS schedules the I/O request
3. **Device overhead**: The device processes the command (seek on HDD, FTL work on SSD, queueing)
4. **Latency**: Time to actually fetch the data (mechanical seek on HDD, flash access on SSD, network round trip on network storage)

For small random reads, you pay this overhead repeatedly. For large sequential reads, you amortize the overhead over many bytes.

**Real-world example**: Imagine processing a 10 GB CSV file to count lines containing "ERROR":

```csharp
// ❌ Bad: Random access (seeking around the file)
public long CountErrorsRandom(string csvPath, long[] lineOffsets)
{
    using var fs = File.OpenRead(csvPath);
    long errorCount = 0;
    var buffer = new byte[1024];

    foreach (var offset in lineOffsets)
    {
        fs.Seek(offset, SeekOrigin.Begin); // Random jump
        int bytesRead = fs.Read(buffer, 0, buffer.Length); // Small read
        string line = System.Text.Encoding.UTF8.GetString(buffer, 0, bytesRead);
        if (line.Contains("ERROR")) errorCount++;
    }

    return errorCount;
}
```

If you have 1 million line offsets, you're doing 1 million seeks and 1 million small reads. On an HDD, this could take hours. On an SSD, it's still slow because you're IOPS-limited (operations per second), not bandwidth-limited.

**Why this matters**: Storage devices are optimized for sequential access. HDDs have mechanical seeks (5–15 ms each). SSDs have per-operation overhead (FTL work, queueing). Network storage has round-trip latency (1–10 ms). Random I/O multiplies these costs.

### Key Terms Explained (Start Here!)

Before diving deeper, let's understand the basic building blocks:

**What is throughput (bandwidth)?** How many bytes per second you can read/write (MB/s, GB/s). This is what you care about for bulk operations like backups, ETL, or log processing. Example: "My HDD can do 150 MB/s sequential reads."

**What is latency?** How long a single I/O operation takes (µs/ms). Tail latency (p95/p99) is what hurts user-facing systems under load. Example: "p99 read latency is 50 ms."

**What is IOPS (I/O Operations Per Second)?** How many read/write operations the device can handle per second. Random workloads with small requests often become IOPS-limited (you hit the operations limit before the bandwidth limit). Example: "My HDD can do 100–200 IOPS for random reads."

**What is seek time (HDD only)?** Mechanical head movement to a different track/sector. This is why random I/O on HDDs is so expensive (5–15 ms per seek). Example: "Each seek costs ~10 ms, so 1000 seeks = 10 seconds just seeking."

**What is queue depth?** Number of in-flight I/O requests. SSDs and NVMe drives can use higher queue depths (32–256+) to reach peak throughput; HDDs benefit less because they're mechanically serialized. Example: "NVMe can handle 256 requests in parallel."

**What is the page cache?** The OS keeps recently read file data in RAM. Many "disk reads" are actually served from memory (fast), which can make benchmarks misleading if you don't account for it. Example: "I benchmarked and got 5 GB/s—but I was reading from the page cache (RAM), not the disk."

**What is read-ahead?** If the OS detects a sequential access pattern, it prefetches upcoming pages automatically, making sequential reads even faster. Example: "When I read sequentially, the OS reads ahead and my next read is already in RAM."

**What is FTL (Flash Translation Layer)?** SSD firmware that maps logical blocks to physical flash pages/blocks. It adds per-operation overhead and can amplify writes (write amplification). Example: "Each SSD I/O has FTL overhead, so many small I/Os are slower than fewer large I/Os."

**What is a hot path?** Code that executes frequently—like code inside loops, frequently called functions, or performance-critical sections. Optimizing hot paths provides the biggest performance gains. Example: "This function processes 10,000 requests/second—it's a hot path."

**What is read amplification?** Reading more bytes than strictly needed. Example: "I need 10 bytes at offset 1000, but I read a 4 KB block to reduce the number of I/O operations—this is read amplification."

### Disk / Storage Types (Why HDD vs SSD vs NVMe Behaves Differently)

Most "random vs sequential" confusion comes from not knowing what storage you're on:

**HDD (spinning disk)**:
- **How it works**: Mechanical platters spin, a head moves to read/write data
- **Sequential**: 100–150 MB/s (limited by platter rotation speed)
- **Random**: 0.5–2 MB/s (limited by ~100–200 IOPS due to seek time)
- **Seek time**: 5–15 ms per seek (mechanical movement)
- **Why sequential is so much faster**: Avoids repeated seeks. The head stays in one area and reads continuously.
- **Example**: A 7200 RPM HDD might do 150 MB/s sequential but only 1 MB/s for small random reads (100 IOPS × 10 KB/read).

**SATA SSD**:
- **How it works**: Flash memory, no mechanical parts, but limited by SATA interface (~550 MB/s)
- **Sequential**: 500–550 MB/s (SATA interface limit)
- **Random**: 50–200 MB/s (limited by ~10,000–100,000 IOPS and per-operation overhead)
- **No seek time**: But still has per-operation overhead (FTL work, command processing)
- **Why sequential is faster**: Amortizes per-operation overhead over more bytes. Large reads use bandwidth more efficiently.
- **Example**: A SATA SSD might do 550 MB/s sequential but only 100 MB/s for small random reads (25,000 IOPS × 4 KB/read).

**NVMe SSD**:
- **How it works**: Flash memory, PCIe interface, multiple queues for parallelism
- **Sequential**: 3–7 GB/s (PCIe bandwidth)
- **Random**: 1–3 GB/s (limited by ~500,000+ IOPS and per-operation overhead)
- **Lower latency**: ~10–100 µs per operation (vs ~1–10 ms for SATA SSD)
- **Why sequential is still faster**: Even with high IOPS, large sequential reads maximize bandwidth and reduce per-byte overhead.
- **Example**: An NVMe SSD might do 5 GB/s sequential but only 2 GB/s for small random reads (500,000 IOPS × 4 KB/read).

**Network-attached storage (NFS, SMB, cloud block storage like EBS)**:
- **How it works**: Storage over the network (adds network latency and shared backend contention)
- **Sequential**: Varies (often 100–500 MB/s, depends on network and backend)
- **Random**: Much slower (limited by network round trips and shared backend IOPS)
- **Network latency**: 1–10 ms per operation (adds to device latency)
- **Why sequential is much faster**: Amortizes network round trips and backend queueing over more bytes.
- **Example**: Cloud block storage might do 200 MB/s sequential but only 10 MB/s for small random reads (1,000 IOPS × 10 KB/read, limited by network latency).

### Common Misconceptions

**"SSDs make random I/O basically free"**
- **The truth**: SSDs are much better than HDDs for random I/O, but small random reads still have per-operation overhead and can become IOPS-limited. Sequential is still faster. Example: An NVMe SSD might do 5 GB/s sequential but only 2 GB/s for small random reads.

**"If I add more threads, I/O will be faster"**
- **The truth**: If you're already saturating the device's IOPS or bandwidth, more threads just increase contention and queueing. More threads help if you're not saturating the device, but won't help if you're already at the limit.

**"I benchmarked and it's fast"**
- **The truth**: You might have benchmarked the OS page cache (RAM), not the disk. Always test with data larger than RAM if you want "real disk" numbers.

**"Sequential I/O is only for HDDs"**
- **The truth**: Sequential I/O helps on all storage types (HDD, SSD, NVMe, network). The gap narrows on faster devices, but sequential is still faster.

**"Random I/O is always bad"**
- **The truth**: Random I/O is necessary for point lookups (e.g., "get user by ID"). The key is to use the right tool (database with indexes) instead of ad-hoc random seeks on flat files.

---

## How It Works

### Understanding Sequential vs Random I/O

**How sequential I/O works** (best case):
1. Application issues a large read (e.g., 1 MB)
2. OS detects sequential pattern and enables read-ahead (prefetches upcoming data)
3. Device streams data continuously (no seeks on HDD, efficient FTL work on SSD)
4. Data is delivered at near-maximum bandwidth (e.g., 150 MB/s on HDD, 5 GB/s on NVMe)
5. Few operations = low overhead per byte

**How random I/O works** (worst case):
1. Application issues a small read at a random offset (e.g., 4 KB at offset 1,234,567)
2. OS cannot use read-ahead (pattern is unpredictable)
3. Device must seek (HDD) or do FTL work (SSD) for each operation
4. Each operation has overhead (syscall, scheduling, device command, latency)
5. Many operations = high overhead per byte

**Key difference**: Sequential I/O amortizes overhead over many bytes. Random I/O pays overhead repeatedly for each small operation.

### Technical Details: What Happens at the Hardware Level

**HDD (spinning disk)**:
- **Sequential**: Head stays in one area, reads continuously. Limited by platter rotation speed (~100–150 MB/s).
- **Random**: Head must seek (move) to each location. Seek time dominates (5–15 ms per seek). Example: 1000 random reads = 1000 seeks = 5–15 seconds just seeking.

**SSD (flash memory)**:
- **Sequential**: FTL can optimize for sequential access (fewer mapping lookups, better parallelism). Reads are fast (~100–1000 µs per operation).
- **Random**: FTL must map each logical block to physical flash pages. More mapping lookups, less parallelism. Still fast, but slower than sequential.

**NVMe (PCIe SSD)**:
- **Sequential**: Multiple queues allow high parallelism. Can reach 3–7 GB/s.
- **Random**: High IOPS (500,000+), but per-operation overhead still exists. Can reach 1–3 GB/s for random, but sequential is still faster.

**Network storage**:
- **Sequential**: Amortizes network round trips over many bytes. Can reach 100–500 MB/s.
- **Random**: Each operation includes network latency (1–10 ms). Limited by network round trips and shared backend IOPS. Often 10–50 MB/s for small random reads.

### What About Writes? (Sequential vs Random Write)

Everything above applies to **writes** too—often with even bigger impact:

**Sequential writes are faster because**:
- **HDD**: No seeks between writes. The head writes continuously to adjacent sectors. Example: 100–150 MB/s sequential vs 1–2 MB/s random.
- **SSD**: Reduces write amplification. SSDs must erase blocks before writing, and sequential writes align better with flash block boundaries. Random writes can cause 2×–10× write amplification (writing 1 KB may require erasing/rewriting 256 KB).
- **Journaling/logging**: Append-only logs (like database WAL, Kafka, log files) are fast because they're purely sequential writes.

**Random writes are slower because**:
- **HDD**: Each write requires a seek. If you write 10,000 small records to random locations, you pay 10,000 seeks.
- **SSD**: Write amplification increases. The FTL must update mapping tables, and random writes can trigger more garbage collection (internal SSD cleanup).
- **Durability overhead**: Each `fsync()` or `FlushFileBuffers()` forces data to disk. Random writes with frequent syncs multiply this cost.

**Real-world write examples**:

**✅ Good: Append-only log (sequential write)**
```csharp
// Fast: sequential writes to end of file
public static void AppendLog(string logPath, string message)
{
    using var fs = new FileStream(
        logPath,
        FileMode.Append,  // Always write to end (sequential)
        FileAccess.Write,
        FileShare.Read,
        bufferSize: 64 * 1024);

    using var writer = new StreamWriter(fs);
    writer.WriteLine($"{DateTime.UtcNow:O} {message}");
    // Optional: writer.Flush(); fs.Flush(true); for durability
}
```

**Why it's fast**: Writes always go to the end of the file (sequential). No seeks. OS can batch writes efficiently.

**❌ Bad: Random writes (updating records in place)**
```csharp
// Slow: random seeks to update records
public static void UpdateRecord(string filePath, long offset, byte[] newData)
{
    using var fs = File.OpenWrite(filePath);
    fs.Seek(offset, SeekOrigin.Begin);  // Random seek
    fs.Write(newData, 0, newData.Length);  // Small write
    fs.Flush(true);  // Force to disk (expensive!)
}
```

**Why it's slow**: Each update requires a seek. If you update 10,000 records, you pay 10,000 seeks + 10,000 syncs.

**Key write patterns**:
- **Append-only logs**: Fast (sequential writes). Used by databases (WAL), Kafka, log files.
- **Update-in-place**: Slow (random writes). Avoid for high-throughput systems.
- **Batch writes**: Buffer multiple writes, then write sequentially. Reduces overhead.

---

## When to Use This Approach

Prefer sequential access patterns when:

- You process **logs, CSV/JSONL, backups, exports, ETL, or large file transforms** where you touch most of the data anyway. Example: "Count lines containing 'ERROR' in a 10 GB log file."
- You scan big files repeatedly (analytics jobs, search indexing, data ingestion). Example: "Build a search index from a 50 GB JSONL file."
- Your storage format supports **append + sequential reads** (write-ahead logs, LSM trees, segment files). Example: "Kafka uses append-only logs for high throughput."
- You can **reorder or batch work** without breaking product requirements (e.g., batch processing instead of real-time point queries). Example: "Process 1000 requests in a batch instead of one-by-one."
- You're writing **logs, events, or append-only data** where sequential writes are natural. Example: "Application logs, audit trails, event streams."

---

## When Not to Use It

Avoid forcing sequentialization when:

- You need **strict low-latency point reads** and you can't batch/sort without breaking user experience (e.g., "get user profile by ID in <10ms"). Use a database with indexes instead.
- A **database or indexed format** is the right tool (don't reinvent random-access storage with flat-file seeks). Databases use B-trees, LSM trees, or other structures optimized for point lookups.
- Your data is already **small and fits in the page cache**, so access pattern doesn't matter much (you're reading from RAM). Example: "My 100 MB file fits in RAM, so random vs sequential doesn't matter."

---

## Performance Impact

Rule-of-thumb expectations (highly workload-dependent):

- **HDD**: Sequential can be **5×–50×** faster than random because seeks dominate random reads. Example: 150 MB/s sequential vs 1 MB/s for small random reads (100 IOPS × 10 KB).
- **SATA SSD**: Sequential can be **2×–5×** faster than small random I/O, especially when random reads are small (4 KB) and queue depth is low. Example: 550 MB/s sequential vs 100 MB/s for small random reads.
- **NVMe SSD**: Sequential can be **1.5×–3×** faster than random if random reads are small and queue depth is low. The gap narrows if you can keep queue depth high (32+) and use larger random reads (64 KB+). Example: 5 GB/s sequential vs 2 GB/s for small random reads.

Important: If your data is served from the **page cache**, both patterns may look fast because you're measuring RAM, not disk. Always test with data larger than RAM if you want "real disk" numbers.

---

## Example Scenarios

### Scenario 1: Bad — Reading a CSV byte-by-byte (anti-pattern)

**Problem**: Reading a CSV file byte-by-byte creates massive overhead. Each byte read is a function call, preventing efficient bulk reads and read-ahead.

**Current code (slow)**:
```csharp
// ❌ Heap allocation for each packet
using System;
using System.IO;

public static class BadCsvReader
{
    // This is intentionally bad: it reads 1 byte at a time.
    public static long CountNewlinesByteByByte(string csvPath)
    {
        using var fs = File.OpenRead(csvPath);
        long lines = 0;

        int b;
        while ((b = fs.ReadByte()) != -1)
        {
            if (b == '\n') lines++;
        }

        return lines;
    }
}
```

**Problems**:
- One function call per byte (massive overhead)
- Prevents efficient bulk reads
- OS cannot use read-ahead
- On a 1 GB file, this might take 10–60 seconds instead of <1 second

**Why this happens in real code**:
- "Cute" abstractions or per-character parsing without buffering
- Using `StreamReader.Read()` (single char) instead of `ReadLine()` or `ReadAsync()` with buffers

**Improved code (faster)**:
```csharp
// ✅ Stack allocation for each packet
using System;
using System.IO;

public static class GoodCsvReader
{
    public static long CountNewlinesStreaming(string csvPath)
    {
        using var fs = new FileStream(
            csvPath,
            FileMode.Open,
            FileAccess.Read,
            FileShare.Read,
            bufferSize: 256 * 1024,  // 256 KB buffer
            options: FileOptions.SequentialScan);  // Hint to OS

        var buffer = new byte[256 * 1024];
        long lines = 0;
        int bytesRead;

        while ((bytesRead = fs.Read(buffer, 0, buffer.Length)) > 0)
        {
            for (int i = 0; i < bytesRead; i++)
            {
                if (buffer[i] == (byte)'\n') lines++;
            }
        }

        return lines;
    }
}
```

**Results**:
- **Throughput**: 100–150 MB/s on HDD, 500+ MB/s on SSD (vs <1 MB/s byte-by-byte)
- **Time**: <1 second for 1 GB file (vs 10–60 seconds)
- **Why it works**: Large buffers (256 KB) reduce syscalls. Sequential access enables OS read-ahead. Single pass through the file.

---

### Scenario 2: Practical — Processing a large log file

**Problem**: Counting lines containing "ERROR" in a 10 GB log file. Sequential scan is fast; random access would be slow.

**Current code (fast)**:
```csharp
using System;
using System.IO;
using System.Text;

public static class LogProcessor
{
    // Example: count lines containing "ERROR" in a 10 GB log file.
    public static long CountErrors(string logPath)
    {
        using var fs = new FileStream(
            logPath,
            FileMode.Open,
            FileAccess.Read,
            FileShare.Read,
            bufferSize: 1024 * 1024,  // 1 MB buffer
            options: FileOptions.SequentialScan);

        using var reader = new StreamReader(fs, Encoding.UTF8, detectEncodingFromByteOrderMarks: true, bufferSize: 1024 * 1024);

        long errorCount = 0;
        string? line;

        while ((line = reader.ReadLine()) != null)
        {
            if (line.Contains("ERROR", StringComparison.OrdinalIgnoreCase))
                errorCount++;
        }

        return errorCount;
    }
}
```

**Why this works well**:
- One sequential pass through the file (no seeks)
- Large buffers (1 MB) minimize syscalls
- The OS can use read-ahead to prefetch upcoming data
- `FileOptions.SequentialScan` hints to the OS to optimize caching

**Performance expectations**:
- **HDD**: ~100–150 MB/s → 10 GB in ~70–100 seconds
- **SATA SSD**: ~500 MB/s → 10 GB in ~20 seconds
- **NVMe SSD**: ~2–5 GB/s → 10 GB in ~2–5 seconds (if not CPU-bound by string parsing)

**When this is the wrong approach**:
- If you only need 10 lines out of 100 million, scanning the whole file is wasteful. In that case, build an index (see Scenario 3).

---

### Scenario 3: When you need random access — Build an index first

**Problem**: Looking up specific records by ID in a large file. Random seeks are slow. Solution: build an index once, then use it for fast lookups.

**Strategy**:
1. **First pass (build index)**: Scan the file sequentially and build an in-memory or on-disk index (e.g., `Dictionary<int, long>` mapping record ID to file offset).
2. **Second pass (lookups)**: Use the index to find offsets, then batch and sort them before reading.

**Code**:
```csharp
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;

public static class IndexedFileReader
{
    // Step 1: Build index (sequential scan)
    public static Dictionary<int, long> BuildIndex(string filePath)
    {
        var index = new Dictionary<int, long>();

        using var fs = new FileStream(filePath, FileMode.Open, FileAccess.Read, FileShare.Read, bufferSize: 1024 * 1024, options: FileOptions.SequentialScan);
        using var reader = new StreamReader(fs);

        string? line;
        long offset = 0;

        while ((line = reader.ReadLine()) != null)
        {
            int id = ParseId(line);
            index[id] = offset;
            offset = fs.Position;
        }

        return index;
    }

    // Step 2: Batch lookups (sort offsets, then read)
    public static List<string> ReadRecords(string filePath, Dictionary<int, long> index, int[] ids)
    {
        var offsets = ids.Select(id => index[id]).OrderBy(o => o).ToArray();
        var results = new List<string>();

        using var fs = File.OpenRead(filePath);
        using var reader = new StreamReader(fs);

        foreach (var offset in offsets)
        {
            fs.Seek(offset, SeekOrigin.Begin);
            string? line = reader.ReadLine();
            if (line != null) results.Add(line);
        }

        return results;
    }

    private static int ParseId(string line)
    {
        // Example: "123,foo,bar" -> 123
        int comma = line.IndexOf(',');
        return int.Parse(line.AsSpan(0, comma > 0 ? comma : line.Length));
    }
}
```

**Why this helps**:
- Building the index is a one-time sequential scan (fast)
- Lookups are sorted and batched, reducing random seeks
- Trade-off: You pay upfront cost to build the index, but subsequent lookups are much faster

**Results**:
- **Index build**: Sequential scan at ~100–500 MB/s (depending on device)
- **Lookups**: Sorted offsets turn random access into "more sequential" access, reducing seek overhead
- **Overall**: Much faster than doing random seeks for every lookup

---

### Scenario 4: Write Pattern — Append-only log vs update-in-place

**Problem**: Writing application logs. Append-only is fast; update-in-place is slow.

**❌ Bad: Update-in-place (random writes)**
```csharp
// Slow: updating records at random offsets
public static void UpdateLogEntry(string logPath, long offset, string newMessage)
{
    using var fs = File.OpenWrite(logPath);
    fs.Seek(offset, SeekOrigin.Begin);  // Random seek
    
    byte[] data = System.Text.Encoding.UTF8.GetBytes(newMessage);
    fs.Write(data, 0, data.Length);
    fs.Flush(true);  // Force to disk (expensive!)
}
```

**Problems**:
- Each update requires a seek
- Small writes amplify overhead
- `Flush(true)` forces data to disk (expensive on every write)
- On HDD: ~10 ms per write (seek + write + sync)
- On SSD: write amplification increases

**✅ Good: Append-only log (sequential writes)**
```csharp
// Fast: always append to end of file
public static class AppendOnlyLogger
{
    private static readonly object _lock = new object();

    public static void AppendLog(string logPath, string message)
    {
        lock (_lock)  // Serialize writes (or use per-thread files)
        {
            using var fs = new FileStream(
                logPath,
                FileMode.Append,  // Always write to end (sequential)
                FileAccess.Write,
                FileShare.Read,
                bufferSize: 64 * 1024);

            using var writer = new StreamWriter(fs, System.Text.Encoding.UTF8, bufferSize: 64 * 1024);
            writer.WriteLine($"{DateTime.UtcNow:O} {message}");
            // Optional: writer.Flush(); fs.Flush(true); for durability
        }
    }
}
```

**Why this works well**:
- Writes always go to the end of the file (sequential)
- No seeks (head stays at end on HDD, FTL optimizes on SSD)
- OS can batch writes efficiently
- Can buffer multiple writes before flushing

**Performance expectations**:
- **HDD**: ~100–150 MB/s (vs ~1–2 MB/s for random writes)
- **SSD**: ~500 MB/s (vs ~50–100 MB/s for random writes with high write amplification)
- **Durability trade-off**: If you `Flush(true)` after every write, throughput drops (but you get durability). Buffer writes for better throughput.

**Real-world pattern (high-throughput logging)**:
```csharp
// Buffer writes, flush periodically
public static class BufferedLogger
{
    private static readonly BlockingCollection<string> _queue = new BlockingCollection<string>(10000);
    private static readonly Thread _writerThread;

    static BufferedLogger()
    {
        _writerThread = new Thread(WriterLoop) { IsBackground = true };
        _writerThread.Start();
    }

    public static void Log(string message)
    {
        _queue.Add($"{DateTime.UtcNow:O} {message}");
    }

    private static void WriterLoop()
    {
        using var fs = new FileStream(
            "app.log",
            FileMode.Append,
            FileAccess.Write,
            FileShare.Read,
            bufferSize: 256 * 1024);

        using var writer = new StreamWriter(fs, System.Text.Encoding.UTF8, bufferSize: 256 * 1024);

        foreach (var message in _queue.GetConsumingEnumerable())
        {
            writer.WriteLine(message);
            
            // Flush every 100 messages or every second (balance throughput vs durability)
            if (_queue.Count == 0)
            {
                writer.Flush();
                // Optional: fs.Flush(true); for durability
            }
        }
    }
}
```

**Why this is even better**:
- Batches writes (reduces syscalls and syncs)
- Single writer thread (no lock contention)
- Sequential writes (fast)
- Can tune flush frequency (throughput vs durability trade-off)

**Results**:
- **Throughput**: 10,000–100,000 log entries/second (vs 100–1,000 with update-in-place)
- **Latency**: Low (buffered, non-blocking for callers)
- **Durability**: Configurable (flush frequency)

---

## Summary and Key Takeaways

Sequential I/O is fast because it maximizes bandwidth and minimizes per-operation cost (fewer syscalls, better OS read-ahead, better device utilization). Random I/O is slow because it's limited by per-operation latency and overhead—especially on HDDs due to mechanical seeks (5–15 ms each), but still meaningful on SSDs due to controller/FTL overhead and IOPS limits. If you can't avoid random access, the best practical move is to **batch work, sort offsets, and coalesce into ranges**—accepting some read amplification to reduce the number of operations. Validate improvements using **MB/s** (throughput) plus **p95/p99** (latency), and be explicit about whether you're testing **warm cache** (data in RAM) or **cold cache** (data from disk). When in doubt, profile first—don't assume I/O is the problem without measuring. Use sequential I/O for logs, CSV/JSONL, backups, ETL, or large file transforms. Use databases with indexes for point lookups—don't reinvent random-access storage with flat-file seeks.

---

<!-- Tags: Storage & I/O, File I/O, Performance, Optimization, Throughput Optimization, Latency Optimization, Benchmarking, Profiling, Measurement, System Design, Architecture -->
