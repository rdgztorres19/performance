# Pin Processes and Threads to Specific CPU Cores for Predictable Performance

**Control CPU affinity to improve execution predictability and reduce unnecessary movement between CPU cores, at the cost of scheduler flexibility.**

---

## Executive Summary (TL;DR)

CPU affinity (also called *CPU pinning*) means telling the operating system that a process or thread should run only on specific CPU cores. By default, the operating system is free to move threads between cores to balance load. While this is usually beneficial, it can hurt performance for latency-sensitive or cache-sensitive workloads.

Pinning execution to specific cores reduces unnecessary movement, improves execution continuity, and makes performance more predictable. Typical improvements range from **5–20%** for cache-sensitive workloads and can be much higher on multi-socket (NUMA) systems. The trade-off is reduced scheduler flexibility and potential load imbalance.

Use CPU pinning when predictability matters more than maximum flexibility.

---

## A Simple Mental Model (Read This First)

Think of a CPU core as a **desk**, and a thread as a **worker**.

- When the worker stays at the same desk, their tools are already laid out.
- When the worker is moved to another desk, they must set everything up again.
- If the worker is moved frequently, more time is spent setting up than working.

CPU pinning simply tells the operating system:  
**“Stop moving this worker between desks.”**

You don’t need to know what the tools are internally.  
Less movement means less wasted time.

---

## Problem Context

Modern operating systems use schedulers designed to keep all CPU cores busy. To achieve this, they dynamically move processes and threads between cores to balance load, improve fairness, and optimize power usage.

For most applications, this behavior is ideal. However, for workloads that:
- require consistent timing
- repeatedly access the same data
- are sensitive to latency spikes

frequent core migration becomes expensive.

Each migration interrupts execution and forces internal state to be rebuilt. Over time, this creates instability and unpredictable performance.

---

## What Is CPU Affinity?

CPU affinity defines **which CPU cores a process or thread is allowed to run on**.

- **Without affinity**: the OS may run your code on any core
- **With affinity**: the OS is restricted to a specific set of cores

Once affinity is set, the scheduler respects that constraint and avoids migrating execution outside the allowed cores.

---

## Default Thread Affinity and Why Threads Move Between Cores

By default, operating systems do **not** strictly bind threads to specific CPU cores.

Instead, the scheduler uses a *soft affinity* strategy:
- It prefers to run a thread on the same core it ran on last
- But it is free to move the thread whenever it decides it is beneficial

This means:
- Threads can run on any core
- Core assignment is dynamic
- There is no guarantee that a thread will stay on the same core

### Why Threads Are Migrated

Threads move between cores for several reasons:

- **Load balancing**  
  To prevent some cores from being overloaded while others are idle

- **Fairness**  
  To ensure all runnable threads get CPU time

- **Power and thermal management**  
  To spread heat and optimize energy usage

- **Oversubscription**  
  When more threads are active than available cores

- **System activity**  
  Interrupt handling, kernel threads, and background services

From the scheduler’s perspective, migration is often the right choice.  
From the application’s perspective, migration can introduce overhead and unpredictability.

CPU pinning exists to override this behavior when predictability matters more than flexibility.

---

## Why Uncontrolled Core Migration Hurts Performance

### 1. Execution Is Repeatedly Interrupted

Each time a thread moves to another core, execution is paused and resumed elsewhere. Even though this is fast, doing it repeatedly adds noticeable overhead.

---

### 2. Internal Execution State Is Rebuilt

While running, the CPU prepares internal state to execute code efficiently. When execution moves to another core, that preparation must be rebuilt, slowing progress.

You don’t need hardware details to understand this:
**movement causes repetition**.

---

### 3. Latency Becomes Unpredictable

Some executions run uninterrupted, others are moved several times. This creates variability and hurts worst-case latency.

---

### 4. Problems Amplify Under Load

As load increases:
- more threads compete for cores
- more migration occurs
- delays compound

This creates a feedback loop that rapidly degrades performance.

---

## Why Pinning Improves Many Things at Once

CPU pinning improves performance not by optimizing one component, but by removing an entire class of inefficiency.

- Less movement → less repeated setup
- Longer uninterrupted execution → more useful work
- Faster task completion → less system pressure
- More stable timing → better tail latency

This is why pinning often improves latency, throughput, and predictability simultaneously.

---

## How CPU Pinning Works (High-Level)

### Hardware Perspective
- Each CPU core maintains its own execution context
- Staying on the same core preserves continuity
- Multi-socket (NUMA) systems amplify the cost of movement

### Operating System Perspective
- The OS maintains an affinity mask for each process or thread
- The scheduler respects this mask
- Context switches still happen, but core migration is reduced or eliminated

---

## Advantages

- More predictable performance
- Reduced execution interruption
- Improved behavior under load
- Strong benefits in NUMA systems
- Better isolation for critical workloads

---

## Disadvantages and Trade-offs

- Reduced scheduler flexibility
- Risk of load imbalance
- Requires hardware awareness
- Can waste resources if misused
- Not suitable for dynamic workloads

CPU pinning trades flexibility for predictability.

---

## Why Thread Affinity Improves Performance (Deeper Explanation)

Thread and CPU affinity improve performance primarily by **preserving execution locality**. When a thread remains on the same CPU core, the system avoids repeatedly rebuilding execution context that was already optimized.

This improvement comes from several reinforcing effects.

---

### 1. Preserving CPU Cache Locality

Each CPU core maintains private caches that store recently used data and instructions. When a thread runs continuously on the same core, the data it needs remains readily available.

When a thread migrates to another core:
- Its data is no longer in the local cache
- The new core must fetch data again
- Execution stalls while data is reloaded

This work was already done once and must now be repeated.

**Thread affinity keeps caches warm**, reducing cache misses and avoiding repeated data loading.

---

### 2. Fewer Cache Misses Means Fewer CPU Stalls

Cache misses force the CPU to wait for data. These waits add latency even if no context switch occurs.

Core migration dramatically increases cache misses:
- Local cache state is lost
- Execution restarts in a colder environment
- Progress slows

By keeping threads on the same core, affinity reduces cache miss frequency and improves sustained execution speed.

---

### 3. Longer Uninterrupted Execution Windows

Thread migration is often correlated with:
- Preemption
- Short execution slices
- Increased context switching

When a thread is pinned to a core:
- It is more likely to continue running
- It completes work faster
- It exits the run queue sooner

**Faster completion reduces contention**, which indirectly reduces both voluntary and involuntary context switches.

---

### 4. Reduced Context Switching Pressure

Although affinity does not eliminate context switching, it reduces **unnecessary switches caused by migration and rebalancing**.

With affinity:
- Scheduler decisions are simpler
- Threads are less likely to be displaced
- Execution becomes more stable

Less scheduling churn means more CPU time is spent executing useful work instead of coordinating execution.

---

### 5. Better Execution Predictability

Without affinity, execution timing varies depending on:
- Which core runs the thread
- Cache warmth
- Migration frequency
- System activity

Affinity reduces these variables.

This leads to:
- Lower latency jitter
- More consistent response times
- Improved worst-case latency

This predictability is often more valuable than raw throughput.

---

### 6. Amplified Benefits on NUMA Systems

On multi-socket systems, memory access latency depends on proximity to the CPU core.

Without affinity:
- Threads may run far from their memory
- Memory access becomes slower
- Execution time increases

With affinity:
- Threads stay close to their memory
- Remote memory access is reduced
- Latency becomes more stable

NUMA systems benefit disproportionately from proper pinning.

---

## When CPU / Thread Affinity Is Most Beneficial

CPU / thread affinity is useful when a thread performs similar work repeatedly and loses performance because it is moved between CPU cores.  
Moving a thread is not free: the CPU must “warm up” again before it can run efficiently.

Affinity helps by **letting a thread stay where it already works well**.

---

### 1. CPU-Bound Applications
**What this means**  
The application spends most of its time doing calculations, not waiting for disk or network.

**What goes wrong without affinity**  
The thread is constantly ready to run, but the scheduler moves it between cores to balance load.  
Each move forces the CPU to restart execution in a colder state.

**Why affinity helps**  
Keeping the thread on one core allows it to run longer without interruption and avoids repeated warm-up.

**Typical effect**
- More work done per second
- Less CPU time wasted restarting execution

---

### 2. Long-Lived Worker Threads
**What this means**  
The application uses a fixed set of worker threads that stay alive for a long time and process similar tasks.

**What goes wrong without affinity**  
Workers are moved between cores even though their work pattern does not change.  
Each move discards progress the CPU already made optimizing execution.

**Why affinity helps**  
A worker that stays on one core becomes more efficient over time and finishes tasks faster.

**Typical effect**
- Better steady-state performance
- More stable timing once the system warms up

---

### 3. Cache-Sensitive Workloads
**What this means**  
The application repeatedly accesses the same data in memory.

**What goes wrong without affinity**  
When a thread moves to another core, the data it was using is no longer nearby.  
The CPU must fetch the same data again, which takes time.

**Why affinity helps**  
Keeping the thread on the same core keeps its data close, so memory access stays fast.

**Typical effect**
- Lower memory latency
- Faster execution of repeated operations

---

### 4. Latency-Sensitive Applications
**What this means**  
Response time matters more than average throughput, and delays must be predictable.

**What goes wrong without affinity**  
Some requests run on warm cores and are fast, others resume on cold cores and are slow.  
This creates unpredictable latency spikes.

**Why affinity helps**  
Threads resume execution in a similar environment every time, reducing randomness.

**Typical effect**
- More consistent response times
- Better worst-case latency (P99 / P999)

---

### 5. Predictable, Steady Workloads
**What this means**  
The workload shape does not change much over time.

**What goes wrong without affinity**  
The scheduler keeps moving threads even though there is no real imbalance to fix.

**Why affinity helps**  
Fixed core assignment avoids unnecessary movement and keeps execution stable.

**Typical effect**
- Repeatable performance
- Easier tuning and capacity planning

---

### 6. Dedicated or Single-Tenant Systems
**What this means**  
One application owns the machine.

**What goes wrong without affinity**  
Threads are moved around for fairness that the application does not need.

**Why affinity helps**  
Restricting movement reduces interference and improves isolation.

**Typical effect**
- Better resource usage
- More predictable system behavior

---

### 7. NUMA Systems (Without NUMA-Aware Code)
**What this means**  
The machine has multiple CPU sockets, each with its own memory.

**What goes wrong without affinity**  
Threads move between sockets and access memory that is far away, which is slower.

**Why affinity helps**  
Keeping threads near their memory avoids expensive remote access.

**Typical effect**
- Lower memory access latency
- More consistent throughput

## Rule of Thumb

> Use CPU / thread affinity when threads do similar work for a long time and lose performance because they are moved between CPU cores.
---

## C# Examples

### Process Affinity (Most Common)

```csharp
using System;
using System.Diagnostics;

class Program
{
    static void Main()
    {
        var process = Process.GetCurrentProcess();

        // Pin process to CPU 0 and 1 (binary mask: 00000011)
        process.ProcessorAffinity = new IntPtr(0b00000011);

        Console.WriteLine("Process pinned to CPU 0 and 1");
    }
}
