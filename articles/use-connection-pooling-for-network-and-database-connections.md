# Use Connection Pooling for Network and Database Connections

**Connection pooling reuses existing connections instead of creating new ones for each request, reducing connection overhead and improving performance. Creating a new connection (TCP handshake, TLS handshake, auth) is expensive; pooling can improve performance by 10x to 100x. The trade-off: pools require configuration and can hold idle connections. Use connection pooling for HTTP clients, database connections, and any high-throughput network communication. Avoid creating a new connection per request—use built-in pooling (IHttpClientFactory, DbContext/ADO.NET) or a dedicated pool.**

---

## Executive Summary (TL;DR)

Connection pooling reuses existing connections instead of creating new ones for each request, reducing connection overhead and improving performance. Creating a new connection involves TCP handshake, TLS negotiation, and often authentication—each step adds latency and CPU cost. Pooling can improve performance by 10x to 100x compared to creating new connections per request. Use connection pooling for HTTP clients, database connections, and any high-throughput network communication. The trade-off: pools require configuration (min/max size, timeouts) and can hold idle connections. Typical benefits: 10x–100x better performance, lower latency, better resource utilization. Common mistakes: creating new HttpClient per request, not using IHttpClientFactory, misconfiguring pool size, not disposing connections properly.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you create a new connection per request?**

Imagine an API that calls an external service or database for every request:

```csharp
// ❌ Bad: New connection per request
public async Task<Data> FetchDataAsync(string id)
{
    using var client = new HttpClient(); // New connection each time
    var response = await client.GetAsync($"https://api.example.com/data/{id}");
    return await response.Content.ReadFromJsonAsync<Data>();
    // What happens: 1000 requests = 1000 TCP handshakes + 1000 TLS handshakes = high latency, poor performance
}
```

**What happens:**
- **New connection**: Each request creates a new `HttpClient` and thus a new TCP connection (or more)
- **TCP handshake**: Three-way handshake (SYN, SYN-ACK, ACK) — round trip
- **TLS handshake**: Certificate exchange, key agreement — multiple round trips
- **Connection teardown**: After the request, connection is closed; next request repeats the cost
- **Performance impact**: Connection setup can add tens to hundreds of milliseconds per request; under load, socket exhaustion is possible

**Why this is slow:**
- **Connection overhead**: TCP + TLS handshakes add latency (e.g., 50–200 ms per new connection)
- **CPU cost**: Handshakes and encryption use CPU
- **Socket exhaustion**: Creating too many connections can exhaust ephemeral ports or hit OS limits
- **No reuse**: Each request pays full connection cost

**With connection pooling:**

```csharp
// ✅ Good: Reuse connections via IHttpClientFactory
public class DataService
{
    private readonly IHttpClientFactory _httpClientFactory;

    public DataService(IHttpClientFactory httpClientFactory)
    {
        _httpClientFactory = httpClientFactory;
    }

    public async Task<Data> FetchDataAsync(string id)
    {
        var client = _httpClientFactory.CreateClient("MyApi"); // Pooled connection
        var response = await client.GetAsync($"https://api.example.com/data/{id}");
        return await response.Content.ReadFromJsonAsync<Data>();
        // What happens: Connections reused from pool = no handshake per request = 10x-100x faster
    }
}
// In Startup: services.AddHttpClient("MyApi", c => c.BaseAddress = new Uri("https://api.example.com"));
```

**What happens:**
- **Pooled connections**: `IHttpClientFactory` manages a pool of connections to the same host
- **Reuse**: Requests reuse existing connections when possible
- **No handshake per request**: Only the first request (or when pool grows) pays connection cost
- **Better performance**: 10x–100x faster than creating a new connection per request

**Improvement**: 10x–100x better performance and lower latency by reusing connections.

### Key Terms Explained (Start Here!)

**What is a connection?** A logical or physical channel between a client and a server (e.g., TCP connection). Data is sent over the connection; when done, the connection can be closed or kept open for reuse. Example: HTTP request over a TCP connection to a server.

**What is connection overhead?** The time and resources needed to establish a connection (TCP handshake, TLS handshake, authentication). Connection overhead is paid once per new connection. Example: 50–200 ms for a new HTTPS connection.

**What is a TCP handshake?** The three-way exchange to establish a TCP connection: client sends SYN, server replies SYN-ACK, client sends ACK. Each round trip adds latency (typically one RTT). Example: 20 ms RTT → handshake adds ~20 ms.

**What is a TLS handshake?** The exchange to establish a secure (HTTPS) connection: protocol version, certificates, key exchange. Usually adds one or more round trips. Example: TLS 1.3 can add 1 RTT; TLS 1.2 often 2 RTTs.

**What is connection pooling?** Keeping a set of established connections (a "pool") and reusing them for multiple requests instead of creating a new connection per request. Example: Pool of 10 connections to a database; 1000 requests share those 10 connections.

**What is keep-alive?** The mechanism that keeps a TCP connection open after an HTTP request/response so it can be reused for more requests. Without keep-alive, the server would close the connection after each response and the client would need a new connection every time. In HTTP/1.1, keep-alive is the default (persistent connections). In HTTP/1.0, the client sends `Connection: keep-alive` to ask the server not to close. Connection pooling depends on keep-alive: the pool holds connections that stay open (alive) between requests.

**What is socket exhaustion?** Running out of available sockets (e.g., ephemeral ports on the client) because too many connections were created and not closed. Can cause "cannot assign requested address" or similar. Example: Creating a new HttpClient per request in a loop can exhaust sockets.

**What is IHttpClientFactory?** A .NET service that creates and manages `HttpClient` instances and their underlying connection pools. Use it instead of `new HttpClient()` so connections are pooled per host. Example: `services.AddHttpClient("MyApi")` registers a named client; `IHttpClientFactory.CreateClient("MyApi")` returns a client that shares the pool.

**What is connection string?** A string that describes how to connect to a resource (e.g., database server, database name, credentials). ADO.NET uses it to connect and, by default, uses connection pooling. Example: `"Server=localhost;Database=MyDb;Integrated Security=true;"`.

**What is latency?** Time to complete an operation (e.g., time from sending a request to receiving the response). Lower latency means faster response. Example: 50 ms latency = 50 ms from request to response.

**What is throughput?** Number of operations completed per unit of time. Connection pooling often increases throughput by reducing per-request overhead. Example: 1000 requests per second = throughput.

### Common Misconceptions

**"HttpClient is cheap to create"**
- **The truth**: Each `HttpClient` can hold connections; creating many `HttpClient` instances can lead to socket exhaustion and no real connection reuse. Use `IHttpClientFactory` or a single long-lived `HttpClient` for the same host.

**"Connection pooling is only for databases"**
- **The truth**: HTTP clients benefit too (e.g., `IHttpClientFactory` pools connections per host). Any protocol that uses connections can benefit from pooling.

**"Pooling always uses more memory"**
- **The truth**: A pool keeps a bounded number of idle connections; the alternative under load is creating/destroying many connections, which can use more CPU and lead to exhaustion. Pools are tuned (min/max size) to balance reuse and resource use.

**"I don't need pooling for a few requests"**
- **The truth**: Even a few requests can benefit from reuse (e.g., 2nd request reuses 1st connection). Using `IHttpClientFactory` or pooled DB access is the default best practice.

**"Pool size should be huge"**
- **The truth**: Pool size should match expected concurrency and server capacity. Too large a pool can overload the server or waste resources; too small can cause queuing.

---

## How It Works

### Understanding How Connection Pooling Works

**Without pooling (one connection per request):**

```
Request 1 → New TCP connection → TLS handshake → Send request → Response → Close connection
Request 2 → New TCP connection → TLS handshake → Send request → Response → Close connection
Request 3 → New TCP connection → TLS handshake → Send request → Response → Close connection
...
Cost per request: connection setup + request/response
```

**With pooling:**

```
Request 1 → Create connection (or take from pool) → Send request → Response → Return connection to pool
Request 2 → Take connection from pool → Send request → Response → Return connection to pool
Request 3 → Take connection from pool → Send request → Response → Return connection to pool
...
Cost per request: (amortized) small pool lookup + request/response; connection setup only when pool grows
```

**What happens:**
1. **Pool**: Maintains a set of open connections (e.g., to a host or database).
2. **Request**: Application asks for a connection; pool returns an existing idle connection or creates a new one (if under max).
3. **Use**: Request is sent over the connection; response is read.
4. **Return**: Connection is returned to the pool (not closed) for reuse.
5. **Limits**: Pool has min/max size and often a timeout for idle connections.

**Key insight**: Pooling amortizes connection cost over many requests, so per-request latency and CPU drop. Throughput increases because less time is spent in handshakes and teardown.

### Keep-Alive Behind the Scenes (Why Pooling Works for HTTP)

Connection pooling for HTTP only works because connections are *kept open* between requests. That is what **keep-alive** does.

**Without keep-alive (HTTP/1.0-style):**
- Client sends request → server sends response → server closes the TCP connection.
- Next request: client must open a new TCP connection (handshake again). No reuse, so there is nothing to pool.

**With keep-alive (HTTP/1.1 default, persistent connections):**
- Client sends request → server sends response → **connection stays open** (not closed).
- Next request: client sends another request on the **same** TCP connection; no new handshake.
- The connection is only closed after a timeout (idle) or when either side sends `Connection: close`.

**Behind the scenes:**
1. **Protocol**: In HTTP/1.1, the connection is persistent unless `Connection: close` is sent. The server does not close after each response; the client can send multiple requests on the same connection (often pipelined or one-after-one). In HTTP/1.0, the client explicitly sends `Connection: keep-alive` in the request and the server may include `Connection: keep-alive` in the response to agree.
2. **Who keeps it open?** Both sides keep the TCP socket open. The client does not call close after reading the response; the server does not close after sending the response. So the same TCP connection carries request 1, response 1, request 2, response 2, …
3. **Relationship to pooling**: The client’s connection pool (e.g., `SocketsHttpHandler` in .NET) holds these *keep-alive* connections. When you need to send a request, the pool gives you an existing open connection instead of creating a new one. Without keep-alive, every request would close the connection and the pool would have nothing to reuse.
4. **Timeouts**: Idle keep-alive connections are closed after a timeout (e.g., server-side 60–120 seconds) to free resources. The pool may create a new connection when the next request arrives. Pool configuration (e.g., max idle time) aligns with these timeouts.

**Summary:** Keep-alive is the protocol-level behavior (“don’t close the connection after this response”). Connection pooling is the application-level strategy (“maintain a set of those open connections and reuse them”). Pooling relies on keep-alive; without it, HTTP would be one-request-per-connection and pooling would not help.

### Technical Details: HTTP Connection Pooling in .NET

**IHttpClientFactory and SocketsHttpHandler:**

- `IHttpClientFactory` creates `HttpClient` instances that share a common `SocketsHttpHandler` (or similar) per "named client."
- `SocketsHttpHandler` maintains a connection pool per endpoint (scheme + host + port). Multiple requests to the same endpoint reuse connections.
- You do not create `HttpClient` with `new HttpClient()` in production; you use `IHttpClientFactory.CreateClient(...)` so pooling and lifetime are managed.

**Registration:**

```csharp
// In Program.cs or Startup.cs
services.AddHttpClient("MyApi", client =>
{
    client.BaseAddress = new Uri("https://api.example.com");
    client.Timeout = TimeSpan.FromSeconds(30);
});
```

**Usage:**

```csharp
var client = _httpClientFactory.CreateClient("MyApi");
var response = await client.GetAsync("/data/123"); // Uses pooled connection
```

**Key insight**: Always use `IHttpClientFactory` for HTTP clients in ASP.NET Core (and similar patterns elsewhere) so connection pooling and proper disposal are handled.

### Technical Details: Database Connection Pooling in .NET

**ADO.NET (SqlConnection, etc.):**

- ADO.NET providers (e.g., for SQL Server) use connection pooling by default.
- Same connection string (including server, database, and other key settings) shares a pool. Different strings get different pools.
- When you `Open()` a connection, the provider takes one from the pool; when you `Dispose()`/`Close()`, it returns to the pool (connection is not closed at the network level).

**Connection string:**

```csharp
var connectionString = "Server=localhost;Database=MyDb;Integrated Security=true;";
using var connection = new SqlConnection(connectionString);
await connection.OpenAsync(); // From pool
// ... use connection ...
// Dispose returns connection to pool
```

**Entity Framework Core:**

- EF Core uses the underlying ADO.NET provider and its pooling. You typically register `DbContext` with a lifetime (e.g., scoped) and use the same connection string; the provider pools connections.

**Key insight**: For databases, use the default pooling (don’t disable it unless you have a special case) and reuse the same connection string so all connections to the same DB share the pool.

---

## Why This Becomes a Bottleneck

Creating a new connection per request becomes a bottleneck because:

**Connection overhead**: Each new connection pays TCP + TLS (and often auth). Example: 100 ms per connection × 1000 requests = 100 seconds spent only on connection setup if each request uses a new connection.

**Socket exhaustion**: The OS has a limited number of ephemeral ports. Creating and closing many connections can exhaust them. Example: "Only one usage of each socket address is normally permitted" or similar errors under load.

**Server load**: Each new connection consumes resources on the server (memory, file descriptors). Too many short-lived connections can overload the server. Example: 10,000 concurrent connections opening and closing vs. a stable pool of 100.

**Latency**: Handshakes add round trips. Example: 2 RTTs for TCP + TLS → 40 ms extra per request on a 20 ms RTT link.

**Throughput**: Time spent in setup and teardown is time not spent sending requests and processing responses. Example: Pooling can increase effective throughput by 10x–100x when connection cost is significant.

---

## Advantages

**Much better performance**: Pooling can improve performance by 10x–100x when connection setup is a large part of request time. Example: 200 ms per request with new connection vs. 5 ms with pooled connection (40x faster).

**Lower latency**: Reused connections skip handshakes. Example: First request 100 ms, subsequent requests 10 ms (only request/response).

**Better resource utilization**: Fewer connections for the same workload; less CPU and memory on client and server. Example: 100 pooled connections serving 1000 req/sec vs. 1000 short-lived connections.

**Avoids socket exhaustion**: Bounded number of connections in the pool avoids running out of ephemeral ports. Example: Pool of 100 connections vs. creating 10,000 connections in a loop.

**Server-friendly**: Stable pool of connections is easier for the server to handle than a flood of connect/disconnect. Example: Fewer connection churn, better server scalability.

---

## Disadvantages and Trade-offs

**Requires pool management**: Pool size, timeouts, and health must be configured. Example: Max connections too low → queuing; too high → server overload or resource waste.

**Can hold idle connections**: Pool keeps connections open for reuse. Example: Idle connections consume server resources; idle timeout is used to close them.

**Connection string / identity**: For DB, pool is per connection string. Different strings (e.g., different users or DBs) get different pools. Example: Must design connection usage so that pooling is effective (same string for same logical resource).

**Stale connections**: Long-lived connections can be closed by the server (e.g., timeout). Pool or application must handle "connection closed" and reconnect. Example: ADO.NET and HTTP stacks typically handle this with retries or new connection from pool.

---

## When to Use This Approach

Use connection pooling when:

- **HTTP clients** (calling APIs or services over HTTP/HTTPS). Example: Use `IHttpClientFactory` in ASP.NET Core; avoid `new HttpClient()` per request.

- **Database connections** (ADO.NET, EF Core). Example: Use default pooling with a single connection string per database; open/close or use scoped DbContext so connections are returned to the pool.

- **High-throughput network communication** (many requests per second). Example: Any client that talks to a remote service repeatedly benefits from pooling.

- **Any production service** that creates multiple connections to the same target. Example: Microservices, background jobs, APIs—all should use pooled HTTP and DB access.

**Recommended approach:**
- **HTTP**: Use `IHttpClientFactory`; register named or typed clients; never hold a static `HttpClient` unless you understand lifetime and DNS.
- **Database**: Use one connection string per database; rely on default ADO.NET/EF Core pooling; use appropriate lifetime (e.g., scoped DbContext).
- **Other protocols**: Use a library or pattern that supports connection pooling (e.g., gRPC, Redis clients) when available.

---

## When Not to Use It

Pooling is usually the default; avoid or tune it only in special cases:

- **Single one-off request** (e.g., tool that runs once). Example: Still fine to use `IHttpClientFactory` or a single `HttpClient`; pooling doesn’t hurt.
- **Different security or routing per request** (e.g., different DB users per request). Example: May need separate connection strings and thus separate pools; design so that pooling is still used within each identity.
- **Provider doesn’t support pooling** (rare). Example: Some legacy or custom clients; then minimize connection creation and reuse where possible.

**Note**: In practice, "when not to use" is rare; the main task is to use built-in pooling correctly (IHttpClientFactory, default DB pooling) and tune pool size and timeouts.
---

## Common Mistakes

**Creating new HttpClient per request**: Using `new HttpClient()` in a loop or per call. Example: Leads to socket exhaustion and no reuse. Use `IHttpClientFactory` or a single long-lived `HttpClient` per host.

**Not using IHttpClientFactory**: In ASP.NET Core, creating `HttpClient` manually instead of injecting `IHttpClientFactory`. Example: Miss connection pooling and good DNS/lifetime behavior. Register with `AddHttpClient` and inject the factory.

**Misconfiguring pool size**: Setting DB or HTTP pool too small (queuing) or too large (server overload). Example: Tune based on load tests and server capacity; use default first, then adjust if needed.

**Not disposing connections**: Not disposing `SqlConnection` or response/content so connections are not returned to the pool. Example: Always use `using` or proper disposal so the provider can return the connection to the pool.

**Different connection strings for same DB**: Using slightly different strings (e.g., different options) so the provider creates multiple pools. Example: Use a single, consistent connection string per database so one pool is used.

---

## Example Scenarios

### Scenario 1: HTTP API client

**Problem**: Service calls an external API for every request; creating a new `HttpClient` per call causes high latency and socket exhaustion under load.

**Bad approach** (new HttpClient per request):

```csharp
// ❌ Bad: New HttpClient per request
public async Task<ApiResponse> GetDataAsync(string id)
{
    using var client = new HttpClient();
    var response = await client.GetAsync($"https://api.example.com/data/{id}");
    return await response.Content.ReadFromJsonAsync<ApiResponse>();
}
```

**Good approach** (IHttpClientFactory / pooled):

```csharp
// ✅ Good: Use IHttpClientFactory for connection pooling
public class ApiClient
{
    private readonly IHttpClientFactory _httpClientFactory;

    public ApiClient(IHttpClientFactory httpClientFactory)
    {
        _httpClientFactory = httpClientFactory;
    }

    public async Task<ApiResponse> GetDataAsync(string id)
    {
        var client = _httpClientFactory.CreateClient("ExternalApi");
        var response = await client.GetAsync($"/data/{id}");
        return await response.Content.ReadFromJsonAsync<ApiResponse>();
    }
}
// Registration: services.AddHttpClient("ExternalApi", c => c.BaseAddress = new Uri("https://api.example.com"));
```

**Results:**
- **Bad**: Each request pays connection cost; under load, socket exhaustion and high latency.
- **Good**: Connections reused; lower latency, higher throughput, no socket exhaustion.

---

### Scenario 2: Database access

**Problem**: Each operation opens a new connection; connection overhead and risk of exhausting connections.

**Bad approach** (new connection every time, no reuse):

```csharp
// ❌ Bad: New connection every time (still pooled by ADO.NET, but pattern is fragile)
public async Task<User> GetUserAsync(int id)
{
    var connectionString = Configuration.GetConnectionString("Default");
    var connection = new SqlConnection(connectionString); // Prefer dependency injection
    await connection.OpenAsync();
    try
    {
        // ... query ...
    }
    finally
    {
        await connection.CloseAsync(); // Return to pool
    }
}
```

**Good approach** (reuse connection string, proper disposal so pool is used):

```csharp
// ✅ Good: Single connection string, proper using → ADO.NET pool is used
public class UserRepository
{
    private readonly string _connectionString;

    public UserRepository(IConfiguration configuration)
    {
        _connectionString = configuration.GetConnectionString("Default");
    }

    public async Task<User> GetUserAsync(int id)
    {
        await using var connection = new SqlConnection(_connectionString);
        await connection.OpenAsync(); // From pool
        // ... query ...
        // Dispose returns connection to pool
    }
}
```

**Results:**
- **Bad**: If connection string is recreated or connections are not disposed, pooling may be ineffective or connections leak.
- **Good**: Same connection string and proper disposal; ADO.NET pool is used; 10x–100x better under load.

---

### Scenario 3: Typed HttpClient with pooling

**Problem**: Need a typed client for an external API with connection reuse.

**Approach** (typed client backed by IHttpClientFactory):

```csharp
// ✅ Good: Typed client uses IHttpClientFactory → connection pooling
public interface IMyApiClient
{
    Task<Data> GetDataAsync(string id, CancellationToken cancellationToken = default);
}

public class MyApiClient : IMyApiClient
{
    private readonly HttpClient _httpClient;

    public MyApiClient(HttpClient httpClient)
    {
        _httpClient = httpClient; // Injected by IHttpClientFactory
    }

    public async Task<Data> GetDataAsync(string id, CancellationToken cancellationToken = default)
    {
        var response = await _httpClient.GetAsync($"/data/{id}", cancellationToken);
        response.EnsureSuccessStatusCode();
        return await response.Content.ReadFromJsonAsync<Data>(cancellationToken: cancellationToken);
    }
}
// Registration: services.AddHttpClient<IMyApiClient, MyApiClient>(c => c.BaseAddress = new Uri("https://api.example.com"));
```

**Results:** Typed client uses the same pooled `HttpClient`; connections are reused; no `new HttpClient()` per call.

---

## Summary and Key Takeaways

Connection pooling reuses connections instead of creating new ones per request, reducing overhead and improving performance by 10x–100x in typical scenarios. Use it for HTTP clients (via `IHttpClientFactory`), database connections (default ADO.NET/EF Core pooling), and any high-throughput network communication. The trade-off: pools need sensible configuration and can hold idle connections. Typical benefits: 10x–100x better performance, lower latency, better resource utilization, avoidance of socket exhaustion. Common mistakes: creating new `HttpClient` per request, not using `IHttpClientFactory`, misconfiguring pool size, not disposing connections. Always use built-in pooling (IHttpClientFactory for HTTP, default pooling for DB) and tune pool size and timeouts based on load and capacity.

---

<!-- Tags: Performance, Optimization, Networking, Connection Pooling, .NET Performance, C# Performance, System Design, Architecture, Latency Optimization, Throughput Optimization -->
