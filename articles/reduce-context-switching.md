# Reduce Context Switching

## Subtitle
Context switching overhead becomes a measurable performance bottleneck in high-throughput systems when too many threads or processes compete for limited CPU resources.

---

## Executive Summary (TL;DR)
Context switching happens when the operating system stops one thread and starts another. Each switch requires coordination work and often forces the CPU to repeat work it was already doing efficiently. When context switching happens too often, the CPU spends more time coordinating execution than doing useful work. Reducing unnecessary context switches improves throughput, lowers latency (especially worst-case latency), and makes performance more predictable. This optimization matters most in CPU-bound workloads where thread count exceeds CPU core count.

---

## Problem Context
A common misconception is that increasing the number of threads automatically improves performance. This leads to designs where a new thread is created for every request or task. While this can work for I/O-bound workloads (where threads mostly wait), it becomes harmful for CPU-bound workloads.

A CPU has a fixed number of cores, and each core can only execute one thread at a time. When an application creates more threads than available cores, the operating system must constantly stop and resume threads so they can take turns using the CPU. This constant stopping and starting introduces overhead and reduces efficiency.

In real systems, this often appears as:
- High CPU usage but low throughput
- Requests becoming slower under load
- Performance degrading as more threads are added

Typical mistakes include:
- Creating thousands of threads
- Blocking threads on I/O
- Misconfigured thread pools
- Ignoring CPU core limits
- Mixing CPU-bound and I/O-bound work in the same execution model

---

## How Context Switching Works
A **context switch** occurs when the operating system:
1. Stops the currently running thread
2. Saves its execution state
3. Loads the execution state of another thread
4. Resumes execution of that thread

Even though modern systems are fast, this process still takes time. More importantly, it interrupts the natural flow of execution. When this interruption happens repeatedly, the CPU becomes less efficient because it cannot build momentum executing useful work.

---

## Voluntary vs Involuntary Context Switching

### Voluntary Context Switching
Occurs when a thread cannot continue and explicitly stops:
- Waiting for disk or network I/O
- Waiting for a lock
- Calling `sleep` or `yield`

### Involuntary Context Switching
Occurs when the operating system interrupts a running thread:
- To give CPU time to another thread
- To enforce fairness when many threads compete

**Key idea**  
Voluntary switches come from waiting.  
Involuntary switches come from too many runnable threads.

---

## Why This Becomes a Bottleneck

Context switching becomes a bottleneck when threads are interrupted so frequently that useful work is broken into very small pieces.

A CPU core can only run one thread at a time. If more threads want to run than there are cores, the operating system must constantly rotate between them. Each rotation introduces overhead.

When the number of threads is small, this overhead is negligible. When the number grows, interruptions become frequent and expensive.

Frequent interruption slows execution. Slower execution means threads stay active longer. Threads that stay active longer increase contention. Increased contention causes even more interruptions.

Blocking behavior worsens the situation. Threads that wait for I/O or locks stop and later resume. Each stop and resume adds more switching and more coordination work.

Movement of threads between CPU cores also hurts efficiency. When a thread resumes execution on a different core, it must re-establish execution context, which slows progress and increases the likelihood of further interruption.

All these effects reinforce each other:
- More threads → more interruptions  
- More interruptions → slower progress  
- Slower progress → more contention  
- More contention → even more interruptions  

This feedback loop explains why systems with excessive threading often show high CPU usage, low throughput, poor scalability, and unstable latency.

---

---

## Time Quantum and Scheduling Behavior

A **time quantum** (also called *time slice*) is the amount of CPU time a runnable thread is allowed to execute before the operating system scheduler may preempt it and switch to another thread.

### Is the time quantum fixed?
No. The time quantum is **not a fixed, universal duration**.

Its effective length varies depending on:
- The operating system scheduler implementation
- The scheduling policy in use
- The number of runnable threads in the run queue
- Thread priorities
- Overall system load

Modern schedulers do not aim to give each thread a fixed time slice. Instead, they aim for **fairness** across all runnable threads.

### How the quantum behaves in practice
- When **few threads** are runnable, each thread runs longer without interruption.
- When **many threads** are runnable, the scheduler reduces how long each thread can run.
- As the run queue grows, uninterrupted execution time per thread shrinks.

In effect:
- More runnable threads → shorter execution windows
- Shorter execution windows → more preemption
- More preemption → more involuntary context switches

This is why oversubscribing the CPU with runnable threads directly increases context switching overhead.

---

## What Causes a Thread to Context Switch

A thread context switch occurs whenever the operating system stops one thread and allows another to run. This can happen for several reasons.

### 1. Time Quantum Expiration (Preemption)
If a thread exhausts its allowed execution window, the scheduler may preempt it to give another runnable thread CPU time.

This is the most common cause of **involuntary context switching**, especially when:
- There are more runnable threads than CPU cores
- Threads are CPU-bound and do not block naturally

---

### 2. Blocking Operations (Voluntary Switching)
A thread voluntarily stops executing when it cannot continue:
- Waiting for disk or network I/O
- Waiting to acquire a lock
- Waiting on a condition variable
- Calling `sleep` or `yield`

The scheduler then switches to another runnable thread.

---

### 3. Lock Contention
When a thread attempts to acquire a lock that is already held:
- The thread blocks
- The scheduler switches to another thread
- The blocked thread later resumes when the lock becomes available

High lock contention causes frequent stop-and-resume cycles and increases context switching.

---

### 4. Higher-Priority Threads Becoming Runnable
If a higher-priority thread becomes runnable:
- The scheduler may immediately preempt the currently running thread
- A context switch occurs even if the current thread is making progress

---

### 5. Thread Migration Between CPU Cores
Threads may resume execution on a different CPU core than where they previously ran.

While this does not always cause an immediate context switch by itself, it:
- Slows execution
- Increases execution time
- Increases the likelihood of further preemption

Indirectly, this leads to more context switching.

---

### 6. Operating System Interrupts
Hardware interrupts (network, disk, timers) temporarily stop user threads so the kernel can handle events.

After interrupt handling completes, the scheduler decides which thread runs next, which may result in a context switch.

---

## Why Fewer Context Switches Improve Overall Performance

Reducing context switching improves performance not because context switches are inherently bad, but because **every context switch interrupts multiple layers of work that are already operating efficiently**. When switches happen too often, those layers never reach a stable, optimized state.

Below are the most important performance relationships affected by context switching, explained without diving into hardware internals.

---

### 1. Longer Uninterrupted Execution Means Less Rework

When a thread runs continuously, it makes steady progress. When it is interrupted frequently, its work is fragmented.

Fragmented work often requires:
- Re-checking conditions
- Re-entering code paths
- Re-establishing execution flow

Even on fast CPUs, restarting work repeatedly is slower than continuing smoothly.

**Fewer context switches → longer uninterrupted execution → less repeated work**

---

### 2. Better Reuse of Data and State

While a thread is running, the system naturally keeps the data it uses readily accessible. When execution switches to another thread, different data is accessed and the previous data becomes less immediately available.

When the original thread resumes, accessing its data again takes longer than if it had continued running.

You don’t need to know hardware details to see the effect:
- Continuous execution keeps data “close”
- Frequent switching pushes data “farther away”

**Fewer context switches → better data reuse → faster execution**

---

### 3. Less Time Spent Coordinating, More Time Doing Work

Every context switch requires the operating system to:
- Decide which thread should run
- Stop one execution
- Start another execution

This coordination work is necessary, but it produces no business value. It does not process requests, compute results, or serve users.

Reducing context switches reduces this pure overhead.

**Fewer context switches → less coordination → more CPU time for real work**

---

### 4. Fewer Artificial Execution Interruptions

Context switches often interrupt threads that are actively making progress. These interruptions break the natural flow of execution and reduce efficiency.

Allowing threads to run longer reduces artificial pauses and improves overall execution efficiency.

**Fewer context switches → smoother execution → higher efficiency**

---

### 5. Faster Completion of Tasks

When threads are interrupted frequently, tasks take longer to complete because they are repeatedly paused and resumed.

Longer task durations create secondary problems:
- Resources are held longer
- More threads remain active
- System pressure increases

Reducing context switching shortens task lifetimes and lowers overall contention.

**Fewer context switches → faster task completion → less system pressure**

---

### 6. More Predictable Performance and Latency

Frequent context switching introduces randomness:
- Some threads are interrupted many times
- Others run longer
- Latency becomes inconsistent

Reducing context switching stabilizes execution timing and improves predictability, especially for worst-case latency.

**Fewer context switches → less variability → more predictable performance**

---

### 7. Better Behavior Under Load

Under load, negative effects amplify:
- Slower execution increases contention
- Increased contention increases switching
- Increased switching slows execution further

Reducing context switching weakens this negative feedback loop and allows the system to degrade more gracefully under stress.

**Fewer context switches → weaker feedback loops → better scalability**

---

## How to Reduce Context Switching

Reducing context switching means **reducing how often threads are stopped and restarted**. The goal is not to eliminate concurrency, but to avoid unnecessary interruptions.

---

### 1. Use Thread Pools Instead of Creating Threads

**What goes wrong**  
Creating a new thread for each task quickly leads to a large number of threads that all want CPU time. Since the CPU can only run a limited number of threads at once, the operating system must constantly stop one thread and start another so they can take turns.

**What changes**  
Thread pools create a fixed number of threads ahead of time and reuse them for many tasks. New work waits in a queue instead of creating new threads.

**Why it helps**  
Because the number of threads is limited, fewer threads compete for the CPU at the same time. This reduces how often the operating system needs to interrupt threads, which directly reduces context switching.

---

### 2. Limit Concurrency to CPU Cores for CPU-Bound Work

**What goes wrong**  
CPU-bound tasks always want to run and do not naturally pause. If there are more CPU-bound threads than CPU cores, the operating system must constantly rotate between them, interrupting each one even while it is making progress.

**What changes**  
Concurrency is limited so that only as many CPU-bound tasks run at the same time as there are CPU cores.

**Why it helps**  
When the number of running threads matches the number of cores, threads can run longer without being interrupted. Fewer forced interruptions mean fewer involuntary context switches.

---

### 3. Use Async / Non-Blocking I/O Instead of Blocking I/O

**What goes wrong**  
With blocking I/O, a thread starts an operation (like a network or disk call) and then stops doing anything while waiting for the result. That thread must later be restarted when the operation completes, causing additional context switches.

**What changes**  
With async I/O, the thread starts the operation and immediately returns. The operating system notifies the application later when the I/O is complete, without stopping a thread just to wait.

**Why it helps**  
Threads are no longer stopped and restarted simply to wait for I/O. Fewer threads move between running and waiting states, which significantly reduces context switching.

---

### 4. Minimize Lock Contention

**What goes wrong**  
When multiple threads need the same lock, some threads must wait. Waiting threads stop running and later resume when the lock becomes available. Each wait and resume introduces context switches.

**What changes**  
Reducing shared state, using finer-grained locks, or redesigning code to avoid locks reduces how often threads must wait.

**Why it helps**  
When threads wait less, they are stopped and restarted less often. This directly reduces both voluntary and involuntary context switches.

---

### 5. Reduce Thread Migration Between CPU Cores

**What goes wrong**  
Threads may resume execution on a different CPU core than the one they previously ran on. When this happens, execution becomes less efficient and threads take longer to complete their work.

**What changes**  
Keeping threads on the same CPU core when possible improves execution continuity.

**Why it helps**  
Threads that run more efficiently finish their work faster and spend less time competing for CPU time, which indirectly reduces context switching.

---

### 6. Avoid `sleep`, `yield`, and Busy Waiting

**What goes wrong**  
Calling `sleep` or `yield` explicitly tells the operating system to stop running the thread, even if no real reason exists. Busy waiting repeatedly checks for work and causes unnecessary scheduling activity.

**What changes**  
Event-driven or signal-based mechanisms allow threads to stop only when real work is unavailable and resume exactly when needed.

**Why it helps**  
Threads are no longer interrupted artificially. They stop and resume only when necessary, reducing unnecessary context switches.

---

### 7. Batch Work Instead of Scheduling Tiny Tasks

**What goes wrong**  
Scheduling many very small tasks causes frequent thread scheduling and frequent interruptions. The overhead of stopping and starting threads can become comparable to the work itself.

**What changes**  
Multiple small tasks are grouped together and processed in a single execution.

**Why it helps**  
Threads do more useful work each time they run, which reduces how often they need to be interrupted and rescheduled.

---

### 8. Use Work-Stealing Schedulers

**What goes wrong**  
Some threads become idle while others are overloaded. Idle threads may block while busy threads continue to accumulate work.

**What changes**  
Work-stealing allows idle threads to take work from busy threads instead of blocking or creating new threads.

**Why it helps**  
Threads remain productive without increasing the total number of threads or causing additional stop-and-restart cycles, reducing context switching overall.

---

## Summary and Key Takeaways
Context switching becomes harmful when threads are interrupted too often. The CPU performs best when threads can run long enough to make meaningful progress.

The goal is not “fewer threads”, but **fewer unnecessary interruptions**.

**Rule of thumb:**  
If adding more threads makes performance worse, excessive context switching is often the reason.

---

## Example in C#

```csharp
// ❌ Bad: create many threads
for (int i = 0; i < 1000; i++)
{
    new Thread(() => DoWork()).Start();
}

// ✅ Good: bounded concurrency
Parallel.For(0, 1000, i => DoWork());

// ✅ Best for I/O-bound work
await Task.WhenAll(
    Enumerable.Range(0, 1000)
        .Select(_ => DoWorkAsync())
);
