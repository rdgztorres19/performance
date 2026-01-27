# Avoid Frequent fsync Calls to Maximize Write Throughput

**Calling fsync (or equivalent operations like FlushFileBuffers, Flush(true)) after every write forces the OS to flush cached data to physical storage immediately. This is extremely expensive and can degrade write throughput by 10×–100×. Batch writes and fsync periodically instead.**

---

## Executive Summary (TL;DR)

fsync (File SYNC) is a system call that forces all buffered (cached) data for a file to be written to physical storage (disk, SSD). When you write data, it normally goes to the OS page cache (RAM) first, and the OS writes it to disk later (asynchronously). fsync blocks until all data is physically on disk, providing durability guarantees but at a massive performance cost. Each fsync can take 1–10 ms (HDD) or 0.1–1 ms (SSD), and calling it after every write turns a throughput-oriented operation into a latency-dominated one. Use fsync sparingly—batch many writes, then fsync once per batch (or on transaction boundaries). This preserves durability while maximizing throughput. The trade-off is increased risk of data loss on crash (unflushed data is lost). Use frequent fsync for critical data (financial transactions, system state), avoid it for high-throughput workloads (logging, analytics). Typical improvements: 10×–100× higher write throughput when reducing fsync frequency.

---

## Problem Context

### Understanding the Basic Problem

**What is fsync?** A system call that forces buffered file data to be written to physical storage. On Linux: `fsync(fd)`. On Windows: `FlushFileBuffers(handle)`. In .NET: `FileStream.Flush(true)`. 

**What happens when you write without fsync?**
1. You call `write()` (or `FileStream.Write()`)
2. Data goes into the OS **page cache** (RAM)
3. The write call returns immediately (fast!)
4. The OS writes data to disk **asynchronously** in the background (seconds later)
5. If the system crashes before data is written, it's lost

**What happens when you call fsync?**
1. You call `fsync(fd)` (or `Flush(true)`)
2. OS flushes all cached data for that file to disk
3. OS waits for disk to confirm data is physically written
4. fsync returns (slow! 1–10 ms on HDD, 0.1–1 ms on SSD)
5. Data is now **durable** (safe even if system crashes)

**Real-world example**: Imagine a logging system that writes 10,000 log entries per second:

```csharp
// ❌ Bad: fsync after every write (1–10 ms per fsync)
public class BadLogger
{
    public void Log(string message)
    {
        using var fs = new FileStream("app.log", FileMode.Append, FileAccess.Write, FileShare.Read);
        byte[] data = System.Text.Encoding.UTF8.GetBytes(message + "\n");
        fs.Write(data, 0, data.Length);
        fs.Flush(true);  // fsync! Blocks 1–10 ms (HDD) or 0.1–1 ms (SSD)
    }
}

// Usage: 10,000 logs/sec
for (int i = 0; i < 10000; i++)
{
    logger.Log($"Log entry {i}");  // Each takes 1–10 ms → impossible!
}
```

**Why this is catastrophically slow:**
- Each fsync blocks for 1–10 ms (HDD) or 0.1–1 ms (SSD)
- You can do at most 100–1000 fsyncs/sec (HDD) or 1,000–10,000 fsyncs/sec (SSD)
- But you need 10,000 logs/sec → physically impossible with per-write fsync!
- Actual throughput: 100–1000 logs/sec instead of 10,000 logs/sec

**Why this matters**: fsync is one of the most expensive operations you can do. It's 100×–1000× slower than an async write. Databases, file systems, message queues—all high-performance systems carefully minimize fsync calls.

### Key Terms Explained (Start Here!)

**What is the page cache?** A RAM cache managed by the OS that holds recently read/written file data. When you write to a file, data goes to the page cache first (fast), and the OS writes it to disk later (async). This makes writes fast but means data isn't immediately durable.

**What is durability?** A guarantee that once an operation succeeds, the data is safe—even if the system crashes immediately after. To achieve durability, you must call fsync to force data to physical storage.

**What is write-back caching?** The OS strategy of accepting writes into RAM (page cache) and writing them to disk later in the background. This makes writes fast (~1 µs) but non-durable until flushed.

**What is write-through?** The opposite: every write goes directly to disk (or waits for disk). This is slow (~1–10 ms per write) but durable. Example: `FileOptions.WriteThrough` in .NET.

**What is a barrier (or fence)?** An operation that ensures all previous writes are durable before proceeding. fsync is a write barrier—it ensures all writes to that file are on disk before returning.

**What is group commit?** A database optimization: batch multiple transactions' writes, then fsync once for the whole batch. Example: 100 transactions write to WAL, then one fsync commits all 100. This amortizes fsync cost.

**What is the fsync latency?** How long fsync takes to complete:
- **HDD**: 1–10 ms (limited by disk rotation speed, seek time)
- **SATA SSD**: 0.1–1 ms (limited by flash write speed, FTL overhead)
- **NVMe SSD**: 0.05–0.5 ms (faster, but still expensive)
- **Network storage (EBS, NFS)**: 1–10 ms (adds network round trip)

**What is fsync amplification?** When fsyncing one file causes other unrelated data to be flushed. Example: On some file systems, fsync on one file can flush metadata for the whole file system, causing unexpected slowdowns.

### Common Misconceptions

**"Writes are fast because they go to the page cache"**
- **The truth**: Yes, writes are fast (~1 µs) **without fsync**. But if you call fsync after every write, you lose this benefit and turn every write into a 1–10 ms operation.

**"SSDs make fsync cheap"**
- **The truth**: SSDs are 10×–100× faster than HDDs for fsync, but fsync is still expensive compared to async writes. Example: Async write = 1 µs, fsync = 0.1–1 ms → fsync is 100×–1000× slower.

**"I need fsync after every write to guarantee durability"**
- **The truth**: You only need fsync at **transaction boundaries** or **logical commit points**. Example: A database doesn't fsync after every SQL statement—it fsyncs when you commit a transaction (group commit). Batch your writes.

**"The OS will flush data automatically, so I don't need fsync"**
- **The truth**: The OS will flush eventually (every 5–30 seconds), but if the system crashes before then, unflushed data is lost. If you need durability, you must call fsync explicitly.

**"Disabling fsync is unsafe"**
- **The truth**: It depends. For critical data (financial transactions, user data), you need fsync. For non-critical data (logs, metrics, caches), you can skip fsync and accept potential data loss on crash.

---

## How It Works

### Understanding fsync and the Page Cache

**How writes work without fsync:**

```csharp
// Fast writes (async, no durability)
using var fs = new FileStream("data.txt", FileMode.Append);
byte[] data = System.Text.Encoding.UTF8.GetBytes("Hello\n");
fs.Write(data, 0, data.Length);  // ~1 µs (goes to page cache)
// Data is in RAM, not on disk yet!
```

**What happens:**
1. `Write()` copies data to the OS page cache (RAM)
2. `Write()` returns immediately (~1 µs)
3. OS schedules async write to disk (happens seconds later in background)
4. **If system crashes before disk write, data is lost**

**How writes work with fsync:**

```csharp
// Slow writes (sync, durable)
using var fs = new FileStream("data.txt", FileMode.Append);
byte[] data = System.Text.Encoding.UTF8.GetBytes("Hello\n");
fs.Write(data, 0, data.Length);  // ~1 µs (goes to page cache)
fs.Flush(true);  // fsync! Blocks 1–10 ms until data is on disk
// Data is now safely on disk
```

**What happens:**
1. `Write()` copies data to page cache (~1 µs)
2. `Flush(true)` calls fsync, which:
   - Flushes all dirty pages for this file to disk
   - Waits for disk to confirm write
   - Returns when data is physically on disk
3. **Total time: 1–10 ms (HDD) or 0.1–1 ms (SSD)**
4. Data is now durable

**Key insight**: fsync is ~1000× slower than an async write. Minimize fsync calls.

### Technical Details: What Happens at the Disk Level

**Why fsync is slow (HDD):**
- Disk must physically write data to magnetic platters
- This requires:
  - Seek to correct track (5–15 ms)
  - Wait for platter to rotate to correct sector (0–8 ms)
  - Write data (1–2 ms)
- Total: ~5–15 ms per fsync

**Why fsync is slow (SSD):**
- Flash memory must be programmed (written)
- This requires:
  - FTL (Flash Translation Layer) overhead (mapping logical to physical pages)
  - Flash program operation (~100–500 µs)
  - Power-loss capacitor flush (ensures data survives power failure)
- Total: ~0.1–1 ms per fsync (10×–100× faster than HDD, but still expensive)

**Why fsync is slow (network storage):**
- Data must be sent over network + written to remote disk
- This requires:
  - Network round trip (1–10 ms)
  - Remote disk write (1–10 ms)
- Total: ~2–20 ms per fsync (worst case)

---

## Why This Becomes a Bottleneck

Frequent fsync becomes a bottleneck because:

**Latency dominates throughput**: Each fsync blocks for 1–10 ms. If you fsync after every write, your throughput is limited by fsync latency. Example: 10 ms per fsync = max 100 operations/sec (far below what the disk can do with async writes).

**Serialization**: fsync is typically serialized (one at a time). Even if you have multiple threads writing, they all wait for the same disk to flush. This creates contention.

**I/O amplification**: fsync can cause other unrelated data to be flushed (filesystem metadata, journal, other files' data). This amplifies I/O.

**Tail latency spikes**: If your system does occasional fsyncs, those fsyncs create latency spikes (p95/p99). Example: 99% of writes take 1 µs, but 1% take 10 ms (due to fsync).

---

## Advantages (of Avoiding Frequent fsync)

**Much higher write throughput**: Avoiding fsync allows writes to be async and batched. Example: 10,000 async writes/sec vs 100 fsync'd writes/sec (100× improvement).

**Lower latency for writes**: Async writes complete in ~1 µs instead of 1–10 ms. Example: p99 write latency goes from 10 ms to 0.1 ms.

**Better CPU efficiency**: fsync requires kernel work (flushing pages, issuing disk commands). Fewer fsyncs = less kernel overhead.

**Less I/O congestion**: Fewer fsyncs = less I/O traffic = more bandwidth available for actual data writes.

---

## Disadvantages and Trade-offs

**Risk of data loss on crash**: Without fsync, unflushed data (in page cache) is lost if the system crashes. This is unacceptable for critical data (financial transactions, user data).

**Complexity of batching**: To avoid frequent fsync while maintaining durability, you must batch writes and fsync at transaction boundaries. This adds complexity.

**Delayed durability**: Data isn't durable until fsynced. If you fsync every 100 ms, there's a 0–100 ms window where data can be lost on crash.

**Replication is not a substitute for fsync**: Even if you replicate data to multiple nodes, if none of them fsync, you can lose data on correlated crashes (e.g., power outage).

---

## When to Use This Approach (Avoid Frequent fsync)

Avoid frequent fsync when:

- **High throughput is critical** (logging, metrics, analytics). Example: Write 10,000 log entries/sec with fsync every 1 second (batching 10,000 writes).
- **Workload is write-heavy** (many small writes). Example: Time-series database ingesting millions of data points/sec.
- **Data loss is acceptable** (non-critical data, caches, temporary files). Example: Application logs (if you lose 1 second of logs on crash, it's not catastrophic).
- **Replication provides durability** (distributed systems). Example: Kafka writes to 3 replicas asynchronously—even without fsync, data is safe if >1 replica survives.
- **You batch writes** (group commit, write-behind cache). Example: Database batches 100 transactions, then fsyncs once.

---

## Common Mistakes

**Calling fsync after every write**: This is almost always wrong unless you're writing critical data (financial transactions). Batch writes and fsync periodically instead.

**Not calling fsync at all for critical data**: If data is critical, you must fsync at transaction boundaries. Skipping fsync for critical data risks data loss.

**Fsyncing too frequently**: Example: Fsyncing every 10 ms might be too frequent. Benchmark to find the right balance (e.g., every 100 ms or every 1000 writes).

**Fsyncing too infrequently**: Example: Fsyncing every 60 seconds means you can lose 60 seconds of data on crash. This might be unacceptable.

**Using `Flush()` instead of `Flush(true)`**: In .NET, `Flush()` only flushes to the OS page cache (not disk). You need `Flush(true)` for actual fsync.

**Ignoring replication**: If you have replication, you might not need fsync on every node. Example: Kafka replicates to 3 nodes asynchronously—data is safe even without fsync.

---

## Example Scenarios

### Scenario 1: High-throughput logging (avoid per-write fsync)

**Problem**: A web server logs 10,000 requests/sec. Calling fsync after every log entry limits throughput to 100–1000 logs/sec.

**Bad code** (fsync after every write):

```csharp
// ❌ Bad: fsync after every write (~100 logs/sec max on HDD)
public class BadLogger
{
    public void Log(string message)
    {
        using var fs = new FileStream("app.log", FileMode.Append, FileAccess.Write, FileShare.Read);
        byte[] data = System.Text.Encoding.UTF8.GetBytes($"{DateTime.UtcNow:O} {message}\n");
        fs.Write(data, 0, data.Length);
        fs.Flush(true);  // fsync! Blocks 1–10 ms
    }
}
```

**Good code** (batch writes, fsync every 1 second):

```csharp
// ✅ Good: batch writes, fsync every 1 second
public class GoodLogger
{
    private readonly BlockingCollection<string> _queue = new BlockingCollection<string>(10000);
    private readonly Thread _writerThread;

    public GoodLogger()
    {
        _writerThread = new Thread(WriterLoop) { IsBackground = true };
        _writerThread.Start();
    }

    public void Log(string message)
    {
        _queue.Add($"{DateTime.UtcNow:O} {message}");  // Fast, non-blocking
    }

    private void WriterLoop()
    {
        using var fs = new FileStream("app.log", FileMode.Append, FileAccess.Write, FileShare.Read, bufferSize: 64 * 1024);

        var lastFlush = DateTime.UtcNow;
        var buffer = new List<byte[]>(1000);

        foreach (var message in _queue.GetConsumingEnumerable())
        {
            buffer.Add(System.Text.Encoding.UTF8.GetBytes(message + "\n"));

            // Flush every 1 second OR when buffer is large
            if ((DateTime.UtcNow - lastFlush).TotalSeconds >= 1.0 || buffer.Count >= 1000)
            {
                foreach (var data in buffer)
                    fs.Write(data, 0, data.Length);

                fs.Flush(true);  // fsync once for entire batch
                lastFlush = DateTime.UtcNow;
                buffer.Clear();
            }
        }
    }
}
```

**Results**:
- **Bad**: 100–1000 logs/sec (limited by fsync)
- **Good**: 10,000 logs/sec (batched, fsync every 1 second)
- **Improvement**: 10×–100×
- **Trade-off**: Can lose up to 1 second of logs on crash (usually acceptable for logs)

---

### Scenario 2: Database transaction commit (group commit)

**Problem**: A database commits 1000 transactions/sec. Calling fsync after every commit limits throughput to 100–1000 tx/sec.

**Bad approach** (fsync per transaction):

```csharp
// ❌ Bad: fsync after every transaction (~100 tx/sec max)
public void CommitTransaction(Transaction tx)
{
    WriteToWAL(tx);  // Write to write-ahead log
    walFile.Flush(true);  // fsync! Blocks 1–10 ms
}
```

**Good approach** (group commit):

```csharp
// ✅ Good: group commit (batch multiple transactions)
private readonly List<Transaction> _pendingTransactions = new List<Transaction>();
private readonly Timer _commitTimer;

public DatabaseWithGroupCommit()
{
    // Commit every 10 ms
    _commitTimer = new Timer(_ => GroupCommit(), null, 10, 10);
}

public void BeginTransaction(Transaction tx)
{
    lock (_pendingTransactions)
    {
        _pendingTransactions.Add(tx);
        WriteToWAL(tx);  // Write to WAL (buffered)
    }
}

private void GroupCommit()
{
    lock (_pendingTransactions)
    {
        if (_pendingTransactions.Count == 0)
            return;

        walFile.Flush(true);  // fsync once for all pending transactions

        // Mark all as committed
        foreach (var tx in _pendingTransactions)
            tx.MarkCommitted();

        _pendingTransactions.Clear();
    }
}
```

**Results**:
- **Bad**: 100–1000 tx/sec (limited by fsync)
- **Good**: 10,000+ tx/sec (group commit every 10 ms)
- **Improvement**: 10×–100×
- **Trade-off**: Transactions wait up to 10 ms for commit (usually acceptable)

---

### Scenario 3: When you MUST use fsync (critical data)

**Problem**: A financial system must guarantee durability for every transaction—if the system crashes, no committed transaction can be lost.

**Correct approach** (fsync after every transaction):

```csharp
// ✅ Correct: fsync after every transaction (slow but safe)
public void ProcessPayment(Payment payment)
{
    // Write payment to database
    WritePaymentToDatabase(payment);

    // Write to write-ahead log
    using var wal = new FileStream("payments.wal", FileMode.Append, FileAccess.Write, FileShare.Read);
    byte[] data = SerializePayment(payment);
    wal.Write(data, 0, data.Length);
    wal.Flush(true);  // fsync! MUST call for durability

    // Now safe to acknowledge payment to user
    AcknowledgePayment(payment);
}
```

**Why this is necessary**:
- Financial data is critical—losing even one transaction is unacceptable
- Regulatory requirements often mandate durability
- User expects "payment successful" means data is safe

**Performance**:
- Limited to 100–1000 payments/sec (HDD) or 1,000–10,000 payments/sec (SSD)
- If you need higher throughput, use group commit (batch payments, fsync once)

---

## Summary and Key Takeaways

fsync forces buffered data to be written to physical storage, providing durability at a massive performance cost (1–10 ms per call on HDD, 0.1–1 ms on SSD). Calling fsync after every write degrades throughput by 10×–100× because you turn async writes (~1 µs) into sync operations (1–10 ms). The solution is to batch writes and fsync periodically (e.g., every 100 ms or every 1000 writes), or use group commit (batch transactions, fsync once). This preserves durability while maximizing throughput. The trade-off is increased risk of data loss on crash (unflushed data is lost). Use frequent fsync for critical data (financial, user data), avoid it for high-throughput non-critical workloads (logging, metrics). Typical improvements: 10×–100× higher write throughput when reducing fsync frequency. Always measure fsync count, write throughput, and latency under realistic load.

---

<!-- Tags: Performance, Optimization, Storage & I/O, File I/O, Database Optimization, Throughput Optimization, Latency Optimization, System Design, Architecture, .NET Performance, C# Performance, Profiling, Measurement -->
