# Linux: Qué se puede medir para application performance

Referencia de métricas y herramientas en Linux. **Separado por alcance**: qué afecta al **sistema en general** (recursos compartidos, bottleneck de máquina) vs qué afecta **por aplicación** (métricas específicas de un proceso).

---

## A. Afecta en general (sistema)

Métricas que reflejan el estado global de la máquina, recursos compartidos y cuellos de botella a nivel de sistema. Afectan a todas las apps en el host.

### A.1 CPU (sistema)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| CPU % por core | `mpstat -P ALL 1` | Uso por núcleo (user + system + idle) |
| Run queue | `vmstat 1` (columna `r`) | Procesos ejecutables esperando CPU |
| Load average | `uptime`, `cat /proc/loadavg` | Promedio runnable + uninterruptible |
| Context switches totales | `vmstat 1` (cs) | Cambios de contexto en todo el sistema |
| Interrupciones por CPU | `cat /proc/interrupts`, `mpstat -I SUM 1` | IRQ por núcleo |
| Softirq | `cat /proc/softirqs` | net_rx, net_tx, block, etc. |

### A.2 Memoria (sistema)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Memoria total, usada, libre | `free -h`, `vmstat 1` | Total, usada, libre, buffers, cache |
| Swap usado | `free` | Swap total del sistema |
| OOM kills | `dmesg \| grep -i "out of memory"`, `journalctl -k -b \| grep -i oom` | Procesos matados por OOM |
| Presión de memoria | `cat /proc/pressure/memory` (PSI, kernel 4.20+) | some/full de memoria (sistema) |

### A.3 I/O (sistema)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Throughput read/write por dispositivo | `iostat -x 1`, `dstat -d` | KB/s o MB/s por disco |
| IOPS por dispositivo | `iostat -x 1` (r/s, w/s) | Operaciones por segundo por disco |
| Latencia I/O por dispositivo | `iostat -x 1` (await, svctm) | Tiempo de espera y servicio |
| Queue depth por dispositivo | `iostat -x 1` (avgqu-sz) | Cola de I/O por disco |
| I/O bloqueado (b) | `vmstat 1` (columna `b`) | Procesos en uninterruptible sleep (I/O) |

### A.4 Red (sistema)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Conexiones por estado | `ss -s`, `netstat -an` | ESTABLISHED, TIME_WAIT, etc. |
| Throughput por interfaz | `iftop`, `vnstat`, `ip -s link` | Bytes/packets por interfaz |
| Errores de red | `ifconfig`, `ip -s link` | Drops, errors por interfaz |
| Retransmisiones TCP | `netstat -s` (TcpRetransSegs) | Retransmisiones totales |
| Conexiones máximas (listen) | `cat /proc/sys/net/core/somaxconn` | Límite de cola de listen |

### A.5 Scheduler y carga (sistema)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Run queue length | `vmstat 1`, `cat /proc/loadavg` | Procesos en runnable |
| Wait queue (D state) | `vmstat 1` (b) | Procesos bloqueados en I/O |

### A.6 Límites y kernel (sistema)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| File descriptors totales | `cat /proc/sys/fs/file-nr` | FD abiertos en el sistema |
| Límites del kernel | `sysctl -a` (parcial) | Parámetros del kernel |
| Límite por proceso (ej. FD) | `ulimit -a` (en shell) | Límites por defecto para nuevos procesos |

### A.7 Histórico (sar)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| CPU histórico | `sar -u` | %user, %system, %idle |
| Memoria histórico | `sar -r` | Memoria, swap |
| I/O histórico | `sar -d` | Disco por dispositivo |
| Red histórico | `sar -n DEV` | Tráfico por interfaz |
| Context switches histórico | `sar -w` | Context switches totales |

---

## B. Afecta por aplicación (por proceso)

Métricas específicas de un proceso (PID). Sirven para diagnosticar el rendimiento de una app concreta.

### B.1 CPU (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| CPU % por proceso | `top`, `htop`, `pidstat -p <pid> 1` | % user + % system |
| CPU % por hilo | `top -H -p <pid>`, `pidstat -t -p <pid> 1` | % CPU por thread |
| Tiempo en user vs system | `pidstat -p <pid> 1` | %usr vs %system |
| Tiempo CPU total acumulado | `ps -o pid,time -p <pid>` | CPU time del proceso |
| Branch mispredictions | `perf stat -e branches,branch-misses -p <pid>` | branches vs branch-misses |
| Instruction cache misses | `perf stat -e L1-icache-load-misses,L1-icache-loads -p <pid>` | Fallos L1 instrucciones |
| Cycles, instructions, IPC | `perf stat -e cycles,instructions -p <pid>` | IPC = instructions/cycles |
| Stalled cycles | `perf stat -e cycles,stalled-cycles-frontend,stalled-cycles-backend -p <pid>` | Ciclos parados |
| Cache L1/L2/L3 misses | `perf stat -e L1-dcache-loads,L1-dcache-load-misses,LLC-loads,LLC-load-misses -p <pid>` | Fallos de caché de datos |
| Migraciones entre cores | `perf stat -e migrations -p <pid>` | Migraciones del proceso |
| CPU donde corre cada hilo | `ps -eLo pid,tid,psr,comm -p <pid>` | CPU actual por TID |
| CPU affinity | `taskset -p <pid>` | Máscara de CPUs permitidas |

### B.2 Context switching (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Context switches por proceso | `pidstat -w -p <pid> 1` | voluntary + nonvoluntary |
| Context switches por hilo | `pidstat -wt -p <pid> 1` | Por thread |
| Switches en /proc | `cat /proc/<pid>/status` (voluntary_ctxt_switches, nonvoluntary_ctxt_switches) | Acumulado del proceso |

### B.3 Syscalls (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Syscalls por tipo | `strace -c -p <pid>` (Ctrl+C para resumen) | Conteo por syscall |
| Syscalls en tiempo real | `strace -p <pid>`, `strace -f -p <pid>` | Traza de llamadas |
| Syscalls agregados | `perf trace -p <pid>` | Menos invasivo que strace |
| Syscalls de archivos | `strace -c -e trace=open,openat,read,write,close,fsync -p <pid>` | Solo operaciones de archivo |
| fsync / fdatasync | `strace -e trace=fsync,fdatasync,sync -f -p <pid>` | Frecuencia de sync a disco |

### B.4 Memoria (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| RSS, VSS | `top`, `ps aux`, `cat /proc/<pid>/status` (VmRSS, VmSize) | Resident y virtual |
| Memoria por thread | `cat /proc/<pid>/task/<tid>/status` | Por hilo |
| Detalle de memoria | `pmap -x <pid>`, `smem -p <pid>` | Mapas y shared/private |
| Swap por proceso | `cat /proc/<pid>/status` (VmSwap) | Swap usado por el proceso |
| Minor page faults | `cat /proc/<pid>/status` (minflt) | Fallos sin I/O de disco |
| Major page faults | `cat /proc/<pid>/status` (majflt) | Fallos con I/O de disco |
| Page faults en tiempo real | `pidstat -r -p <pid> 1` | Majflt y minflt por segundo |
| Page faults con perf | `perf stat -e page-faults,minor-faults,major-faults -p <pid>` | Fallos de página |

### B.5 I/O (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| I/O por proceso | `iotop -o`, `pidstat -d -p <pid> 1` | Lecturas/escrituras KB/s |
| Archivos abiertos | `lsof -p <pid>`, `ls -l /proc/<pid>/fd` | FD y archivos abiertos |
| I/O por archivo | `strace -e trace=open,read,write -p <pid>` | Operaciones por archivo |
| Límite de FD | `cat /proc/<pid>/limits` | Soft/hard limit de FD |

### B.6 Red (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Conexiones por proceso | `ss -tunap \| grep <pid>`, `lsof -p <pid> -i` | Sockets y conexiones TCP/UDP |
| Tráfico por proceso | `nethogs` | Bytes in/out por proceso |
| Puertos en uso (por proceso) | `ss -tunap \| grep <pid>` | Puertos que usa el proceso |

### B.7 Proceso y threads
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Estado del proceso | `ps -o pid,state,wchan -p <pid>` | R, S, D, Z, … |
| Prioridad, nice | `ps -o pid,ni,pri -p <pid>` | Nice value y prioridad |
| Número de threads | `ps -o pid,nlwp -p <pid>`, `cat /proc/<pid>/status` (Threads) | Hilos del proceso |
| Lista de threads | `ps -eLo pid,tid,state,wchan,comm -p <pid>` | Todos los TIDs |
| CPU por thread | `top -H -p <pid>` | % CPU por TID |
| Stack traces | `gdb -p <pid> -ex "thread apply all bt" -ex quit` | Backtraces de hilos |
| Locks (futex) | `perf record -e sched:sched_switch -p <pid>`, análisis de futex | Contención en futex |
| Scheduler stats | `cat /proc/<pid>/sched` | runnable time, sum_exec_runtime, nr_switches |

### B.8 NUMA (por proceso)
| Métrica | Herramienta | Descripción |
|---------|-------------|-------------|
| Localidad NUMA | `numastat -p <pid>` | Memoria local vs remota |
| Política NUMA | `numactl --show` (al lanzar) | Política aplicada al proceso |

### B.9 Profiling (por proceso)
| Comando | Descripción |
|---------|-------------|
| `perf stat -p <pid>` | Contadores (cycles, instructions, cache, branches) |
| `perf record -g -p <pid>` + `perf report` | CPU flame graph |
| `perf record -e sched:sched_switch -p <pid>` | Scheduler events |
| `perf record -e block:block_rq_issue -p <pid>` | I/O block events |

---

## C. Herramientas: qué sirve para qué

| Herramienta | En general (sistema) | Por aplicación |
|-------------|----------------------|----------------|
| `vmstat` | ✅ CPU, memoria, I/O, cs, run queue | — |
| `mpstat` | ✅ CPU por core | — |
| `iostat` | ✅ Disco por dispositivo | — |
| `free` | ✅ Memoria, swap | — |
| `pidstat` | — | ✅ CPU, mem, I/O, cs, faults por PID |
| `top` / `htop` | Vista global | ✅ Por proceso (con filtro) |
| `strace` | — | ✅ Syscalls por PID |
| `iotop` | Vista global | ✅ I/O por proceso |
| `perf stat` | Sin `-p` = sistema | Con `-p <pid>` = por app |
| `perf record` | — | ✅ Por PID |
| `ss` / `lsof` | Vista global | ✅ Filtrado por PID |
| `numastat` | Vista global | ✅ Con `-p <pid>` |
| `sar` | ✅ Histórico sistema | — |

---

## D. Relación con el Performance Resume

| Concepto del resume | En general | Por aplicación |
|---------------------|------------|----------------|
| Branch misprediction | — | `perf stat -e branch-misses -p <pid>` |
| Context switching | `vmstat` (cs) | `pidstat -w -p <pid>`, `/proc/<pid>/status` |
| Page faults | — | `pidstat -r -p <pid>`, `perf stat -e page-faults -p <pid>` |
| Cache misses | — | `perf stat -e cache-misses -p <pid>` |
| I/O throughput | `iostat` (disco) | `pidstat -d -p <pid>`, `iotop` |
| fsync overhead | — | `strace -e fsync,fdatasync -p <pid>` |
| Lock contention | — | `perf lock`, backtraces en futex |
| Thread migration | — | `perf stat -e migrations -p <pid>`, `ps -eLo psr` |
| Syscall overhead | — | `strace -c -p <pid>`, `perf trace -p <pid>` |
| Memory usage | `free`, `vmstat` | `smem -p <pid>`, `pmap`, `/proc/<pid>/smaps` |
| OOM | `dmesg`, `journalctl` | Ver qué proceso mató OOM killer |
| NUMA locality | `numactl --show` | `numastat -p <pid>` |

---

## E. Timeline: qué verificar cuando hay problemas de performance

Orden cronológico para ir al grano. Cada paso: mirar → si hay señal → profundizar; si no → siguiente.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. CARGA Y CPU (5 seg)                                                      │
│    uptime  →  load avg alto?  →  vmstat 1  (r, b)  →  mpstat -P ALL 1       │
│    Si r >> cores → CPU saturado. Si b alto → I/O bloqueando.                 │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. ¿QUÉ PROCESO? (10 seg)                                                   │
│    top -o %CPU  o  pidstat 1 5  →  Anotar PID del proceso sospechoso         │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 3. MEMORIA (5 seg)                                                          │
│    free -h  →  swap usado?  →  cat /proc/<pid>/status (VmRSS, VmSwap)        │
│    dmesg | grep -i oom  →  ¿OOM kills recientes?                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 4. I/O (5 seg)                                                              │
│    iostat -x 1 3  →  await alto? r/s+w/s alto?  →  iotop -o  (qué proceso)   │
│    pidstat -d -p <pid> 1  →  I/O del proceso                                │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 5. CONTEXT SWITCHES (5 seg)                                                 │
│    vmstat 1 (cs)  →  cs muy alto?  →  pidstat -w -p <pid> 1                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                        │
                                        ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 6. PROFUNDIZAR EN EL PROCESO (según síntoma)                                │
│    • CPU alto pero poco útil?  →  perf stat -p <pid>  (IPC, branches)        │
│    • Syscalls sospechosos?     →  strace -c -p <pid>  (Ctrl+C resumen)       │
│    • Bloqueos / locks?         →  perf record -e sched:sched_switch -p <pid> │
│    • Page faults?              →  pidstat -r -p <pid> 1                      │
│    • Memoria detallada?        →  smem -p <pid>, pmap -x <pid>               │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Resumen en una línea (orden)

```
uptime → top/pidstat (PID) → free → iostat → vmstat (cs) → pidstat -w -p PID → perf/strace según síntoma
```
