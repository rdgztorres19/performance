# Performance Test Apps

Demostraciones de anti-patrones vs. prácticas correctas basadas en el resume. Cada carpeta tiene `incorrect.py`/`incorrect.js` (forma mala) y `correct.py`/`correct.js` (forma buena).

## Estructura

```
perf-test-apps/
  NN-nombre-tema/
    incorrect.py    # Forma incorrecta
    correct.py      # Forma correcta
    incorrect.js
    correct.js
```

## Ejecución

Por defecto muchos scripts corren **5 minutos** (DURATION_SEC=300). Ctrl+C para parar. Algunos son one-shot (fsync, heap-fragmentation).

```bash
# Python
cd perf-test-apps
DURATION_SEC=60 python3 01-context-switching/incorrect.py
python3 01-context-switching/correct.py

# Node.js
node 01-context-switching/incorrect.js
node 01-context-switching/correct.js
```

## Tópicos

| Carpeta | Tema | Cómo medir |
|---------|------|------------|
| 01-context-switching | Demasiados threads vs cores | vmstat 1, pidstat -w |
| 02-busy-wait | Spin loop vs Event/sleep | top, vmstat |
| 03-per-item-batching | Per-item vs batch | perf stat |
| 04-memory-allocation | Allocations vs pooling | tracemalloc, --trace-gc |
| 05-branch-misprediction | Ramas impredecibles vs sorted | perf stat -e branch-misses |
| 06-false-sharing | Shared array vs local | perf stat |
| 07-exceptions-control-flow | try/except vs check | tiempo |
| 08-closures-allocations | Closure vs parámetro | tiempo |
| 09-random-io | Random vs sequential I/O | iostat -x 1 |
| 10-blocking-io | Sync vs async I/O | pidstat |
| 11-lock-contention | Lock vs Atomics | pidstat -w |
| 12-many-small-files | Muchos archivos vs uno | iostat, time |
| 13-memory-pooling | new buffer vs pool | tiempo |
| 14-heap-fragmentation | Mixed allocs vs grouped | one-shot |
| 15-memory-access-patterns | Random vs sequential access | tiempo |
| 16-write-batching | flush per write vs batch | tiempo |
| 17-append-only | Update in place vs append | tiempo |
| 18-avoid-fsync | fsync cada write vs batch | one-shot |
| 19-preallocate-file | Grow vs preallocate | tiempo |
| 20-stream-files | read() todo vs stream | tiempo |
| 21-io-chunk-sizes | Buffer 64B vs 64KB | tiempo |
| 22-file-locks | Exclusive vs shared | tiempo |
| 23-thread-pools | new Thread vs pool | tiempo |
| 24-memory-mapped-io | read vs mmap | tiempo (solo Python) |

## Tópicos no implementables como script

- **Prefer Fewer Fast Cores**: hardware
- **Thread Affinity**: requiere privilegios / APIs específicas
- **Stack Allocation**: no stackalloc en Python/Node
- **Memory Barriers**: demasiado low-level
- **Compression tradeoff**: depende del bottleneck I/O vs CPU
- **Connection pooling**: requiere servidor real
- **Batch network**: requiere API batch
- **Binary protocols**: requiere deps (msgpack, protobuf)

## Herramientas Linux

```bash
vmstat 1
pidstat -w -p <pid> 1
iostat -x 1
perf stat -e cycles,instructions,branches,branch-misses python script.py
```
