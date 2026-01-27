# Preallocate File Space to Reduce Fragmentation and Improve Write Performance

**Preallocating file space reserves contiguous disk blocks before writing data, preventing fragmentation and eliminating the overhead of dynamic allocation during writes. This can improve write throughput by 10×–50× on HDDs and 2×–5× on SSDs, especially for large files or sequential writes.**

---

## Executive Summary (TL;DR)

When you write to a file without preallocation, the filesystem allocates disk blocks on-demand as the file grows. This causes fragmentation (file data scattered across non-contiguous blocks) and allocation overhead (repeated metadata updates). Each allocation requires updating the inode, allocation tables, and potentially splitting extents. On HDDs, fragmentation forces multiple seeks during reads/writes (5–15 ms per seek). On SSDs, fragmentation increases FTL (Flash Translation Layer) overhead and write amplification. Preallocating file space using `fallocate()` (Linux), `SetFileValidData()` (Windows), or `FileStream.SetLength()` (.NET) reserves contiguous disk blocks upfront, eliminating allocation overhead and fragmentation. The trade-off is immediate disk space consumption (even if data isn't written yet) and wasted space if the file doesn't grow to the preallocated size. Use preallocation for large files (>100 MB), sequential writes, databases, logs, and video/media files. Avoid it for small files (<1 MB), dynamic-size files, or when disk space is constrained. Typical improvements: 10×–50× faster writes on HDD (fragmented vs contiguous), 2×–5× faster on SSD.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you write to a file without preallocation?**

Imagine you're writing a 1 GB log file incrementally (1 MB at a time):

1. **First write (1 MB)**:
   - Filesystem finds free blocks (e.g., blocks 1000–1255)
   - Allocates blocks, updates inode and allocation tables
   - Writes data

2. **Second write (1 MB)**:
   - Filesystem finds more free blocks (e.g., blocks 5000–5255) ← **Not contiguous!**
   - Allocates blocks, updates metadata
   - Writes data

3. **After 1000 writes**:
   - File is scattered across 1000 non-contiguous regions
   - Reading the file requires 1000 seeks on HDD (~10 seconds of seek time)
   - Each write paid allocation overhead (~0.1–1 ms per allocation)

**Real-world example: Database write-ahead log (WAL)**

A database writes to a WAL file continuously:
- **Without preallocation**: Every write triggers allocation overhead + fragmentation. After hours of operation, the WAL is fragmented across thousands of regions. Checkpoint operations (which read the entire WAL) become extremely slow (10× slower).
- **With preallocation**: The WAL is allocated as contiguous 1 GB blocks. Writes are fast (no allocation overhead), and checkpoint reads are sequential (no seeks).

### Key Terms Explained (Start Here!)

**What is file preallocation?** Reserving disk space for a file before writing data. The filesystem allocates contiguous blocks and updates the file's metadata (inode) to reflect the new size, but the blocks contain garbage data (not zeros, unless you use zero-fill preallocation). This makes subsequent writes faster because allocation is already done.

**What is fragmentation?** When a file's data is scattered across non-contiguous disk blocks. Example: A 10 MB file might occupy blocks [100–199], [500–599], [1000–1099] instead of [100–2599]. On HDDs, reading fragmented files requires multiple seeks (5–15 ms each). On SSDs, fragmentation increases FTL overhead.

**What is an extent?** A contiguous range of disk blocks. Modern filesystems (ext4, XFS, NTFS) use extents to represent file storage efficiently. Example: An extent might represent "blocks 1000–1999" (1000 contiguous blocks). A fragmented file has many extents; a contiguous file has one or few extents.

**What is allocation overhead?** The cost of finding free blocks, updating the inode, updating allocation tables (bitmap, B-tree), and potentially splitting extents. Each allocation costs ~0.1–1 ms. For a 1 GB file written in 1 MB chunks, that's 1000 allocations × 0.5 ms = 500 ms of pure overhead.

**What is `fallocate()`?** A Linux system call that preallocates file space. Modes:
- `FALLOC_FL_KEEP_SIZE`: Preallocate without changing file size (space is reserved but file appears empty)
- Default: Preallocate and set file size (file appears full of garbage data)
- `FALLOC_FL_ZERO_RANGE`: Preallocate and zero-fill (slower but secure)

**What is `SetFileValidData()`?** A Windows API that sets the "valid data length" of a file without zero-filling. This is fast but leaves garbage data in unwritten regions (security risk).

**What is write amplification?** On SSDs, writing 1 KB of data might require erasing and rewriting an entire 256 KB block (256× write amplification). Fragmentation exacerbates write amplification because the SSD must manage more small, scattered writes.

### Common Misconceptions

**"SSDs don't suffer from fragmentation"**
- **The truth**: SSDs don't have seek overhead like HDDs, but fragmentation still causes FTL overhead (more mapping entries, more garbage collection) and write amplification. Preallocation improves SSD performance by 2×–5× for large sequential writes.

**"Preallocation wastes disk space"**
- **The truth**: Yes, preallocation consumes disk space immediately, even if data isn't written yet. But the performance gain (10×–50× on HDD) often justifies the space cost. For critical files (databases, logs), it's a worthwhile trade-off.

**"SetLength() in .NET preallocates space"**
- **The truth**: `SetLength()` sets the file size but may not preallocate disk blocks (depends on OS and filesystem). On Linux with ext4/XFS, `SetLength()` triggers `fallocate()` and preallocates. On Windows with NTFS, it may create a sparse file (no actual space allocated). To guarantee preallocation on Windows, use P/Invoke to call `SetFileValidData()`.

**"I need to zero-fill preallocated space"**
- **The truth**: Zero-filling is only necessary for security (prevent reading old data). For performance, use non-zero preallocation (`fallocate()` default, `SetFileValidData()` on Windows). Zero-filling is much slower (requires writing zeros to every block).

---

## How It Works

### Understanding File Allocation

**How filesystems allocate space (without preallocation):**

1. **Initial file creation**:
   - Create inode with size = 0, no blocks allocated
   - File appears empty

2. **First write (1 MB)**:
   - Filesystem searches allocation bitmap/B-tree for 256 free blocks (4 KB blocks)
   - Allocates blocks, updates inode with extent: [blocks 1000–1255]
   - Updates allocation tables
   - Writes data
   - **Cost: 0.5 ms allocation + 5 ms write = 5.5 ms**

3. **Second write (1 MB)**:
   - Filesystem searches for 256 more free blocks
   - May not find contiguous space (disk is busy, other files allocated in between)
   - Allocates blocks [5000–5255] (not contiguous!)
   - Updates inode with new extent: [1000–1255, 5000–5255]
   - **Cost: 0.5 ms allocation + 5 ms write + 10 ms seek = 15.5 ms** (seek penalty!)

4. **After many writes**:
   - File has many extents (fragmented)
   - Inode is bloated with extent list
   - Reads require many seeks

**How preallocation works:**

```csharp
// ❌ Without preallocation (slow, fragmented)
public void WriteLogWithoutPreallocation(string logPath)
{
    using var fs = new FileStream(logPath, FileMode.Append, FileAccess.Write);
    for (int i = 0; i < 1000; i++)
    {
        byte[] data = GenerateLogData(1024 * 1024); // 1 MB
        fs.Write(data, 0, data.Length);  // Each write: allocation overhead + seek
    }
    // Total: 1000 allocations + fragmentation = slow
}

// ✅ With preallocation (fast, contiguous)
public void WriteLogWithPreallocation(string logPath)
{
    using var fs = new FileStream(logPath, FileMode.Create, FileAccess.Write);
    
    // Preallocate 1 GB (1000 MB)
    fs.SetLength(1024L * 1024 * 1024);  // On Linux ext4/XFS, this calls fallocate()
    fs.Position = 0;
    
    for (int i = 0; i < 1000; i++)
    {
        byte[] data = GenerateLogData(1024 * 1024); // 1 MB
        fs.Write(data, 0, data.Length);  // Fast: no allocation, contiguous writes
    }
    // Total: 1 allocation upfront + sequential writes = fast
}
```

**Performance comparison:**

| Operation | Without preallocation | With preallocation | Improvement |
|-----------|----------------------|-------------------|-------------|
| **Allocation overhead** | 1000 × 0.5 ms = 500 ms | 1 × 0.5 ms = 0.5 ms | **1000×** |
| **Seek overhead (HDD)** | ~500 seeks × 10 ms = 5000 ms | 0 seeks = 0 ms | **Infinite** |
| **Write time** | 1000 × 5 ms = 5000 ms | 1000 × 5 ms = 5000 ms | **Same** |
| **Total (HDD)** | **10,500 ms** | **5000 ms** | **2.1×** |
| **Total (SSD)** | **500 ms + 5000 ms = 5500 ms** | **5000 ms** | **1.1×** |

**Key insight**: On HDD, preallocation eliminates seek overhead (massive win). On SSD, it eliminates allocation overhead (modest win).

### Technical Details: Filesystem Internals

**What happens during `fallocate()` (Linux):**

1. **Find contiguous free space**:
   - Filesystem searches for a contiguous region of free blocks large enough for the requested size
   - Example: For 1 GB file (256,000 blocks), find blocks [1,000,000–1,256,000]

2. **Allocate blocks**:
   - Mark blocks as allocated in allocation bitmap/B-tree
   - Update inode with single extent: [1,000,000–1,256,000]

3. **Update metadata**:
   - Set file size to 1 GB (or keep size = 0 with `FALLOC_FL_KEEP_SIZE`)
   - No data is written (blocks contain garbage)

4. **Future writes**:
   - Writes go directly to preallocated blocks (no allocation overhead)
   - Sequential writes remain contiguous (no seeks on HDD)

**Why preallocation is fast:**
- **One-time allocation cost**: Pay allocation overhead once (0.5 ms) instead of 1000 times (500 ms)
- **Contiguous layout**: Guarantees contiguous blocks (no fragmentation, no seeks)
- **No block search**: Subsequent writes don't need to search for free blocks

**Why preallocation is guaranteed contiguous (usually):**
- Allocating a large block at once increases the likelihood of finding contiguous space
- Small incremental allocations compete with other files and get scattered
- Exception: If disk is very full (>90%), even preallocation may be fragmented

---

## Why This Becomes a Bottleneck

Not preallocating becomes a bottleneck because:

**Allocation overhead accumulates**: Writing a 1 GB file in 1 MB chunks requires 1000 allocations. At 0.5 ms per allocation, that's 500 ms of pure overhead (10% of total time for a 5-second write).

**Fragmentation kills HDD performance**: A fragmented file on HDD requires seeks between every non-contiguous region. 500 seeks × 10 ms = 5 seconds of wasted seek time.

**FTL overhead on SSDs**: Fragmented writes force the SSD's FTL to manage many small, scattered mappings. This increases garbage collection overhead and write amplification (2×–5× slower).

**Metadata overhead**: Each extent requires an entry in the inode's extent list. A file with 1000 extents has a bloated inode, slowing metadata operations (stat, listing).

---

## Advantages

**Eliminates allocation overhead**: Preallocate once instead of allocating on every write. Example: 1000 allocations × 0.5 ms = 500 ms saved.

**Prevents fragmentation**: Guarantees contiguous layout (one extent instead of 1000). On HDD, this eliminates seek overhead. Example: 500 seeks × 10 ms = 5000 ms saved.

**Faster writes**: Without allocation overhead and seeks, writes are purely data transfer speed. Example: 1 GB write improves from 10.5 seconds to 5 seconds (2.1× on HDD).

**Predictable performance**: Contiguous files have consistent read/write latency (no random seeks). This is critical for real-time applications (video recording, databases).

**Reduced write amplification on SSDs**: Contiguous writes reduce the number of FTL mapping updates and garbage collection events.

---

## Disadvantages and Trade-offs

**Immediate disk space consumption**: Preallocated space is reserved immediately, even if data isn't written yet. This can waste space if the file doesn't grow to the preallocated size.

**Wasted space if overestimated**: If you preallocate 1 GB but only write 500 MB, you waste 500 MB. This is problematic if disk space is constrained.

**Security risk (non-zero preallocation)**: Preallocating without zero-filling leaves garbage data (old deleted files) in unwritten regions. If the file is readable before being fully written, sensitive data might leak. Use zero-fill preallocation for security-critical files.

**Not portable**: `fallocate()` is Linux-specific. Windows has `SetFileValidData()`, but it requires `SeManageVolumePrivilege` (admin-only). Portable code must use `SetLength()` and hope the OS preallocates.

**Fragmentation if disk is full**: If the disk is >90% full, the filesystem may not find contiguous space, and preallocation will still result in fragmentation (though less than incremental allocation).

---

## When to Use This Approach

Preallocate file space when:

- **Large files** (>100 MB). Example: Database files, video recordings, disk images. The allocation overhead and fragmentation are significant for large files.
- **Sequential writes dominate**. Example: Logs, write-ahead logs (WAL), time-series data. Sequential writes benefit most from contiguous layout.
- **Predictable file size**. Example: Video files (you know the duration and bitrate), database tablespaces (you allocate in fixed-size chunks). Preallocation works best when you can estimate the final size.
- **HDD storage**. Example: Any large file on HDD. The seek overhead saved by preallocation (5–10 seconds for a 1 GB file) is massive.
- **Database files**. Example: PostgreSQL, MySQL use preallocation for tablespaces to avoid fragmentation and allocation pauses.

---

## When Not to Use It

Avoid preallocation when:

- **Small files** (<1 MB). Example: Config files, small JSON files. Allocation overhead is negligible (<0.5 ms), and fragmentation is not a concern.
- **Dynamic-size files** (unpredictable growth). Example: User uploads, variable-length logs. Preallocating too much wastes space; preallocating too little doesn't help.
- **Disk space is constrained**. Example: Embedded systems, containers with small quotas. Preallocating 1 GB immediately might fill the disk.
- **Security-critical files** (unless zero-filled). Example: User-readable files that might contain old data. Use zero-fill preallocation or avoid preallocation.
- **SSD-only workloads with small writes**. Example: Many small random writes (<4 KB). Preallocation's benefit is smaller on SSDs (2× instead of 10×), and small random writes don't benefit from contiguous layout.

---

## Common Mistakes

**Not preallocating database files**: Databases that grow incrementally become fragmented, causing slow queries. Preallocate tablespaces in fixed-size chunks (e.g., 1 GB).

**Preallocating too much**: Preallocating 10 GB for a file that grows to 1 GB wastes 9 GB. Estimate conservatively or grow in chunks.

**Using `SetLength()` on Windows and assuming preallocation**: On Windows NTFS, `SetLength()` might create a sparse file (no actual space allocated). Use `SetFileValidData()` for true preallocation.

**Not checking for `fallocate()` support**: Older filesystems (FAT32, ext3 without extents) don't support `fallocate()`. Check the return value and fall back gracefully.

**Preallocating without zero-fill for security-critical files**: Non-zero preallocation leaves garbage data. For files that must not leak old data, use `FALLOC_FL_ZERO_RANGE` (Linux) or write zeros manually.

**Preallocating on nearly-full disks**: If disk is >90% full, preallocation may fail or result in fragmentation. Monitor disk space and fail gracefully.

---

## Example Scenarios

### Scenario 1: Database write-ahead log (WAL)

**Problem**: A database writes to a WAL file continuously. Without preallocation, the WAL becomes fragmented, slowing checkpoint operations (which read the entire WAL).

**Bad approach** (no preallocation):

```csharp
// ❌ Bad: No preallocation (fragmented WAL)
public class DatabaseWALWithoutPreallocation
{
    private FileStream _wal;

    public void Initialize(string walPath)
    {
        _wal = new FileStream(walPath, FileMode.Append, FileAccess.Write);
    }

    public void WriteTransaction(byte[] txData)
    {
        _wal.Write(txData, 0, txData.Length);  // Incremental allocation → fragmentation
    }

    public void Checkpoint()
    {
        _wal.Flush();
        // Read entire WAL (fragmented → many seeks on HDD → slow)
    }
}
```

**Good approach** (preallocate WAL):

```csharp
// ✅ Good: Preallocate WAL (contiguous, fast)
public class DatabaseWALWithPreallocation
{
    private FileStream _wal;
    private long _walSize = 1024L * 1024 * 1024; // 1 GB

    public void Initialize(string walPath)
    {
        _wal = new FileStream(walPath, FileMode.Create, FileAccess.Write);
        
        // Preallocate 1 GB
        _wal.SetLength(_walSize);  // On Linux ext4/XFS, calls fallocate()
        _wal.Position = 0;
    }

    public void WriteTransaction(byte[] txData)
    {
        _wal.Write(txData, 0, txData.Length);  // Fast: no allocation, contiguous
    }

    public void Checkpoint()
    {
        _wal.Flush();
        // Read entire WAL (contiguous → sequential read → fast)
    }
}
```

**Results**:
- **Bad**: WAL becomes fragmented over time (100+ extents), checkpoint reads take 15 seconds (HDD)
- **Good**: WAL remains contiguous (1 extent), checkpoint reads take 5 seconds (HDD)
- **Improvement**: 3× faster checkpoints

---

### Scenario 2: Video recording

**Problem**: Recording a 1-hour 1080p video (5 GB) to disk. Without preallocation, the file becomes fragmented, causing dropped frames during write spikes.

**Bad approach** (no preallocation):

```csharp
// ❌ Bad: No preallocation (dropped frames)
public class VideoRecorderWithoutPreallocation
{
    private FileStream _videoFile;

    public void StartRecording(string videoPath)
    {
        _videoFile = new FileStream(videoPath, FileMode.Create, FileAccess.Write);
    }

    public void WriteFrame(byte[] frameData)
    {
        _videoFile.Write(frameData, 0, frameData.Length);  // Allocation overhead → latency spike → dropped frame
    }
}
```

**Good approach** (preallocate video file):

```csharp
// ✅ Good: Preallocate video file (no dropped frames)
public class VideoRecorderWithPreallocation
{
    private FileStream _videoFile;

    public void StartRecording(string videoPath, long estimatedSize)
    {
        _videoFile = new FileStream(videoPath, FileMode.Create, FileAccess.Write);
        
        // Preallocate estimated size (e.g., 5 GB for 1 hour 1080p)
        _videoFile.SetLength(estimatedSize);
        _videoFile.Position = 0;
    }

    public void WriteFrame(byte[] frameData)
    {
        _videoFile.Write(frameData, 0, frameData.Length);  // Fast: no allocation, no latency spikes
    }
}
```

**Results**:
- **Bad**: Allocation overhead causes latency spikes (1–10 ms), dropped frames during recording
- **Good**: Consistent write latency (<1 ms), no dropped frames
- **Improvement**: Zero dropped frames vs 5–10 dropped frames per minute

---

### Scenario 3: Log file rotation

**Problem**: A logging system rotates log files daily. Each new log file grows to ~1 GB. Without preallocation, the log becomes fragmented.

**Bad approach** (no preallocation):

```csharp
// ❌ Bad: No preallocation (fragmented logs)
public class LoggerWithoutPreallocation
{
    private FileStream _logFile;

    public void RotateLog(string logPath)
    {
        _logFile?.Dispose();
        _logFile = new FileStream(logPath, FileMode.Create, FileAccess.Write);
    }

    public void Log(string message)
    {
        byte[] data = Encoding.UTF8.GetBytes($"{DateTime.UtcNow:O} {message}\n");
        _logFile.Write(data, 0, data.Length);  // Incremental allocation → fragmentation
    }
}
```

**Good approach** (preallocate log file):

```csharp
// ✅ Good: Preallocate log file (contiguous)
public class LoggerWithPreallocation
{
    private FileStream _logFile;

    public void RotateLog(string logPath)
    {
        _logFile?.Dispose();
        _logFile = new FileStream(logPath, FileMode.Create, FileAccess.Write);
        
        // Preallocate 1 GB (estimated daily log size)
        _logFile.SetLength(1024L * 1024 * 1024);
        _logFile.Position = 0;
    }

    public void Log(string message)
    {
        byte[] data = Encoding.UTF8.GetBytes($"{DateTime.UtcNow:O} {message}\n");
        _logFile.Write(data, 0, data.Length);  // Fast: no allocation
    }
}
```

**Results**:
- **Bad**: Log file has 100+ extents, reading full log takes 15 seconds (HDD)
- **Good**: Log file has 1 extent, reading full log takes 5 seconds (HDD)
- **Improvement**: 3× faster log reads (for analysis, archival)

---

## Summary and Key Takeaways

Preallocating file space reserves contiguous disk blocks before writing data, eliminating allocation overhead and fragmentation. Without preallocation, a 1 GB file written in 1 MB chunks requires 1000 allocations (500 ms overhead) and becomes fragmented (500 seeks on HDD = 5 seconds). Preallocation pays allocation cost once (0.5 ms) and guarantees contiguous layout (no seeks). This improves write throughput by 10×–50× on HDD (5–50 seconds → 5 seconds) and 2×–5× on SSD. The trade-off is immediate disk space consumption and wasted space if the file doesn't grow to the preallocated size. Use preallocation for large files (>100 MB), sequential writes, databases, logs, and videos. Avoid it for small files (<1 MB), dynamic-size files, or constrained disk space. On Linux, use `fallocate()` (via `FileStream.SetLength()`). On Windows, use `SetFileValidData()` (requires admin). Always measure fragmentation with `filefrag` (Linux) or `fsutil file queryextents` (Windows). Preallocated files should have 1 extent, not 100+.

---

<!-- Tags: Performance, Optimization, Storage & I/O, File I/O, System Design, .NET Performance, C# Performance, Throughput Optimization, Latency Optimization, Database Optimization, Profiling, Measurement -->
