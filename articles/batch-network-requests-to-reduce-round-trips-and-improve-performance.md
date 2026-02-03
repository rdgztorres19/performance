# Batch Network Requests to Reduce Round-Trips and Improve Performance

**Batching means sending multiple operations in one network request instead of one request per operation. Each network request has a fixed cost: round-trip time (RTT), connection setup, and per-request overhead. Doing one batch request instead of N separate requests can cut total latency and improve throughput by 5x–50x when RTT is significant. The trade-off: you need batching logic, and the first item may wait for the batch to fill. Use batching when you have many related operations and the API or backend supports it.**

---

## Executive Summary (TL;DR)

Batching means grouping multiple operations into one network request (or a few) instead of sending one request per operation. Each request pays a round-trip: latency to the server and back, plus per-request overhead. Sending 100 requests for 100 items means 100 round-trips; sending 1 batch request for 100 items means 1 round-trip—often 5x–50x faster when RTT is high. Use batching for APIs that support it (e.g. batch GET, batch write), or for client-side grouping (e.g. collect items and send one request). Trade-offs: batching logic and possibly higher latency for the first item while the batch fills. Typical benefits: fewer round-trips, lower total latency, higher throughput. Common mistakes: batching when the API does not support it, batches too large (timeouts), or confusing parallel requests with true batching.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you send one request per item?**

Imagine you need to load 50 items by ID from an API:

```csharp
// ❌ Bad: One request per item
public async Task<List<Item>> GetItemsAsync(IEnumerable<int> ids)
{
    var items = new List<Item>();
    foreach (var id in ids)
    {
        var item = await _httpClient.GetAsync($"/api/items/{id}");
        items.Add(await item.Content.ReadFromJsonAsync<Item>());
    }
    return items;
    // What happens: 50 items = 50 round-trips. If RTT is 50 ms, that's 2.5 seconds just in network latency.
}
```

**What happens:**
- **One round-trip per item**: Each `GetAsync` is a full request–response cycle over the network.
- **RTT adds up**: If round-trip time (RTT) is 50 ms, 50 requests take at least 50 × 50 ms = 2.5 s in latency, even if the server responds instantly.
- **Per-request overhead**: Each request has headers, possibly connection setup, and server-side handling; that cost is paid N times.

**Why this is slow:**
- **Latency is dominated by RTT**: The time for a packet to go to the server and back is fixed per round-trip. More round-trips → more total time.
- **No amortization**: You pay the full cost of a round-trip for every single item.
- **Throughput cap**: The number of items per second is limited by how many round-trips you can do (e.g. 1 request every 50 ms → at most 20 items/sec per connection).

**With batching:**

```csharp
// ✅ Good: One batch request for many items
public async Task<List<Item>> GetItemsBatchAsync(IEnumerable<int> ids)
{
    var idsList = ids.ToList();
    var response = await _httpClient.PostAsync("/api/items/batch",
        JsonContent.Create(new { ids = idsList }));
    var result = await response.Content.ReadFromJsonAsync<BatchResponse>();
    return result.Items;
    // What happens: 50 items = 1 round-trip. Same 50 ms RTT, but paid once → ~50 ms total instead of 2.5 s.
}
```

**What happens:**
- **One round-trip for many items**: The server accepts a list of IDs and returns a list of items in one response.
- **RTT paid once**: One request–response cycle; total latency is roughly one RTT plus server time to resolve all IDs.
- **Better throughput**: Same network capacity can deliver many more items per second because you are not doing one round-trip per item.

**Improvement**: 5x–50x lower total latency and much higher throughput when RTT is significant, by reducing the number of round-trips.

### Key Terms Explained (Start Here!)

**What is a round-trip (or RTT)?** The time for a message to go from client to server and for the response to come back. Every separate request pays at least one RTT. Example: 50 ms RTT means each request adds at least 50 ms of latency.

**What is latency?** Time to complete an operation (e.g. time from sending a request to receiving the response). Lower latency means faster response. Batching reduces total latency when you need many items by doing one round-trip instead of many.

**What is throughput?** Number of operations (or items) completed per unit of time. Batching can increase throughput because you spend less time in round-trips and per-request overhead.

**What is batching?** Sending multiple operations or keys in a single request (and receiving multiple results in one response) instead of one request per operation. Example: one “get items by IDs” request instead of 50 “get item by ID” requests.

**What is a batch API?** An API that accepts multiple items (e.g. a list of IDs) and returns multiple results in one call. Not all APIs support this; when they do, batching is the main way to reduce round-trips.

**What is parallel requests?** Sending many requests at the same time (e.g. 50 concurrent GETs) instead of one after another. This reduces *elapsed* time (you wait for one RTT instead of 50), but you still do 50 round-trips and use more connections. True batching is one (or few) round-trips for many items.

**What is request overhead?** The fixed cost per request: headers, parsing, routing, connection handling. Batching amortizes this cost over many items.

### Common Misconceptions

**"Parallel requests are the same as batching"**
- **The truth**: Parallel requests (e.g. `Task.WhenAll` of 50 GETs) reduce *wall-clock* time by doing 50 requests concurrently, but you still perform 50 round-trips and 50× request overhead. A real batch (one request with 50 IDs) does 1 round-trip and 1× overhead—often much better for latency and server load.

**"Batching always improves performance"**
- **The truth**: It helps when RTT or per-request overhead dominates. If the API does not support batching, you cannot batch; if batches are too large, you may hit timeouts or memory limits.

**"Bigger batches are always better"**
- **The truth**: Larger batches mean fewer round-trips but more data per request, longer server processing, and higher risk of timeouts. You must tune batch size (e.g. 50–200 items) to balance latency and reliability.

**"The first item is always slow with batching"**
- **The truth**: If you wait to fill a batch before sending (e.g. collect 50 IDs then send), the first item waits for the batch to fill or a timeout. You can use a small batch window or send partial batches to bound latency for the first item.

---

## How It Works

### Understanding How Batching Works

**Without batching (one request per item):**

```
Request 1 (id=1)  → RTT → Response 1
Request 2 (id=2)  → RTT → Response 2
...
Request N (id=N) → RTT → Response N
Total time: N × RTT + N × (server time + overhead)
```

**With batching (one request for N items):**

```
Request [id=1, id=2, ..., id=N] → RTT → Response [item1, item2, ..., itemN]
Total time: 1 × RTT + (server time for N items + one request overhead)
```

**What happens:**
1. **Client**: Collects multiple operations (e.g. IDs to fetch) into one payload.
2. **One request**: Sends a single HTTP (or other) request with that payload.
3. **Server**: Processes all items in one go and returns one response with all results.
4. **Client**: Parses the response and maps results back to the original items.

**Key insight**: The network cost (RTT) and per-request overhead are paid once per batch instead of once per item, so total latency and server load drop when you have many items.

### Technical Details: Batch vs Parallel

**Parallel requests (no batch API):**
- You send N requests at the same time (e.g. 50 GETs with `Task.WhenAll`).
- Elapsed time ≈ 1 RTT (you wait for the slowest response), but you still do N round-trips and N× overhead. Good when the API has no batch endpoint and you want to avoid serial latency.

**True batching (batch API):**
- You send one request with N keys/IDs and get one response with N results.
- Elapsed time ≈ 1 RTT + server time for N items. Fewer round-trips, less overhead, usually better for throughput and server load.

**Client-side batching (when the API does not support batch):**
- You can still “batch” in the sense of grouping: e.g. collect 50 IDs, then fire 50 parallel requests in one go. That improves wall-clock time but not the number of round-trips. True batching requires server support.

### When the API Supports Batching

Many backends offer batch or bulk endpoints, for example:
- **REST**: `POST /api/items/batch` with body `{ "ids": [1,2,...,N] }`, response `{ "items": [...] }`.
- **GraphQL**: Query multiple entities in one request (one round-trip).
- **gRPC**: Client can send a batch message; server responds with a batch result.
- **Databases**: Bulk GET by keys, batch inserts, etc.

Use these when available; they are the main way to get 5x–50x gains from batching.

---

## Why This Becomes a Bottleneck

Sending one request per item becomes a bottleneck because:

**RTT dominates**: On a 50 ms RTT link, 100 items with 100 requests take at least 5 s in network latency. One batch request takes about 50 ms plus server time. The more items, the more you lose without batching.

**Per-request overhead**: Each request has headers, parsing, routing, and connection handling. Doing this 100 times wastes CPU and increases total time.

**Throughput limit**: Maximum items per second is limited by round-trips per second. One round-trip per item caps you at ~(1/RTT) items/sec per connection. Batching raises the cap (items per round-trip can be large).

**Server load**: Many small requests create more connections, more context switches, and more work per item on the server. One batch request often uses less resources per item.

---

## Advantages

**Much lower total latency**: One round-trip for N items instead of N round-trips. Example: 50 ms RTT, 50 items → 2.5 s serial vs ~50 ms with one batch (50x better).

**Higher throughput**: Same link can deliver many more items per second because each round-trip carries many items.

**Less per-request overhead**: One request to parse, route, and handle instead of N.

**Server-friendly**: Fewer connections and fewer requests for the same amount of work; often better for scalability.

**Scalable to many items**: As N grows, serial “one request per item” gets worse; batching keeps cost near one RTT plus server time.

---

## Disadvantages and Trade-offs

**Requires batch support**: The server (or API) must support batch operations. If it does not, you cannot do true batching; you are limited to parallel requests.

**Batching logic**: Client must collect operations, build the batch, send, and map results back. More code and edge cases (partial failures, timeouts).

**Latency for the first item**: If you wait to fill a batch (e.g. 50 items or 10 ms), the first item can wait up to that time. You can use small windows or max batch size to bound this.

**Larger payloads**: One request carries more data; may hit size limits or timeouts if the batch is too big. Tune batch size (e.g. 50–200 items).

**Partial failure**: If the batch fails or times out, you may need retry or fallback for the whole batch or per item, depending on API design.

---

## When to Use This Approach

Use batching when:

- **Many related operations**: You need to fetch or write many items (e.g. by ID) and the API supports batch or bulk.
- **High RTT**: The round-trip time is significant (e.g. cross-region, mobile). Batching reduces the number of round-trips.
- **Throughput matters**: You want to maximize items per second; batching increases throughput by amortizing RTT and overhead.
- **APIs that support it**: REST batch endpoints, GraphQL (multiple queries in one request), gRPC batch, DB bulk operations.

**Recommended approach:** Prefer batch APIs when available. If not, parallel requests (`Task.WhenAll`) can reduce wall-clock time but not round-trip count. Design backends with batch endpoints for high-throughput clients.

---

## When Not to Use It

- **Single item or few items**: For one or two operations, batching adds complexity with little gain.
- **API has no batch**: If the server does not support batch, you cannot do true batching; use parallel requests only to reduce elapsed time.
- **Very large batches**: Batches that are too big can cause timeouts, memory issues, or poor UX; cap batch size and use multiple batches if needed.
- **Strict per-item latency**: If every single item must have minimal latency and you cannot wait for a batch, batching may hurt the first item; consider smaller windows or no batching for that path.

---

## Common Mistakes

**Confusing parallel with batch**: Using `Task.WhenAll` of N GETs is not batching; it is N concurrent round-trips. Use a real batch endpoint when possible.

**No batch size limit**: Sending 10,000 IDs in one request can timeout or fail. Cap batch size (e.g. 100–200) and send multiple batches.

**Ignoring first-item latency**: Waiting forever to fill a batch delays the first item. Use a max wait time (e.g. 10–50 ms) and/or max batch size so the first item does not wait too long.

**Assuming all APIs support batch**: Check the API; if it does not, you cannot reduce round-trips with batching—only with parallel requests (which still do N round-trips).

**Not handling partial failure**: If the batch request fails, have a strategy: retry whole batch, fallback to single requests, or surface errors clearly.

---

## Example Scenarios

### Scenario 1: Fetch many items by ID (batch API)

**Problem**: You need 100 items by ID; one GET per ID takes 100 × 50 ms = 5 s on a 50 ms RTT link.

**Bad approach** (one request per item, serial):

```csharp
// ❌ Bad: One request per item
var items = new List<Item>();
foreach (var id in ids)
{
    var item = await GetItemAsync(id);
    items.Add(item);
}
```

**Good approach** (batch endpoint):

```csharp
// ✅ Good: One batch request
var items = await GetItemsBatchAsync(ids); // POST /api/items/batch { "ids": [1,2,...] }
```

**Results:**
- **Bad**: 100 round-trips, ~5 s latency (RTT-dominated).
- **Good**: 1 round-trip, ~50 ms + server time; 5x–50x faster.

---

### Scenario 2: No batch API — parallel requests

**Problem**: API has no batch endpoint; you still need 50 items with lower wall-clock time.

**Bad approach** (serial):

```csharp
// ❌ Bad: Serial
foreach (var id in ids)
    items.Add(await GetItemAsync(id));
```

**Good approach** (parallel, same API):

```csharp
// ✅ Good: Parallel (still 50 round-trips, but concurrent)
var tasks = ids.Select(id => GetItemAsync(id));
var items = (await Task.WhenAll(tasks)).ToList();
```

**Results:**
- **Bad**: 50 × RTT elapsed.
- **Good**: ~1 × RTT elapsed. You did not reduce round-trips (no batch API), but you reduced waiting time. Prefer adding a batch API for real gains.

---

### Scenario 3: Client-side batch buffer (e.g. writes)

**Problem**: Many small write requests; you want to group them into fewer, larger requests.

**Approach** (buffer and flush on size or time):

```csharp
// ✅ Good: Buffer and send in batches
private readonly Channel<WriteOp> _buffer = Channel.CreateBounded<WriteOp>(1000);
private const int BatchSize = 50;
private const int FlushMs = 20;

// Producer
await _buffer.Writer.WriteAsync(new WriteOp { Key = k, Value = v });

// Consumer: flush when BatchSize reached or FlushMs elapsed
await foreach (var batch in _buffer.Reader.ReadAllAsync().Buffer(BatchSize).WithTimeout(FlushMs))
{
    await _api.WriteBatchAsync(batch);
}
```

**Results:** Fewer round-trips and less overhead than one request per write; first item may wait up to FlushMs or until the batch fills.

---

## Summary and Key Takeaways

Batching means sending multiple operations in one network request instead of one per operation, reducing round-trips and per-request overhead. When RTT is significant, batching can improve total latency and throughput by 5x–50x. Use it when you have many related operations and the API supports batch or bulk. Trade-offs: batching logic, possible delay for the first item, and tuning batch size. Prefer real batch APIs over parallel requests when available; parallel requests reduce elapsed time but not the number of round-trips. Cap batch size, bound wait time for the first item, and handle partial failures.

---

<!-- Tags: Performance, Optimization, Networking, Network Optimization, .NET Performance, C# Performance, Latency Optimization, Throughput Optimization, System Design, Architecture -->
