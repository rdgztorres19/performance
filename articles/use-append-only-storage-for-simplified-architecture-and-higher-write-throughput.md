# Use Append-Only Storage for Simplified Architecture and Higher Write Throughput

**Append-only storage restricts writes to adding data at the end of a file or log, never modifying or deleting existing data in place. This eliminates update overhead, simplifies concurrency, and maximizes sequential write performance.**

---

## Executive Summary (TL;DR)

Append-only storage means you only add new data to the end of a file, log, or data structure?never modify or delete existing data in place. Updates are represented as new append operations (e.g., "user X changed email to Y"), and deletions are represented as tombstone markers. This design eliminates the overhead of in-place updates (no seeks to find old data, no read-modify-write cycles), enables simple concurrency (writers don't conflict), and maximizes sequential write performance (all writes go to the end, no fragmentation). The trade-off is space amplification (old/deleted data remains until compaction), increased read complexity (must scan/skip tombstones), and the need for periodic compaction (background process to remove obsolete data). Use append-only storage for write-heavy workloads (logging, event sourcing, time-series data, Write-Ahead Logs) where writes vastly outnumber updates/deletes. Avoid it for workloads with frequent updates to the same record or strict space constraints. Typical improvements: 5??20? higher write throughput, simpler concurrency, but 2??5? more space usage before compaction.

---

## Problem Context

### Understanding the Basic Problem

**What is append-only storage?** A storage pattern where you only add data to the end of a file, log, or data structure. You never modify existing data in place. If you need to "update" a record, you append a new version. If you need to "delete" a record, you append a tombstone (a marker indicating deletion).

**What is update-in-place storage?** The traditional approach: when you update a record, you seek to its location on disk, read it, modify it, and write it back. This requires random I/O (seeks on HDD, FTL overhead on SSD) and complicates concurrency (multiple writers might conflict).

**Real-world example**: Imagine a user profile system that stores user data:

```csharp
// ? Bad: Update-in-place (random writes)
public class UpdateInPlaceUserStore
{
    public void UpdateUserEmail(int userId, string newEmail)
    {
        // 1. Read user record from disk (random I/O)
        var user = ReadUser(userId);  // Seek to offset, read
        
        // 2. Modify in memory
        user.Email = newEmail;
        
        // 3. Write back to same location (random I/O)
        WriteUser(userId, user);  // Seek to offset, write
    }
}
```

**Why this is slow:**
- Each update requires 2 random I/O operations (read + write)
- On HDD: 2 seeks (~10?30 ms total)
- On SSD: 2 FTL operations + potential write amplification
- Concurrency is hard: What if two threads update the same user simultaneously?

**Append-only alternative:**

```csharp
// ? Good: Append-only (sequential writes)
public class AppendOnlyUserStore
{
    public void UpdateUserEmail(int userId, string newEmail)
    {
        // Just append the change to the end of the log
        AppendEvent(new UserEmailChanged
        {
            UserId = userId,
            NewEmail = newEmail,
            Timestamp = DateTime.UtcNow
        });
    }
}
```

**Why this is fast:**
- Only 1 sequential write (append to end)
- On HDD: No seek (head stays at end)
- On SSD: No read-modify-write, less write amplification
- Concurrency is simple: Multiple writers just append independently

**Why this matters**: Many real-world systems are write-heavy:
- **Logging**: Application logs, audit trails, event streams
- **Event sourcing**: Store all state changes as events
- **Time-series data**: Metrics, sensor data, stock prices
- **Write-Ahead Logs (WAL)**: Database durability mechanism
- **Message queues**: Kafka, Apache Pulsar

If your workload is write-heavy (many writes, few updates/deletes), append-only can dramatically improve throughput and simplify architecture.

### Key Terms Explained (Start Here!)

**What is sequential I/O?** Reading or writing data in order, from start to end. This is fast because:
- On HDD: No seeks (disk head stays in one area)
- On SSD: Better FTL performance, less write amplification
- OS can optimize (read-ahead, write coalescing)

**What is random I/O?** Jumping around the file to read/write at scattered locations. This is slow because:
- On HDD: Each jump requires a seek (~5?15 ms)
- On SSD: More FTL overhead, higher write amplification
- OS cannot optimize as effectively

**What is an event?** A record of something that happened. Example: "User 123 changed email to foo@example.com at 2025-01-22T10:00:00Z". Events are immutable?once they happen, they can't change.

**What is a tombstone?** A marker indicating that a record is deleted. Example: "User 123 deleted at 2025-01-22T11:00:00Z". The old data remains on disk, but the tombstone tells readers to ignore it.

**What is compaction?** A background process that removes obsolete data (old versions, tombstones) to reclaim space. Example: If user 123's email changed 10 times, compaction keeps only the latest version and discards the rest.

**What is Write-Ahead Log (WAL)?** A durability mechanism used by databases. Before modifying data in place, the database writes the change to an append-only log. If the system crashes, the WAL is replayed to recover. Examples: PostgreSQL, MySQL, Redis.

**What is LSM tree (Log-Structured Merge tree)?** A data structure that uses append-only writes plus periodic compaction. Used by Cassandra, RocksDB, LevelDB, HBase. Writes go to memory (MemTable), then flush to disk as immutable files (SSTables), then compact in the background.

**What is event sourcing?** An architecture pattern where you store all state changes as events, rather than storing current state. To get current state, you replay events. Example: Instead of storing "User 123's email is foo@example.com", you store "User 123 created with email bar@example.com" + "User 123 changed email to foo@example.com".

### Common Misconceptions

**"Append-only means I can never delete data"**
- **The truth**: You can delete data by appending a tombstone (deletion marker) and running compaction to reclaim space. Deletion is just slower (not instant) and requires periodic maintenance.

**"Append-only wastes too much space"**
- **The truth**: Yes, space usage grows until compaction runs. But for write-heavy workloads, the write throughput gain (5??20?) often justifies 2??5? more space usage. If space is critical, compact more frequently (trade-off: more CPU/I/O for compaction).

**"Append-only makes reads slow"**
- **The truth**: Reads can be slower because you must scan/skip obsolete data or use indexes. But if you're write-heavy (99% writes, 1% reads), optimizing for writes is correct. For read-heavy workloads, append-only is a bad fit.

**"Append-only is only for logging"**
- **The truth**: Logging is the most obvious use case, but append-only is also used in databases (WAL), message queues (Kafka), time-series databases (InfluxDB), and event-sourced systems. It's a general pattern for write-heavy workloads.

---

## How It Works

### Understanding Append-Only vs Update-In-Place

**How update-in-place works:**

1. To **create** a record: Find free space (requires metadata scan or free list), write record there
2. To **update** a record: Seek to record's location, read it, modify it, write it back (read-modify-write cycle)
3. To **delete** a record: Seek to record's location, mark it free (or remove it), update metadata

**Why it's slow:**
- All operations require random I/O (seeks/FTL overhead)
- Updates and deletes require reading before writing
- Concurrency is complex (need locks to prevent conflicts)

**How append-only works:**

1. To **create** a record: Append it to the end (sequential write)
2. To **update** a record: Append a new version to the end (sequential write)
3. To **delete** a record: Append a tombstone to the end (sequential write)
4. To **read** a record: Scan from end to start (or use an index) until you find the latest version (skip tombstones and old versions)

**Why it's fast (for writes):**
- All writes are sequential (append to end)
- No reads before writes (no read-modify-write)
- Concurrency is simple (each writer appends independently)

**Why reads can be slower:**
- Must scan/skip obsolete data (unless you have an index)
- Example: If user 123's email changed 10 times, you must skip 9 old versions

**Key insight**: Append-only optimizes for write throughput at the expense of read complexity and space usage. This is correct for write-heavy workloads.

### Technical Details: Compaction

Compaction is the process of removing obsolete data to reclaim space. There are several strategies:

**Strategy 1: Size-tiered compaction**
- Group files by size (small files are newer, large files are older)
- Compact files of similar size together
- Example: 4 files of 10 MB each ? 1 file of 40 MB (with obsolete data removed)
- **Pros**: Simple, good write throughput
- **Cons**: Can amplify reads (must scan multiple files)

**Strategy 2: Leveled compaction**
- Organize files into levels (Level 0, Level 1, etc.)
- Each level is ~10? larger than the previous level
- Compact data from Level N to Level N+1
- **Pros**: Better read performance (fewer files to scan)
- **Cons**: More write amplification (data is compacted multiple times)

**Strategy 3: Time-window compaction**
- Compact data within time windows (e.g., hourly, daily)
- Good for time-series data (old data is rarely updated)
- **Pros**: Predictable compaction, good for time-series
- **Cons**: Not ideal if updates are scattered across time

### Real-World Example: Kafka (Append-Only Message Queue)

Kafka is a distributed message queue built on append-only logs:

1. **Write** (produce): Append message to end of log partition (sequential write, ~100 MB/s per partition)
2. **Read** (consume): Read messages sequentially from log (sequential read, ~100?500 MB/s)
3. **Retention**: Delete old log segments based on time/size (e.g., keep last 7 days)

**Why it's so fast:**
- All writes are sequential appends (no seeks)
- All reads are sequential scans (no seeks)
- Simple concurrency (each partition is single-writer, multi-reader)

**Performance**: Single Kafka broker can handle 100,000+ messages/sec with sub-millisecond latency.

---

## Why This Becomes a Bottleneck (When You DON'T Use Append-Only)

Update-in-place becomes a bottleneck for write-heavy workloads:

**Random I/O overhead**: Each update requires seeking to the record's location. On HDD, seeks dominate (5?15 ms each). On SSD, FTL overhead and write amplification slow updates.

**Read-modify-write overhead**: To update a record, you must read it first, modify it in memory, then write it back. This doubles I/O cost.

**Write amplification**: On SSDs, updating a 1 KB record might require erasing and rewriting a 256 KB block (256? write amplification). Append-only avoids this by always writing new data.

**Concurrency complexity**: Update-in-place requires locks or MVCC to prevent conflicts when multiple writers update the same record. Append-only is simpler?each writer appends independently.

---

## When to Use This Approach

Use append-only storage when:

- **Write-heavy workload** (many writes, few updates/deletes). Example: Logging, event streams, time-series data.
- **Writes vastly outnumber reads** (99% writes, 1% reads). Example: Event sourcing, WAL, audit logs.
- **Immutability is valuable** (you want a complete history of changes). Example: Audit trails, event sourcing, blockchain.
- **Concurrency is important** (many writers, no coordination). Example: Distributed systems, message queues.
- **Sequential I/O is much faster than random I/O** (HDD, or SSD with high write amplification). Example: HDD-based storage, cloud block storage.
---

## Example Scenarios

### Scenario 1: Application logging (append-only)

**Problem**: A web application that logs every request. Logging must be fast (low overhead) and never block request processing.

**Solution**: Use append-only log files. Each log entry is appended to the end.

```csharp
// ? Good: Append-only logging
public class AppendOnlyLogger
{
    private readonly string _logPath;
    private readonly SemaphoreSlim _semaphore = new SemaphoreSlim(1, 1);

    public AppendOnlyLogger(string logPath)
    {
        _logPath = logPath;
    }

    public async Task LogAsync(string message)
    {
        await _semaphore.WaitAsync();
        try
        {
            // Append to end (FileMode.Append)
            using var writer = new StreamWriter(_logPath, append: true);
            await writer.WriteLineAsync($"{DateTime.UtcNow:O} {message}");
        }
        finally
        {
            _semaphore.Release();
        }
    }

    // Read logs sequentially (fast)
    public async Task<List<string>> ReadLogsAsync()
    {
        var logs = new List<string>();
        using var reader = new StreamReader(_logPath);
        
        string line;
        while ((line = await reader.ReadLineAsync()) != null)
        {
            logs.Add(line);
        }
        
        return logs;
    }
}
```

**Why it works**:
- All writes are appends (sequential, fast)
- No seeks (head stays at end on HDD)
- Simple concurrency (one writer at a time via semaphore)
- Reads are sequential (fast for "get all logs")

**Results**:
- **Write throughput**: ~50?150 MB/s (HDD), ~200?500 MB/s (SSD)
- **Write latency**: ~1?5 ms per log entry
- **Space**: Grows unbounded (rotate/delete old logs periodically)

---

### Scenario 2: Event sourcing (append-only event store)

**Problem**: An e-commerce system that tracks all user actions (orders, payments, shipments) as events. You need complete history for auditing and debugging.

**Solution**: Store all events in an append-only log. Current state is derived by replaying events.

```csharp
// Event store (append-only)
public class EventStore
{
    private readonly string _eventLogPath;

    public EventStore(string eventLogPath)
    {
        _eventLogPath = eventLogPath;
    }

    // Append event (sequential write)
    public async Task AppendEventAsync(Event e)
    {
        using var writer = new StreamWriter(_eventLogPath, append: true);
        string json = JsonSerializer.Serialize(e);
        await writer.WriteLineAsync(json);
    }

    // Read all events for an entity (scan log)
    public async Task<List<Event>> GetEventsAsync(string entityId)
    {
        var events = new List<Event>();
        using var reader = new StreamReader(_eventLogPath);
        
        string line;
        while ((line = await reader.ReadLineAsync()) != null)
        {
            var e = JsonSerializer.Deserialize<Event>(line);
            if (e.EntityId == entityId)
                events.Add(e);
        }
        
        return events;
    }

    // Rebuild current state from events
    public async Task<Order> GetOrderAsync(string orderId)
    {
        var events = await GetEventsAsync(orderId);
        var order = new Order { OrderId = orderId };
        
        foreach (var e in events)
        {
            order.Apply(e);  // Apply each event to rebuild state
        }
        
        return order;
    }
}

// Example events
public record Event(string EntityId, string Type, DateTime Timestamp);
public record OrderCreated(string OrderId, string UserId, DateTime Timestamp) : Event(OrderId, "OrderCreated", Timestamp);
public record OrderPaid(string OrderId, decimal Amount, DateTime Timestamp) : Event(OrderId, "OrderPaid", Timestamp);
```

**Why it works**:
- Complete audit trail (every change is an event)
- Immutable events (can't be tampered with)
- Fast writes (sequential appends)
- Can rebuild state by replaying events

**Trade-offs**:
- Reads are slower (must replay all events or maintain snapshots)
- Space grows unbounded (need periodic compaction or archival)

---

### Scenario 3: Write-Ahead Log (WAL) for durability

**Problem**: A database needs to guarantee durability?once a transaction commits, data is safe even if the system crashes.

**Solution**: Write all changes to an append-only WAL before applying them to the main data files.

```csharp
// Simplified WAL implementation
public class WriteAheadLog
{
    private readonly string _walPath;

    public WriteAheadLog(string walPath)
    {
        _walPath = walPath;
    }

    // Append transaction to WAL (durable)
    public async Task LogTransactionAsync(Transaction tx)
    {
        using var writer = new StreamWriter(_walPath, append: true);
        string json = JsonSerializer.Serialize(tx);
        await writer.WriteLineAsync(json);
        await writer.FlushAsync();  // Force to disk (durability)
    }

    // Replay WAL after crash (recovery)
    public async Task ReplayAsync(Action<Transaction> applyTransaction)
    {
        using var reader = new StreamReader(_walPath);
        
        string line;
        while ((line = await reader.ReadLineAsync()) != null)
        {
            var tx = JsonSerializer.Deserialize<Transaction>(line);
            applyTransaction(tx);  // Re-apply transaction
        }
    }
}

public record Transaction(string Id, string Operation, string Data, DateTime Timestamp);
```

**Why it works**:
- Durability: Once written to WAL, data is safe
- Fast writes: Append-only (sequential)
- Crash recovery: Replay WAL to restore state

**Used by**: PostgreSQL, MySQL, Redis, Cassandra, MongoDB

---

## Summary and Key Takeaways

Append-only storage optimizes for write throughput by only adding data to the end of a file or log, never modifying existing data in place. This eliminates update overhead (no seeks, no read-modify-write cycles), simplifies concurrency (multiple writers append independently), and maximizes sequential write performance. The trade-offs are space amplification (old data remains until compaction), increased read complexity (must scan/skip obsolete data), and the need for periodic compaction. Use append-only for write-heavy workloads (logging, event sourcing, time-series data, WAL) where writes vastly outnumber updates/deletes. Avoid it for update-heavy or read-heavy workloads. Typical improvements: 5??20? higher write throughput, but 2??5? more space usage before compaction. Always measure write throughput, read latency, and space usage under realistic load.

---

<!-- Tags: Performance, Optimization, Storage & I/O, File I/O, Throughput Optimization, System Design, Architecture, Event Sourcing, Logging, Database Optimization, .NET Performance, C# Performance -->
