# Use Cancellation Tokens for Cooperative Cancellation

**Cancellation tokens allow operations to be cancelled cooperatively, avoiding wasted resources on work that is no longer needed. Using cancellation tokens improves resource utilization, enables faster response to user cancellations, and prevents unnecessary work. The trade-off: code must check tokens and pass them through call chains. Use cancellation tokens for long-running operations, operations that can be cancelled (e.g., user-initiated, timeouts), and async operations whenever possible. Avoid ignoring cancellation in async methods—pass and check tokens to enable responsive cancellation.**

---

## Executive Summary (TL;DR)

Cancellation tokens allow operations to be cancelled cooperatively, avoiding wasted resources on work that is no longer needed. Using cancellation tokens improves resource utilization, enables faster response to user cancellations, and prevents unnecessary work (CPU, I/O, memory). The trade-off: code must check tokens and pass them through call chains. Use cancellation tokens for long-running operations, operations that can be cancelled (user-initiated, timeouts, request aborts), and async operations whenever possible. Typical benefits: better resource utilization, faster response to cancellations, better user experience, avoidance of unnecessary work. Common mistakes: not passing cancellation tokens, not checking tokens in loops, ignoring cancellation in async methods, not linking tokens for timeouts.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you don't use cancellation tokens?**

Imagine a scenario where a user starts a long-running operation and then cancels it (e.g., closes a dialog, navigates away):

```csharp
// ❌ Bad: No cancellation support
public async Task ProcessItemsAsync(List<Item> items)
{
    foreach (var item in items)
    {
        await ProcessItemAsync(item); // No cancellation - work continues even if user cancelled
    }
    // What happens: User cancels but operation continues = wasted CPU, I/O, memory = poor UX
}
```

**What happens:**
- **No cancellation**: Operation continues even after user cancels
- **Wasted resources**: CPU, I/O, and memory used for work that is no longer needed
- **Poor responsiveness**: User waits for operation to "finish" or abandons the app
- **Resource exhaustion**: Many cancelled-but-still-running operations can exhaust resources
- **Performance impact**: Unnecessary work reduces overall system efficiency

**Why this is bad:**
- **Wasted CPU**: Work continues on cancelled operations
- **Wasted I/O**: Database queries, HTTP requests continue unnecessarily
- **Wasted memory**: Allocations for cancelled work
- **Poor UX**: User expects cancellation to stop work quickly
- **Scalability**: Cancelled operations consume resources that could serve other requests

**With cancellation tokens:**

```csharp
// ✅ Good: Cooperative cancellation with CancellationToken
public async Task ProcessItemsAsync(List<Item> items, CancellationToken cancellationToken)
{
    foreach (var item in items)
    {
        cancellationToken.ThrowIfCancellationRequested(); // Check before each item
        await ProcessItemAsync(item, cancellationToken); // Pass token to async operations
    }
    // What happens: User cancels → token signals → operation stops quickly = no wasted work
}
```

**What happens:**
- **Cooperative cancellation**: Operation checks token and stops when cancellation is requested
- **Resource savings**: No work done after cancellation
- **Faster response**: Operation stops quickly when user cancels
- **Better UX**: User sees immediate effect of cancellation
- **Better efficiency**: Resources freed for other work

**Improvement**: Better resource utilization, faster response to cancellations, better user experience.

### Key Terms Explained (Start Here!)

**What is a cancellation token?** An object that signals that an operation should be cancelled. When cancellation is requested, the token is in a "cancelled" state and operations checking it should stop. Example: `CancellationToken` is passed to async methods so they can stop when the user cancels.

**What is cooperative cancellation?** Cancellation where the operation itself checks for cancellation and stops voluntarily (as opposed to forcefully killing a thread). Cooperative cancellation is safe and allows clean shutdown. Example: Loop checks `cancellationToken.ThrowIfCancellationRequested()` and exits when cancellation is requested.

**What is CancellationTokenSource?** The object that creates and controls a `CancellationToken`. When you call `Cancel()` on the source, the token signals cancellation. Example: `var cts = new CancellationTokenSource(); var token = cts.Token;` — later `cts.Cancel()` signals cancellation.

**What is ThrowIfCancellationRequested?** A method on `CancellationToken` that throws `OperationCanceledException` if cancellation has been requested. Use it to exit loops or methods when cancellation is requested. Example: `cancellationToken.ThrowIfCancellationRequested()` in a loop.

**What is OperationCanceledException?** An exception thrown when an operation is cancelled. It is the standard way to signal that cancellation occurred (not a fault). Example: Catching `OperationCanceledException` separately from other exceptions to handle cancellation.

**What is a long-running operation?** An operation that takes significant time (seconds or more). Long-running operations should support cancellation so users can cancel if they no longer need the result. Example: Processing 10,000 items, loading a large file, running a report.

**What is request abort?** When an HTTP request is aborted (e.g., client disconnects). In ASP.NET Core, the request's `CancellationToken` is cancelled when the client disconnects. Example: Pass `HttpContext.RequestAborted` to async methods so they stop when the client disconnects.

**What is token linking?** Combining multiple cancellation tokens (e.g., user cancellation + timeout). `CancellationTokenSource.CreateLinkedTokenSource(token1, token2)` creates a token that is cancelled when any of the source tokens is cancelled. Example: Link user token with a 30-second timeout token.

**What is token registration?** Registering a callback to run when cancellation is requested. Use for cleaning up resources. Example: `cancellationToken.Register(() => cleanup())`.

### Common Misconceptions

**"Cancellation is only for UI apps"**
- **The truth**: Cancellation is important in servers too. When a client disconnects, the server should cancel the request's work to free resources.

**"Checking the token has high overhead"**
- **The truth**: Checking `CancellationToken` is very cheap (a few nanoseconds). The benefit of stopping unnecessary work far outweighs the cost.

**"I can just ignore the token in internal methods"**
- **The truth**: If you don't pass the token down, child operations can't be cancelled. Always pass the token through the call chain.

**"Cancellation is the same as timeout"**
- **The truth**: Cancellation is the mechanism; timeout is one way to trigger it. You can link a timeout token with a user token so that either user cancel or timeout triggers cancellation.

**"OperationCanceledException is an error"**
- **The truth**: OperationCanceledException indicates normal cancellation, not a fault. Handle it separately from other exceptions (e.g., log differently or not at all).

---

## How It Works

### Understanding How Cancellation Tokens Work

**How cancellation is signaled:**

```csharp
var cts = new CancellationTokenSource();
CancellationToken token = cts.Token;

// Later, when cancellation is needed (e.g., user clicks Cancel):
cts.Cancel(); // Token is now in "cancelled" state
```

**What happens:**
1. **Token source created**: `CancellationTokenSource` creates a token
2. **Token passed to operation**: Operation receives the token
3. **Cancel() called**: Something (user, timeout, request abort) calls `cts.Cancel()`
4. **Token state**: Token's `IsCancellationRequested` becomes true
5. **Operation checks token**: Operation calls `ThrowIfCancellationRequested()` or checks `IsCancellationRequested` and stops

**How operations check for cancellation:**

```csharp
// Option 1: Throw if cancelled (common in async methods)
cancellationToken.ThrowIfCancellationRequested();

// Option 2: Check and exit loop
if (cancellationToken.IsCancellationRequested)
    break;

// Option 3: Check and return
if (cancellationToken.IsCancellationRequested)
    return;
```

**Key insight**: Cancellation is cooperative. The operation must check the token; the token does not automatically stop the operation. Checking the token regularly (e.g., per item in a loop) ensures responsive cancellation.

### Technical Details: Passing Tokens Through Call Chains

**Why passing the token matters:**

```csharp
// ❌ Bad: Token not passed - child operation cannot be cancelled
public async Task ProcessItemsAsync(List<Item> items, CancellationToken cancellationToken)
{
    foreach (var item in items)
    {
        cancellationToken.ThrowIfCancellationRequested();
        await ProcessItemAsync(item); // Token not passed - ProcessItemAsync cannot cancel
    }
}
```

**What happens:** If user cancels, the loop exits but any in-flight `ProcessItemAsync` continues until it completes. I/O and CPU work in that call are wasted.

**Good approach:**

```csharp
// ✅ Good: Token passed - entire call chain can respond to cancellation
public async Task ProcessItemsAsync(List<Item> items, CancellationToken cancellationToken)
{
    foreach (var item in items)
    {
        cancellationToken.ThrowIfCancellationRequested();
        await ProcessItemAsync(item, cancellationToken); // Token passed
    }
}

public async Task ProcessItemAsync(Item item, CancellationToken cancellationToken)
{
    await SomeAsyncOperation(item, cancellationToken); // Token passed to I/O
    cancellationToken.ThrowIfCancellationRequested();
    await AnotherAsyncOperation(item, cancellationToken);
}
```

**What happens:** When cancellation is requested, any method in the chain that checks the token can stop. Async I/O operations that accept the token can cancel underlying work where supported.

**Key insight**: Pass `CancellationToken` as the last parameter to every async method that can be cancelled. This is the standard .NET convention.

### Technical Details: Linking Tokens (Timeout + User Cancel)

**Scenario:** You want to cancel either when the user cancels or when a timeout (e.g., 30 seconds) expires.

```csharp
public async Task ProcessWithTimeoutAsync(List<Item> items, CancellationToken userToken)
{
    using var timeoutCts = new CancellationTokenSource(TimeSpan.FromSeconds(30));
    using var linkedCts = CancellationTokenSource.CreateLinkedTokenSource(userToken, timeoutCts.Token);
    CancellationToken token = linkedCts.Token;

    await ProcessItemsAsync(items, token); // Cancels on user cancel OR timeout
}
```

**What happens:**
1. **Timeout source**: `timeoutCts` will cancel automatically after 30 seconds
2. **Linked source**: `linkedCts` is cancelled when either `userToken` or `timeoutCts.Token` is cancelled
3. **Single token**: Pass `linkedCts.Token` to the operation; it cancels on either condition

**Key insight**: Use `CancellationTokenSource.CreateLinkedTokenSource` to combine multiple cancellation triggers (user, timeout, request abort).

---

## Why This Becomes a Bottleneck

Ignoring cancellation becomes a problem because:

**Wasted resources**: Cancelled operations continue to use CPU, I/O, and memory. Example: User cancels a search but the server continues querying the database for 100 more items = wasted DB and CPU.

**Reduced throughput**: Resources used by cancelled work are not available for other requests. Example: 10 cancelled requests each holding a DB connection and thread = fewer resources for new requests.

**Poor user experience**: Users expect cancellation to stop work quickly. Example: User clicks Cancel but the UI stays busy for several more seconds = frustration.

**Request abort not honored**: In web apps, when the client disconnects, the request's token is cancelled. If you don't pass it, the server keeps doing work for a dead connection. Example: Client closes browser but server continues building a large response = wasted resources.

**Scalability**: In high-load scenarios, many cancelled-but-still-running operations can exhaust thread pool, connections, or memory. Example: Many users cancel; without cancellation support, all those operations keep running = resource exhaustion.

---

## Advantages

**Better resource utilization**: Cancelled work stops immediately, freeing CPU, I/O, and memory. Example: User cancels → no more items processed → resources freed for other work.

**Faster response to cancellations**: Operations that check the token frequently stop quickly. Example: Check token per item in a loop → cancellation detected within one item's processing time.

**Better user experience**: Users see immediate effect when they cancel. Example: Cancel button → operation stops → UI becomes responsive.

**Avoidance of unnecessary work**: No work is done for results that will never be used. Example: User navigates away → cancel token fires → no point continuing the request.

**Honoring request abort**: In ASP.NET Core, passing `HttpContext.RequestAborted` ensures that when the client disconnects, server-side work stops. Example: Client closes tab → request aborted → server stops processing.

**Support for timeouts**: Linked tokens allow timeouts (e.g., 30 seconds) so operations don't run forever. Example: Link user token with timeout token → operation stops on user cancel or after 30 seconds.

---

## Disadvantages and Trade-offs

**Requires checking the token**: Code must call `ThrowIfCancellationRequested()` or check `IsCancellationRequested` in loops and long-running sections. Example: Forget to check in one loop → that loop runs to completion even after cancel.

**Requires passing the token**: Every async method in the call chain that should respond to cancellation needs the token as a parameter. Example: Adding cancellation to a deep call chain requires updating many method signatures.

**Can require code changes**: Existing code that doesn't support cancellation needs to be updated. Example: Adding token parameter to 20 methods.

**Exception for cancellation**: `ThrowIfCancellationRequested()` throws `OperationCanceledException`. Callers must handle it (or let it propagate). Example: Top-level handler should treat `OperationCanceledException` as normal cancellation, not log as error.

**Not all APIs accept tokens**: Some older or third-party APIs don't take a `CancellationToken`. You can only cancel between calls, not during. Example: Legacy library doesn't accept token → you check token between calls but not during the call.

---

## When to Use This Approach

Use cancellation tokens when:

- **Long-running operations** (operations that take seconds or more). Example: Processing large lists, generating reports, long-running queries. Users and systems expect to be able to cancel.

- **Operations that can be cancelled** (user-initiated, timeouts, request abort). Example: User clicks Cancel, request timeout, client disconnects. Pass the appropriate token.

- **Async operations** (whenever possible). Example: All async methods that do I/O or long-running work should accept a `CancellationToken` as the last parameter.

- **ASP.NET Core / web apps** (for request lifetime). Example: Use `HttpContext.RequestAborted` so that when the client disconnects, server stops work.

- **Timeouts** (to cap operation duration). Example: Link a timeout token so the operation never runs longer than N seconds.

**Recommended approach:**
- **Add token parameter**: Add `CancellationToken cancellationToken = default` to async methods (default allows callers to omit it).
- **Check in loops**: Call `cancellationToken.ThrowIfCancellationRequested()` at the start of each loop iteration or before expensive steps.
- **Pass token down**: Pass the token to all async methods you call.
- **Handle OperationCanceledException**: At the top level, handle cancellation separately (e.g., return a cancelled result, don't log as error).

---

## When Not to Use It

Don't worry about cancellation when:

- **Operation is very fast** (milliseconds) and cancellation is unlikely. Example: Single small API call. Still, passing the token is cheap and future-proofs the code.

- **API doesn't support it** (legacy or third-party). Example: Library method doesn't accept a token. Check token between calls where you can.

- **Operation must run to completion** (e.g., critical transaction). Example: Commit phase of a transaction. Document why cancellation is not supported.

**Note**: In practice, it's almost always better to accept and pass a `CancellationToken` in async code. Use `= default` so callers can omit it when they don't have one.

---

## Performance Impact

Typical benefits when using cancellation tokens:

- **Resource utilization**: Cancelled operations stop immediately, freeing CPU, I/O, and memory. Impact is largest when many operations are cancelled (e.g., users cancel often, or many request aborts).

- **Throughput**: Fewer resources tied up in cancelled work means more capacity for other requests. Example: 20% of requests cancelled → without tokens, 20% of work is wasted; with tokens, those resources are freed.

- **Latency for cancelled requests**: User-visible "time to stop" improves from "when the operation would have finished" to "within one check interval" (e.g., one item in a loop).

**Important**: The benefit scales with how often cancellation happens. If cancellations are rare, the main gain is correctness and UX. If cancellations are frequent (e.g., search-as-you-type with many aborted requests), resource savings and throughput improvement are significant.

---

## Common Mistakes

**Not passing the token**: Accepting a token but not passing it to child async calls. Example: `await ProcessItemAsync(item)` instead of `await ProcessItemAsync(item, cancellationToken)`. Child work cannot be cancelled.

**Not checking the token in loops**: Only checking at the start of the method. Example: Long loop over 10,000 items with no check inside → cancellation only detected after the method is entered, not during the loop.

**Ignoring cancellation in async methods**: Async methods that don't accept or check a token. Example: Public async API that runs for a long time but has no `CancellationToken` parameter.

**Not handling OperationCanceledException**: Treating cancellation as an error. Example: Logging `OperationCanceledException` as an error or showing an error message to the user. Handle it as normal cancellation.

**Not linking tokens for timeout**: Only supporting user cancel, not timeout. Example: Operation can run forever if user doesn't cancel. Use `CreateLinkedTokenSource` with a timeout token.

**Forgetting to pass request aborted in ASP.NET Core**: Not using `HttpContext.RequestAborted` in request-handling code. Example: Client disconnects but server continues building response. Pass the request's cancellation token into your async work.

---

## How to Measure and Validate

Track **cancellation rate**, **resource usage**, and **time-to-cancel**:
- **Cancellation rate**: How often operations are cancelled (e.g., user cancel, timeout, request abort). Log when operations complete vs. are cancelled.
- **Resource usage**: CPU, I/O, connections with and without cancellation support. After cancellation, resources should be released quickly.
- **Time-to-cancel**: Time from cancel request to operation stop. Should be on the order of one "check interval" (e.g., one item in a loop).

**Practical validation:**
1. **Add tokens**: Add `CancellationToken` to async methods and pass through.
2. **Check in loops**: Add `ThrowIfCancellationRequested()` in loops and before expensive steps.
3. **Test cancellation**: Trigger cancel (e.g., cancel button, close client) and verify work stops and resources are released.
4. **Test timeout**: Use a linked timeout token and verify operation stops after the timeout.

**Tools:** Application logging (cancellation vs. completion), metrics (request duration for cancelled vs. completed), debugging (break when token is cancelled).

---

## Example Scenarios

### Scenario 1: Processing a list with cancellation

**Problem**: Process a large list of items; user may cancel. Without a token, work continues even after cancel.

**Bad approach** (no cancellation):

```csharp
// ❌ Bad: No cancellation support
public async Task ProcessItemsAsync(List<Item> items)
{
    foreach (var item in items)
    {
        await ProcessItemAsync(item); // Continues even if user cancelled
    }
}
```

**Good approach** (with cancellation token):

```csharp
// ✅ Good: Check token and pass to child operations
public async Task ProcessItemsAsync(List<Item> items, CancellationToken cancellationToken = default)
{
    foreach (var item in items)
    {
        cancellationToken.ThrowIfCancellationRequested();
        await ProcessItemAsync(item, cancellationToken);
    }
}

public async Task ProcessItemAsync(Item item, CancellationToken cancellationToken = default)
{
    await SomeAsyncOperation(item, cancellationToken);
    cancellationToken.ThrowIfCancellationRequested();
    await AnotherAsyncOperation(item, cancellationToken);
}
```

**Results:**
- **Bad**: User cancels but all items are still processed = wasted work, poor UX.
- **Good**: User cancels → token is set → next check throws → loop and child work stop = no wasted work, good UX.

---

### Scenario 2: ASP.NET Core and request abort

**Problem**: Long-running API action; when the client disconnects, the server should stop work.

**Bad approach** (ignoring request abort):

```csharp
// ❌ Bad: Not using request cancellation token
[HttpGet]
public async Task<IActionResult> GetReport()
{
    var data = await BuildLargeReportAsync(); // No token - continues even if client disconnected
    return Ok(data);
}
```

**Good approach** (pass request aborted token):

```csharp
// ✅ Good: Pass request cancellation token
[HttpGet]
public async Task<IActionResult> GetReport(CancellationToken cancellationToken)
{
    // cancellationToken is typically HttpContext.RequestAborted
    var data = await BuildLargeReportAsync(cancellationToken);
    return Ok(data);
}

private async Task<Report> BuildLargeReportAsync(CancellationToken cancellationToken)
{
    var report = new Report();
    foreach (var section in GetSections())
    {
        cancellationToken.ThrowIfCancellationRequested();
        report.Add(await BuildSectionAsync(section, cancellationToken));
    }
    return report;
}
```

**Results:**
- **Bad**: Client disconnects but server keeps building the report = wasted CPU and memory.
- **Good**: Client disconnects → request token is cancelled → report building stops = resources freed.

---

### Scenario 3: User cancel with timeout

**Problem**: Run an operation that should stop on user cancel or after 30 seconds.

**Approach** (linked token source):

```csharp
// ✅ Good: Link user token with timeout
public async Task ProcessWithTimeoutAsync(List<Item> items, CancellationToken userToken)
{
    using var timeoutCts = new CancellationTokenSource(TimeSpan.FromSeconds(30));
    using var linkedCts = CancellationTokenSource.CreateLinkedTokenSource(userToken, timeoutCts.Token);

    await ProcessItemsAsync(items, linkedCts.Token);
}
```

**Results:** Operation stops when the user cancels or when 30 seconds have elapsed, whichever comes first. Resources are not held beyond that.

---

## Summary and Key Takeaways

Cancellation tokens allow operations to be cancelled cooperatively, avoiding wasted resources on work that is no longer needed. Use cancellation tokens for long-running operations, operations that can be cancelled (user, timeout, request abort), and async operations whenever possible. Pass the token through the call chain and check it in loops and before expensive steps. The trade-off: code must check the token and pass it on; the benefit is better resource utilization, faster response to cancellations, and better user experience. Common mistakes: not passing the token, not checking in loops, ignoring cancellation in async methods, not handling OperationCanceledException appropriately, not linking tokens for timeouts. In ASP.NET Core, use the request's cancellation token (e.g., from the action) so that client disconnect stops server-side work. Always pass and check cancellation tokens in async code when cancellation is possible.

---

<!-- Tags: Performance, Optimization, Concurrency, .NET Performance, C# Performance, Async/Await, System Design, Architecture, Latency Optimization -->
