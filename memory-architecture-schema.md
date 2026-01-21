# Memory Architecture: Complete Relationship Schema

**A comprehensive guide to understanding how memory components relate: CPU caches, virtual memory, pages, cache lines, buffers, and locality principles.**

---

## Table of Contents

1. [Overview: The Memory Hierarchy](#overview-the-memory-hierarchy)
2. [CPU Cache System (L1, L2, L3)](#cpu-cache-system-l1-l2-l3)
3. [Virtual Memory System](#virtual-memory-system)
4. [Physical Memory (RAM)](#physical-memory-ram)
5. [Storage (Disk/SSD)](#storage-diskssd)
6. [Kernel vs User Space](#kernel-vs-user-space)
7. [Data Transfer Flow](#data-transfer-flow)
8. [Locality Principles](#locality-principles)
9. [Complete Data Flow Examples](#complete-data-flow-examples)

---

## Overview: The Memory Hierarchy

```
┌─────────────────────────────────────────────────────────────────┐
│                     APPLICATION CODE                             │
│                   (Your C# / Program)                            │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ Virtual Addresses
                              │ (0x0000 - 0xFFFFFFFF...)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  VIRTUAL ADDRESS SPACE                           │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │              PAGE TABLE (OS-managed)                      │   │
│  │  Maps Virtual Address → Physical Address or "Not Present" │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              │ Page Fault Handler               │
│                              │ (if page not in RAM)             │
│                              ▼                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ Physical Addresses
                              │ (MMU Translation)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      PHYSICAL MEMORY (RAM)                       │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐          │
│  │  Kernel Space│  │  User Space  │  │   File Cache │          │
│  │  (OS buffers)│  │  (App data)  │  │  (OS-managed)│          │
│  └──────────────┘  └──────────────┘  └──────────────┘          │
│                              │                                   │
│                              │ Cache Line (64 bytes)            │
│                              │ Load/Store Units                 │
│                              ▼                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ Memory Bus
                              │ (Load/Store Operations)
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      CPU CACHE HIERARCHY                         │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  L1 Cache (Per Core)                                     │   │
│  │  - Instruction Cache (L1i): ~32KB                        │   │
│  │  - Data Cache (L1d): ~32KB                               │   │
│  │  - Cache Line: 64 bytes                                  │   │
│  │  - Latency: ~1-3 cycles                                  │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              │ (Cache Miss → Load from L2)      │
│                              ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  L2 Cache (Per Core)                                     │   │
│  │  - Unified (Instructions + Data)                         │   │
│  │  - Size: ~256KB - 1MB                                    │   │
│  │  - Cache Line: 64 bytes                                  │   │
│  │  - Latency: ~10-20 cycles                                │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              │ (Cache Miss → Load from L3)      │
│                              ▼                                   │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │  L3 Cache (Shared, Last Level Cache - LLC)               │   │
│  │  - Shared by all cores                                   │   │
│  │  - Size: ~8MB - 64MB                                     │   │
│  │  - Cache Line: 64 bytes                                  │   │
│  │  - Latency: ~40-75 cycles                                │   │
│  └──────────────────────────────────────────────────────────┘   │
│                              │                                   │
│                              │ (Cache Miss → Load from RAM)     │
│                              ▼                                   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              │ Memory Controller
                              │ (Load entire Cache Line)
                              ▼
                    ┌─────────────────────┐
                    │   PHYSICAL RAM      │
                    │  (100-300 cycles)   │
                    └─────────────────────┘
                              │
                              │ (Page Fault → Load from Disk)
                              ▼
                    ┌─────────────────────┐
                    │  STORAGE (Disk/SSD) │
                    │  (5-10ms disk /     │
                    │   100-500μs SSD)    │
                    └─────────────────────┘
```

**Key Relationships:**
- **Application** uses **Virtual Addresses** (program sees continuous memory)
- **Virtual Address Space** is mapped by **Page Table** to **Physical RAM**
- **Physical RAM** loads data in **Cache Lines** (64 bytes) to **CPU Caches**
- **CPU Caches** (L1 → L2 → L3) filter memory access
- **Storage** is accessed via **Page Faults** when data not in RAM

---

## CPU Cache System (L1, L2, L3)

### Cache Hierarchy Details

```
┌─────────────────────────────────────────────────────────────┐
│                     CPU CORE                                │
│                                                              │
│  ┌──────────────────┐         ┌──────────────────┐         │
│  │   L1 Instruction │         │    L1 Data       │         │
│  │   Cache (L1i)    │         │    Cache (L1d)   │         │
│  │                  │         │                  │         │
│  │  Size: ~32KB     │         │  Size: ~32KB     │         │
│  │  Associativity:  │         │  Associativity:  │         │
│  │  4-8 way         │         │  4-8 way         │         │
│  │  Latency: 1-3    │         │  Latency: 1-3    │         │
│  │  cycles          │         │  cycles          │         │
│  └──────────────────┘         └──────────────────┘         │
│           │                            │                    │
│           └────────────┬───────────────┘                    │
│                        │                                    │
│                        ▼                                    │
│              ┌──────────────────┐                          │
│              │   L2 Cache       │                          │
│              │   (Unified)      │                          │
│              │                  │                          │
│              │  Size: 256KB-1MB │                          │
│              │  Associativity:  │                          │
│              │  8-16 way        │                          │
│              │  Latency: 10-20  │                          │
│              │  cycles          │                          │
│              └──────────────────┘                          │
│                        │                                    │
└────────────────────────┼────────────────────────────────────┘
                         │
                         ▼
┌─────────────────────────────────────────────────────────────┐
│              L3 Cache (Shared, Last Level Cache)            │
│                                                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Shared by ALL CPU cores                             │   │
│  │                                                       │   │
│  │  Size: 8MB - 64MB                                    │   │
│  │  Associativity: 16-32 way                            │   │
│  │  Latency: 40-75 cycles                               │   │
│  │                                                       │   │
│  │  Serves as filter before accessing RAM               │   │
│  └──────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
                         │
                         │ Cache Miss
                         │ (Data not in L1, L2, or L3)
                         ▼
              ┌──────────────────┐
              │   Physical RAM   │
              │   (100-300 cycles│
              │    latency)      │
              └──────────────────┘
```

### Cache Line Structure

**What is a Cache Line?**
- **Size**: 64 bytes (standard on modern CPUs)
- **Unit of transfer**: When you access 1 byte, the entire 64-byte cache line is loaded
- **Why**: Spatial locality - if you access one byte, nearby bytes are likely accessed soon

```
┌─────────────────────────────────────────────────────────────┐
│                    CACHE LINE (64 bytes)                     │
├─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┬─────┤
│  0  │  1  │  2  │  3  │ ... │ ... │ ... │ ... │ 61  │ 62  │
│  1  │  2  │  3  │  4  │ ... │ ... │ ... │ ... │ 62  │ 63  │
│byte │byte │byte │byte │     │     │     │     │byte │byte │
└─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┘
│                                                               │
│  When you access byte at offset 5:                           │
│  1. CPU loads entire 64-byte cache line                      │
│  2. Byte 5 is now available in cache                         │
│  3. Bytes 0-63 are also in cache (spatial locality!)         │
│                                                               │
│  This is why accessing nearby memory is fast!                │
└─────────────────────────────────────────────────────────────┘
```

### Cache Coherency (MESI Protocol)

**Multi-core systems**: Multiple CPU cores can have copies of the same cache line. MESI protocol ensures consistency:

```
┌─────────────────────────────────────────────────────────────┐
│                    MESI STATES                               │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  M - Modified: This core has the only valid copy, modified  │
│  E - Exclusive: This core has the only copy, unmodified     │
│  S - Shared: Multiple cores may have copies, unmodified     │
│  I - Invalid: Cache line is invalid/stale                   │
│                                                               │
│  When Core A writes to a cache line in S (Shared) state:    │
│  1. Invalidates copies in other cores (S → I)               │
│  2. Core A's copy becomes Modified (S → M)                  │
│  3. Other cores must reload from Core A or RAM              │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Virtual Memory System

### Virtual Address Space Layout

```
┌─────────────────────────────────────────────────────────────┐
│            VIRTUAL ADDRESS SPACE (64-bit process)            │
│                                                               │
│  0x0000000000000000  ┌──────────────────────────────┐       │
│                      │  Reserved (Null pointer)     │       │
│                      │  ~0-128MB                    │       │
│  0x0000008000000000  ├──────────────────────────────┤       │
│                      │  User Space                  │       │
│                      │  - Application code          │       │
│                      │  - Heap (malloc/new)         │       │
│                      │  - Stack (function calls)    │       │
│                      │  - Memory-mapped files       │       │
│                      │  - Shared libraries          │       │
│                      │  Size: ~128TB                │       │
│  0x00007FFFFFFFFFFF  ├──────────────────────────────┤       │
│                      │  Kernel Space (128TB gap)    │       │
│                      │  (Guarded by OS)             │       │
│  0xFFFF800000000000  ├──────────────────────────────┤       │
│                      │  Kernel Space                │       │
│                      │  - OS code                   │       │
│                      │  - Kernel buffers            │       │
│                      │  - Device memory             │       │
│                      │  Size: ~128TB                │       │
│  0xFFFFFFFFFFFFFFFF  └──────────────────────────────┘       │
│                                                               │
│  Note: Virtual addresses are NOT physical addresses!         │
│  The OS maps virtual addresses to physical RAM via           │
│  page tables.                                                │
└─────────────────────────────────────────────────────────────┘
```

### Page Table Structure

**What is a Page Table?**
- OS-maintained data structure
- Maps Virtual Addresses → Physical Addresses (or "Not Present")
- Used by CPU's MMU (Memory Management Unit) for address translation

```
┌─────────────────────────────────────────────────────────────┐
│                    PAGE TABLE ENTRY                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Virtual Address: 0x00007FFF12345678                         │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Page Table Entry (64 bits on x86-64)               │   │
│  ├──────┬──────┬──────┬──────┬──────┬──────┬──────┬────┤   │
│  │  63  │  62  │  51  │  50  │  12  │  11  │  2   │ 1,0│   │
│  ├──────┼──────┼──────┼──────┼──────┼──────┼──────┼────┤   │
│  │  N   │  P   │      Physical Page Address (40 bits)  │   │
│  │  X   │  W   │      → Points to physical RAM page    │   │
│  │  B   │  T   │                                        │   │
│  │  i   │  S   │                                        │   │
│  │  t   │  D   │                                        │   │
│  │  s   │      │                                        │   │
│  └──────┴──────┴────────────────────────────────────────┘   │
│                                                               │
│  Flags (important bits):                                     │
│  - Present (P): Is page in physical RAM?                    │
│    - 1 = Page is in RAM (can access)                        │
│    - 0 = Page fault! (not in RAM, load from disk/swap)      │
│                                                               │
│  - Read/Write (RW): Can write to page?                      │
│  - User/Supervisor (US): Can user code access?              │
│  - Dirty (D): Has page been modified?                       │
│  - Accessed (A): Has page been accessed recently?           │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Page Structure

**What is a Page?**
- Fixed-size block of memory (typically 4KB on x86-64, 16KB on some ARM)
- Unit of virtual memory management
- Pages are what get loaded from disk, swapped, and mapped

```
┌─────────────────────────────────────────────────────────────┐
│                       PAGE (4KB)                             │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  Offset 0     │  1 byte                             │   │
│  │  Offset 1     │  1 byte                             │   │
│  │  Offset 2     │  1 byte                             │   │
│  │  ...          │  ...                                │   │
│  │  Offset 4095  │  1 byte (last byte in page)         │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Total: 4096 bytes (4KB)                                     │
│                                                               │
│  When you access Virtual Address 0x00007FFF12345678:         │
│  1. CPU extracts page number: 0x00007FFF12345                │
│  2. MMU looks up page in page table                         │
│  3. If Present=1: Physical page address found               │
│  4. MMU combines: Physical Page + Offset = Physical Address │
│  5. Access proceeds to physical RAM                         │
│                                                               │
│  If Present=0: PAGE FAULT! (must load from disk)            │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Address Translation Process

```
┌─────────────────────────────────────────────────────────────┐
│              ADDRESS TRANSLATION (Virtual → Physical)        │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Step 1: CPU generates Virtual Address                      │
│          Example: 0x00007FFF12345678                        │
│                                                               │
│  Step 2: MMU splits address:                                │
│          ┌────────────────┬──────────────────┐              │
│          │  Page Number   │  Page Offset     │              │
│          │  0x00007FFF12345│  678 (0x2A6)    │              │
│          └────────────────┴──────────────────┘              │
│                                                               │
│  Step 3: MMU looks up Page Table Entry (PTE)                │
│          ┌────────────────────────────────────┐             │
│          │  Virtual Page Number → PTE         │             │
│          │  Checks "Present" bit              │             │
│          └────────────────────────────────────┘             │
│                                                               │
│  Step 4A: If Present = 1 (Page in RAM):                     │
│           ┌────────────────────────────────────┐            │
│           │  Physical Page Address found       │            │
│           │  Example: 0x000000012345000        │            │
│           └────────────────────────────────────┘            │
│           Combine: Physical Page + Offset                   │
│           Result: 0x0000000123452A6 (Physical Address)      │
│           → Access RAM at this address                      │
│                                                               │
│  Step 4B: If Present = 0 (Page NOT in RAM):                 │
│           ┌────────────────────────────────────┐            │
│           │  PAGE FAULT INTERRUPT!             │            │
│           │  CPU switches to kernel mode       │            │
│           │  OS page fault handler runs        │            │
│           │  - Load page from disk/swap        │            │
│           │  - Allocate physical RAM page      │            │
│           │  - Update page table (Present=1)   │            │
│           │  - Retry memory access             │            │
│           └────────────────────────────────────┘            │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Physical Memory (RAM)

### RAM Structure and Organization

```
┌─────────────────────────────────────────────────────────────┐
│                  PHYSICAL MEMORY (RAM)                       │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Physical Address: 0x0000000000000000                        │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  OS Kernel Code & Data                               │   │
│  │  - Kernel buffers                                    │   │
│  │  - Page tables                                       │   │
│  │  - Device memory                                     │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Physical Address: (varies by OS)                            │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  User Process Memory                                 │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  Process A: Code, Heap, Stack               │    │   │
│  │  │  Process B: Code, Heap, Stack               │    │   │
│  │  │  Process C: Code, Heap, Stack               │    │   │
│  │  │  ...                                        │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  OS File Cache / Buffer Cache                        │   │
│  │  - Cached file pages                                 │   │
│  │  - Network buffers                                   │   │
│  │  - Shared memory regions                             │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Physical Address: (top of RAM)                              │
│                                                               │
│  Key Points:                                                 │
│  - Physical RAM is limited (e.g., 8GB, 16GB, 32GB)         │
│  - Multiple processes share physical RAM                    │
│  - OS manages allocation via page tables                    │
│  - Physical addresses are hardware addresses                │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Storage (Disk/SSD)

### Storage Hierarchy

```
┌─────────────────────────────────────────────────────────────┐
│                      STORAGE LAYER                           │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  SWAP / PAGE FILE                                     │   │
│  │  - Virtual memory extension                           │   │
│  │  - Swapped-out RAM pages                              │   │
│  │  - Accessed via page faults                           │   │
│  │  - Latency: 5-10ms (disk) / 100-500μs (SSD)          │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  FILES (Storage)                                      │   │
│  │  - Application files                                  │   │
│  │  - Database files                                     │   │
│  │  - Log files                                          │   │
│  │  - Can be memory-mapped                               │   │
│  │  - Accessed via file I/O or page faults (if mapped)   │   │
│  │  - Latency: 5-10ms (disk) / 100-500μs (SSD)          │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Note: Storage is accessed when:                             │
│  - Page fault occurs (page not in RAM)                      │
│  - Explicit file I/O (read/write)                           │
│  - Memory-mapped file access (page fault loads file page)   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Kernel vs User Space

### Kernel vs User Space Separation

```
┌─────────────────────────────────────────────────────────────┐
│              KERNEL SPACE vs USER SPACE                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  USER SPACE (Ring 3 - Lower Privilege)               │   │
│  │                                                       │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  Your Application Code (C#, Java, etc.)      │    │   │
│  │  │  - Uses Virtual Addresses                    │    │   │
│  │  │  - Cannot directly access physical memory    │    │   │
│  │  │  - Cannot access kernel space                │    │   │
│  │  │  - System calls switch to kernel mode        │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  │                                                       │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  USER BUFFERS                                │    │   │
│  │  │  - Allocated in user space                   │    │   │
│  │  │  - Example: byte[] buffer = new byte[1024]   │    │   │
│  │  │  - Accessed via virtual addresses            │    │   │
│  │  │  - Cannot directly access kernel buffers     │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                              │                                │
│                              │ System Call (e.g., read/write) │
│                              │ Mode Switch (User → Kernel)    │
│                              ▼                                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  KERNEL SPACE (Ring 0 - Higher Privilege)            │   │
│  │                                                       │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  OS Kernel Code                               │    │   │
│  │  │  - Page table management                      │    │   │
│  │  │  - Process scheduling                         │    │   │
│  │  │  - Device drivers                             │    │   │
│  │  │  - System call handlers                       │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  │                                                       │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  KERNEL BUFFERS                               │    │   │
│  │  │  - Socket buffers                             │    │   │
│  │  │  - File system buffers                        │    │   │
│  │  │  - Device buffers                             │    │   │
│  │  │  - Accessed via physical/virtual addresses    │    │   │
│  │  │  - Cannot be directly accessed by user code   │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Why Separation?                                             │
│  - Security: Prevents user code from crashing OS            │
│  - Isolation: User processes cannot access each other       │
│  - Stability: Kernel bugs affect entire system              │
│                                                               │
│  Data Transfer:                                               │
│  - User → Kernel: System calls (copy via CPU)               │
│  - Kernel → User: System calls (copy via CPU)               │
│  - Zero-copy: OS can map kernel buffers to user space       │
│    (eliminates copy, but complex)                           │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### User Buffer vs Kernel Buffer

```
┌─────────────────────────────────────────────────────────────┐
│           USER BUFFER vs KERNEL BUFFER                       │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Example: Reading from a file                                │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  USER SPACE                                          │   │
│  │                                                       │   │
│  │  byte[] userBuffer = new byte[1024];  ← Allocated   │   │
│  │  fileStream.Read(userBuffer, 0, 1024); ← System call│   │
│  │                                                       │   │
│  │  ┌────────────────────────────────────────────┐      │   │
│  │  │  USER BUFFER (userBuffer)                  │      │   │
│  │  │  Virtual Address: 0x00007FFF12340000       │      │   │
│  │  │  Physical Address: (mapped via page table) │      │   │
│  │  │  Accessible by: Your application only      │      │   │
│  │  └────────────────────────────────────────────┘      │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                              │                                │
│                              │ System Call                    │
│                              │ (copy data from kernel)        │
│                              ▼                                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  KERNEL SPACE                                        │   │
│  │                                                       │   │
│  │  ┌────────────────────────────────────────────┐      │   │
│  │  │  KERNEL BUFFER (OS-managed)                │      │   │
│  │  │  Virtual Address: (kernel space)           │      │   │
│  │  │  Physical Address: (direct mapping)        │      │   │
│  │  │  Contains: File data from disk             │      │   │
│  │  │  Accessible by: OS kernel only             │      │   │
│  │  └────────────────────────────────────────────┘      │   │
│  │                                                       │   │
│  │  What happens:                                       │   │
│  │  1. User code calls fileStream.Read()               │   │
│  │  2. System call switches to kernel mode             │   │
│  │  3. Kernel reads from disk into kernel buffer       │   │
│  │  4. Kernel copies data from kernel buffer to        │   │
│  │     user buffer (memcpy)                            │   │
│  │  5. Kernel returns to user mode                     │   │
│  │  6. User code accesses data from user buffer        │   │
│  │                                                       │   │
│  │  This copy (kernel → user) is expensive!            │   │
│  │  Zero-copy techniques eliminate this copy.           │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Data Transfer Flow

### Complete Memory Access Flow

```
┌─────────────────────────────────────────────────────────────┐
│          COMPLETE MEMORY ACCESS FLOW                         │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Scenario: Your code accesses a variable                     │
│            int value = array[1000];                          │
│                                                               │
│  Step 1: CPU generates Virtual Address                       │
│          Virtual Address: 0x00007FFF12345678                 │
│          (Address of array[1000])                            │
│                                                               │
│  Step 2: Check L1 Data Cache                                 │
│          ┌──────────────────────────────────────┐           │
│          │  Is address in L1d cache?            │           │
│          │  → Cache Hit?                        │           │
│          │    YES: Return data (1-3 cycles)     │           │
│          │    NO: Go to Step 3                  │           │
│          └──────────────────────────────────────┘           │
│                              │                                │
│                              │ Cache Miss                     │
│                              ▼                                │
│  Step 3: Check L2 Cache                                      │
│          ┌──────────────────────────────────────┐           │
│          │  Is address in L2 cache?             │           │
│          │  → Cache Hit?                        │           │
│          │    YES: Load into L1d (10-20 cycles) │           │
│          │    NO: Go to Step 4                  │           │
│          └──────────────────────────────────────┘           │
│                              │                                │
│                              │ Cache Miss                     │
│                              ▼                                │
│  Step 4: Check L3 Cache                                      │
│          ┌──────────────────────────────────────┐           │
│          │  Is address in L3 cache?             │           │
│          │  → Cache Hit?                        │           │
│          │    YES: Load into L2, then L1        │           │
│          │         (40-75 cycles)               │           │
│          │    NO: Go to Step 5                  │           │
│          └──────────────────────────────────────┘           │
│                              │                                │
│                              │ Cache Miss                     │
│                              ▼                                │
│  Step 5: MMU Translates Virtual → Physical                   │
│          ┌──────────────────────────────────────┐           │
│          │  Extract Page Number: 0x00007FFF12345│           │
│          │  Look up Page Table Entry (PTE)      │           │
│          │  Check "Present" bit                 │           │
│          └──────────────────────────────────────┘           │
│                              │                                │
│                    ┌─────────┴─────────┐                     │
│                    │                   │                     │
│            Present=1              Present=0                  │
│            (in RAM)              (NOT in RAM)                │
│                    │                   │                     │
│                    │                   │                     │
│                    ▼                   ▼                     │
│  Step 6A: Access RAM              Step 6B: PAGE FAULT        │
│          ┌──────────────────┐        ┌──────────────────┐   │
│          │  Physical Address│        │  Page Fault       │   │
│          │  found           │        │  Interrupt!       │   │
│          │                  │        │                   │   │
│          │  Load Cache Line │        │  1. Switch to     │   │
│          │  (64 bytes) from │        │     kernel mode   │   │
│          │  RAM             │        │  2. OS handler    │   │
│          │                  │        │     runs          │   │
│          │  → L3 → L2 → L1d│        │  3. Load page     │   │
│          │                  │        │     from disk     │   │
│          │  Latency:        │        │  4. Allocate RAM  │   │
│          │  100-300 cycles  │        │  5. Update PTE    │   │
│          │                  │        │  6. Retry access  │   │
│          │  Return data     │        │                   │   │
│          └──────────────────┘        │  Latency:          │   │
│                                      │  10,000-100,000+   │   │
│                                      │  cycles + disk I/O │   │
│                                      └──────────────────┘   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### File I/O Data Flow (Traditional)

```
┌─────────────────────────────────────────────────────────────┐
│          TRADITIONAL FILE I/O DATA FLOW                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Scenario: Reading 1KB from a file                           │
│            byte[] buffer = new byte[1024];                   │
│            fileStream.Read(buffer, 0, 1024);                 │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  USER SPACE                                          │   │
│  │                                                       │   │
│  │  byte[] userBuffer = new byte[1024];                 │   │
│  │  fileStream.Read(userBuffer, 0, 1024);               │   │
│  │  ┌──────────────────────────────────────────┐        │   │
│  │  │  USER BUFFER (empty, waiting for data)   │        │   │
│  │  └──────────────────────────────────────────┘        │   │
│  └──────────────────────────────────────────────────────┘   │
│                              │                                │
│                              │ System Call (read)             │
│                              │ Mode Switch                    │
│                              ▼                                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  KERNEL SPACE                                        │   │
│  │                                                       │   │
│  │  1. Kernel checks OS File Cache                      │   │
│  │     ┌──────────────────────────────────────┐         │   │
│  │     │  Is file page in cache?              │         │   │
│  │     │  → Cache Hit?                        │         │   │
│  │     │    YES: Data already in RAM          │         │   │
│  │     │    NO: Go to Step 2                  │         │   │
│  │     └──────────────────────────────────────┘         │   │
│  │                              │                        │   │
│  │                              │ Cache Miss             │   │
│  │                              ▼                        │   │
│  │  2. Kernel reads from disk                           │   │
│  │     ┌──────────────────────────────────────┐         │   │
│  │     │  DISK I/O                            │         │   │
│  │     │  - Seek to file offset (if needed)   │         │   │
│  │     │  - Read data from disk/SSD           │         │   │
│  │     │  - Latency: 5-10ms (disk)            │         │   │
│  │     │           100-500μs (SSD)            │         │   │
│  │     └──────────────────────────────────────┘         │   │
│  │                              │                        │   │
│  │                              ▼                        │   │
│  │  3. Kernel stores in File Cache                      │   │
│  │     ┌──────────────────────────────────────┐         │   │
│  │     │  KERNEL BUFFER (OS File Cache)       │         │   │
│  │     │  - Stores file page (4KB)            │         │   │
│  │     │  - Available for future reads        │         │   │
│  │     │  - Shared by all processes           │         │   │
│  │     └──────────────────────────────────────┘         │   │
│  │                              │                        │   │
│  │                              ▼                        │   │
│  │  4. Kernel copies to User Buffer (memcpy)            │   │
│  │     ┌──────────────────────────────────────┐         │   │
│  │     │  CPU copies 1KB:                     │         │   │
│  │     │  Kernel Buffer → User Buffer         │         │   │
│  │     │  - memcpy operation                  │         │   │
│  │     │  - CPU cycles: ~1000-10000           │         │   │
│  │     │  - Cache pollution                   │         │   │
│  │     └──────────────────────────────────────┘         │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                              │                                │
│                              │ Return from System Call        │
│                              │ Mode Switch                    │
│                              ▼                                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  USER SPACE                                          │   │
│  │                                                       │   │
│  │  ┌──────────────────────────────────────────┐        │   │
│  │  │  USER BUFFER (now contains file data)    │        │   │
│  │  │  - Data accessible by your code          │        │   │
│  │  │  - In your process's virtual address     │        │   │
│  │  │    space                                 │        │   │
│  │  └──────────────────────────────────────────┘        │   │
│  │                                                       │   │
│  │  Note: This involves TWO copies:                     │   │
│  │  1. Disk → Kernel Buffer (OS File Cache)             │   │
│  │  2. Kernel Buffer → User Buffer (memcpy)             │   │
│  │                                                       │   │
│  │  Zero-copy techniques eliminate copy #2.             │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Memory-Mapped File I/O Data Flow

```
┌─────────────────────────────────────────────────────────────┐
│          MEMORY-MAPPED FILE I/O DATA FLOW                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Scenario: Accessing a memory-mapped file                    │
│            var value = accessor.ReadInt32(offset);           │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  USER SPACE                                          │   │
│  │                                                       │   │
│  │  // File is mapped into virtual address space        │   │
│  │  using (var mmf = MemoryMappedFile.CreateFromFile()) │   │
│  │  using (var accessor = mmf.CreateViewAccessor())     │   │
│  │  {                                                    │   │
│  │      // Virtual address mapped to file offset        │   │
│  │      var value = accessor.ReadInt32(0x1000);         │   │
│  │  }                                                    │   │
│  │                                                       │   │
│  │  Virtual Address: 0x00007FFF12345000                 │   │
│  │  (Mapped to file offset 0x1000)                      │   │
│  └──────────────────────────────────────────────────────┘   │
│                              │                                │
│                              │ Memory Access                  │
│                              ▼                                │
│  Step 1: CPU checks Caches (L1 → L2 → L3)                   │
│          → Cache Miss (first access)                         │
│                                                               │
│  Step 2: MMU Translates Virtual Address                      │
│          ┌──────────────────────────────────────┐           │
│          │  Virtual Address: 0x00007FFF12345000 │           │
│          │  Look up Page Table Entry            │           │
│          │  PTE indicates: "Mapped to file"     │           │
│          │  Present bit: 0 (not in RAM yet)     │           │
│          └──────────────────────────────────────┘           │
│                              │                                │
│                              │ PAGE FAULT                     │
│                              ▼                                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  KERNEL SPACE (Page Fault Handler)                   │   │
│  │                                                       │   │
│  │  1. OS identifies page is from memory-mapped file    │   │
│  │  2. OS calculates file offset for this virtual addr  │   │
│  │     File offset = Virtual Address - Mapping Start    │   │
│  │  3. OS reads 4KB page from file                      │   │
│  │     ┌──────────────────────────────────────┐         │   │
│  │     │  DISK I/O                            │         │   │
│  │     │  - Read file page (4KB)              │         │   │
│  │     │  - Latency: 5-10ms (disk)            │         │   │
│  │     │           100-500μs (SSD)            │         │   │
│  │     └──────────────────────────────────────┘         │   │
│  │                              │                        │   │
│  │                              ▼                        │   │
│  │  4. OS allocates physical RAM page                   │   │
│  │  5. OS loads file data into RAM page                 │   │
│  │  6. OS updates Page Table Entry:                     │   │
│  │     - Present = 1 (page now in RAM)                  │   │
│  │     - Physical Address = RAM page address            │   │
│  │  7. OS returns from page fault handler               │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                              │                                │
│                              │ Retry Memory Access            │
│                              ▼                                │
│  Step 3: CPU Retries Memory Access                           │
│          ┌──────────────────────────────────────┐           │
│          │  MMU translates virtual address      │           │
│          │  PTE now has Present=1               │           │
│          │  Physical Address found              │           │
│          │  Access proceeds to RAM              │           │
│          └──────────────────────────────────────┘           │
│                              │                                │
│                              ▼                                │
│  Step 4: Load from RAM → L3 → L2 → L1 → Return Data         │
│          ┌──────────────────────────────────────┐           │
│          │  Data now in CPU cache               │           │
│          │  Subsequent accesses are fast        │           │
│          │  (no page faults, cache hits)        │           │
│          └──────────────────────────────────────┘           │
│                                                               │
│  Key Benefits:                                               │
│  - No copy from kernel to user (direct access)              │
│  - OS automatically caches pages                            │
│  - Lazy loading (only accessed pages loaded)                │
│  - Shared memory (multiple processes can share pages)       │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Locality Principles

### Cache Locality

```
┌─────────────────────────────────────────────────────────────┐
│                    CACHE LOCALITY                            │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Cache Locality = How well your access patterns utilize     │
│                   CPU cache                                  │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  GOOD CACHE LOCALITY                                 │   │
│  │                                                       │   │
│  │  int sum = 0;                                        │   │
│  │  for (int i = 0; i < array.Length; i++)             │   │
│  │  {                                                    │   │
│  │      sum += array[i];  // Sequential access          │   │
│  │  }                                                    │   │
│  │                                                       │   │
│  │  What happens:                                       │   │
│  │  - Access array[0]: Load cache line (64 bytes)       │   │
│  │    Contains: array[0-15] (assuming int=4 bytes)      │   │
│  │  - Access array[1]: Cache HIT! (already loaded)      │   │
│  │  - Access array[2]: Cache HIT!                       │   │
│  │  - ...                                               │   │
│  │  - Access array[16]: Load new cache line             │   │
│  │                                                       │   │
│  │  Cache Hit Rate: ~90-95%                             │   │
│  │  Performance: Excellent                               │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  POOR CACHE LOCALITY                                 │   │
│  │                                                       │   │
│  │  int sum = 0;                                        │   │
│  │  foreach (var node in linkedList)                    │   │
│  │  {                                                    │   │
│  │      sum += node.Value;  // Random access            │   │
│  │  }                                                    │   │
│  │                                                       │   │
│  │  What happens:                                       │   │
│  │  - Node 1: Load cache line (node 1 data)             │   │
│  │  - Follow pointer to Node 2: Different memory        │   │
│  │  - Node 2: Cache MISS! (new location)                │   │
│  │  - Load cache line (node 2 data)                     │   │
│  │  - Follow pointer to Node 3: Different memory        │   │
│  │  - Node 3: Cache MISS!                               │   │
│  │  - ...                                               │   │
│  │                                                       │   │
│  │  Cache Hit Rate: ~30-50%                             │   │
│  │  Performance: Poor                                    │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  How to Improve Cache Locality:                              │
│  - Use arrays instead of linked lists (when possible)       │
│  - Access data sequentially                                 │
│  - Keep related data together (Array of Structs for         │
│    multiple fields, Struct of Arrays for single field)      │
│  - Organize data in cache-friendly layouts                  │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Spatial Locality

```
┌─────────────────────────────────────────────────────────────┐
│                   SPATIAL LOCALITY                           │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Spatial Locality = If you access a memory location,        │
│                     you'll likely access nearby locations    │
│                     soon                                      │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  MEMORY LAYOUT                                       │   │
│  │                                                       │   │
│  │  Address: 0x1000  0x1004  0x1008  0x100C  ...       │   │
│  │  Data:    [ A ]    [ B ]    [ C ]    [ D ]  ...     │   │
│  │           └────┬─────┴────┬─────┴────┬─────┘        │   │
│  │                │           │           │              │   │
│  │            Cache Line 1 (64 bytes)                   │   │
│  │                                                       │   │
│  │  When you access address 0x1004 (B):                 │   │
│  │  1. CPU loads entire cache line                      │   │
│  │  2. Cache line contains addresses 0x1000-0x103F      │   │
│  │  3. Now A, B, C, D are all in cache!                 │   │
│  │  4. Accessing 0x1000, 0x1008, 0x100C is FAST         │   │
│  │     (cache hits)                                      │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  GOOD SPATIAL LOCALITY                               │   │
│  │                                                       │   │
│  │  // Array of Structs - accessing multiple fields     │   │
│  │  struct Point { int x; int y; }                      │   │
│  │  Point[] points = new Point[1000];                   │   │
│  │                                                       │   │
│  │  foreach (var point in points)                       │   │
│  │  {                                                    │   │
│  │      // x and y are next to each other in memory     │   │
│  │      sum += point.x + point.y;  // Both in same      │   │
│  │                                 // cache line!        │   │
│  │  }                                                    │   │
│  │                                                       │   │
│  │  Benefit: Accessing x loads y into cache too         │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  POOR SPATIAL LOCALITY                               │   │
│  │                                                       │   │
│  │  // Struct of Arrays - accessing scattered data      │   │
│  │  int[] x = new int[1000];                            │   │
│  │  int[] y = new int[1000];  // Stored separately!     │   │
│  │                                                       │   │
│  │  for (int i = 0; i < 1000; i++)                      │   │
│  │  {                                                    │   │
│  │      // x[i] and y[i] are in different arrays        │   │
│  │      sum += x[i] + y[i];  // Two separate cache      │   │
│  │                          // loads!                    │   │
│  │  }                                                    │   │
│  │                                                       │   │
│  │  Problem: Accessing x[i] doesn't help y[i]           │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Note: Spatial locality is why cache lines are 64 bytes      │
│        (not 1 byte). If you access one byte, nearby bytes    │
│        are likely accessed soon.                              │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Temporal Locality

```
┌─────────────────────────────────────────────────────────────┐
│                   TEMPORAL LOCALITY                          │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Temporal Locality = If you access a memory location,       │
│                      you'll likely access it again soon      │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  EXAMPLE: Loop Variable                              │   │
│  │                                                       │   │
│  │  int sum = 0;  // Accessed frequently                │   │
│  │  for (int i = 0; i < array.Length; i++)             │   │
│  │  {                                                    │   │
│  │      sum += array[i];  // sum accessed every         │   │
│  │                        // iteration (temporal!)       │   │
│  │  }                                                    │   │
│  │                                                       │   │
│  │  Benefit: sum stays in register/cache                │   │
│  │           (accessed repeatedly)                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  CPU Caches Exploit Temporal Locality:                       │
│  - Recently accessed data stays in cache                     │
│  - LRU (Least Recently Used) eviction policy                 │
│  - Frequently accessed data remains cached                   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Complete Data Flow Examples

### Example 1: Reading from Memory-Mapped File

```
┌─────────────────────────────────────────────────────────────┐
│  EXAMPLE: First Access to Memory-Mapped File                │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Code: var value = accessor.ReadInt32(0x1000);              │
│                                                               │
│  1. CPU generates Virtual Address: 0x00007FFF12345000       │
│                                                               │
│  2. Check L1 Cache: MISS (first access)                     │
│     Check L2 Cache: MISS                                    │
│     Check L3 Cache: MISS                                    │
│                                                               │
│  3. MMU translates Virtual Address:                          │
│     - Page Number: 0x00007FFF12345                           │
│     - Look up Page Table Entry                              │
│     - Present = 0 (page not in RAM yet)                     │
│                                                               │
│  4. PAGE FAULT!                                              │
│     - CPU interrupts, switches to kernel mode               │
│     - OS page fault handler runs                            │
│                                                               │
│  5. OS identifies page is from memory-mapped file:           │
│     - File offset = Virtual Address - Mapping Start         │
│     - File offset = 0x1000 (what you requested)             │
│                                                               │
│  6. OS reads 4KB page from disk:                             │
│     - Disk I/O: 5-10ms (disk) / 100-500μs (SSD)            │
│     - Reads file page containing offset 0x1000              │
│                                                               │
│  7. OS allocates physical RAM page                           │
│                                                               │
│  8. OS loads file data into RAM page                         │
│                                                               │
│  9. OS updates Page Table Entry:                             │
│     - Present = 1                                            │
│     - Physical Address = RAM page address                   │
│                                                               │
│  10. OS returns from page fault handler                      │
│                                                               │
│  11. CPU retries memory access:                              │
│      - MMU translates: Present = 1, Physical Address found  │
│      - Access proceeds to RAM                                │
│                                                               │
│  12. Load 64-byte cache line from RAM:                       │
│      - RAM → L3 Cache → L2 Cache → L1 Data Cache            │
│      - Latency: ~100-300 cycles                              │
│                                                               │
│  13. Return data to CPU register:                            │
│      - Extract int32 at offset 0x1000                        │
│      - Value returned                                        │
│                                                               │
│  Total Latency: 10,000-100,000+ cycles + disk I/O           │
│                 (mostly disk I/O: 5-10ms)                    │
│                                                               │
│  Subsequent Access to Same Page:                             │
│  - Cache HIT in L1/L2/L3                                    │
│  - Latency: ~1-75 cycles (fast!)                            │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Example 2: Traditional File Read with Buffer Copy

```
┌─────────────────────────────────────────────────────────────┐
│  EXAMPLE: Traditional File Read (fileStream.Read)           │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Code: byte[] buffer = new byte[1024];                       │
│        fileStream.Read(buffer, 0, 1024);                     │
│                                                               │
│  1. Allocate User Buffer:                                    │
│     - Virtual Address: 0x00007FFF12340000                    │
│     - MMU translates to Physical Address                     │
│     - Physical RAM allocated (if not already allocated)      │
│                                                               │
│  2. System Call (read):                                      │
│     - CPU switches to kernel mode                            │
│     - Kernel receives read request                           │
│                                                               │
│  3. Kernel checks OS File Cache:                             │
│     - Is file page in cache?                                 │
│     - Cache HIT: Data in RAM (fast path)                     │
│     - Cache MISS: Go to Step 4                               │
│                                                               │
│  4. Kernel reads from disk:                                  │
│     - Disk I/O: 5-10ms (disk) / 100-500μs (SSD)            │
│     - Reads file data into Kernel Buffer (OS File Cache)     │
│                                                               │
│  5. Kernel stores in OS File Cache:                          │
│     - Kernel Buffer (physical RAM)                           │
│     - Available for future reads                             │
│                                                               │
│  6. Kernel copies data to User Buffer:                       │
│     - memcpy: Kernel Buffer → User Buffer                    │
│     - CPU cycles: ~1000-10000                                │
│     - Cache pollution (two buffers in cache)                 │
│                                                               │
│  7. Return from System Call:                                 │
│     - CPU switches back to user mode                         │
│     - User code accesses data from User Buffer               │
│                                                               │
│  Total Latency: Disk I/O + memcpy                            │
│                 (5-10ms disk + ~1-10μs copy)                 │
│                                                               │
│  Note: This involves TWO copies:                             │
│  1. Disk → Kernel Buffer (OS File Cache)                     │
│  2. Kernel Buffer → User Buffer (memcpy)                     │
│                                                               │
│  Memory-Mapped I/O eliminates copy #2 (zero-copy benefit)    │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Additional Concepts You May Have Missed

### Translation Lookaside Buffer (TLB)

```
┌─────────────────────────────────────────────────────────────┐
│              TRANSLATION LOOKASIDE BUFFER (TLB)              │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Problem: Page table lookups are slow (multiple memory       │
│           accesses to walk page table)                       │
│                                                               │
│  Solution: TLB caches recent Virtual → Physical translations│
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  TLB (Per CPU Core)                                  │   │
│  │                                                       │   │
│  │  Virtual Page Number → Physical Page Number         │   │
│  │  (Cache of Page Table Entries)                       │   │
│  │                                                       │   │
│  │  Size: ~64-1536 entries                              │   │
│  │  Latency: ~1-3 cycles (very fast!)                   │   │
│  │                                                       │   │
│  │  When MMU translates Virtual Address:                │   │
│  │  1. Check TLB first (fast cache lookup)              │   │
│  │  2. TLB HIT: Use cached translation (1-3 cycles)     │   │
│  │  3. TLB MISS: Walk page table (100-300 cycles)       │   │
│  │     Then update TLB with new translation             │   │
│  │                                                       │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  TLB Miss Penalty: 100-300 cycles                            │
│  TLB Hit: 1-3 cycles                                          │
│                                                               │
│  This is why memory-mapped files benefit from good           │
│  access patterns - TLB entries are reused!                   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Memory Bus and Load/Store Units

```
┌─────────────────────────────────────────────────────────────┐
│              MEMORY BUS & LOAD/STORE UNITS                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  CPU CORE                                            │   │
│  │                                                       │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  Load/Store Units                            │    │   │
│  │  │  - Execute load/store instructions           │    │   │
│  │  │  - Generate memory requests                  │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  │           │                                           │   │
│  │           │ Memory Request                            │   │
│  │           ▼                                           │   │
│  │  ┌──────────────────────────────────────────────┐    │   │
│  │  │  Memory Controller                           │    │   │
│  │  │  - Manages memory requests                   │    │   │
│  │  │  - Handles cache coherency (MESI)           │    │   │
│  │  │  - Interfaces with memory bus                │    │   │
│  │  └──────────────────────────────────────────────┘    │   │
│  │           │                                           │   │
│  └───────────┼───────────────────────────────────────────┘   │
│              │                                                │
│              │ Memory Bus (64-bit, high-speed)               │
│              │ - Transfers cache lines (64 bytes)            │
│              │ - Bandwidth: ~25-50 GB/s per channel          │
│              │                                                │
│              ▼                                                │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  PHYSICAL RAM                                        │   │
│  │  - Organized in channels (DDR4/DDR5)                │   │
│  │  - Cache line aligned                                │   │
│  │  - Latency: 100-300 cycles                           │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  Key Point: Memory transfers are in cache line units         │
│             (64 bytes), not individual bytes!                │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Write-Back vs Write-Through Caches

```
┌─────────────────────────────────────────────────────────────┐
│         WRITE-BACK vs WRITE-THROUGH CACHES                   │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  WRITE-THROUGH                                       │   │
│  │                                                       │   │
│  │  CPU writes to cache                                 │   │
│  │       │                                               │   │
│  │       ├─→ Write to RAM (immediately)                 │   │
│  │       │                                               │   │
│  │       └─→ Update cache                               │   │
│  │                                                       │   │
│  │  Pros: Simple, RAM always consistent                 │   │
│  │  Cons: Slower (every write hits RAM)                 │   │
│  │                                                       │   │
│  │  Used for: L1 cache (sometimes)                      │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  WRITE-BACK (Modern CPUs)                            │   │
│  │                                                       │   │
│  │  CPU writes to cache                                 │   │
│  │       │                                               │   │
│  │       └─→ Mark cache line as "Dirty"                 │   │
│  │           (modified, not yet written to RAM)         │   │
│  │                                                       │   │
│  │  When cache line is evicted:                         │   │
│  │       │                                               │   │
│  │       └─→ Write to RAM (lazy write-back)             │   │
│  │                                                       │   │
│  │  Pros: Faster (writes don't wait for RAM)            │   │
│  │  Cons: RAM may be stale (but that's OK)              │   │
│  │                                                       │   │
│  │  Used for: L2, L3 caches                             │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  This is why memory-mapped file writes may be buffered       │
│  until the cache line is evicted or explicitly flushed.      │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

### Direct Memory Access (DMA)

```
┌─────────────────────────────────────────────────────────────┐
│              DIRECT MEMORY ACCESS (DMA)                      │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  Problem: CPU copying data from disk to RAM is slow          │
│           (CPU cycles wasted on simple copying)              │
│                                                               │
│  Solution: DMA engines perform transfers without CPU         │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  TRADITIONAL (CPU COPY)                              │   │
│  │                                                       │   │
│  │  Disk → CPU Register → RAM                           │   │
│  │         (CPU involved in every byte)                 │   │
│  │         (Wastes CPU cycles)                          │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
│  ┌──────────────────────────────────────────────────────┐   │
│  │  DMA (DIRECT MEMORY ACCESS)                          │   │
│  │                                                       │   │
│  │  Disk → DMA Engine → RAM                             │   │
│  │         (CPU sets up transfer, then continues        │   │
│  │          other work)                                  │   │
│  │         (DMA engine handles transfer)                │   │
│  │                                                       │   │
│  │  Benefits:                                            │   │
│  │  - CPU free to do other work                         │   │
│  │  - Faster transfers (dedicated hardware)             │   │
│  │  - Used for disk I/O, network I/O                    │   │
│  │                                                       │   │
│  │  Note: DMA transfers still load into OS File Cache   │   │
│  │        (physical RAM), then may be copied to user    │   │
│  │        buffer (traditional I/O) or accessed directly  │   │
│  │        (memory-mapped I/O)                            │   │
│  └──────────────────────────────────────────────────────┘   │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Summary: How Everything Relates

```
┌─────────────────────────────────────────────────────────────┐
│              COMPLETE MEMORY ARCHITECTURE                    │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  1. Your Code uses Virtual Addresses                         │
│     ↓                                                         │
│  2. CPU Caches (L1/L2/L3) filter memory access              │
│     ↓ (cache miss)                                           │
│  3. MMU translates Virtual → Physical using Page Table      │
│     ↓                                                         │
│  4. Page Table checked (via TLB cache for speed)            │
│     ↓                                                         │
│  5. If Present=1: Access Physical RAM                       │
│     If Present=0: Page Fault → Load from Storage            │
│     ↓                                                         │
│  6. Physical RAM organized in Pages (4KB)                   │
│     ↓                                                         │
│  7. RAM accessed via Memory Bus in Cache Lines (64 bytes)   │
│     ↓                                                         │
│  8. Data flows back: RAM → L3 → L2 → L1 → CPU Register     │
│                                                               │
│  Key Relationships:                                          │
│  - Virtual Address Space provides isolation & abstraction   │
│  - Page Table maps Virtual → Physical                       │
│  - Pages are unit of memory management (loading, swapping)  │
│  - Cache Lines are unit of cache transfer (64 bytes)        │
│  - CPU Caches filter memory access (reduce RAM latency)     │
│  - Kernel vs User Space provides security & isolation        │
│  - Spatial Locality enables efficient caching               │
│  - Cache Locality measures how well you use cache           │
│                                                               │
│  Memory-Mapped Files:                                        │
│  - Map file pages to Virtual Address Space                  │
│  - Page Faults load file pages from disk                    │
│  - OS caches pages in physical RAM                          │
│  - Access like memory, OS handles I/O                       │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

## Quick Reference: Latencies

```
┌─────────────────────────────────────────────────────────────┐
│              MEMORY ACCESS LATENCIES                         │
├─────────────────────────────────────────────────────────────┤
│                                                               │
│  L1 Cache Hit:        ~1-3 cycles      (~1-3 ns @ 1GHz)    │
│  L2 Cache Hit:        ~10-20 cycles    (~10-20 ns)          │
│  L3 Cache Hit:        ~40-75 cycles    (~40-75 ns)          │
│  RAM Access:          ~100-300 cycles  (~100-300 ns)        │
│  SSD Access:          ~100-500μs       (100,000-500,000 ns) │
│  Disk Access:         ~5-10ms          (5,000,000-10,000,000│
│                                         ns)                  │
│                                                               │
│  Page Fault (Minor):  ~1,000-10,000 cycles                  │
│  Page Fault (Major):  ~10,000-100,000+ cycles + disk I/O    │
│                                                               │
│  TLB Hit:             ~1-3 cycles                            │
│  TLB Miss:            ~100-300 cycles (page table walk)     │
│                                                               │
│  System Call:         ~1,000-10,000 cycles (mode switch)    │
│  Context Switch:      ~1,000-10,000 cycles                  │
│                                                               │
│  Key Insight: Cache misses and page faults are the          │
│               primary performance bottlenecks!               │
│                                                               │
└─────────────────────────────────────────────────────────────┘
```

---

**End of Memory Architecture Schema**

This document provides a comprehensive understanding of how memory components relate, from your application code down to physical storage, and how data flows between different layers of the memory hierarchy.
