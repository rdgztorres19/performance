# Running the demos in Docker

Every topic ships as its own Compose file, and everything is **disabled by
default**. You turn on the topics you care about in `.env`, then run one
command. Only the enabled topics are created.

## Quick start

```bash
cd perf-test-apps

# 1. See what exists and what is on (nothing, on a fresh checkout)
./run.sh list

# 2. Enable a topic
#    edit .env and set:  ENABLE_09_RANDOM_IO=true

# 3. Run it
./run.sh up
```

Each enabled topic starts up to four containers so you can compare directly:

```
09-random-io-py-incorrect     09-random-io-py-correct
09-random-io-js-incorrect     09-random-io-js-correct
```

They run side by side and print their results to the same log stream, so the
difference between the bad and the good implementation is visible in one place.

## Commands

| Command | What it does |
|---|---|
| `./run.sh list` | Show all 39 topics, which are enabled, and their languages |
| `./run.sh up` | Build the images and run every enabled topic |
| `./run.sh up random-io` | Run one topic, ignoring its `.env` switch |
| `./run.sh build` | Build the two runner images without running anything |
| `./run.sh logs` | Follow the logs of running demos |
| `./run.sh down` | Stop and remove all demo containers |

## The .env file

`.env` is the only file you edit. `.env.example` is the committed default with
every topic set to `false`; `.env` is created from it on first run.

### Global settings

| Variable | Default | Meaning |
|---|---|---|
| `DURATION_SEC` | `20` | Seconds each long-running demo runs. The scripts themselves default to 300. |
| `RUN_PYTHON` | `true` | Start the Python containers |
| `RUN_NODE` | `true` | Start the Node containers |
| `RUN_INCORRECT` | `true` | Start the anti-pattern containers |
| `RUN_CORRECT` | `true` | Start the good-practice containers |
| `CPU_LIMIT` | `2` | CPU cores per container |
| `MEMORY_LIMIT` | `1g` | Memory per container |

Pinning CPU and memory matters here. Several demos are about CPU contention and
scheduling, so a fixed core count makes runs comparable between machines.

### Topic switches

One line per topic, all `false` by default:

```bash
ENABLE_01_CONTEXT_SWITCHING=false      # 01-context-switching  [python, node]
ENABLE_09_RANDOM_IO=false              # 09-random-io          [python, node]
ENABLE_37_CONNECTION_POOLING=false     # 37-connection-pooling [python]
```

Set one to `true` and it is included on the next `./run.sh up`. Everything left
at `false` is never built and never started.

## Examples

```bash
# Compare only Python, only the two I/O topics
#   .env: ENABLE_09_RANDOM_IO=true, ENABLE_21_IO_CHUNK_SIZES=true, RUN_NODE=false
./run.sh up

# Longer run for a more stable measurement
DURATION_SEC=120 ./run.sh up

# Run a single topic without touching .env
./run.sh up false-sharing

# Only look at the anti-patterns
#   .env: RUN_CORRECT=false
./run.sh up
```

## How it works

Compose has no boolean "enable this service" flag, so each service is assigned a
[profile](https://docs.docker.com/compose/profiles/) named after its switch, for
example `ENABLE_09_RANDOM_IO_PY_CORRECT`. `run.sh` reads `.env`, works out which
profiles should be active, and passes only those to `docker compose`. A service
whose profile is not requested is never created.

Two images are built once and shared by all 115 services:

- `perf-demo-python:local` from `docker/Dockerfile.python`
- `perf-demo-node:local` from `docker/Dockerfile.node`

Neither installs anything. Every demo uses only the standard library of its
language, so the images are just the official base plus the source tree.

Each container mounts a 512 MB `tmpfs` at `/data` and sets `TMPDIR=/data`, so the
file-I/O demos write to a scratch area that disappears with the container and
never touches your working tree.

## Language coverage

| Language | Topics | Why |
|---|---|---|
| Python | 39 | Every topic |
| JavaScript | 17 | Where Node exposes the needed API |
| C | 12 | The low-level topics the other two cannot demonstrate |

`./run.sh list` shows the languages per topic. A topic with all three starts six
containers; one with only Python starts two.

### Why C exists here

Python and JavaScript genuinely cannot demonstrate about a dozen of these
topics. The interpreter, the GIL, and the garbage collector sit between the code
and the CPU and dominate the measurement. This is not a theoretical objection:
measuring every Python and JavaScript pair in this repo showed that only 28 of
54 produced a clear result, and `06-false-sharing` came out **backwards**,
because the GIL prevents two threads from writing the same cache line
concurrently, which is the very condition the topic is about.

C compiles to machine code with no runtime in between, so the hardware effect is
what gets measured. The difference is large:

Measured in the Linux container with `CPU_LIMIT=8`:

| Topic | Python | C |
|---|---|---|
| 11 Lock contention | 1.01x (no signal) | **800x** |
| 15 Memory access patterns | 12.9x | **14x**, and honest |
| 04 Memory allocation | not measurable | **10x** |
| 27 Zero-copy | not measurable | **10x** |
| 01 Context switching | 1.00x (no signal) | **7x** |
| 26 Page faults | not measurable | **6x** |
| 28 Cache-friendly layouts | not measurable | **4.5x** |
| 13 Memory pooling | not measurable | **2.3x** |
| 06 False sharing | 0.34x (backwards) | **1.8x**, correct direction |
| 05 Branch misprediction | no signal | **1.6x** |
| 29 Memory barriers | not measurable | 15,259 real anomalies vs 0 |
| 25 Thread affinity | not measurable | Linux only; see below |

Two of these deserve their honest caveat rather than a flattering number:

- **05-branch-misprediction** is only 1.6x, not the 3-6x the classic x86 demo
  reports. At `-O2` the compiler first auto-vectorizes the loop and then
  if-converts it to a conditional select, leaving no branch to mispredict at
  all. The demo forces a real branch by making the taken path expensive enough
  that the compiler declines to speculate it. 1.6x is what survives that. The
  file documents this, because "simplifying" the hot path turns the demo back
  into a no-op.
- **01-context-switching** does not reproduce from oversubscription alone.
  500 CPU-bound threads versus one per core measured within 3% on modern
  schedulers, which give long time slices. The reproducible cost is thread
  *creation*: 100,000 `pthread_create` calls versus 2,800 for identical work.

The C topics are: 01, 04, 05, 06, 11, 13, 15, 25, 26, 27, 28, 29.

### Reading the C results

Two of these measure correctness rather than speed:

- **29-memory-barriers** counts how many times the reader observed the ready
  flag before the data it was supposed to publish. The incorrect version
  reports a non-zero count on weakly-ordered CPUs (arm64); the correct version
  is structurally zero. A zero on the incorrect version means the race was not
  observed in that run, not that the code is correct.
- **25-thread-affinity** prints whether pinning is `ACTIVE` or `UNAVAILABLE`.
  Linux supports `pthread_setaffinity_np`, so it is ACTIVE in these containers.
  macOS has no equivalent API, so running the file natively there reports
  UNAVAILABLE and both versions perform identically, which is the honest result.

The multi-core topics need real parallelism. Raise `CPU_LIMIT` to 8 or more
before drawing conclusions from 01, 06, 11, or 25: `06-false-sharing` measures
1.1x at `CPU_LIMIT=2` and 1.8x at `CPU_LIMIT=8`.

### Measuring with hardware counters

The C image includes `procps` and `time`. On a Linux host with `perf` available,
the demos are designed to be proven with hardware counters, and each file's
header names the exact ones:

```bash
perf stat -e branches,branch-misses      ./05-branch-misprediction/incorrect
perf stat -e cache-misses,LLC-load-misses ./15-memory-access-patterns/incorrect
perf stat -e context-switches,cpu-migrations ./01-context-switching/incorrect
```

## Topics needing a helper server

`37-connection-pooling` and `38-batch-network` measure network behavior, so they
need something to talk to. Each ships a small standard-library TCP server that
Compose starts automatically as an extra container on the project network.

The clients find it through `SERVER_HOST` and `SERVER_PORT`, which the compose
file sets to the server's container name. Running a script directly outside
Docker still works: both variables fall back to `127.0.0.1` and the original
port, so `python3 38-batch-network/_batch_server.py` in one terminal and the
demo in another behaves as before.

The server containers are gated behind a healthcheck that connects to their own
port, so the demo containers do not start until the server is actually accepting
connections. Because these servers run until stopped, `run.sh` waits only on the
demo containers and then tears the whole stack down.

## Troubleshooting

**Nothing runs.** Everything is disabled by default. Check `./run.sh list` and
set at least one topic to `true` in `.env`.

**A demo seems to hang.** Long-running demos honor `DURATION_SEC`; one-shot
demos (fsync, heap fragmentation, binary protocols) finish in well under a
second and ignore it. If a container really is stuck, `./run.sh down` removes
everything the project created.

**Results differ between runs.** These are timing measurements on a shared
machine. Pin `CPU_LIMIT` and close other workloads for comparable numbers, and
raise `DURATION_SEC` so each run averages over more work.
