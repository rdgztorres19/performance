# Use Binary Protocols for High-Throughput and Low-Payload Scenarios

**Binary protocols (e.g. Protocol Buffers, MessagePack) encode data in compact binary form instead of text (JSON, XML). They typically reduce payload size by 50–80% and serialization/deserialization cost by 3–10x compared to JSON. The trade-off: less human-readable, requires a schema or contract, and less universal tooling. Use binary protocols for internal services, high-throughput APIs, and when payload size or CPU cost of serialization matters.**

---

## Executive Summary (TL;DR)

Binary protocols encode structured data as compact binary bytes instead of text (JSON, XML). Smaller payloads mean less network bandwidth and less time to send/receive; faster serialization and deserialization mean less CPU per message. In practice you often see 50–80% smaller payloads and 3–10x faster serialization than JSON. Use them for internal services, high-throughput or latency-sensitive APIs, and when payload size or CPU is a bottleneck. Trade-offs: not human-readable, require a schema (e.g. .proto for protobuf) or agreed format, and fewer generic tools than JSON. Typical benefits: smaller size, lower latency, higher throughput, less CPU. Common mistakes: using binary for public APIs where JSON is expected, skipping schema versioning, or over-optimizing when JSON is fast enough.

---

## Problem Context

### Understanding the Basic Problem

**What happens when you use JSON (or XML) for every message?**

Imagine an API that sends and receives many small objects per second:

```csharp
// ❌ Text (JSON): larger payload, more CPU to serialize/deserialize
public async Task<Response> CallApiAsync(Request req)
{
    var json = JsonSerializer.Serialize(req);           // Text: "{\"Id\":1,\"Name\":\"Test\",\"Count\":42}"
    var content = new StringContent(json, Encoding.UTF8, "application/json");
    var response = await _httpClient.PostAsync("/api/process", content);
    var responseJson = await response.Content.ReadAsStringAsync();
    return JsonSerializer.Deserialize<Response>(responseJson);
    // Payload: ~40+ bytes for a tiny object; CPU: parse/generate string, escape quotes, numbers as text
}
```

**What happens:**
- **Text encoding**: Numbers and structure are represented as human-readable text (digits, quotes, commas). The same logical data takes more bytes than a compact binary representation.
- **Serialization cost**: The runtime must convert each field to a string (numbers → digits, strings → escaped UTF-8), allocate buffers, and format. Deserialization must scan the string, parse numbers, and build objects.
- **Payload size**: Keys and punctuation are repeated in every message (e.g. `"Id":`, `"Name":`). Over many messages this adds up in bandwidth and latency.

**Why this can be slow or heavy:**
- **Larger payloads**: More bytes to send and receive → more network time and more memory.
- **CPU cost**: Text parsing and generation are more expensive than reading/writing fixed or length-prefixed binary fields.
- **Throughput cap**: When you send millions of messages per second, serialization cost and payload size can dominate.

**With a binary protocol:**

```csharp
// ✅ Binary (e.g. protobuf): smaller payload, less CPU
public async Task<Response> CallApiAsync(Request req)
{
    var bytes = req.ToByteArray();   // Binary: compact, no key names in payload
    var content = new ByteArrayContent(bytes);
    content.Headers.ContentType = new MediaTypeHeaderValue("application/x-protobuf");
    var response = await _httpClient.PostAsync("/api/process", content);
    var responseBytes = await response.Content.ReadAsByteArrayAsync();
    return Response.Parser.ParseFrom(responseBytes);
    // Payload: often 50–80% smaller; CPU: 3–10x faster than JSON for typical structs
}
```

**What happens:**
- **Binary encoding**: Fields are encoded as compact binary (e.g. variable-length integers, length-delimited strings). No repeated key names; types and order come from the schema.
- **Less CPU**: Serialization and deserialization are mostly memory copies and simple numeric encoding; no string parsing or formatting.
- **Smaller payload**: Same logical data in fewer bytes → less bandwidth and often lower latency.

**Improvement**: Typically 50–80% smaller payloads and 3–10x faster serialization/deserialization than JSON for structured messages; use when payload size or CPU is a bottleneck.

### Key Terms Explained (Start Here!)

**What is serialization?** Converting an in-memory object (e.g. a C# class instance) into a form that can be stored or sent over the network (e.g. bytes or a string). Example: `JsonSerializer.Serialize(obj)` turns an object into a JSON string.

**What is deserialization?** The reverse: turning stored or received data (e.g. bytes or a string) back into an in-memory object. Example: `JsonSerializer.Deserialize<MyType>(json)` turns a JSON string into an object.

**What is a schema?** A description of the structure of the data (field names, types, optional/required). JSON often has no formal schema (you just agree on shape). Binary protocols like Protocol Buffers require a schema (e.g. a `.proto` file) so both sides know how to encode and decode. Example: `message Request { int32 id = 1; string name = 2; }`.

**What is a text protocol?** A format that represents data as human-readable text (e.g. JSON, XML). Advantages: easy to inspect and debug; disadvantages: larger size and higher CPU cost than compact binary for the same data.

**What is a binary protocol?** A format that represents data as compact binary (bytes). Advantages: smaller payload and usually faster encode/decode; disadvantages: not human-readable and typically requires a schema or agreed contract. Examples: Protocol Buffers (protobuf), MessagePack.

**What is Protocol Buffers (protobuf)?** A binary serialization format and schema language from Google. You define messages in a `.proto` file; the compiler generates code to serialize/deserialize. Very compact (field numbers instead of names) and fast. Example: gRPC uses protobuf by default.

**What is MessagePack?** A binary format that is a compact, binary equivalent of JSON (same data model: maps, arrays, primitives). No separate schema file; structure is encoded in the stream. Smaller than JSON and usually faster to serialize/deserialize.

**What is payload size?** The size in bytes of the data you send or receive (e.g. the body of an HTTP request or response). Smaller payloads use less bandwidth and often take less time to transfer.

**What is throughput?** Number of operations (or messages) completed per unit of time. Binary protocols often increase throughput by reducing payload size and CPU per message.

**What is compression?** A general-purpose algorithm (e.g. gzip, Brotli, Deflate) that reduces the size of a byte stream by removing redundancy. You can compress any payload—JSON or binary—before sending and decompress after receiving. Compression reduces bandwidth but adds CPU for compress/decompress on both sides. Example: `Content-Encoding: gzip` on HTTP compresses the body; the client decompresses before parsing.

### Common Misconceptions

**"JSON is always fast enough"**
- **The truth**: For low message rates and small payloads, JSON is often fine. For high throughput, large payloads, or CPU-bound serialization, binary protocols can give 3–10x gains in size and speed.

**"Binary protocols are only for gRPC"**
- **The truth**: Protobuf and MessagePack can be used over HTTP, TCP, or any transport. gRPC uses protobuf by default, but you can use binary formats in REST-style APIs too (e.g. `application/x-protobuf`).

**"Binary means no schema"**
- **The truth**: Protobuf requires a schema (.proto). MessagePack does not require a separate schema file but still has an agreed data shape. Schema evolution (adding/removing fields) must be designed (e.g. protobuf’s field numbers and optional fields).

**"Binary is unreadable so we can’t debug"**
- **The truth**: You can log hex dumps or use tools that decode known schemas. For internal services, the gains in size and speed often outweigh the loss of eyeballing raw payloads; you can still add logging of key fields.

---

## How It Works

### Understanding How Binary Encoding Differs from Text

**Text (JSON) for a small object:**
- Keys repeated every time: `"Id":1,"Name":"Test","Count":42` → many bytes for structure.
- Numbers as text: `42` is two ASCII characters; in binary it can be one byte (or a few with variable-length encoding).
- Strings as UTF-8 with quotes and escaping → more bytes than raw UTF-8 length-prefixed.

**Binary (e.g. protobuf):**
- Field identity by number (defined in schema), not by name → no key strings in the payload.
- Numbers encoded as variable-length integers or fixed-size types → minimal bytes.
- Strings as length-prefixed raw bytes → no extra punctuation.
- Result: same logical data in 50–80% fewer bytes in typical cases; encode/decode is mostly memory and simple arithmetic.

**What happens:**
1. **Schema**: For protobuf, you define messages in a `.proto` file; for MessagePack, you rely on the same in-memory structure (maps/arrays) as JSON.
2. **Encode**: Writer writes field number + type + value (e.g. varint, length-delimited string) into a byte buffer.
3. **Decode**: Reader reads bytes, uses schema (or type tags in MessagePack) to know what each byte sequence means, and reconstructs the object.
4. **No parsing of text**: No tokenizer, no number parsing from digits; just copy and numeric decode.

**Key insight**: Binary protocols trade human readability and universal tooling for smaller size and lower CPU cost. When network or CPU is a bottleneck, that trade-off is often worth it for internal or high-performance APIs.

### Binary Protocols vs Compression

**Why compare them?** Both reduce payload size, but they work differently. It’s useful to know when to use binary, when to use compression, and when to use both.

**What compression does:** Compression (e.g. gzip, Brotli) is a general-purpose algorithm that takes any byte stream and shrinks it by removing redundancy. You can compress JSON before sending and decompress on the receiver; the result is smaller over the wire. Compression reduces **bandwidth** but adds **CPU** for compress and decompress on both sides. It does **not** remove the cost of JSON serialization and deserialization: you still serialize to JSON, then compress; on the receiver you decompress, then parse JSON.

**Binary vs compression (different mechanisms):**
- **Binary protocol**: Encodes the *structure* of the data in a compact way (no key names, compact numbers, length-prefixed strings). Smaller payload and **less CPU** for encode/decode than JSON. No separate “compression” step; the format itself is compact.
- **Compression**: Shrinks *any* byte stream (JSON, binary, or other). Reduces size over the wire but adds CPU for compress/decompress. Does not change the fact that JSON still has to be serialized and parsed—so you still pay JSON CPU cost.

**When compression alone helps:** If the main bottleneck is **bandwidth** (e.g. slow or metered links) and you want to keep JSON for readability or tooling, compressing JSON (e.g. `Content-Encoding: gzip`) can cut payload size significantly (often 60–80% for text). You still pay full JSON serialize/parse cost; you only save network time and bandwidth.

**When binary helps more:** When **CPU** for serialization/deserialization is a bottleneck, binary wins: it avoids JSON parse/serialize entirely and is often smaller than JSON even before compression. For small, structured messages, binary (e.g. protobuf) is frequently smaller than *compressed* JSON because binary doesn’t repeat key names at all—compression can’t remove what isn’t there. So: binary → smaller payload and less CPU; compressed JSON → smaller payload but same JSON CPU cost.

**When to use both:** You can compress binary payloads too (e.g. protobuf + gzip). Use this when you need maximum size reduction (e.g. very slow links, or huge batches). Then you pay: binary encode/decode + compress/decompress. Often still faster than JSON + compress because binary encode/decode is cheap.

**Summary:** Compression reduces size over the wire for any format; binary reduces size *and* CPU by changing the format. Use compression when you want to keep JSON and only care about bandwidth; use binary when CPU or total latency matters; use both (binary + compression) when you need the smallest possible payload and can afford the extra CPU.

### Technical Details: Protocol Buffers in .NET

- **Schema**: Define `.proto` files; use `protoc` (or built-in tooling) to generate C# classes (e.g. `Request`, `Response`) with `ToByteArray()` and `Parser.ParseFrom(bytes)`.
- **Usage**: Serialize with `message.ToByteArray()`; deserialize with `MyMessage.Parser.ParseFrom(bytes)`. Use with gRPC or with HTTP (e.g. `ByteArrayContent`, `application/x-protobuf`).
- **Evolution**: Add new fields with new field numbers; keep old numbers for backward compatibility. Optional fields allow old clients to ignore new fields.

### Technical Details: MessagePack in .NET

- **No separate schema**: You serialize your existing C# objects; MessagePack encodes property names (or contract) and values in a compact binary form. You can use attributes to control layout and omit keys (e.g. `[MessagePackObject]`, `[Key(n)]`).
- **Usage**: `MessagePackSerializer.Serialize(obj)` and `MessagePackSerializer.Deserialize<T>(bytes)`. Good when you want smaller and faster than JSON but keep a JSON-like model without a separate .proto step.

---

## Why This Becomes a Bottleneck

Using text (JSON/XML) for everything can become a bottleneck because:

**Payload size**: Larger payloads use more bandwidth and take longer to transfer. On slow or congested links, or when sending many messages per second, size dominates latency and throughput.

**CPU cost**: Text serialization and deserialization involve string handling, parsing, and allocation. At high message rates, serialization can dominate CPU and limit throughput.

**Memory**: Larger payloads and temporary strings increase allocation and GC pressure. Binary formats often reduce allocations and working set.

**Throughput limit**: When each message is serialized and deserialized, the cost per message caps how many messages per second you can process. Binary protocols raise that cap (3–10x in many cases).

---

## Advantages

**Smaller payloads**: Typically 50–80% smaller than JSON for the same logical data. Less bandwidth, less transfer time, often lower latency.

**Faster serialization/deserialization**: Often 3–10x faster than JSON for structured data. Less CPU per message → higher throughput.

**Lower allocation**: Compact encoding and simpler encode/decode paths often mean fewer allocations and less GC pressure than text parsing.

**Good for high throughput**: When you send or receive many messages per second, binary protocols reduce both size and CPU, so you can handle more messages on the same hardware.

**Schema and evolution**: With protobuf, a clear schema and field numbers allow controlled evolution (add/remove/optional fields) with backward compatibility.

---

## Disadvantages and Trade-offs

**Not human-readable**: You can’t eyeball the payload in a browser or log; you need tools or hex dumps. Debugging is harder than with JSON.

**Requires schema or contract**: Protobuf needs a .proto and code generation; MessagePack still requires agreement on types and structure. Schema changes must be managed (versioning, compatibility).

**Less universal tooling**: Many tools (browsers, Postman, ad-hoc scripts) expect JSON. Binary payloads need specific clients or decoders.

**Not a substitute for good API design**: Binary protocols optimize size and CPU; they don’t fix bad APIs or unnecessary data. Use them where the bottleneck is real.

---

## When to Use This Approach

Use binary protocols when:

- **Internal services**: Service-to-service communication where both sides can agree on a schema and don’t need human-readable payloads.
- **High throughput**: You send or receive many messages per second and serialization or payload size is a measurable cost.
- **Latency-sensitive APIs**: Smaller payloads and less CPU can reduce end-to-end latency when serialization or network is in the path.
- **Payload size matters**: Mobile, metered bandwidth, or large messages (e.g. event streams) where 50–80% size reduction helps.
- **gRPC or similar**: gRPC uses protobuf by default; using it keeps a consistent, efficient format.

**Recommended approach:** Use JSON for public-facing or developer-friendly APIs where readability and tooling matter. Use binary (protobuf, MessagePack) for internal, high-throughput, or latency-sensitive paths where size and CPU matter. Version your schema and plan for evolution.

---

## When Not to Use It

- **Public APIs where JSON is expected**: Many clients and tools assume JSON; forcing binary adds friction unless you offer both or target only binary-capable clients.
- **Low message rate, small payloads**: When serialization and payload size are negligible, JSON is simpler and easier to debug; binary adds complexity with little gain.
- **No schema or contract possible**: If you can’t maintain a schema or agree on structure, MessagePack can still help (JSON-like model), but protobuf is a poor fit.
- **Debugging and ad-hoc inspection are top priority**: If you need to inspect every request/response as text often, JSON may be the better default.

---

## Common Mistakes

**Using binary for every API**: Use binary where size and CPU matter; keep JSON where readability and ecosystem matter.

**Skipping schema versioning**: Adding or removing fields without a strategy breaks clients. Use field numbers (protobuf), optional fields, and versioned APIs.

**Assuming binary fixes all performance issues**: Binary helps serialization and payload size; it doesn’t fix bad queries, N+1 calls, or missing indexes. Measure first.

**Ignoring compatibility**: Changing field semantics or reusing field numbers in protobuf can break existing clients. Plan evolution and test backward/forward compatibility.

**Over-optimizing**: If JSON is fast enough for your scale, binary adds complexity (schema, tooling) for little benefit. Profile and then decide.

---

## Example Scenarios

### Scenario 1: High-throughput internal API (protobuf)

**Problem**: Internal service processes millions of small requests per second; JSON serialization and payload size are consuming CPU and bandwidth.

**Bad approach** (JSON for everything):

```csharp
// ❌ Bad: JSON for high-throughput internal API
var json = JsonSerializer.Serialize(request);
var content = new StringContent(json, Encoding.UTF8, "application/json");
var response = await _httpClient.PostAsync("/process", content);
var result = await JsonSerializer.Deserialize<Response>(await response.Content.ReadAsStringAsync());
```

**Good approach** (protobuf):

```csharp
// ✅ Good: Protobuf for internal API
var bytes = request.ToByteArray();
var content = new ByteArrayContent(bytes);
content.Headers.ContentType = new MediaTypeHeaderValue("application/x-protobuf");
var response = await _httpClient.PostAsync("/process", content);
var responseBytes = await response.Content.ReadAsByteArrayAsync();
var result = Response.Parser.ParseFrom(responseBytes);
```

**Results:**
- **Bad**: Larger payloads, higher CPU for serialize/deserialize; throughput limited by JSON cost.
- **Good**: 50–80% smaller payloads, 3–10x faster encode/decode; higher throughput and lower latency when serialization/network matter.

---

### Scenario 2: Smaller payload without .proto (MessagePack)

**Problem**: You want smaller and faster than JSON but don’t want to introduce .proto and code generation yet.

**Approach** (MessagePack):

```csharp
// ✅ Good: MessagePack — binary, compact, no separate schema file
var bytes = MessagePackSerializer.Serialize(new MyMessage { Id = 1, Name = "Test", Count = 42 });
var content = new ByteArrayContent(bytes);
content.Headers.ContentType = new MediaTypeHeaderValue("application/msgpack");
// ... send ...
var result = MessagePackSerializer.Deserialize<MyMessage>(responseBytes);
```

**Results:** Smaller payload than JSON, faster serialize/deserialize, same C# types; no .proto step. Good middle ground when you want binary benefits without protobuf tooling.

---

### Scenario 3: gRPC (protobuf by default)

**Problem**: You need RPC between services with low latency and high throughput.

**Approach** (gRPC uses protobuf):

```csharp
// ✅ Good: gRPC = protobuf by default
// .proto defines service and messages; generated client/server use binary encoding
var reply = await _grpcClient.ProcessAsync(new Request { Id = 1, Name = "Test" });
// Request and reply are serialized with protobuf; one round-trip, compact, fast
```

**Results:** Binary encoding by default; small payloads and fast serialization; streaming and tooling built on top. Use when you want RPC + binary in one stack.

---

## Summary and Key Takeaways

Binary protocols (e.g. Protocol Buffers, MessagePack) encode data in compact binary form, typically giving 50–80% smaller payloads and 3–10x faster serialization/deserialization than JSON. Use them for internal services, high-throughput or latency-sensitive APIs, and when payload size or CPU is a bottleneck. Trade-offs: not human-readable, require a schema or contract (protobuf) or agreed structure (MessagePack), and less universal tooling than JSON. Use JSON for public or developer-friendly APIs; use binary where size and CPU matter. Version schemas and plan evolution; don’t assume binary fixes all performance issues—measure first.

---

<!-- Tags: Performance, Optimization, Networking, Network Optimization, .NET Performance, C# Performance, Latency Optimization, Throughput Optimization, System Design, Architecture -->
