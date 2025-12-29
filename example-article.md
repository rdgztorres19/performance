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

## When to Use CPU Pinning

- Real-time or near-real-time systems
- Latency-critical services
- Multi-socket (NUMA) servers
- Cache-sensitive, predictable workloads
- Performance isolation requirements

---

## When Not to Use It

- General-purpose web applications
- Highly dynamic or bursty workloads
- Cloud environments with abstracted CPUs
- Batch processing jobs
- Development and testing environments

---

## Common Mistakes

- Pinning without profiling
- Pinning everything instead of critical parts
- Ignoring NUMA topology
- Confusing hyperthreads with physical cores
- Over-pinning and starving system processes

---

## How to Measure and Validate

Before pinning:
- Measure latency percentiles
- Measure throughput
- Observe CPU migrations

After pinning:
- Verify migrations are reduced
- Check for load imbalance
- Validate latency improvement
- Monitor over time

If you can’t measure the improvement, the pinning is not justified.

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
