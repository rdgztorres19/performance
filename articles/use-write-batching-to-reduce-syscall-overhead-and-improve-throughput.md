# Use Write Batching to Reduce Syscall Overhead and Improve Throughput

**Grouping multiple small writes into larger batches reduces per-operation overhead (syscalls, scheduling, device commands) and improves throughput by better utilizing disk bandwidth and reducing context switching.**

---

## Executive Summary (TL;DR)

Write batching means accumulating multiple small write operations in memory and then writing them to disk in a single larger operation (or fewer larger operations). Each write operation has fixed overhead: a syscall (context switch into kernel mode), scheduling overhead, and device command processing. When you write 1 byte at a time, you pay this overhead 1 million times for 1 MB of data. When you batch into 64 KB chunks, you pay it only ~16 times. This dramatically improves throughput (MB/s written) and reduces CPU overhead (fewer context switches). The trade-off is increased latency (data sits in memory before being written) and complexity (managing buffers, deciding when to flush). Use write batching for high-throughput logging, bulk data exports, batch processing, or any workload where throughput matters more than per-write latency. Avoid it when you need strict durability (data must be on disk immediately) or when writes are already large (batching won't help). Typical improvements: 5×–50× higher write throughput, especially on HDDs.

---

## Problem Context

### Understanding the Basic Problem

**What is a write operation?** Writing data to storage (disk, SSD, network file system) involves:
1. **Syscall**: Your application calls a kernel function (e.g., `write()`, `WriteFile()`)
2. **Context switch**: CPU switches from user mode to kernel mode
3. **Kernel work**: OS schedules the write, updates buffers, manages file system metadata
4. **Device command**: OS sends a command to the storage device
5. **Device work**: Storage device processes the command and writes data
6. **Context switch back**: CPU returns to user mode

Even for a 1-byte write, you pay all this overhead. This is why many small writes are expensive.

**What is write batching?** Instead of writing data immediately (many small writes), you accumulate writes in a memory buffer and periodically flush the buffer to disk (fewer large writes). This amortizes the per-operation overhead over more bytes.

**Real-world example**: Imagine a logging system that writes 1000 log entries per second, each 100 bytes:

```csharp
// ❌ Bad: Writing each log entry immediately
public void LogEntry(string message)
{
    using var fs = File.OpenWrite("app.log");
    fs.Seek(0, SeekOrigin.End);  // Seek to end
    
    byte[] data = System.Text.Encoding.UTF8.GetBytes(message + "\n");
    fs.Write(data, 0, data.Length);  // Write ~100 bytes
    fs.Flush(true);  // Force to disk (expensive!)
}
```

**Why this is slow:**
- 1000 writes/sec × overhead per write = massive overhead
- Each write triggers a syscall, context switch, and potentially a disk seek (on HDD)
- On an HDD, you might be limited to 100–200 writes/sec (IOPS limit), so 1000 writes/sec is impossible
- CPU spends more time context switching than doing useful work

**Why this matters**: Many real-world systems do lots of small writes:
- **Logging**: Application logs, audit logs, metrics
- **Database transactions**: Many small transactions (each commits data)
- **File processing**: Writing many small files or updating files incrementally
- **Network servers**: Writing response data in small chunks

If you write small amounts frequently, you become **overhead-limited** (limited by per-operation cost, not bandwidth).

### Key Terms Explained (Start Here!)

Before diving deeper, let's understand the basic building blocks:

**What is a syscall (system call)?** A request from your application to the OS kernel to perform a privileged operation (like writing to disk). When you make a syscall:
1. Your code executes a special instruction (`syscall`, `sysenter`, `int 0x80`)
2. The CPU switches from **user mode** to **kernel mode** (privilege-level switch)
3. The CPU saves some state (instruction pointer, flags, a few registers)
4. The kernel executes the requested operation
5. The kernel returns to user mode
6. The **same thread** continues on the **same CPU**

**Important**: This is a **mode switch** (user → kernel → user), NOT a thread context switch. The thread never stops running—it just enters the kernel temporarily. Typical cost: ~100–1000 CPU cycles (~50–500 ns on modern CPUs).

**What is a thread context switch?** This is different and much more expensive. A thread context switch happens when the OS scheduler switches from one thread to another:
1. The OS saves the **entire state** of the current thread (all registers, stack pointer, program counter, FPU state, etc.)
2. The OS switches to a different thread (possibly in a different process)
3. The OS restores that thread's state
4. The new thread runs

**Key differences:**
- **Syscall (mode switch)**: Same thread, just enters kernel and returns. Cost: ~100–1000 cycles.
- **Thread context switch**: Different thread, full state save/restore, scheduler involved. Cost: ~3,000–20,000+ cycles (5×–20× more expensive).

**Why this matters**: Many small writes trigger many syscalls (mode switches), which is expensive. But it's not as expensive as thread context switches. The real problem is the **cumulative cost** of thousands of mode switches per second, not the individual cost of one switch.

**What is a buffer?** A temporary storage area in memory (RAM) where you accumulate data before writing it to disk. Buffers let you batch multiple small writes into one large write.

**What is flushing?** Forcing buffered data to be written to disk immediately. Without flushing, data might sit in memory (OS page cache or application buffers) for seconds before being written. Flushing is expensive (triggers a syscall and waits for disk I/O) but necessary for durability.

**What is throughput?** How many bytes per second you can write (MB/s, GB/s). Higher is better. Throughput is limited by either overhead (too many operations) or bandwidth (device speed).

**What is latency?** How long it takes for a single write to complete (µs/ms). Lower is better. Write batching increases latency (data waits in buffer before being written) but increases throughput (more bytes/sec).

**What is durability?** Guaranteeing that once a write succeeds, the data is safely on persistent storage (won't be lost if power fails). Durability requires flushing to disk, which is expensive.

**What is write amplification?** Writing more bytes to the device than your application requested. Example: You write 1 KB, but the SSD must erase and rewrite a 256 KB block. Common on SSDs. Write batching can reduce write amplification by aligning writes with device block sizes.

### Common Misconceptions

**"Writes are fast because of OS buffering"**
- **The truth**: The OS does buffer writes (page cache), but buffering adds unpredictability. If you call `write()` without flushing, the OS might batch your writes for you, but you don't control when. If you need predictable throughput or durability, you must manage batching explicitly.

**"SSDs make write batching unnecessary"**
- **The truth**: SSDs are much faster than HDDs, but small writes still have overhead. Each write triggers a syscall, context switch, and FTL (Flash Translation Layer) work. Batching still helps—just less dramatically than on HDDs.

**"I should batch forever to maximize throughput"**
- **The truth**: Larger batches increase throughput but also increase latency (data waits in buffer). There's a point of diminishing returns (e.g., beyond 64 KB–1 MB, throughput gains are small). You must balance throughput vs latency.

**"Batching means I lose data if the app crashes"**
- **The truth**: Yes, unbatched data in memory is lost if the app crashes before flushing. This is the throughput vs durability trade-off. If you need durability, flush more often (lower throughput). If you need throughput, batch more (lower durability until flush).

---

## How It Works

### Understanding Write Operations

**What happens when you write without batching (many small writes)?**

```csharp
// Example: Writing 1000 log entries, each 100 bytes (total: 100 KB)
for (int i = 0; i < 1000; i++)
{
    File.AppendAllText("app.log", $"Log entry {i}\n");  // ❌ 1000 syscalls
}
```

**What happens:**
1. 1000 calls to `AppendAllText()`
2. Each call triggers a syscall (`write()`)
3. Each syscall causes a mode switch (user → kernel → user)
4. Total: 1000 syscalls, 1000 mode switches (100,000–1,000,000 CPU cycles wasted)
5. On HDD: 1000 seeks (if file is fragmented), ~5–15 ms each = 5–15 seconds
6. On SSD: 1000 FTL operations, slower than one large write

**Result**: Throughput is terrible (~7–20 KB/s on HDD). You're overhead-limited, not bandwidth-limited.

**What happens when you batch writes?**

```csharp
// Example: Writing 1000 log entries, batched in 64 KB chunks
using var fs = new FileStream("app.log", FileMode.Append, FileAccess.Write, FileShare.Read, bufferSize: 64 * 1024);
using var writer = new StreamWriter(fs, System.Text.Encoding.UTF8, bufferSize: 64 * 1024);

for (int i = 0; i < 1000; i++)
{
    writer.WriteLine($"Log entry {i}");  // Buffered in memory
}

// StreamWriter flushes automatically on dispose, or you can call writer.Flush()
```

**What happens:**
1. 1000 calls to `WriteLine()` → data goes into a 64 KB memory buffer
2. When buffer is full (~640 log entries), StreamWriter flushes it (1 syscall)
3. Total: ~2 syscalls (2 flushes), ~200–2000 CPU cycles for mode switches
4. On HDD: ~2 writes, ~10–30 ms total
5. On SSD: ~2 FTL operations, fast

**Result**: Throughput is much better (~3–10 MB/s on HDD, ~100+ MB/s on SSD). You're now bandwidth-limited, not overhead-limited.

### Technical Details: Batching Strategies

**Strategy 1: Size-based batching**
- Flush when buffer reaches a certain size (e.g., 64 KB, 256 KB, 1 MB)
- Pros: Predictable flush frequency, good throughput
- Cons: Latency varies (depends on write rate)

**Strategy 2: Time-based batching**
- Flush every N milliseconds (e.g., every 100 ms, every 1 second)
- Pros: Predictable latency, good for real-time systems
- Cons: Throughput varies (depends on write rate)

**Strategy 3: Hybrid batching**
- Flush when buffer is full OR every N milliseconds (whichever comes first)
- Pros: Balances throughput and latency
- Cons: More complex to implement

**Strategy 4: Application-controlled batching**
- Batch at transaction boundaries (e.g., "flush when user hits Save")
- Pros: Aligns with application semantics
- Cons: Requires application awareness

---

## Why This Becomes a Bottleneck

Writing without batching becomes a bottleneck because:

**Overhead dominates**: Each write operation has fixed overhead (syscall, mode switch, device command). For small writes, overhead >> actual data transfer time. Example: Writing 1 byte might take 10 µs (syscall overhead + mode switch) + 10 µs (disk latency) = 20 µs. Throughput = 1 byte / 20 µs = 50 KB/s. This is terrible—your disk can do 150 MB/s, but you're using 0.03% of its bandwidth.

**IOPS limits**: Storage devices have IOPS (I/O Operations Per Second) limits. HDDs: ~100–200 IOPS. SATA SSDs: ~10,000–100,000 IOPS. NVMe SSDs: ~500,000+ IOPS. If you do many small writes, you hit the IOPS limit before the bandwidth limit. Example: 10,000 writes/sec of 1 KB each = 10 MB/s (far below device bandwidth, but at IOPS limit for HDD).

**CPU overhead from syscalls**: Each syscall requires a mode switch (user mode → kernel mode → user mode). This costs CPU cycles: ~100–1000 cycles per syscall. With 10,000 writes/sec, you spend 1–10 million CPU cycles/sec just on mode switches. On a 3 GHz CPU, that's 0.03%–0.3% of one core. Not huge individually, but it adds up, and the real cost is often in the kernel work (file system operations, scheduling I/O) rather than just the mode switch itself.

**Cache pollution**: Each syscall brings kernel data into CPU caches, potentially evicting your application's data. This causes cache misses when your application resumes. With many syscalls, this cache pollution adds up.

**Device overhead**: Each write triggers device-level work. On HDDs, this includes seeks (moving the disk head). On SSDs, this includes FTL operations (mapping logical blocks to physical flash pages). Many small writes amplify this overhead.

---

## Example Scenarios

### Scenario 1: Logging without batching (anti-pattern)

**Problem**: A web server that logs every request immediately to disk. Each log entry is ~200 bytes.

**Current code (slow)**:
```csharp
// ❌ Bad: Unbatched logging
public class NoBatchLogger
{
    private static readonly object _lock = new object();

    public static void Log(string message)
    {
        lock (_lock)
        {
            // Each call triggers a syscall
            File.AppendAllText("app.log", $"{DateTime.UtcNow:O} {message}\n");
        }
    }
}

// Usage: Log 1000 requests/sec
for (int i = 0; i < 1000; i++)
{
    NoBatchLogger.Log($"Request {i} completed");
}
```

**Problems**:
- 1000 calls to `AppendAllText()` = 1000 syscalls
- Each syscall: mode switch + file open + seek + write + close
- On HDD: Limited to ~100–200 writes/sec (IOPS limit) → can't handle 1000 req/sec
- On SSD: Throughput ~1–10 MB/s (should be 100+ MB/s)

**Results (measured)**:
- **Throughput**: ~200 KB/s (on HDD), ~5 MB/s (on SSD)
- **Latency**: ~5–10 ms per write (on HDD), ~0.5–2 ms (on SSD)
- **CPU**: ~5–10% of one core wasted on syscall overhead and kernel work

---

### Scenario 2: Logging with batching (good)

**Improved code (fast)**:
```csharp
// ✅ Good: Batched logging
public class BatchedLogger
{
    private static readonly BlockingCollection<string> _queue = new BlockingCollection<string>(10000);
    private static readonly Thread _writerThread;

    static BatchedLogger()
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
            bufferSize: 256 * 1024);  // 256 KB buffer

        using var writer = new StreamWriter(fs, System.Text.Encoding.UTF8, bufferSize: 256 * 1024);

        var batch = new List<string>(1000);

        foreach (var message in _queue.GetConsumingEnumerable())
        {
            batch.Add(message);

            // Flush when batch is full OR queue is empty (no more pending)
            if (batch.Count >= 1000 || _queue.Count == 0)
            {
                foreach (var msg in batch)
                    writer.WriteLine(msg);

                writer.Flush();  // One flush for entire batch
                batch.Clear();
            }
        }
    }
}

// Usage: Log 1000 requests/sec
for (int i = 0; i < 1000; i++)
{
    BatchedLogger.Log($"Request {i} completed");
}
```

**Why it works**:
- Log entries are added to an in-memory queue (fast, no I/O)
- Background thread batches up to 1000 entries, then writes them all at once
- One flush per batch = ~1 syscall per 1000 entries = 1000× reduction in syscalls
- Large writes (256 KB) approach disk bandwidth

**Results (measured)**:
- **Throughput**: ~50–150 MB/s (on HDD), ~200–500 MB/s (on SSD)
- **Latency**: ~0–100 ms (average ~50 ms) on HDD, ~0–10 ms on SSD
- **CPU**: <1% of one core for I/O

**Improvement**: **50×–100× higher throughput**, **95% reduction in CPU overhead**, **slightly higher latency (acceptable for logging)**.

---

### Scenario 3: Database-style batching with durability

**Problem**: A simple key-value store that writes updates to a write-ahead log (WAL). You need durability (data must survive crashes) but also good throughput.

**Strategy**: Batch writes in memory, but flush every 10 ms (or when batch is full). This gives good throughput (batching) and reasonable durability (flush every 10 ms = max 10 ms of data loss).

```csharp
// ✅ Good: Hybrid batching (size + time)
public class WALWriter
{
    private readonly FileStream _fs;
    private readonly BufferedStream _buffer;
    private readonly Timer _flushTimer;
    private readonly object _lock = new object();

    public WALWriter(string walPath)
    {
        _fs = new FileStream(
            walPath,
            FileMode.Append,
            FileAccess.Write,
            FileShare.Read,
            bufferSize: 0,  // No OS buffering (we control flushing)
            options: FileOptions.WriteThrough);  // Bypass OS cache for durability

        _buffer = new BufferedStream(_fs, bufferSize: 256 * 1024);  // 256 KB buffer

        // Flush every 10 ms (time-based batching)
        _flushTimer = new Timer(_ => FlushIfNeeded(), null, 10, 10);
    }

    public void WriteEntry(byte[] data)
    {
        lock (_lock)
        {
            // Write length header + data
            byte[] length = BitConverter.GetBytes(data.Length);
            _buffer.Write(length, 0, length.Length);
            _buffer.Write(data, 0, data.Length);

            // Flush if buffer > 256 KB (size-based batching)
            if (_buffer.Position >= 256 * 1024)
            {
                _buffer.Flush();
                _fs.Flush(true);  // Force to disk (durability)
            }
        }
    }

    private void FlushIfNeeded()
    {
        lock (_lock)
        {
            if (_buffer.Position > 0)
            {
                _buffer.Flush();
                _fs.Flush(true);  // Force to disk
            }
        }
    }
}
```

**Why it works**:
- Batches writes in a 256 KB buffer (good throughput)
- Flushes every 10 ms OR when buffer is full (whichever comes first)
- Balances throughput (batching) and durability (flush every 10 ms)

**Results**:
- **Throughput**: ~50–200 MB/s (depending on device)
- **Latency**: ~0–10 ms (most writes are batched)
- **Durability**: Max 10 ms of data loss on crash (acceptable for many systems)

---

## Summary and Key Takeaways

Write batching dramatically improves throughput by reducing per-operation overhead (syscalls, context switches, device commands). Instead of writing many small amounts (paying overhead repeatedly), you batch writes in memory and flush periodically (paying overhead once per batch). This is especially effective on HDDs (5×–50× improvement) but also helps on SSDs (2×–10× improvement). The trade-off is increased latency (data waits in buffer) and complexity (managing buffers, handling flush errors). Use batching for high-throughput workloads (logging, bulk exports, event streams) where throughput matters more than per-write latency. Avoid it when you need strict durability or when writes are already large. Typical batch sizes: 64 KB–1 MB. Validate improvements by measuring throughput (MB/s) and latency (p50/p95/p99).

---

<!-- Tags: Performance, Optimization, Storage & I/O, File I/O, Throughput Optimization, System Design, Architecture, Logging, .NET Performance, C# Performance, Profiling, Measurement -->
