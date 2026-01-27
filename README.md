# Performance Optimization Checklist

Una guía completa de técnicas y estrategias para optimizar el rendimiento de aplicaciones, con enfoque especial en .NET y C#.

## Tabla de Contenidos

1. [Hardware and Operating System](#hardware-and-operating-system)
2. [Memory Management](#memory-management)
3. [Disk and Storage](#disk-and-storage)
4. [File IO](#file-io)
5. [Networking and IO](#networking-and-io)
6. [Databases](#databases)
7. [Caching](#caching)
8. [Message Queues and Event Streaming](#message-queues-and-event-streaming)
9. [Search Engines](#search-engines)
10. [Concurrency and Threading](#concurrency-and-threading)
11. [Data Structures](#data-structures)
12. [Algorithms](#algorithms)
13. [System Design](#system-design)
14. [.NET and C# Performance](#net-and-c-performance)
15. [Logging and Observability](#logging-and-observability)
16. [Media and Content Optimization](#media-and-content-optimization)
17. [Compilation and Code Generation](#compilation-and-code-generation)
18. [Measurement and Optimization](#measurement-and-optimization)
19. [Performance Anti Patterns](#performance-anti-patterns)

---

## Hardware and Operating System

Esta sección cubre optimizaciones a nivel de hardware y sistema operativo que pueden impactar significativamente el rendimiento de las aplicaciones.

### Prefer fewer fast CPU cores over many slow ones depending on workload (OK)

**Cómo funciona:**
La elección entre muchos núcleos lentos versus pocos núcleos rápidos depende del tipo de carga de trabajo. Para aplicaciones con alta concurrencia y paralelismo (como servidores web con muchas conexiones simultáneas), más núcleos pueden ser beneficiosos. Sin embargo, para cargas de trabajo que requieren baja latencia y procesamiento secuencial rápido, núcleos más rápidos con mejor IPC (Instructions Per Cycle) son preferibles.

**Ventajas:**
- Núcleos rápidos ofrecen mejor rendimiento single-threaded
- Menor latencia para operaciones críticas
- Mejor eficiencia energética en algunos casos
- Menos overhead de sincronización entre núcleos

**Desventajas:**
- Menor capacidad de paralelización
- Puede ser más costoso
- No escala bien para cargas altamente paralelas

**Cuándo usar:**
- Aplicaciones con operaciones críticas de baja latencia
- Procesamiento de datos secuencial
- Aplicaciones que no pueden paralelizarse fácilmente
- Sistemas en tiempo real

**Impacto en performance:**
Puede mejorar el rendimiento single-threaded en un 20-40% dependiendo de la arquitectura. Reduce la latencia de operaciones críticas significativamente.

---

### Reduce context switching (OK)

**Cómo funciona:**
El context switching ocurre cuando el sistema operativo cambia de un proceso/hilo a otro. Cada cambio requiere guardar el estado del proceso actual (registros, stack pointer, etc.) y restaurar el estado del nuevo proceso. Esto consume tiempo de CPU y puede invalidar cachés.

**Ventajas:**
- Reduce overhead de CPU
- Mejora la localidad de caché
- Menor latencia en operaciones críticas
- Mejor utilización de recursos

**Desventajas:**
- Puede reducir la capacidad de respuesta del sistema
- Requiere diseño cuidadoso de la aplicación
- Puede afectar la equidad en el scheduling

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Sistemas con cargas de trabajo bien definidas
- Cuando el overhead de context switching es medible

**Impacto en performance:**
Reducir context switches puede mejorar el rendimiento en un 5-15% en aplicaciones intensivas en CPU. En sistemas con muchos hilos, el impacto puede ser mayor.

**Ejemplo en C#:**
```csharp
// Evitar crear demasiados threads - usar ThreadPool
// ❌ Malo: Crear muchos threads manualmente
for (int i = 0; i < 1000; i++)
{
    var thread = new Thread(() => DoWork(i));
    thread.Start(); // Muchos context switches
}

// ✅ Bueno: Usar ThreadPool que reutiliza threads
Parallel.For(0, 1000, i => DoWork(i));

// O mejor aún, usar async/await que es más eficiente
await Task.WhenAll(Enumerable.Range(0, 1000)
    .Select(i => DoWorkAsync(i)));
```

---

### CPU affinity and pinning (Ok)

**Cómo funciona:**
CPU affinity permite asignar procesos o threads a núcleos específicos de CPU. Esto evita que el sistema operativo mueva el proceso entre núcleos, mejorando la localidad de caché y reduciendo migraciones de procesos.

**Ventajas:**
- Mejora la localidad de caché (L1, L2, L3)
- Reduce migraciones entre núcleos
- Mejor rendimiento predecible
- Útil para aplicaciones en tiempo real

**Desventajas:**
- Puede causar desbalanceo de carga
- Reduce flexibilidad del scheduler del OS
- Requiere conocimiento de la arquitectura del hardware
- Puede empeorar el rendimiento si no se configura correctamente

**Cuándo usar:**
- Aplicaciones de tiempo real
- Sistemas con NUMA
- Cuando la localidad de caché es crítica
- Aplicaciones con patrones de acceso a memoria predecibles

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% en aplicaciones sensibles a la localidad de caché. En sistemas NUMA, el impacto puede ser mayor (hasta 30-40%).

**Ejemplo en C#:**
```csharp
using System;
using System.Diagnostics;
using System.Threading;

// En .NET, puedes usar Process.ProcessorAffinity
var process = Process.GetCurrentProcess();
process.ProcessorAffinity = new IntPtr(0x0001); // Pinar a CPU 0

// Para threads específicos, usar Thread.BeginThreadAffinity
Thread.BeginThreadAffinity();
try
{
    // Código que debe ejecutarse en el mismo thread
    PerformCriticalWork();
}
finally
{
    Thread.EndThreadAffinity();
}
```

---

### NUMA awareness

**Cómo funciona:**
NUMA (Non-Uniform Memory Access) es una arquitectura donde diferentes partes de la memoria tienen diferentes tiempos de acceso dependiendo de la distancia al procesador. NUMA awareness implica asignar memoria y threads de manera que accedan a la memoria local del nodo NUMA, evitando acceso remoto que es más lento.

**Ventajas:**
- Reduce latencia de acceso a memoria
- Mejora el ancho de banda de memoria
- Mejor escalabilidad en sistemas multi-socket
- Reduce contención en buses de memoria

**Desventajas:**
- Requiere conocimiento de la topología del hardware
- Puede ser complejo de implementar
- No todos los sistemas son NUMA
- Puede requerir configuración manual

**Cuándo usar:**
- Sistemas multi-socket (servidores)
- Aplicaciones con alto uso de memoria
- Sistemas de alto rendimiento
- Bases de datos y sistemas de procesamiento de datos

**Impacto en performance:**
En sistemas NUMA, puede mejorar el rendimiento en un 20-50% al evitar acceso remoto a memoria. El impacto es mayor en aplicaciones intensivas en memoria.

**Ejemplo en C#:**
```csharp
// .NET no expone directamente APIs NUMA, pero puedes:
// 1. Usar CPU affinity para mantener threads en el mismo nodo
// 2. Configurar el proceso para usar memoria local

// En Windows, puedes usar SetThreadAffinityMask
[DllImport("kernel32.dll")]
static extern IntPtr SetThreadAffinityMask(IntPtr hThread, IntPtr dwThreadAffinityMask);

// Para aplicaciones .NET, considera usar bibliotecas como:
// - libnuma en Linux
// - NUMA APIs de Windows

// Alternativamente, usa ThreadLocal para datos por thread
private static ThreadLocal<byte[]> localBuffer = 
    new ThreadLocal<byte[]>(() => new byte[1024 * 1024]);
```

---

### Avoid false sharing and cache line contention (OK)

**Cómo funciona:**
False sharing ocurre cuando múltiples threads acceden a variables diferentes que están en la misma línea de caché (típicamente 64 bytes). Aunque los threads acceden a datos diferentes, comparten la misma línea de caché, causando invalidaciones constantes entre CPUs.

**Ventajas:**
- Elimina invalidaciones innecesarias de caché
- Mejora el rendimiento en aplicaciones multi-threaded
- Reduce contención en el bus de memoria
- Mejor escalabilidad

**Desventajas:**
- Requiere padding de memoria (más uso de memoria)
- Puede ser difícil de detectar
- Requiere conocimiento de la arquitectura de caché

**Cuándo usar:**
- Aplicaciones multi-threaded de alto rendimiento
- Cuando múltiples threads acceden a datos frecuentemente
- Sistemas con muchos núcleos
- Cuando el profiling muestra contención de caché

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% en aplicaciones multi-threaded afectadas por false sharing. En casos extremos, puede ser la diferencia entre escalar o no.

**Ejemplo en C#:**
```csharp
// ❌ Malo: False sharing - múltiples counters en la misma línea de caché
public class BadCounter
{
    public long Counter1;
    public long Counter2; // Probablemente en la misma línea de caché
    public long Counter3;
}

// ✅ Bueno: Padding para evitar false sharing
public class GoodCounter
{
    private long _counter1;
    private long _padding1, _padding2, _padding3, _padding4,
                 _padding5, _padding6, _padding7; // Padding a 64 bytes
    
    private long _counter2;
    private long _padding8, _padding9, _padding10, _padding11,
                 _padding12, _padding13, _padding14, _padding15;
    
    private long _counter3;
}

// Mejor aún: Usar ThreadLocal o variables separadas por thread
public class BestCounter
{
    private readonly ThreadLocal<long> _counter = new ThreadLocal<long>();
    
    public void Increment() => _counter.Value++;
    
    public long GetTotal() => /* sumar todos los valores de threads */;
}

// O usar Interlocked en variables separadas
[StructLayout(LayoutKind.Explicit, Size = 128)] // Alineación a 128 bytes
public struct PaddedLong
{
    [FieldOffset(0)]
    public long Value;
}
```

---

### SIMD and vectorization

**Cómo funciona:**
SIMD (Single Instruction, Multiple Data) permite procesar múltiples elementos de datos en paralelo con una sola instrucción. Las CPUs modernas tienen unidades SIMD (SSE, AVX, AVX-512) que pueden procesar 4, 8, 16 o más valores simultáneamente.

**Ventajas:**
- Procesamiento paralelo de datos
- Mejor utilización de unidades de ejecución
- Reducción significativa de tiempo de ejecución
- Eficiencia energética mejorada

**Desventajas:**
- Requiere datos alineados
- No todos los algoritmos son vectorizables
- Puede requerir código específico de plataforma
- Consume más energía (especialmente AVX-512)

**Cuándo usar:**
- Operaciones matemáticas en arrays
- Procesamiento de imágenes/video
- Cálculos científicos
- Operaciones de búsqueda y filtrado en arrays grandes

**Impacto en performance:**
Puede mejorar el rendimiento en un 4-16x para operaciones vectorizables. Para operaciones matemáticas simples, el speedup puede ser dramático.

**Ejemplo en C#:**
```csharp
using System;
using System.Numerics;
using System.Runtime.Intrinsics;
using System.Runtime.Intrinsics.X86;

// ✅ Usar Vector<T> para operaciones SIMD automáticas
public static void VectorizedAdd(float[] a, float[] b, float[] result)
{
    int vectorSize = Vector<float>.Count; // 8 floats en AVX
    
    int i = 0;
    for (; i <= a.Length - vectorSize; i += vectorSize)
    {
        var va = new Vector<float>(a, i);
        var vb = new Vector<float>(b, i);
        (va + vb).CopyTo(result, i);
    }
    
    // Procesar elementos restantes
    for (; i < a.Length; i++)
    {
        result[i] = a[i] + b[i];
    }
}

// ✅ Usar hardware intrinsics para control fino (requiere .NET Core 3.0+)
public static unsafe void Avx2Sum(float[] array)
{
    fixed (float* ptr = array)
    {
        var sum = Vector256<float>.Zero;
        int i = 0;
        
        if (Avx2.IsSupported)
        {
            for (; i <= array.Length - 8; i += 8)
            {
                var vec = Avx.LoadVector256(ptr + i);
                sum = Avx.Add(sum, vec);
            }
        }
        
        // Procesar elementos restantes
        float result = 0;
        for (int j = 0; j < 8; j++)
        {
            result += sum.GetElement(j);
        }
        for (; i < array.Length; i++)
        {
            result += array[i];
        }
    }
}
```

---

### Branch prediction friendly code (Ok)

**Cómo funciona:**
Los procesadores modernos usan branch prediction para predecir qué camino tomará un branch (if/else, loops, etc.) antes de que se evalúe la condición. Si la predicción es correcta, el pipeline continúa sin interrupciones. Si es incorrecta, hay un "branch misprediction penalty" que puede costar 10-20 ciclos de CPU.

**Ventajas:**
- Reduce branch mispredictions
- Mejor utilización del pipeline de CPU
- Menor latencia en código crítico
- Mejor rendimiento predecible

**Desventajas:**
- Puede requerir reordenar código
- A veces va en contra de la legibilidad
- Requiere profiling para identificar branches problemáticos

**Cuándo usar:**
- Hot paths en aplicaciones de alto rendimiento
- Loops con condiciones
- Código crítico de baja latencia
- Después de identificar branch mispredictions en profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-30% en código con muchos branches. En casos extremos con muchos branches impredecibles, el impacto puede ser mayor.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Branch difícil de predecir
public static int ProcessItems(List<Item> items)
{
    int count = 0;
    foreach (var item in items)
    {
        if (item.IsValid && item.IsActive && item.Value > 100) // Múltiples branches
        {
            count++;
        }
    }
    return count;
}

// ✅ Bueno: Separar datos por condición primero (si es posible)
public static int ProcessItemsOptimized(List<Item> items)
{
    // Separar items válidos primero
    var validItems = items.Where(i => i.IsValid && i.IsActive).ToList();
    
    // Ahora el branch es más predecible
    int count = 0;
    foreach (var item in validItems)
    {
        if (item.Value > 100) // Branch más predecible
        {
            count++;
        }
    }
    return count;
}

// ✅ Mejor: Usar branchless cuando sea posible
public static int CountGreaterThan(int[] values, int threshold)
{
    int count = 0;
    foreach (var value in values)
    {
        // Branchless: siempre suma, pero puede ser 0
        count += (value > threshold) ? 1 : 0;
    }
    return count;
}

// ✅ Mejor aún: Usar operaciones vectorizadas cuando sea posible
public static int CountGreaterThanVectorized(int[] values, int threshold)
{
    int count = 0;
    var thresholdVec = new Vector<int>(threshold);
    
    int i = 0;
    for (; i <= values.Length - Vector<int>.Count; i += Vector<int>.Count)
    {
        var vec = new Vector<int>(values, i);
        var comparison = Vector.GreaterThan(vec, thresholdVec);
        // Contar elementos que cumplen la condición
        // (requiere lógica adicional para contar bits)
    }
    
    // Procesar restante...
    return count;
}
```

---

### Avoid busy-wait loops (Ok)

**Cómo funciona:**
Busy-wait loops son loops que consumen CPU constantemente mientras esperan una condición, sin ceder el control al scheduler. Esto desperdicia ciclos de CPU y puede causar problemas de rendimiento y consumo energético.

**Ventajas:**
- Reduce consumo de CPU innecesario
- Mejora la eficiencia energética
- Permite que otros threads/procesos ejecuten
- Mejor comportamiento del sistema

**Desventajas:**
- Puede introducir latencia adicional
- Requiere usar mecanismos de sincronización apropiados
- Puede ser más complejo de implementar

**Cuándo usar:**
- Siempre que sea posible evitar busy-wait
- En aplicaciones de alto rendimiento
- Sistemas con restricciones de energía
- Cuando hay muchos threads compitiendo

**Impacto en performance:**
Eliminar busy-wait puede liberar 10-100% de CPU dependiendo de la situación. También mejora significativamente la eficiencia energética.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Busy-wait loop
public class BadWait
{
    private bool _flag = false;
    
    public void WaitForFlag()
    {
        while (!_flag) // Consume CPU constantemente
        {
            // Hacer nada - desperdiciando CPU
        }
    }
}

// ✅ Bueno: Usar mecanismos de sincronización apropiados
public class GoodWait
{
    private readonly ManualResetEventSlim _event = new ManualResetEventSlim(false);
    
    public void WaitForFlag()
    {
        _event.Wait(); // Cede el control al scheduler
    }
    
    public void SetFlag()
    {
        _event.Set();
    }
}

// ✅ Mejor: Usar async/await
public class BestWait
{
    private readonly TaskCompletionSource<bool> _tcs = new TaskCompletionSource<bool>();
    
    public async Task WaitForFlagAsync()
    {
        await _tcs.Task; // No bloquea threads
    }
    
    public void SetFlag()
    {
        _tcs.SetResult(true);
    }
}

// ✅ Para spin-wait de muy corta duración, usar SpinWait
public class OptimizedWait
{
    private volatile bool _flag = false;
    
    public void WaitForFlag()
    {
        var spinWait = new SpinWait();
        while (!_flag)
        {
            spinWait.SpinOnce(); // Optimizado para esperas cortas
        }
    }
}
```

---

### Batch processing instead of per-item execution (Ok)   

**Cómo funciona:**
En lugar de procesar items uno por uno, procesar múltiples items en lotes reduce el overhead de llamadas a funciones, mejora la localidad de caché, y permite optimizaciones como vectorización.

**Ventajas:**
- Reduce overhead de llamadas
- Mejora localidad de caché
- Permite optimizaciones (vectorización, paralelización)
- Mejor throughput
- Reduce overhead de sincronización

**Desventajas:**
- Puede aumentar latencia
- Requiere más memoria
- Más complejo de implementar
- Puede requerir batching logic

**Cuándo usar:**
- Procesamiento de grandes volúmenes de datos
- Operaciones de I/O
- Operaciones de base de datos
- Cuando el overhead de procesamiento individual es alto

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x dependiendo del overhead de procesamiento individual. Para I/O, el impacto puede ser dramático (10-100x).

**Ejemplo en C#:**
```csharp
// ❌ Malo: Procesar items uno por uno
public async Task ProcessItemsBad(List<Item> items)
{
    foreach (var item in items)
    {
        await ProcessItemAsync(item); // Muchas llamadas async
    }
}

// ✅ Bueno: Procesar en lotes
public async Task ProcessItemsGood(List<Item> items, int batchSize = 100)
{
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize).ToList();
        await ProcessBatchAsync(batch); // Una llamada por lote
    }
}

// ✅ Mejor: Procesar lotes en paralelo
public async Task ProcessItemsBest(List<Item> items, int batchSize = 100)
{
    var batches = items
        .Select((item, index) => new { item, index })
        .GroupBy(x => x.index / batchSize)
        .Select(g => g.Select(x => x.item).ToList())
        .ToList();
    
    await Task.WhenAll(batches.Select(batch => ProcessBatchAsync(batch)));
}

// ✅ Para base de datos: Batch inserts
public async Task InsertItemsBatch(List<Item> items)
{
    const int batchSize = 1000;
    
    for (int i = 0; i < items.Count; i += batchSize)
    {
        var batch = items.Skip(i).Take(batchSize);
        
        // Usar Dapper para batch insert
        await connection.ExecuteAsync(
            "INSERT INTO Items (Id, Name, Value) VALUES (@Id, @Name, @Value)",
            batch);
    }
}
```

---

### Avoid CPU throttling (Ok)

**Cómo funciona:**
CPU throttling ocurre cuando el sistema reduce la frecuencia del CPU debido a temperatura alta, restricciones de energía, o políticas del sistema. Esto reduce el rendimiento significativamente.

**Ventajas:**
- Mantiene rendimiento consistente
- Evita degradación inesperada
- Mejor experiencia de usuario
- Rendimiento predecible

**Desventajas:**
- Puede requerir mejor refrigeración
- Puede aumentar consumo energético
- Requiere monitoreo de temperatura
- Puede requerir configuración del sistema

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Sistemas que requieren rendimiento consistente
- Servidores de producción
- Aplicaciones en tiempo real

**Impacto en performance:**
Evitar throttling puede mantener el rendimiento en un 20-50% más alto durante cargas sostenidas. En sistemas con throttling activo, el impacto puede ser dramático.

---

### Disable unstable turbo boost for latency-sensitive workloads

**Cómo funciona:**
Turbo boost aumenta dinámicamente la frecuencia del CPU, pero puede causar variabilidad en la latencia. Para cargas de trabajo sensibles a latencia, deshabilitar turbo boost puede proporcionar latencia más predecible, aunque a costa de un menor rendimiento máximo.

**Ventajas:**
- Latencia más predecible
- Menor variabilidad en tiempos de respuesta
- Mejor para aplicaciones en tiempo real
- Comportamiento más consistente

**Desventajas:**
- Menor rendimiento máximo
- Puede reducir throughput
- No siempre es necesario

**Cuándo usar:**
- Aplicaciones de baja latencia crítica
- Sistemas en tiempo real
- Cuando la consistencia es más importante que el rendimiento máximo
- Trading systems, gaming servers

**Impacto en performance:**
Puede reducir la latencia P99 en un 10-30% al eliminar spikes causados por cambios de frecuencia. El throughput puede reducirse en un 5-15%.

---

### Disable Transparent HugePages (THP) for database workloads

**Cómo funciona:**
Transparent HugePages es una característica del kernel Linux que automáticamente usa páginas de 2MB en lugar de 4KB. Sin embargo, para algunas cargas de trabajo (especialmente bases de datos), puede causar fragmentación y latencia variable debido a la compactación de memoria.

**Ventajas:**
- Latencia más predecible
- Evita fragmentación de memoria
- Mejor para bases de datos
- Comportamiento más consistente

**Desventajas:**
- Puede aumentar uso de memoria
- Requiere configuración del sistema
- No siempre es necesario

**Cuándo usar:**
- Bases de datos (PostgreSQL, MySQL, etc.)
- Aplicaciones con patrones de acceso a memoria predecibles
- Cuando THP causa problemas de latencia

**Impacto en performance:**
Puede reducir la latencia P99 en un 20-50% en bases de datos. El impacto en throughput puede ser positivo o negativo dependiendo de la carga de trabajo.

---

### Configure explicit HugePages to reduce TLB misses

**Cómo funciona:**
HugePages (páginas grandes) reducen el número de entradas en la TLB (Translation Lookaside Buffer), reduciendo TLB misses y mejorando el rendimiento de acceso a memoria.

**Ventajas:**
- Reduce TLB misses
- Mejor rendimiento de acceso a memoria
- Menor overhead de traducción de direcciones
- Mejor para aplicaciones con grandes espacios de memoria

**Desventajas:**
- Requiere configuración manual
- Puede fragmentar memoria
- No siempre está disponible
- Requiere privilegios de administrador

**Cuándo usar:**
- Aplicaciones con grandes espacios de memoria
- Bases de datos grandes
- Aplicaciones de procesamiento de datos
- Cuando el profiling muestra muchos TLB misses

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% para aplicaciones intensivas en memoria. En bases de datos grandes, el impacto puede ser mayor.

---

### Adjust filesystem cache pressure for database workloads

**Cómo funciona:**
El sistema operativo usa memoria para caché de archivos. Para bases de datos que tienen su propio caché, puede ser beneficioso reducir la presión del caché del filesystem para dar más memoria a la base de datos.

**Ventajas:**
- Más memoria disponible para la aplicación
- Mejor control sobre el uso de memoria
- Optimización específica para bases de datos
- Mejor rendimiento predecible

**Desventajas:**
- Requiere conocimiento del sistema
- Puede afectar otras aplicaciones
- Requiere configuración del kernel
- Puede no ser necesario en todos los casos

**Cuándo usar:**
- Bases de datos con caché propio
- Aplicaciones que gestionan su propia memoria
- Cuando hay competencia por memoria entre OS y aplicación

**Impacto en performance:**
Puede mejorar el rendimiento de la base de datos en un 10-30% al darle más memoria disponible. El impacto depende de cuánta memoria está usando el filesystem cache.

---

### Control memory overcommit behavior

**Cómo funciona:**
Memory overcommit permite que el sistema asigne más memoria de la físicamente disponible, asumiendo que no todos los procesos usarán toda su memoria asignada. Para aplicaciones de alto rendimiento, puede ser mejor deshabilitar overcommit para garantizar que la memoria esté realmente disponible.

**Ventajas:**
- Garantiza que la memoria esté disponible
- Evita OOM (Out of Memory) kills inesperados
- Comportamiento más predecible
- Mejor para aplicaciones críticas

**Desventajas:**
- Puede limitar la capacidad del sistema
- Requiere más memoria física
- Puede causar fallos de asignación más tempranos

**Cuándo usar:**
- Aplicaciones críticas
- Sistemas con memoria suficiente
- Cuando la predecibilidad es más importante que la capacidad
- Bases de datos y sistemas de alto rendimiento

**Impacto en performance:**
Evita degradación inesperada por falta de memoria. Puede mejorar la estabilidad significativamente, aunque el impacto directo en rendimiento puede ser menor.

---

### Optimize CPU governor settings for performance

**Cómo funciona:**
El CPU governor controla cómo el sistema ajusta la frecuencia del CPU. Configurarlo a "performance" mantiene el CPU a máxima frecuencia, mejorando el rendimiento a costa de mayor consumo energético.

**Ventajas:**
- Máximo rendimiento del CPU
- Latencia más baja
- Comportamiento más predecible
- Mejor para servidores

**Desventajas:**
- Mayor consumo energético
- Mayor generación de calor
- Puede reducir vida útil del hardware
- No es necesario para todas las aplicaciones

**Cuándo usar:**
- Servidores de producción
- Aplicaciones de alto rendimiento
- Cuando el rendimiento es más importante que la eficiencia energética
- Sistemas con refrigeración adecuada

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mantener el CPU a máxima frecuencia. El impacto es mayor en aplicaciones que no están limitadas por I/O.

---

### Disable CPU frequency scaling for latency-sensitive workloads

**Cómo funciona:**
Similar a deshabilitar turbo boost, deshabilitar frequency scaling mantiene el CPU a una frecuencia constante, eliminando variabilidad en latencia causada por cambios de frecuencia.

**Ventajas:**
- Latencia más predecible
- Menor variabilidad
- Mejor para aplicaciones en tiempo real
- Comportamiento consistente

**Desventajas:**
- Mayor consumo energético
- Menor eficiencia energética
- No siempre es necesario

**Cuándo usar:**
- Aplicaciones de baja latencia crítica
- Sistemas en tiempo real
- Cuando la consistencia es crítica
- Trading systems, gaming

**Impacto en performance:**
Reduce la variabilidad de latencia en un 20-40%. El impacto en throughput puede ser positivo o negativo dependiendo de la carga.

---

### Use CPU isolation (isolcpus) for dedicated workloads

**Cómo funciona:**
CPU isolation reserva núcleos específicos para uso exclusivo de una aplicación, evitando que el scheduler del OS ejecute otros procesos en esos núcleos.

**Ventajas:**
- Recursos dedicados
- Sin interferencia de otros procesos
- Latencia más predecible
- Mejor para aplicaciones en tiempo real

**Desventajas:**
- Reduce recursos disponibles para otros procesos
- Puede causar desbalanceo
- Requiere configuración del kernel
- Puede no ser necesario

**Cuándo usar:**
- Aplicaciones críticas de tiempo real
- Sistemas con recursos suficientes
- Cuando se necesita garantizar recursos
- Aplicaciones de baja latencia

**Impacto en performance:**
Puede mejorar la latencia P99 en un 20-50% al eliminar interferencia. El impacto en throughput puede variar.

---

### Optimize interrupt handling and CPU affinity

**Cómo funciona:**
Asignar interrupciones de hardware (red, disco) a núcleos específicos puede mejorar el rendimiento al reducir migraciones y mejorar la localidad.

**Ventajas:**
- Mejor localidad
- Menos migraciones
- Mejor rendimiento de I/O
- Comportamiento más predecible

**Desventajas:**
- Requiere configuración del sistema
- Puede ser complejo
- No siempre es necesario

**Cuándo usar:**
- Sistemas de alto rendimiento
- Aplicaciones intensivas en I/O
- Cuando el profiling muestra problemas de interrupciones
- Servidores de red de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de I/O en un 5-15%. El impacto es mayor en sistemas con mucho tráfico de red o disco.

---

### Configure IRQ balancing

**Cómo funciona:**
IRQ balancing distribuye interrupciones de hardware entre múltiples CPUs para balancear la carga y evitar saturar un solo núcleo.

**Ventajas:**
- Mejor balanceo de carga
- Evita saturación de un núcleo
- Mejor utilización de recursos
- Mejor escalabilidad

**Desventajas:**
- Puede aumentar latencia
- Puede reducir localidad
- Requiere configuración
- Puede no ser óptimo para todas las cargas

**Cuándo usar:**
- Sistemas multi-core
- Aplicaciones con mucho I/O
- Cuando hay desbalanceo de interrupciones
- Sistemas de alto throughput

**Impacto en performance:**
Puede mejorar el throughput en un 10-30% al balancear mejor la carga. El impacto en latencia puede ser positivo o negativo.

---

### Use RDT (Resource Director Technology) for cache and memory bandwidth control

**Cómo funciona:**
RDT es una tecnología Intel que permite monitorear y controlar el uso de caché y ancho de banda de memoria por aplicación, permitiendo QoS (Quality of Service) a nivel de hardware.

**Ventajas:**
- Control fino de recursos
- QoS a nivel de hardware
- Mejor para multi-tenancy
- Mejor aislamiento entre aplicaciones

**Desventajas:**
- Solo disponible en CPUs Intel modernas
- Requiere configuración
- Puede ser complejo
- No siempre es necesario

**Cuándo usar:**
- Sistemas multi-tenant
- Cuando se necesita garantizar recursos
- Aplicaciones críticas en sistemas compartidos
- Cloud providers

**Impacto en performance:**
Puede mejorar la predecibilidad del rendimiento en un 20-40% en sistemas multi-tenant. El impacto depende de la configuración y carga de trabajo.

---

## Memory Management

La gestión eficiente de memoria es crítica para el rendimiento de aplicaciones .NET. Esta sección cubre técnicas para optimizar el uso de memoria, reducir allocations, y mejorar la localidad de datos.

### Avoid page faults (Ok)

**Cómo funciona:**
Los page faults ocurren cuando el sistema necesita cargar una página de memoria desde el disco (swap) o cuando se accede a memoria que no está mapeada. Cada page fault puede costar miles de ciclos de CPU y acceso a disco, causando latencia significativa.

**Ventajas:**
- Elimina latencia de acceso a disco
- Mejor rendimiento predecible
- Menor uso de I/O
- Mejor experiencia de usuario

**Desventajas:**
- Requiere suficiente memoria física
- Puede requerir pre-carga de datos
- Puede aumentar uso de memoria

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Sistemas con memoria suficiente
- Cuando el acceso a memoria es crítico
- Aplicaciones en tiempo real

**Impacto en performance:**
Evitar page faults puede mejorar el rendimiento en un 10-100x para operaciones que acceden a memoria frecuentemente. El impacto es dramático cuando hay swapping activo.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Acceso a memoria que puede causar page faults
public class BadMemoryAccess
{
    private byte[] _largeArray = new byte[100_000_000]; // 100MB
    
    public void ProcessData()
    {
        // Si el array no está en memoria, causa page fault
        for (int i = 0; i < _largeArray.Length; i++)
        {
            _largeArray[i] = (byte)(i % 256);
        }
    }
}

// ✅ Bueno: Pre-cargar memoria y acceder de manera secuencial
public class GoodMemoryAccess
{
    private byte[] _largeArray;
    
    public GoodMemoryAccess()
    {
        _largeArray = new byte[100_000_000];
        // Pre-cargar todas las páginas tocándolas
        for (int i = 0; i < _largeArray.Length; i += 4096) // Tamaño de página
        {
            _largeArray[i] = 0; // Toca cada página
        }
    }
    
    public void ProcessData()
    {
        // Ahora el acceso es más rápido, menos page faults
        for (int i = 0; i < _largeArray.Length; i++)
        {
            _largeArray[i] = (byte)(i % 256);
        }
    }
}

// ✅ Mejor: Usar Memory<T> y Span<T> para acceso eficiente
public void ProcessDataEfficient(Span<byte> data)
{
    // Span permite acceso eficiente sin allocations adicionales
    for (int i = 0; i < data.Length; i++)
    {
        data[i] = (byte)(i % 256);
    }
}
```

---

### Avoid swapping

**Cómo funciona:**
Swapping ocurre cuando el sistema operativo mueve páginas de memoria al disco cuando hay presión de memoria. Esto causa latencia extrema (milisegundos vs nanosegundos) y puede degradar el rendimiento dramáticamente.

**Ventajas:**
- Evita latencia extrema
- Mejor rendimiento consistente
- Menor uso de I/O de disco
- Mejor experiencia de usuario

**Desventajas:**
- Requiere suficiente memoria física
- Puede requerir limitar uso de memoria
- Puede requerir monitoreo

**Cuándo usar:**
- Siempre que sea posible
- Aplicaciones de alto rendimiento
- Sistemas con memoria suficiente
- Aplicaciones críticas

**Impacto en performance:**
Evitar swapping puede mejorar el rendimiento en un 100-1000x cuando el swapping está activo. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Usar demasiada memoria puede causar swapping
public class BadMemoryUsage
{
    private List<byte[]> _largeArrays = new List<byte[]>();
    
    public void AllocateMemory()
    {
        // Puede causar swapping si se asigna demasiada memoria
        for (int i = 0; i < 1000; i++)
        {
            _largeArrays.Add(new byte[10_000_000]); // 10MB cada uno
        }
    }
}

// ✅ Bueno: Monitorear uso de memoria y limitar allocations
public class GoodMemoryUsage
{
    private readonly long _maxMemoryBytes = 100_000_000; // 100MB límite
    private long _currentMemoryBytes = 0;
    private List<byte[]> _largeArrays = new List<byte[]>();
    
    public bool TryAllocateMemory(int sizeBytes)
    {
        if (_currentMemoryBytes + sizeBytes > _maxMemoryBytes)
        {
            return false; // No hay suficiente memoria
        }
        
        _largeArrays.Add(new byte[sizeBytes]);
        _currentMemoryBytes += sizeBytes;
        return true;
    }
}

// ✅ Mejor: Usar ArrayPool para reutilizar memoria
public class BestMemoryUsage
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData(int size)
    {
        var buffer = _pool.Rent(size); // Reutiliza memoria
        try
        {
            // Usar buffer
            ProcessBuffer(buffer);
        }
        finally
        {
            _pool.Return(buffer); // Devuelve al pool
        }
    }
}
```

---

### Use memory pooling (Ok)

**Cómo funciona:**
Memory pooling reutiliza bloques de memoria pre-asignados en lugar de asignar y liberar memoria constantemente. Esto reduce la presión sobre el garbage collector y mejora el rendimiento.

**Ventajas:**
- Reduce allocations
- Menos presión en GC
- Mejor rendimiento
- Menor fragmentación

**Desventajas:**
- Requiere gestión manual
- Puede usar más memoria
- Más complejo de implementar

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando hay muchas allocations temporales
- Hot paths con allocations frecuentes
- Sistemas con restricciones de GC

**Impacto en performance:**
Puede reducir allocations en un 50-90% y mejorar el rendimiento en un 10-30% en aplicaciones con muchas allocations temporales.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Allocations constantes
public class BadPooling
{
    public void ProcessItems(List<Item> items)
    {
        foreach (var item in items)
        {
            var buffer = new byte[1024]; // Nueva allocation cada vez
            ProcessItem(item, buffer);
        }
    }
}

// ✅ Bueno: Usar ArrayPool
public class GoodPooling
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessItems(List<Item> items)
    {
        foreach (var item in items)
        {
            var buffer = _pool.Rent(1024); // Reutiliza memoria
            try
            {
                ProcessItem(item, buffer);
            }
            finally
            {
                _pool.Return(buffer); // Devuelve al pool
            }
        }
    }
}

// ✅ Mejor: Pool personalizado para objetos específicos
public class ObjectPool<T> where T : class, new()
{
    private readonly ConcurrentQueue<T> _pool = new ConcurrentQueue<T>();
    private readonly Func<T> _factory;
    
    public ObjectPool(Func<T> factory)
    {
        _factory = factory;
    }
    
    public T Rent()
    {
        if (_pool.TryDequeue(out var item))
        {
            return item;
        }
        return _factory();
    }
    
    public void Return(T item)
    {
        // Resetear estado si es necesario
        _pool.Enqueue(item);
    }
}

// Uso:
var pool = new ObjectPool<StringBuilder>(() => new StringBuilder());
var sb = pool.Rent();
try
{
    sb.Append("Hello");
    // Usar sb
}
finally
{
    pool.Return(sb);
}
```

---

### Reuse buffers

**Cómo funciona:**
Reutilizar buffers evita allocations repetidas del mismo tamaño. Esto es especialmente importante en hot paths donde se crean buffers frecuentemente.

**Ventajas:**
- Reduce allocations
- Mejor rendimiento
- Menor presión en GC
- Menor fragmentación

**Desventajas:**
- Requiere gestión manual
- Puede requerir limpieza entre usos
- Más complejo

**Cuándo usar:**
- Hot paths con buffers temporales
- Cuando el tamaño del buffer es conocido
- Aplicaciones de alto rendimiento
- Cuando hay muchas operaciones de I/O

**Impacto en performance:**
Puede reducir allocations en un 50-80% y mejorar el rendimiento en un 5-20% en código con buffers frecuentes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear nuevos buffers constantemente
public class BadBufferReuse
{
    public void ProcessData(Stream stream)
    {
        while (true)
        {
            var buffer = new byte[4096]; // Nueva allocation cada vez
            int bytesRead = stream.Read(buffer, 0, buffer.Length);
            if (bytesRead == 0) break;
            ProcessBuffer(buffer, bytesRead);
        }
    }
}

// ✅ Bueno: Reutilizar buffer
public class GoodBufferReuse
{
    private readonly byte[] _buffer = new byte[4096]; // Reutilizado
    
    public void ProcessData(Stream stream)
    {
        while (true)
        {
            int bytesRead = stream.Read(_buffer, 0, _buffer.Length);
            if (bytesRead == 0) break;
            ProcessBuffer(_buffer, bytesRead);
        }
    }
}

// ✅ Mejor: Usar ArrayPool para buffers de diferentes tamaños
public class BestBufferReuse
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData(Stream stream)
    {
        var buffer = _pool.Rent(4096);
        try
        {
            while (true)
            {
                int bytesRead = stream.Read(buffer, 0, 4096);
                if (bytesRead == 0) break;
                ProcessBuffer(buffer, bytesRead);
            }
        }
        finally
        {
            _pool.Return(buffer);
        }
    }
}
```

---

### Zero-copy patterns (Ok)

**Cómo funciona:**
Zero-copy evita copiar datos entre buffers en memoria, reduciendo el uso de CPU y memoria. En lugar de copiar datos, se pasan referencias o se usan técnicas del sistema operativo como sendfile().

**Ventajas:**
- Reduce uso de CPU
- Menor uso de memoria
- Mejor rendimiento
- Menor latencia

**Desventajas:**
- Puede ser más complejo
- Requiere APIs específicas
- Puede requerir unsafe code

**Cuándo usar:**
- Operaciones de I/O de alto rendimiento
- Transferencia de archivos grandes
- Networking de alto throughput
- Cuando hay muchas copias de datos

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al eliminar copias innecesarias. Para operaciones de red, el impacto puede ser mayor.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Múltiples copias de datos
public class BadZeroCopy
{
    public void SendData(Stream source, Stream destination)
    {
        var buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = source.Read(buffer, 0, buffer.Length)) > 0)
        {
            var copy = new byte[bytesRead]; // Copia innecesaria
            Array.Copy(buffer, copy, bytesRead);
            destination.Write(copy, 0, bytesRead);
        }
    }
}

// ✅ Bueno: Usar Span<T> y Memory<T> para evitar copias
public class GoodZeroCopy
{
    public void SendData(Stream source, Stream destination)
    {
        var buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = source.Read(buffer, 0, buffer.Length)) > 0)
        {
            var span = new Span<byte>(buffer, 0, bytesRead); // Sin copia
            destination.Write(span);
        }
    }
}

// ✅ Mejor: Usar System.IO.Pipelines para zero-copy
using System.IO.Pipelines;

public class BestZeroCopy
{
    public async Task SendDataAsync(PipeReader reader, PipeWriter writer)
    {
        while (true)
        {
            var result = await reader.ReadAsync();
            var buffer = result.Buffer;
            
            foreach (var segment in buffer)
            {
                await writer.WriteAsync(segment); // Zero-copy
            }
            
            reader.AdvanceTo(buffer.End);
            
            if (result.IsCompleted)
                break;
        }
    }
}

// ✅ Para archivos: Usar MemoryMappedFile (cuando sea posible)
using System.IO.MemoryMappedFiles;

public void ProcessLargeFile(string filePath)
{
    using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
    using (var accessor = mmf.CreateViewAccessor())
    {
        // Acceso directo sin cargar todo en memoria
        byte[] buffer = new byte[4096];
        accessor.ReadArray(0, buffer, 0, buffer.Length);
    }
}
```

---

### Stack allocation when possible (Ok)

**Cómo funciona:**
Las variables locales en el stack se asignan y liberan automáticamente cuando la función retorna, sin overhead del garbage collector. Esto es mucho más rápido que heap allocation.

**Ventajas:**
- Muy rápido (solo ajuste de stack pointer)
- Sin overhead de GC
- Automático (no requiere liberación manual)
- Mejor localidad de caché

**Desventajas:**
- Tamaño limitado (típicamente 1-8MB)
- Solo para variables locales
- No puede retornarse de la función (excepto copias)

**Cuándo usar:**
- Variables temporales pequeñas
- Buffers pequeños (< 1KB típicamente)
- Hot paths con allocations temporales
- Cuando el tamaño es conocido en compile-time

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al eliminar allocations del heap. El impacto es mayor en hot paths.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Heap allocation para datos temporales pequeños
public class BadStackAllocation
{
    public int ProcessData(int[] data)
    {
        var temp = new int[100]; // Heap allocation
        // Procesar datos
        return temp[0];
    }
}

// ✅ Bueno: Usar stackalloc para arrays pequeños
public class GoodStackAllocation
{
    public unsafe int ProcessData(int[] data)
    {
        Span<int> temp = stackalloc int[100]; // Stack allocation
        // Procesar datos
        return temp[0];
    }
}

// ✅ Mejor: Usar stackalloc con Span<T> (safe)
public class BestStackAllocation
{
    public int ProcessData(int[] data)
    {
        Span<int> temp = stackalloc int[100]; // Stack allocation, type-safe
        // Procesar datos
        return temp[0];
    }
}

// ✅ Para strings pequeños, usar stackalloc char[]
public string FormatNumber(int value)
{
    Span<char> buffer = stackalloc char[32];
    if (value.TryFormat(buffer, out int written))
    {
        return buffer.Slice(0, written).ToString();
    }
    return value.ToString(); // Fallback
}
```

---

### Avoid heap fragmentation (Ok)

**Cómo funciona:**
Heap fragmentation ocurre cuando hay muchos objetos pequeños entre objetos grandes, dejando espacios libres que son demasiado pequeños para usar. Esto puede causar que el GC tenga que compactar más frecuentemente o que las allocations fallen.

**Ventajas:**
- Mejor utilización de memoria
- Menos compactación del GC
- Menos allocations fallidas
- Mejor rendimiento del GC

**Desventajas:**
- Puede requerir cambios en el diseño
- Puede requerir pooling
- Puede ser difícil de detectar

**Cuándo usar:**
- Aplicaciones de larga duración
- Cuando hay muchas allocations de diferentes tamaños
- Sistemas con memoria limitada
- Cuando el GC está causando problemas

**Impacto en performance:**
Puede mejorar el rendimiento del GC en un 20-50% y reducir la frecuencia de collections. También puede prevenir OutOfMemoryExceptions.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Mezclar allocations de diferentes tamaños
public class BadFragmentation
{
    private List<object> _objects = new List<object>();
    
    public void AllocateObjects()
    {
        // Mezclar objetos pequeños y grandes causa fragmentación
        _objects.Add(new byte[100]);      // Pequeño
        _objects.Add(new byte[10000]);    // Grande
        _objects.Add(new byte[100]);       // Pequeño
        _objects.Add(new byte[10000]);     // Grande
        // Patrón repetido causa fragmentación
    }
}

// ✅ Bueno: Agrupar objetos por tamaño o usar pooling
public class GoodFragmentation
{
    private readonly ArrayPool<byte> _smallPool = ArrayPool<byte>.Shared;
    private readonly ArrayPool<byte> _largePool = ArrayPool<byte>.Shared;
    
    public byte[] RentSmall(int size)
    {
        return _smallPool.Rent(size); // Objetos pequeños juntos
    }
    
    public byte[] RentLarge(int size)
    {
        return _largePool.Rent(size); // Objetos grandes juntos
    }
}

// ✅ Mejor: Usar structs para objetos pequeños
public struct SmallData
{
    public int Value1;
    public int Value2;
    public int Value3;
    // Struct va en el stack o inline, no causa fragmentación
}

// ✅ Agrupar allocations similares
public class BestFragmentation
{
    private List<SmallData> _smallItems = new List<SmallData>();
    private List<LargeData> _largeItems = new List<LargeData>();
    
    // Mantener objetos de tamaño similar juntos
}
```

---

### Avoid Large Object Heap fragmentation in .NET

**Cómo funciona:**
En .NET, objetos mayores a 85KB van al Large Object Heap (LOH), que no se compacta en generaciones anteriores. Esto puede causar fragmentación significativa y eventualmente OutOfMemoryException incluso cuando hay memoria disponible.

**Ventajas:**
- Evita fragmentación del LOH
- Previene OutOfMemoryExceptions
- Mejor rendimiento del GC
- Mejor utilización de memoria

**Desventajas:**
- Requiere pooling o reutilización
- Puede requerir cambios en el diseño
- Más complejo de implementar

**Cuándo usar:**
- Aplicaciones que crean muchos objetos grandes
- Aplicaciones de larga duración
- Cuando hay problemas de memoria
- Sistemas con memoria limitada

**Impacto en performance:**
Puede prevenir OutOfMemoryExceptions y mejorar el rendimiento del GC en un 30-50% en aplicaciones con muchos objetos grandes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear muchos objetos grandes (>85KB)
public class BadLOHUsage
{
    public void ProcessData()
    {
        for (int i = 0; i < 1000; i++)
        {
            var buffer = new byte[100_000]; // Va al LOH, causa fragmentación
            ProcessBuffer(buffer);
        }
    }
}

// ✅ Bueno: Reutilizar buffers grandes
public class GoodLOHUsage
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData()
    {
        var buffer = _pool.Rent(100_000); // Reutiliza buffer del LOH
        try
        {
            ProcessBuffer(buffer);
        }
        finally
        {
            _pool.Return(buffer); // Devuelve al pool
        }
    }
}

// ✅ Mejor: Usar Memory<T> y evitar allocations grandes cuando sea posible
public class BestLOHUsage
{
    public void ProcessData(Span<byte> data)
    {
        // Procesar en chunks más pequeños si es posible
        const int chunkSize = 8192; // Menor que 85KB
        for (int i = 0; i < data.Length; i += chunkSize)
        {
            var chunk = data.Slice(i, Math.Min(chunkSize, data.Length - i));
            ProcessChunk(chunk);
        }
    }
}

// ✅ Para .NET Core 3.0+: LOH puede compactarse con GCSettings
// (aunque pooling sigue siendo mejor)
```

---

### Memory alignment

**Cómo funciona:**
Memory alignment asegura que los datos estén alineados en direcciones de memoria que son múltiplos de su tamaño. Los procesadores acceden más eficientemente a datos alineados, y el acceso a datos desalineados puede causar penalizaciones de rendimiento.

**Ventajas:**
- Mejor rendimiento de acceso a memoria
- Mejor uso de caché
- Compatible con instrucciones SIMD
- Mejor rendimiento en algunos procesadores

**Desventajas:**
- Puede usar más memoria (padding)
- Requiere conocimiento de la arquitectura
- Puede no ser necesario en todos los casos

**Cuándo usar:**
- Estructuras de datos críticas para rendimiento
- Cuando se usan instrucciones SIMD
- Hot paths con acceso a memoria frecuente
- Sistemas embebidos o de bajo nivel

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% para estructuras de datos accedidas frecuentemente. El impacto es mayor en sistemas con acceso a memoria desalineado costoso.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Estructura desalineada
[StructLayout(LayoutKind.Sequential)]
public struct BadAlignment
{
    public byte B;      // 1 byte
    public long L;      // 8 bytes - puede no estar alineado
    public int I;       // 4 bytes
}

// ✅ Bueno: Estructura alineada explícitamente
[StructLayout(LayoutKind.Sequential, Pack = 8)]
public struct GoodAlignment
{
    public long L;      // 8 bytes - alineado
    public int I;       // 4 bytes
    public byte B;      // 1 byte
    // Padding automático para alineación
}

// ✅ Mejor: Usar StructLayout explícito para control total
[StructLayout(LayoutKind.Explicit, Size = 16)]
public struct BestAlignment
{
    [FieldOffset(0)]
    public long L;      // Alineado a 8 bytes
    
    [FieldOffset(8)]
    public int I;       // Alineado a 4 bytes
    
    [FieldOffset(12)]
    public byte B;      // 1 byte
    // Padding a 16 bytes para alineación SIMD
}

// ✅ Para arrays de structs, asegurar alineación
public unsafe void ProcessAlignedArray(float[] data)
{
    fixed (float* ptr = data)
    {
        // ptr está alineado si el array está alineado
        // Para SIMD, asegurar alineación a 16 o 32 bytes
        if (((ulong)ptr & 0xF) == 0) // Alineado a 16 bytes
        {
            // Usar instrucciones SIMD
        }
    }
}
```

---

### Memory prefetching (ok)

**Cómo funciona:**
Memory prefetching carga datos en caché antes de que se necesiten, reduciendo la latencia de acceso a memoria. Esto puede ser hecho por el hardware (prefetchers automáticos) o explícitamente por el software.

**Ventajas:**
- Reduce latencia de acceso a memoria
- Mejor utilización de ancho de banda
- Mejor rendimiento en loops
- Reduce stalls del CPU

**Desventajas:**
- Puede cargar datos innecesarios
- Puede evictar datos útiles de caché
- Requiere conocimiento de patrones de acceso
- Puede no ser efectivo si los patrones son impredecibles

**Cuándo usar:**
- Loops con patrones de acceso predecibles
- Procesamiento secuencial de arrays grandes
- Hot paths con acceso a memoria
- Cuando el profiling muestra muchos cache misses

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para loops con acceso a memoria. El impacto es mayor cuando hay muchos cache misses.

**Ejemplo en C#:**
```csharp
// .NET no tiene prefetch directo, pero podemos optimizar patrones de acceso

// ❌ Malo: Acceso a memoria no secuencial
public class BadPrefetch
{
    public int SumRandomAccess(int[] data, int[] indices)
    {
        int sum = 0;
        foreach (var index in indices)
        {
            sum += data[index]; // Acceso aleatorio, difícil de prefetchear
        }
        return sum;
    }
}

// ✅ Bueno: Acceso secuencial (prefetcher hardware puede ayudar)
public class GoodPrefetch
{
    public int SumSequential(int[] data)
    {
        int sum = 0;
        for (int i = 0; i < data.Length; i++)
        {
            sum += data[i]; // Acceso secuencial, prefetcher funciona bien
        }
        return sum;
    }
}

// ✅ Mejor: Procesar en bloques para mejor localidad
public class BestPrefetch
{
    public int SumBlocked(int[] data)
    {
        const int blockSize = 64; // Tamaño de línea de caché
        int sum = 0;
        
        for (int i = 0; i < data.Length; i += blockSize)
        {
            int end = Math.Min(i + blockSize, data.Length);
            // Procesar bloque completo en caché
            for (int j = i; j < end; j++)
            {
                sum += data[j];
            }
        }
        return sum;
    }
}

// Para código unsafe, se puede usar prefetch intrinsics (requiere .NET 5+)
// Nota: Esto es específico de plataforma
```

---

### Huge pages

**Cómo funciona:**
Huge pages son páginas de memoria más grandes (típicamente 2MB en lugar de 4KB). Esto reduce el número de entradas en la TLB (Translation Lookaside Buffer), reduciendo TLB misses y mejorando el rendimiento.

**Ventajas:**
- Reduce TLB misses
- Mejor rendimiento de acceso a memoria
- Menor overhead de traducción de direcciones
- Mejor para aplicaciones con grandes espacios de memoria

**Desventajas:**
- Requiere configuración del sistema
- Puede fragmentar memoria
- No siempre está disponible
- Requiere privilegios de administrador

**Cuándo usar:**
- Aplicaciones con grandes espacios de memoria
- Bases de datos grandes
- Aplicaciones de procesamiento de datos
- Cuando el profiling muestra muchos TLB misses

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% para aplicaciones intensivas en memoria. En bases de datos grandes, el impacto puede ser mayor.

**Nota:** En .NET, esto se configura a nivel del sistema operativo, no directamente desde código C#.

---

### Avoid unnecessary allocations (Ok)

**Cómo funciona:**
Cada allocation en el heap requiere trabajo del garbage collector. Evitar allocations innecesarias reduce la presión en el GC y mejora el rendimiento.

**Ventajas:**
- Menos presión en GC
- Mejor rendimiento
- Menor uso de memoria
- Menor fragmentación

**Desventajas:**
- Puede requerir cambios en el código
- Puede requerir pooling
- Puede reducir legibilidad en algunos casos

**Cuándo usar:**
- Siempre que sea posible
- Hot paths
- Aplicaciones de alto rendimiento
- Cuando el profiling muestra muchas allocations

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% dependiendo de cuántas allocations se eviten. El impacto es mayor en hot paths.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Allocations innecesarias
public class BadAllocations
{
    public string ProcessData(int value)
    {
        return "Value: " + value.ToString(); // Allocation de string
    }
    
    public void ProcessList(List<int> items)
    {
        var filtered = items.Where(x => x > 0).ToList(); // Allocation de lista
        ProcessFiltered(filtered);
    }
}

// ✅ Bueno: Evitar allocations cuando sea posible
public class GoodAllocations
{
    // Usar string interpolation solo cuando sea necesario
    public string ProcessData(int value)
    {
        // Para hot paths, considerar StringBuilder o stackalloc
        return $"Value: {value}"; // Más eficiente que concatenación
    }
    
    public void ProcessList(List<int> items)
    {
        // Procesar directamente sin crear nueva lista
        foreach (var item in items)
        {
            if (item > 0)
            {
                ProcessItem(item);
            }
        }
    }
}

// ✅ Mejor: Usar Span<T> y stackalloc para datos temporales
public class BestAllocations
{
    public int SumValues(Span<int> values)
    {
        int sum = 0;
        foreach (var value in values)
        {
            sum += value; // Sin allocations
        }
        return sum;
    }
    
    public string FormatNumber(int value)
    {
        Span<char> buffer = stackalloc char[32];
        if (value.TryFormat(buffer, out int written))
        {
            return buffer.Slice(0, written).ToString(); // Solo una allocation
        }
        return value.ToString();
    }
}
```

---

### Cache-friendly memory layouts (Ok)

**Cómo funciona:**
Organizar datos en memoria de manera que los datos accedidos juntos estén cerca mejora la localidad de caché y reduce cache misses.

**Ventajas:**
- Mejor localidad de caché
- Menos cache misses
- Mejor rendimiento
- Mejor utilización de ancho de banda de memoria

**Desventajas:**
- Puede requerir reorganización de datos
- Puede requerir cambios en el diseño
- Puede no ser intuitivo

**Cuándo usar:**
- Hot paths con acceso a memoria frecuente
- Estructuras de datos accedidas frecuentemente
- Cuando el profiling muestra muchos cache misses
- Aplicaciones intensivas en memoria

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% para estructuras de datos accedidas frecuentemente. El impacto es mayor cuando hay muchos cache misses.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Layout no cache-friendly (Array of Structs cuando se itera)
public struct Item
{
    public int Id;
    public string Name;      // Referencia, puede estar lejos
    public DateTime Created;
    public double Value;
}

public class BadLayout
{
    private Item[] _items = new Item[1000];
    
    public int SumIds()
    {
        int sum = 0;
        foreach (var item in _items)
        {
            sum += item.Id; // Accede a diferentes partes de cada struct
        }
        return sum;
    }
}

// ✅ Bueno: Struct of Arrays para iteración (cuando solo se necesita un campo)
public class GoodLayout
{
    private int[] _ids = new int[1000];
    private string[] _names = new string[1000];
    private DateTime[] _created = new DateTime[1000];
    private double[] _values = new double[1000];
    
    public int SumIds()
    {
        int sum = 0;
        foreach (var id in _ids) // Acceso secuencial, cache-friendly
        {
            sum += id;
        }
        return sum;
    }
}

// ✅ Mejor: Compactar datos frecuentemente accedidos
[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct CacheFriendlyItem
{
    public int Id;           // 4 bytes
    public float Value;      // 4 bytes
    // Datos frecuentemente accedidos juntos, en 8 bytes (una línea de caché)
    
    public string Name;      // Referencia, accedida menos frecuentemente
}
```

---

### Struct of Arrays instead of Array of Structs when iterating (Skipped)

**Cómo funciona:**
Struct of Arrays (SoA) almacena cada campo en un array separado, mientras que Array of Structs (AoS) almacena structs completos. Para iteraciones que solo acceden a un campo, SoA es más cache-friendly.

**Ventajas:**
- Mejor localidad de caché cuando se itera sobre un campo
- Menos datos cargados en caché
- Mejor rendimiento para operaciones vectorizables
- Mejor para SIMD

**Desventajas:**
- Más complejo de manejar
- Menos intuitivo
- Puede ser peor cuando se accede a múltiples campos juntos
- Más difícil de mantener

**Cuándo usar:**
- Cuando se itera sobre un campo específico frecuentemente
- Operaciones vectorizables/SIMD
- Hot paths con acceso a un campo
- Cuando el profiling muestra cache misses

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% para iteraciones sobre un campo. El impacto es mayor con SIMD.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Array of Structs - carga datos innecesarios
public struct Point
{
    public float X;
    public float Y;
    public float Z;
    public int Color;
}

public class BadAoS
{
    private Point[] _points = new Point[1000000];
    
    public float SumX()
    {
        float sum = 0;
        foreach (var point in _points)
        {
            sum += point.X; // Carga X, Y, Z, Color pero solo usa X
        }
        return sum;
    }
}

// ✅ Bueno: Struct of Arrays - solo carga lo necesario
public class GoodSoA
{
    private float[] _x = new float[1000000];
    private float[] _y = new float[1000000];
    private float[] _z = new float[1000000];
    private int[] _color = new int[1000000];
    
    public float SumX()
    {
        float sum = 0;
        foreach (var x in _x) // Solo carga X, cache-friendly
        {
            sum += x;
        }
        return sum;
    }
    
    // ✅ Mejor aún: Usar SIMD para SoA
    public float SumXVectorized()
    {
        var sum = Vector<float>.Zero;
        int i = 0;
        
        for (; i <= _x.Length - Vector<float>.Count; i += Vector<float>.Count)
        {
            var vec = new Vector<float>(_x, i);
            sum += vec;
        }
        
        float result = 0;
        for (int j = 0; j < Vector<float>.Count; j++)
        {
            result += sum[j];
        }
        
        for (; i < _x.Length; i++)
        {
            result += _x[i];
        }
        
        return result;
    }
}
```

---

### Memory-mapped I/O for large file access (Ok)

**Cómo funciona:**
Memory-mapped I/O mapea un archivo directamente en el espacio de direcciones virtual del proceso, permitiendo acceso directo a los datos del archivo como si estuvieran en memoria. El sistema operativo maneja la carga y descarga de páginas automáticamente.

**Ventajas:**
- Acceso eficiente a archivos grandes
- El OS maneja el caching automáticamente
- Puede compartir memoria entre procesos
- No requiere cargar todo el archivo en memoria

**Desventajas:**
- Puede causar page faults si el archivo no está en caché
- Menos control sobre cuándo se carga/descarga
- Puede requerir sincronización para escritura

**Cuándo usar:**
- Archivos grandes que no caben en memoria
- Acceso aleatorio a archivos grandes
- Cuando múltiples procesos necesitan acceder al mismo archivo
- Bases de datos y sistemas de archivos

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% para acceso a archivos grandes comparado con lectura secuencial. El impacto es mayor para acceso aleatorio.

**Ejemplo en C#:**
```csharp
using System.IO.MemoryMappedFiles;

// ✅ Usar MemoryMappedFile para archivos grandes
public class MemoryMappedFileAccess
{
    public void ProcessLargeFile(string filePath)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(
            filePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.Read))
        {
            using (var accessor = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.Read))
            {
                // Acceso directo como si fuera memoria
                byte value = accessor.ReadByte(0);
                int intValue = accessor.ReadInt32(1000);
                // El OS maneja la carga de páginas automáticamente
            }
        }
    }
    
    // ✅ Para acceso aleatorio eficiente
    public void RandomAccess(string filePath, long[] offsets)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(filePath))
        using (var accessor = mmf.CreateViewAccessor())
        {
            foreach (var offset in offsets)
            {
                var value = accessor.ReadInt32(offset); // Acceso directo
                ProcessValue(value);
            }
        }
    }
}
```

---

### Use mlock() to prevent memory from being swapped (Skipped)

**Cómo funciona:**
mlock() (y mlockall() en Linux) previene que el sistema operativo mueva páginas de memoria al swap, garantizando que estén siempre en RAM. Esto es crítico para aplicaciones de baja latencia.

**Ventajas:**
- Garantiza que la memoria esté en RAM
- Elimina latencia de swapping
- Comportamiento predecible
- Mejor para aplicaciones en tiempo real

**Desventajas:**
- Requiere privilegios (root en Linux)
- Reduce memoria disponible para swapping
- Puede causar problemas si se usa demasiada memoria
- No disponible directamente en .NET

**Cuándo usar:**
- Aplicaciones de baja latencia crítica
- Sistemas en tiempo real
- Cuando el swapping causa problemas
- Aplicaciones con memoria crítica

**Impacto en performance:**
Puede eliminar latencia de swapping completamente, mejorando la latencia P99 en un 50-90% cuando hay swapping activo.

**Nota:** En .NET, esto requiere P/Invoke a funciones del sistema operativo.

---

### Memory prefetching hints (__builtin_prefetch) (Pending)

**Cómo funciona:**
Los hints de prefetch le dicen al procesador que cargue datos en caché antes de que se necesiten. Esto es útil cuando se conoce el patrón de acceso futuro.

**Ventajas:**
- Reduce latencia de acceso a memoria
- Mejor utilización de ancho de banda
- Mejor para loops con patrones predecibles

**Desventajas:**
- No disponible directamente en C#
- Requiere código unsafe o P/Invoke
- Puede cargar datos innecesarios
- Efectividad depende del hardware

**Cuándo usar:**
- Loops con acceso a memoria predecible
- Cuando se conoce el patrón de acceso futuro
- Hot paths con muchos cache misses

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-15% para loops con acceso a memoria predecible.

**Nota:** .NET no expone prefetch directamente, pero el hardware prefetcher automático generalmente maneja esto bien.

---

### Cache line alignment (64-byte alignment) (Skipped)

**Cómo funciona:**
Las líneas de caché típicamente son de 64 bytes. Alinear estructuras de datos a límites de 64 bytes puede mejorar el rendimiento al evitar que una estructura cruce múltiples líneas de caché.

**Ventajas:**
- Mejor localidad de caché
- Menos líneas de caché usadas
- Mejor rendimiento de acceso
- Compatible con instrucciones SIMD

**Desventajas:**
- Puede usar más memoria (padding)
- Requiere conocimiento de la arquitectura
- Puede no ser necesario en todos los casos

**Cuándo usar:**
- Estructuras de datos críticas para rendimiento
- Cuando se usan instrucciones SIMD
- Hot paths con acceso frecuente
- Para evitar false sharing

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-15% para estructuras accedidas frecuentemente. El impacto es mayor cuando hay false sharing.

**Ejemplo en C#:**
```csharp
// ✅ Alinear a 64 bytes para evitar false sharing
[StructLayout(LayoutKind.Explicit, Size = 64)]
public struct CacheAlignedCounter
{
    [FieldOffset(0)]
    public long Value;
    // Padding automático a 64 bytes
}

// ✅ Para arrays, usar alineación explícita
public unsafe class AlignedArray
{
    private IntPtr _alignedPtr;
    
    public AlignedArray(int size)
    {
        // Alocar memoria alineada a 64 bytes
        _alignedPtr = Marshal.AllocHGlobal(size + 64);
        var addr = (long)_alignedPtr;
        var aligned = (addr + 63) & ~63; // Alinear a 64
        _alignedPtr = (IntPtr)aligned;
    }
}
```

---

### Memory barriers for lock-free programming (Ok)

**Cómo funciona:**
Memory barriers (fences) aseguran que las operaciones de memoria se completen en un orden específico, crítico para programación lock-free donde múltiples threads acceden a datos compartidos sin locks.

**Ventajas:**
- Permite programación lock-free
- Mejor rendimiento que locks
- Evita race conditions
- Mejor escalabilidad

**Desventajas:**
- Muy complejo de usar correctamente
- Difícil de depurar
- Puede tener bugs sutiles
- Requiere conocimiento profundo

**Cuándo usar:**
- Programación lock-free
- Estructuras de datos lock-free
- Cuando los locks son un cuello de botella
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% comparado con locks en casos de alta contención. El impacto es mayor con muchos threads.

**Ejemplo en C#:**
```csharp
// ✅ Usar Interlocked para operaciones atómicas (incluye memory barriers)
public class LockFreeCounter
{
    private long _value = 0;
    
    public void Increment()
    {
        Interlocked.Increment(ref _value); // Incluye memory barrier
    }
    
    public long Read()
    {
        return Interlocked.Read(ref _value); // Incluye memory barrier
    }
}

// ✅ Memory barriers explícitos (raro en C#)
public class ExplicitBarriers
{
    private int _value = 0;
    private bool _ready = false;
    
    public void Write(int value)
    {
        _value = value;
        Thread.MemoryBarrier(); // Asegura que _value se escriba antes de _ready
        _ready = true;
    }
    
    public int Read()
    {
        if (_ready)
        {
            Thread.MemoryBarrier(); // Asegura que _ready se lea antes de _value
            return _value;
        }
        return 0;
    }
}

// ✅ Mejor: Usar Volatile.Read/Write
public class VolatileAccess
{
    private int _value = 0;
    
    public void Write(int value)
    {
        Volatile.Write(ref _value, value); // Incluye memory barrier
    }
    
    public int Read()
    {
        return Volatile.Read(ref _value); // Incluye memory barrier
    }
}
```

---

### Use memory arenas/pools for allocation patterns

**Cómo funciona:**
Memory arenas (también llamadas regiones o zones) son pools de memoria que se liberan de una vez, en lugar de liberar objetos individualmente. Esto es muy eficiente para patrones de allocation temporales.

**Ventajas:**
- Muy eficiente para allocations temporales
- Liberación en bloque (O(1))
- Sin fragmentación
- Mejor rendimiento

**Desventajas:**
- No se puede liberar objetos individuales
- Requiere conocer el patrón de uso
- Puede usar más memoria
- Más complejo

**Cuándo usar:**
- Allocations temporales con patrón conocido
- Procesamiento por lotes
- Parsers y compiladores
- Cuando se puede liberar todo de una vez

**Impacto en performance:**
Puede mejorar el rendimiento en un 50-90% para patrones de allocation temporales. El impacto es dramático cuando hay muchas allocations pequeñas.

**Ejemplo en C#:**
```csharp
// ✅ Implementación simple de memory arena
public class MemoryArena : IDisposable
{
    private readonly List<byte[]> _blocks = new List<byte[]>();
    private readonly int _blockSize;
    private byte[] _currentBlock;
    private int _currentOffset;
    
    public MemoryArena(int blockSize = 65536)
    {
        _blockSize = blockSize;
        _currentBlock = new byte[blockSize];
        _blocks.Add(_currentBlock);
    }
    
    public Span<byte> Allocate(int size)
    {
        if (_currentOffset + size > _currentBlock.Length)
        {
            _currentBlock = new byte[Math.Max(_blockSize, size)];
            _blocks.Add(_currentBlock);
            _currentOffset = 0;
        }
        
        var span = new Span<byte>(_currentBlock, _currentOffset, size);
        _currentOffset += size;
        return span;
    }
    
    public void Reset()
    {
        // Liberar todo de una vez
        _currentOffset = 0;
        if (_blocks.Count > 1)
        {
            _blocks.RemoveRange(1, _blocks.Count - 1);
            _currentBlock = _blocks[0];
        }
    }
    
    public void Dispose()
    {
        _blocks.Clear();
    }
}

// Uso:
using (var arena = new MemoryArena())
{
    var buffer1 = arena.Allocate(1024);
    var buffer2 = arena.Allocate(2048);
    // Todo se libera al final del using
}
```

---

### Custom allocators for specific use cases

**Cómo funciona:**
Allocators personalizados pueden optimizarse para patrones de uso específicos, como objetos del mismo tamaño, allocations temporales, o allocations de larga duración.

**Ventajas:**
- Optimizado para casos específicos
- Mejor rendimiento que allocator general
- Menos fragmentación
- Mejor control

**Desventajas:**
- Más complejo de implementar
- Requiere conocimiento profundo
- Puede tener bugs
- No siempre es necesario

**Cuándo usar:**
- Patrones de allocation muy específicos
- Cuando el allocator general es un cuello de botella
- Sistemas embebidos
- Aplicaciones de muy alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% para casos específicos. El impacto depende del patrón de uso.

**Ejemplo en C#:**
```csharp
// ✅ Allocator para objetos de tamaño fijo
public class FixedSizeAllocator<T> where T : class, new()
{
    private readonly ConcurrentQueue<T> _pool = new ConcurrentQueue<T>();
    private readonly Func<T> _factory;
    
    public FixedSizeAllocator(Func<T> factory)
    {
        _factory = factory;
    }
    
    public T Rent()
    {
        if (_pool.TryDequeue(out var item))
        {
            return item;
        }
        return _factory();
    }
    
    public void Return(T item)
    {
        _pool.Enqueue(item);
    }
}

// ✅ Allocator con buckets por tamaño
public class BucketAllocator
{
    private readonly Dictionary<int, ConcurrentQueue<byte[]>> _buckets 
        = new Dictionary<int, ConcurrentQueue<byte[]>>();
    
    public byte[] Rent(int size)
    {
        var bucket = GetBucket(size);
        if (bucket.TryDequeue(out var buffer))
        {
            return buffer;
        }
        return new byte[size];
    }
    
    public void Return(byte[] buffer)
    {
        var bucket = GetBucket(buffer.Length);
        bucket.Enqueue(buffer);
    }
    
    private ConcurrentQueue<byte[]> GetBucket(int size)
    {
        // Redondear a potencia de 2
        var bucketSize = (int)Math.Pow(2, Math.Ceiling(Math.Log2(size)));
        return _buckets.GetOrAdd(bucketSize, _ => new ConcurrentQueue<byte[]>());
    }
}
```

---

### Memory compaction strategies

**Cómo funciona:**
Memory compaction mueve objetos en memoria para reducir fragmentación y crear bloques contiguos de memoria libre. En .NET, el GC hace esto automáticamente, pero se puede influir en cuándo ocurre.

**Ventajas:**
- Reduce fragmentación
- Mejor utilización de memoria
- Previene OutOfMemoryException
- Mejor rendimiento a largo plazo

**Desventajas:**
- Puede causar pausas del GC
- Requiere actualizar referencias
- Puede ser costoso
- No siempre es necesario

**Cuándo usar:**
- Aplicaciones de larga duración
- Cuando hay fragmentación significativa
- Sistemas con memoria limitada
- Cuando se observa fragmentación en profiling

**Impacto en performance:**
Puede prevenir OutOfMemoryExceptions y mejorar la utilización de memoria. El impacto en rendimiento puede ser positivo o negativo dependiendo de la frecuencia de compactación.

**Ejemplo en C#:**
```csharp
// En .NET, el GC maneja la compactación automáticamente
// Pero puedes influir en cuándo ocurre:

// ✅ Forzar GC y compactación (usar con cuidado)
public class MemoryCompaction
{
    public void ForceCompaction()
    {
        // Forzar GC de todas las generaciones
        GC.Collect(2, GCCollectionMode.Forced, true, true);
        // Parámetros: generation, mode, blocking, compacting
    }
    
    // ✅ Configurar modo de servidor GC (mejor para compactación)
    // En app.config o runtimeconfig.json:
    // <gcServer enabled="true"/>
    // <gcConcurrent enabled="false"/> // Para mejor compactación
}
```

---

### TLB optimization (Translation Lookaside Buffer)

**Cómo funciona:**
La TLB cachea traducciones de direcciones virtuales a físicas. Optimizar el uso de la TLB reduce TLB misses, que pueden ser costosos.

**Ventajas:**
- Reduce TLB misses
- Mejor rendimiento de acceso a memoria
- Menor overhead de traducción
- Mejor para aplicaciones con grandes espacios de memoria

**Desventajas:**
- Requiere conocimiento de la arquitectura
- Puede requerir cambios en el diseño
- No siempre es aplicable

**Cuándo usar:**
- Aplicaciones con grandes espacios de memoria
- Cuando el profiling muestra muchos TLB misses
- Bases de datos grandes
- Aplicaciones intensivas en memoria

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% para aplicaciones con grandes espacios de memoria. El impacto es mayor cuando hay muchos TLB misses.

**Nota:** En .NET, esto se logra principalmente usando HugePages a nivel del sistema operativo.

---

### Memory bandwidth optimization (Skipped)

**Cómo funciona:**
Memory bandwidth optimization implica organizar el acceso a memoria para maximizar el uso del ancho de banda disponible, típicamente mediante acceso secuencial y prefetching.

**Ventajas:**
- Mejor utilización del ancho de banda
- Mejor rendimiento
- Menos stalls del CPU
- Mejor para operaciones intensivas en memoria

**Desventajas:**
- Puede requerir reorganización de datos
- Puede requerir cambios en algoritmos
- No siempre es posible

**Cuándo usar:**
- Operaciones intensivas en memoria
- Procesamiento de arrays grandes
- Cuando el ancho de banda es un cuello de botella
- Hot paths con mucho acceso a memoria

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para operaciones intensivas en memoria. El impacto es mayor cuando el ancho de banda es limitado.

**Ejemplo en C#:**
```csharp
// ✅ Acceso secuencial para mejor ancho de banda
public class BandwidthOptimized
{
    public void ProcessArray(int[] data)
    {
        // Acceso secuencial utiliza mejor el ancho de banda
        for (int i = 0; i < data.Length; i++)
        {
            data[i] = ProcessValue(data[i]);
        }
    }
    
    // ✅ Procesar en bloques para mejor localidad
    public void ProcessArrayBlocked(int[] data, int blockSize = 64)
    {
        for (int i = 0; i < data.Length; i += blockSize)
        {
            int end = Math.Min(i + blockSize, data.Length);
            // Procesar bloque completo en caché
            for (int j = i; j < end; j++)
            {
                data[j] = ProcessValue(data[j]);
            }
        }
    }
}
```

---

### NUMA-aware memory allocation

**Cómo funciona:**
En sistemas NUMA, la memoria local a un nodo NUMA es más rápida que la memoria remota. NUMA-aware allocation asigna memoria en el nodo local al CPU que la usará.

**Ventajas:**
- Mejor rendimiento de acceso a memoria
- Menor latencia
- Mejor ancho de banda
- Mejor escalabilidad en sistemas multi-socket

**Desventajas:**
- Requiere conocimiento de la topología
- Más complejo
- No todos los sistemas son NUMA
- Puede requerir configuración

**Cuándo usar:**
- Sistemas multi-socket
- Aplicaciones intensivas en memoria
- Sistemas de alto rendimiento
- Cuando el acceso remoto a memoria es costoso

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% en sistemas NUMA al evitar acceso remoto a memoria. El impacto es mayor en aplicaciones intensivas en memoria.

**Nota:** .NET no expone APIs NUMA directamente, pero se puede lograr mediante CPU affinity y configuración del sistema.

---

### Memory deduplication when appropriate

**Cómo funciona:**
Memory deduplication identifica páginas de memoria idénticas y las comparte, reduciendo el uso total de memoria. Esto es útil cuando hay muchos datos duplicados.

**Ventajas:**
- Reduce uso de memoria
- Mejor utilización de recursos
- Puede mejorar rendimiento del sistema
- Útil para virtualización

**Desventajas:**
- Overhead de CPU para identificar duplicados
- Puede causar problemas de seguridad (side-channel attacks)
- No siempre es efectivo
- Puede requerir configuración

**Cuándo usar:**
- Sistemas virtualizados
- Cuando hay muchos datos duplicados
- Sistemas con memoria limitada
- Cuando el overhead es aceptable

**Impacto en performance:**
Puede reducir el uso de memoria en un 10-50% cuando hay muchos datos duplicados. El impacto en rendimiento puede ser positivo o negativo dependiendo del overhead.

**Nota:** Esto típicamente se configura a nivel del sistema operativo o hypervisor, no desde código de aplicación.

---

### Use memory-mapped files for shared data (Skipped)

**Cómo funciona:**
Memory-mapped files permiten que múltiples procesos compartan la misma memoria mapeada desde un archivo, permitiendo comunicación eficiente entre procesos.

**Ventajas:**
- Compartir memoria entre procesos eficientemente
- No requiere serialización
- Muy rápido
- El OS maneja la sincronización

**Desventajas:**
- Requiere sincronización manual para escritura
- Puede causar problemas de seguridad
- Más complejo que otras formas de IPC
- Limitado por tamaño de archivo

**Cuándo usar:**
- Comunicación entre procesos
- Compartir datos grandes entre procesos
- Cuando la serialización es costosa
- Bases de datos y sistemas de archivos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con otras formas de IPC para datos grandes. El impacto es mayor para datos grandes.

**Ejemplo en C#:**
```csharp
using System.IO.MemoryMappedFiles;
using System.Threading;

// ✅ Compartir memoria entre procesos
public class SharedMemory
{
    private const string MapName = "MySharedMemory";
    private const int MapSize = 1024;
    
    public void CreateSharedMemory()
    {
        using (var mmf = MemoryMappedFile.CreateNew(MapName, MapSize))
        {
            using (var accessor = mmf.CreateViewAccessor())
            {
                // Escribir datos compartidos
                accessor.Write(0, 42);
                
                // Otros procesos pueden leer esto
                Thread.Sleep(10000); // Mantener vivo
            }
        }
    }
    
    public void AccessSharedMemory()
    {
        using (var mmf = MemoryMappedFile.OpenExisting(MapName))
        {
            using (var accessor = mmf.CreateViewAccessor())
            {
                // Leer datos compartidos
                int value = accessor.ReadInt32(0);
            }
        }
    }
}
```

---

## Disk and Storage

Esta sección cubre optimizaciones relacionadas con almacenamiento en disco, incluyendo elección de hardware, patrones de I/O, y estrategias de almacenamiento.

### Prefer SSD or NVMe over HDD

**Cómo funciona:**
SSD (Solid State Drive) y NVMe (Non-Volatile Memory Express) usan memoria flash en lugar de discos mecánicos, eliminando la latencia de búsqueda mecánica y proporcionando acceso aleatorio mucho más rápido.

**Ventajas:**
- Latencia mucho menor (microsegundos vs milisegundos)
- Mejor rendimiento de I/O aleatorio
- Sin partes móviles (más confiable)
- Menor consumo energético
- Mejor rendimiento general

**Desventajas:**
- Más costoso por GB
- Vida útil limitada (aunque generalmente suficiente)
- Puede degradarse con el tiempo

**Cuándo usar:**
- Siempre que sea posible para aplicaciones de alto rendimiento
- Bases de datos
- Aplicaciones con mucho I/O
- Sistemas que requieren baja latencia

**Impacto en performance:**
Puede mejorar el rendimiento de I/O en un 10-100x comparado con HDD. Para I/O aleatorio, el impacto es dramático (100-1000x).

---

### Sequential IO over random IO (OK)

**Cómo funciona:**
El acceso secuencial a disco es mucho más rápido que el acceso aleatorio porque aprovecha mejor el ancho de banda del disco y reduce la latencia de búsqueda.

**Ventajas:**
- Mucho más rápido (especialmente en HDD)
- Mejor utilización del ancho de banda
- Menor latencia
- Mejor para streaming

**Desventajas:**
- Puede requerir reorganización de datos
- Puede no ser posible para todos los casos de uso
- Puede requerir cambios en el diseño

**Cuándo usar:**
- Cuando es posible organizar datos secuencialmente
- Procesamiento de logs
- Streaming de datos
- Cuando el rendimiento de I/O es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x para HDD y 2-5x para SSD. El impacto es mayor en HDD.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Acceso aleatorio
public class BadRandomIO
{
    public void ReadRandomPositions(string filePath, long[] positions)
    {
        using (var file = File.OpenRead(filePath))
        {
            foreach (var position in positions)
            {
                file.Seek(position, SeekOrigin.Begin); // Acceso aleatorio
                var buffer = new byte[1024];
                file.Read(buffer, 0, buffer.Length);
            }
        }
    }
}

// ✅ Bueno: Agrupar accesos y ordenar por posición
public class GoodSequentialIO
{
    public void ReadOptimized(string filePath, long[] positions)
    {
        // Ordenar posiciones para acceso más secuencial
        var sorted = positions.OrderBy(p => p).ToArray();
        
        using (var file = File.OpenRead(filePath))
        {
            foreach (var position in sorted)
            {
                file.Seek(position, SeekOrigin.Begin);
                var buffer = new byte[1024];
                file.Read(buffer, 0, buffer.Length);
            }
        }
    }
}

// ✅ Mejor: Leer secuencialmente cuando sea posible
public class BestSequentialIO
{
    public void ReadSequential(string filePath)
    {
        using (var file = File.OpenRead(filePath))
        {
            var buffer = new byte[65536]; // Buffer grande
            int bytesRead;
            while ((bytesRead = file.Read(buffer, 0, buffer.Length)) > 0)
            {
                ProcessChunk(buffer, bytesRead); // Procesar secuencialmente
            }
        }
    }
}
```

---

### Asynchronous IO (Ok)

**Cómo funciona:**
Asynchronous IO permite que el sistema operativo y la aplicación continúen procesando otras operaciones mientras se espera que se complete una operación de I/O, mejorando el throughput y la capacidad de respuesta.

**Ventajas:**
- Mejor utilización de recursos
- Mejor escalabilidad
- No bloquea threads
- Mejor throughput

**Desventajas:**
- Más complejo que I/O síncrono
- Requiere async/await en todo el stack
- Puede ser más lento para I/O muy rápido

**Cuándo usar:**
- Siempre que sea posible en aplicaciones modernas
- Operaciones de I/O que pueden tomar tiempo
- Aplicaciones de servidor
- Cuando se necesita alto throughput

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir más operaciones concurrentes. El impacto es mayor con muchas operaciones de I/O.

**Ejemplo en C#:**
```csharp
// ✅ Usar async I/O en .NET
public async Task ProcessFileAsync(string filePath)
{
    using (var file = new FileStream(filePath, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous))
    {
        var buffer = new byte[4096];
        int bytesRead;
        while ((bytesRead = await file.ReadAsync(buffer, 0, buffer.Length)) > 0)
        {
            await ProcessChunkAsync(buffer, bytesRead);
        }
    }
}
```

---

### Write batching (Ok)

**Cómo funciona:**
Agrupar múltiples escrituras en un solo batch reduce el número de llamadas al sistema y mejora el rendimiento al aprovechar mejor el ancho de banda del disco.

**Ventajas:**
- Menos llamadas al sistema
- Mejor rendimiento
- Mejor utilización del ancho de banda
- Menor overhead

**Desventajas:**
- Puede aumentar latencia
- Requiere gestión de buffers
- Puede requerir flush explícito

**Cuándo usar:**
- Cuando se hacen muchas escrituras pequeñas
- Operaciones de logging
- Bases de datos
- Cuando el throughput es más importante que la latencia

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 5-50x para muchas escrituras pequeñas. El impacto es mayor en HDD.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Escrituras individuales
public class BadBatching
{
    public void WriteLogs(List<string> logs)
    {
        using (var writer = new StreamWriter("log.txt"))
        {
            foreach (var log in logs)
            {
                writer.WriteLine(log); // Cada WriteLine puede ser una llamada al sistema
            }
        }
    }
}

// ✅ Bueno: Batching
public class GoodBatching
{
    public void WriteLogs(List<string> logs)
    {
        using (var writer = new StreamWriter("log.txt"))
        {
            var buffer = new StringBuilder();
            foreach (var log in logs)
            {
                buffer.AppendLine(log);
                if (buffer.Length > 65536) // Flush cuando el buffer es grande
                {
                    writer.Write(buffer.ToString());
                    buffer.Clear();
                }
            }
            if (buffer.Length > 0)
            {
                writer.Write(buffer.ToString());
            }
        }
    }
}
```

---

### Read-ahead tuning

**Cómo funciona:**
Read-ahead (pre-lectura) carga datos en caché antes de que se necesiten, basándose en patrones de acceso secuencial. Ajustar el tamaño de read-ahead puede mejorar el rendimiento.

**Ventajas:**
- Reduce latencia de acceso
- Mejor para acceso secuencial
- Mejor utilización del ancho de banda
- Automático en la mayoría de sistemas

**Desventajas:**
- Puede cargar datos innecesarios
- Usa más memoria
- Puede no ser efectivo para acceso aleatorio

**Cuándo usar:**
- Acceso secuencial a archivos
- Procesamiento de logs
- Streaming de datos
- Cuando el patrón de acceso es predecible

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% para acceso secuencial. El impacto es mayor en HDD.

**Nota:** En .NET, el sistema operativo maneja read-ahead automáticamente. Para control fino, se puede usar FileOptions.SequentialScan.

---

### Append-only storage (Ok)

**Cómo funciona:**
Append-only storage solo permite agregar datos al final, nunca modificar o eliminar datos existentes. Esto simplifica la implementación y mejora el rendimiento de escritura.

**Ventajas:**
- Muy rápido para escrituras
- Sin overhead de actualización
- Mejor para logs y eventos
- Simplifica la implementación

**Desventajas:**
- No permite actualizaciones
- Requiere compactación periódica
- Puede usar más espacio
- Más complejo para queries

**Cuándo usar:**
- Sistemas de logging
- Event sourcing
- WAL (Write-Ahead Logging)
- Cuando las escrituras son mucho más frecuentes que las lecturas

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 5-20x comparado con almacenamiento que permite actualizaciones. El impacto es mayor en HDD.

**Ejemplo en C#:**
```csharp
// ✅ Append-only para logs
public class AppendOnlyLog
{
    private readonly string _logPath;
    
    public AppendOnlyLog(string logPath)
    {
        _logPath = logPath;
    }
    
    public async Task AppendAsync(string message)
    {
        using (var writer = new StreamWriter(_logPath, append: true))
        {
            await writer.WriteLineAsync($"{DateTime.UtcNow:O} {message}");
        }
    }
    
    // Para lectura eficiente, usar FileOptions.SequentialScan
    public async Task ReadAllAsync()
    {
        using (var file = new FileStream(_logPath, FileMode.Open, FileAccess.Read, FileShare.Read, 65536, FileOptions.SequentialScan))
        using (var reader = new StreamReader(file))
        {
            string line;
            while ((line = await reader.ReadLineAsync()) != null)
            {
                ProcessLine(line);
            }
        }
    }
}
```

---

### Memory-mapped files (Skipped)

**Cómo funciona:**
Memory-mapped files mapean un archivo directamente en el espacio de direcciones virtual del proceso, permitiendo acceso directo a los datos del archivo como si estuvieran en memoria.

**Ventajas:**
- Acceso eficiente a archivos grandes
- El OS maneja el caching automáticamente
- Puede compartir memoria entre procesos
- No requiere cargar todo el archivo en memoria

**Desventajas:**
- Puede causar page faults si el archivo no está en caché
- Menos control sobre cuándo se carga/descarga
- Puede requerir sincronización para escritura

**Cuándo usar:**
- Archivos grandes que no caben en memoria
- Acceso aleatorio a archivos grandes
- Cuando múltiples procesos necesitan acceder al mismo archivo
- Bases de datos y sistemas de archivos

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% para acceso a archivos grandes comparado con lectura secuencial. El impacto es mayor para acceso aleatorio.

**Ejemplo en C#:**
```csharp
using System.IO.MemoryMappedFiles;

// ✅ Usar MemoryMappedFile para archivos grandes
public class MemoryMappedFileAccess
{
    public void ProcessLargeFile(string filePath)
    {
        using (var mmf = MemoryMappedFile.CreateFromFile(
            filePath, 
            FileMode.Open, 
            null, 
            0, 
            MemoryMappedFileAccess.Read))
        {
            using (var accessor = mmf.CreateViewAccessor(0, 0, MemoryMappedFileAccess.Read))
            {
                // Acceso directo como si fuera memoria
                byte value = accessor.ReadByte(0);
                int intValue = accessor.ReadInt32(1000);
                // El OS maneja la carga de páginas automáticamente
            }
        }
    }
}
```

---

### Avoid frequent fsync calls (Ok)

**Cómo funciona:**
fsync fuerza que los datos en caché del sistema operativo se escriban al disco. Esto es costoso y hacerlo frecuentemente degrada el rendimiento significativamente.

**Ventajas:**
- Mejor rendimiento
- Menos I/O síncrono
- Mejor throughput
- Menor latencia

**Desventajas:**
- Mayor riesgo de pérdida de datos en caso de fallo
- Requiere balancear durabilidad vs rendimiento
- Puede requerir estrategias alternativas

**Cuándo usar:**
- Cuando la durabilidad inmediata no es crítica
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar pérdida de datos recientes
- Sistemas con replicación

**Impacto en performance:**
Evitar fsync frecuente puede mejorar el rendimiento de escritura en un 10-100x. El impacto es dramático cuando se hace fsync después de cada escritura.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Flush frecuente (equivalente a fsync en algunos casos)
public class BadFsync
{
    public void WriteData(string filePath, IEnumerable<string> data)
    {
        using (var writer = new StreamWriter(filePath))
        {
            foreach (var item in data)
            {
                writer.WriteLine(item);
                writer.Flush(); // Costoso, similar a fsync
            }
        }
    }
}

// ✅ Bueno: Flush solo cuando sea necesario
public class GoodFsync
{
    public void WriteData(string filePath, IEnumerable<string> data)
    {
        using (var writer = new StreamWriter(filePath))
        {
            foreach (var item in data)
            {
                writer.WriteLine(item);
                // No hacer flush en cada escritura
            }
            // Flush al final o periódicamente
        } // Auto-flush al cerrar
    }
}

// ✅ Mejor: Usar FileOptions para control
public void WriteDataOptimized(string filePath, IEnumerable<string> data)
{
    using (var file = new FileStream(filePath, FileMode.Create, FileAccess.Write, FileShare.None, 65536, FileOptions.WriteThrough))
    using (var writer = new StreamWriter(file))
    {
        // WriteThrough puede ser más lento pero más seguro
        // Para máximo rendimiento, no usar WriteThrough
    }
}
```

---

### Avoid many small files (OK)

**Cómo funciona:**
Tener muchos archivos pequeños aumenta el overhead de metadatos del filesystem y puede causar fragmentación, degradando el rendimiento.

**Ventajas:**
- Menos overhead de metadatos
- Menor fragmentación
- Mejor rendimiento
- Mejor utilización del espacio

**Desventajas:**
- Puede requerir reorganización
- Puede requerir cambios en el diseño
- Puede ser menos conveniente

**Cuándo usar:**
- Siempre que sea posible
- Sistemas de almacenamiento
- Cuando se crean muchos archivos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al reducir el overhead de metadatos. El impacto es mayor en filesystems con mucho overhead.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Muchos archivos pequeños
public class BadManyFiles
{
    public void SaveItems(List<Item> items)
    {
        foreach (var item in items)
        {
            var filePath = $"items/{item.Id}.json";
            File.WriteAllText(filePath, JsonSerializer.Serialize(item));
        }
    }
}

// ✅ Bueno: Consolidar en menos archivos
public class GoodManyFiles
{
    public void SaveItems(List<Item> items)
    {
        // Agrupar items en batches
        const int batchSize = 1000;
        for (int i = 0; i < items.Count; i += batchSize)
        {
            var batch = items.Skip(i).Take(batchSize).ToList();
            var filePath = $"items/batch_{i / batchSize}.json";
            File.WriteAllText(filePath, JsonSerializer.Serialize(batch));
        }
    }
}

// ✅ Mejor: Usar base de datos o almacenamiento optimizado
public class BestManyFiles
{
    public async Task SaveItemsAsync(List<Item> items, IDbConnection connection)
    {
        // Usar base de datos en lugar de muchos archivos
        await connection.ExecuteAsync(
            "INSERT INTO Items (Id, Data) VALUES (@Id, @Data)",
            items.Select(i => new { i.Id, Data = JsonSerializer.Serialize(i) }));
    }
}
```

---

### Preallocate files (Ok)

**Cómo funciona:**
Pre-asignar espacio para archivos evita la fragmentación y mejora el rendimiento al asegurar que el archivo tenga espacio contiguo en el disco.

**Ventajas:**
- Evita fragmentación
- Mejor rendimiento de escritura
- Espacio contiguo garantizado
- Mejor para archivos grandes

**Desventajas:**
- Usa espacio inmediatamente
- Puede desperdiciar espacio si no se usa todo
- Requiere conocer el tamaño aproximado

**Cuándo usar:**
- Archivos de tamaño conocido o estimable
- Archivos grandes
- Cuando la fragmentación es un problema
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-30% al evitar fragmentación. El impacto es mayor en HDD.

**Ejemplo en C#:**
```csharp
// ✅ Pre-asignar espacio para archivo
public class PreallocateFile
{
    public void CreatePreallocatedFile(string filePath, long size)
    {
        using (var file = File.Create(filePath))
        {
            file.SetLength(size); // Pre-asignar espacio
            // Ahora el archivo tiene espacio contiguo
        }
    }
    
    // ✅ Para .NET Core 3.0+, usar FileStream con preallocation
    public void CreatePreallocatedFileModern(string filePath, long size)
    {
        // En Linux, FileStream puede usar fallocate automáticamente
        using (var file = new FileStream(filePath, FileMode.Create, FileAccess.Write, FileShare.None, 1, FileOptions.None))
        {
            file.SetLength(size);
        }
    }
}
```

---

### Balance compression versus IO cost (Ok)

**Cómo funciona:**
La compresión reduce el tamaño de datos pero aumenta el uso de CPU. El balance depende de si el cuello de botella es I/O o CPU.

**Ventajas:**
- Reduce tamaño de datos
- Menor uso de ancho de banda
- Menor uso de almacenamiento
- Mejor para transferencia de red

**Desventajas:**
- Aumenta uso de CPU
- Aumenta latencia
- Puede no ser beneficioso si CPU es el cuello de botella

**Cuándo usar:**
- Cuando I/O es el cuello de botella
- Datos que se comprimen bien
- Transferencia de red
- Almacenamiento limitado

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x cuando I/O es el cuello de botella. Puede degradar el rendimiento si CPU es el cuello de botella.

**Ejemplo en C#:**
```csharp
using System.IO.Compression;

// ✅ Comprimir cuando I/O es el cuello de botella
public class CompressionExample
{
    public void WriteCompressed(string filePath, string data)
    {
        using (var file = File.Create(filePath))
        using (var gzip = new GZipStream(file, CompressionLevel.Optimal))
        using (var writer = new StreamWriter(gzip))
        {
            writer.Write(data); // Comprimido automáticamente
        }
    }
    
    public string ReadCompressed(string filePath)
    {
        using (var file = File.OpenRead(filePath))
        using (var gzip = new GZipStream(file, CompressionMode.Decompress))
        using (var reader = new StreamReader(gzip))
        {
            return reader.ReadToEnd();
        }
    }
    
    // ✅ Para mejor rendimiento, usar compresión más rápida
    public void WriteFastCompressed(string filePath, string data)
    {
        using (var file = File.Create(filePath))
        using (var gzip = new GZipStream(file, CompressionLevel.Fastest)) // Más rápido, menos compresión
        using (var writer = new StreamWriter(gzip))
        {
            writer.Write(data);
        }
    }
}
```

---

### Reduce filesystem metadata operations

**Cómo funciona:**
Las operaciones de metadatos (crear archivos, cambiar permisos, etc.) son costosas. Reducirlas mejora el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menos overhead
- Menor latencia
- Mejor para alto throughput

**Desventajas:**
- Puede requerir cambios en el diseño
- Puede ser menos flexible

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando se crean muchos archivos
- Sistemas de logging
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al reducir el overhead de metadatos. El impacto es mayor cuando hay muchas operaciones de metadatos.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Muchas operaciones de metadatos
public class BadMetadata
{
    public void CreateManyFiles(List<string> data)
    {
        foreach (var item in data)
        {
            var fileName = $"file_{Guid.NewGuid()}.txt";
            File.WriteAllText(fileName, item); // Crea archivo, escribe, cierra - muchas operaciones
        }
    }
}

// ✅ Bueno: Reducir operaciones de metadatos
public class GoodMetadata
{
    public void CreateManyFiles(List<string> data)
    {
        // Reutilizar archivo abierto
        using (var writer = new StreamWriter("output.txt"))
        {
            foreach (var item in data)
            {
                writer.WriteLine(item); // Solo escritura, sin crear/cerrar archivos
            }
        }
    }
}
```

---

## File IO

Esta sección cubre optimizaciones para operaciones de archivo, críticas para aplicaciones que procesan archivos grandes o realizan mucho I/O.

### Stream files instead of loading entire files into memory

**Cómo funciona:**
En lugar de cargar un archivo completo en memoria, procesarlo como stream permite procesar archivos de cualquier tamaño sin limitaciones de memoria, procesando datos en chunks.

**Ventajas:**
- Puede procesar archivos de cualquier tamaño
- Menor uso de memoria
- Mejor para archivos grandes
- Permite procesamiento incremental

**Desventajas:**
- Puede ser más lento para archivos pequeños
- Requiere gestión de buffers
- Más complejo que cargar todo

**Cuándo usar:**
- Archivos grandes (>100MB típicamente)
- Cuando la memoria es limitada
- Procesamiento de logs
- Streaming de datos

**Impacto en performance:**
Puede permitir procesar archivos que de otra manera causarían OutOfMemoryException. Para archivos grandes, puede ser la única opción viable.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Cargar archivo completo
public class BadFileLoading
{
    public void ProcessFile(string filePath)
    {
        var content = File.ReadAllText(filePath); // Carga todo en memoria
        ProcessContent(content);
    }
}

// ✅ Bueno: Procesar como stream
public class GoodFileStreaming
{
    public async Task ProcessFileAsync(string filePath)
    {
        using (var reader = new StreamReader(filePath))
        {
            string line;
            while ((line = await reader.ReadLineAsync()) != null)
            {
                ProcessLine(line); // Procesa línea por línea
            }
        }
    }
}

// ✅ Mejor: Usar System.IO.Pipelines para alto rendimiento
using System.IO.Pipelines;

public class BestFileStreaming
{
    public async Task ProcessFilePipelinesAsync(string filePath)
    {
        var pipe = new Pipe();
        var writing = FillPipeAsync(filePath, pipe.Writer);
        var reading = ReadPipeAsync(pipe.Reader);
        
        await Task.WhenAll(writing, reading);
    }
    
    private async Task FillPipeAsync(string filePath, PipeWriter writer)
    {
        using (var file = File.OpenRead(filePath))
        {
            while (true)
            {
                var memory = writer.GetMemory(4096);
                int bytesRead = await file.ReadAsync(memory);
                if (bytesRead == 0) break;
                writer.Advance(bytesRead);
                var result = await writer.FlushAsync();
                if (result.IsCompleted) break;
            }
        }
        await writer.CompleteAsync();
    }
}
```

---

### Choose correct IO chunk sizes

**Cómo funciona:**
El tamaño del buffer usado para I/O afecta el rendimiento. Buffers muy pequeños causan muchas llamadas al sistema, mientras que buffers muy grandes pueden desperdiciar memoria.

**Ventajas:**
- Mejor rendimiento de I/O
- Menos llamadas al sistema
- Mejor utilización de recursos

**Desventajas:**
- Requiere experimentación para encontrar el tamaño óptimo
- Puede variar según el sistema

**Cuándo usar:**
- Siempre que se haga I/O
- Operaciones de archivo frecuentes
- Cuando el I/O es un cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento de I/O en un 20-50% al reducir el número de llamadas al sistema. El tamaño óptimo típicamente está entre 4KB y 64KB.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Buffer muy pequeño
public class BadBufferSize
{
    public void ReadFile(string filePath)
    {
        var buffer = new byte[64]; // Muy pequeño, muchas llamadas
        using (var file = File.OpenRead(filePath))
        {
            int bytesRead;
            while ((bytesRead = file.Read(buffer, 0, buffer.Length)) > 0)
            {
                ProcessChunk(buffer, bytesRead);
            }
        }
    }
}

// ✅ Bueno: Buffer de tamaño apropiado
public class GoodBufferSize
{
    private const int BufferSize = 65536; // 64KB - buen tamaño
    
    public void ReadFile(string filePath)
    {
        var buffer = new byte[BufferSize];
        using (var file = File.OpenRead(filePath))
        {
            int bytesRead;
            while ((bytesRead = file.Read(buffer, 0, buffer.Length)) > 0)
            {
                ProcessChunk(buffer, bytesRead);
            }
        }
    }
}

// ✅ Mejor: Usar FileStream con buffer size configurado
public class BestBufferSize
{
    public void ReadFile(string filePath)
    {
        using (var file = new FileStream(
            filePath, 
            FileMode.Open, 
            FileAccess.Read, 
            FileShare.Read, 
            65536, // Buffer size
            FileOptions.SequentialScan)) // Hint para OS
        {
            var buffer = new byte[65536];
            int bytesRead;
            while ((bytesRead = file.Read(buffer, 0, buffer.Length)) > 0)
            {
                ProcessChunk(buffer, bytesRead);
            }
        }
    }
}
```

---

### Use buffered streams

**Cómo funciona:**
Buffered streams mantienen un buffer interno que reduce el número de llamadas al sistema operativo, agrupando múltiples operaciones pequeñas.

**Ventajas:**
- Reduce llamadas al sistema
- Mejor rendimiento para I/O pequeño
- Automático en .NET para FileStream

**Desventajas:**
- Puede usar más memoria
- Puede causar problemas si se olvida hacer flush

**Cuándo usar:**
- Siempre que sea posible
- Operaciones de I/O pequeñas frecuentes
- Cuando se hacen muchas operaciones pequeñas

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% para operaciones de I/O pequeñas frecuentes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Sin buffering explícito
public class BadBuffering
{
    public void WriteManyLines(string filePath, IEnumerable<string> lines)
    {
        using (var file = File.CreateText(filePath))
        {
            foreach (var line in lines)
            {
                file.WriteLine(line); // Cada WriteLine puede ser una llamada al sistema
            }
        }
    }
}

// ✅ Bueno: BufferedStream (aunque FileStream ya está buffered)
public class GoodBuffering
{
    public void WriteManyLines(string filePath, IEnumerable<string> lines)
    {
        using (var file = File.Create(filePath))
        using (var buffered = new BufferedStream(file, 65536))
        using (var writer = new StreamWriter(buffered))
        {
            foreach (var line in lines)
            {
                writer.WriteLine(line); // Agrupado en buffer
            }
            writer.Flush(); // Asegurar que se escriba
        }
    }
}

// ✅ Mejor: Agrupar escrituras cuando sea posible
public class BestBuffering
{
    public async Task WriteManyLinesAsync(string filePath, IEnumerable<string> lines)
    {
        using (var writer = new StreamWriter(filePath))
        {
            var buffer = new StringBuilder();
            foreach (var line in lines)
            {
                buffer.AppendLine(line);
                if (buffer.Length > 65536) // Flush cuando el buffer es grande
                {
                    await writer.WriteAsync(buffer.ToString());
                    buffer.Clear();
                }
            }
            if (buffer.Length > 0)
            {
                await writer.WriteAsync(buffer.ToString());
            }
        }
    }
}
```

---

### Async file APIs

**Cómo funciona:**
Las APIs async de archivo permiten que el thread se libere mientras espera I/O, permitiendo que otros trabajos se ejecuten. Esto mejora el throughput y la capacidad de respuesta.

**Ventajas:**
- Mejor utilización de threads
- Mejor escalabilidad
- No bloquea threads
- Mejor capacidad de respuesta

**Desventajas:**
- Más complejo que APIs síncronas
- Requiere async/await en todo el stack
- Puede ser más lento para I/O muy rápido

**Cuándo usar:**
- Siempre que sea posible en aplicaciones async
- Operaciones de I/O que pueden tomar tiempo
- Aplicaciones de alto rendimiento
- Servidores y aplicaciones web

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir que más operaciones se ejecuten concurrentemente. El impacto es mayor con muchas operaciones de I/O concurrentes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: APIs síncronas bloquean threads
public class BadAsyncIO
{
    public void ProcessFiles(string[] filePaths)
    {
        foreach (var path in filePaths)
        {
            var content = File.ReadAllText(path); // Bloquea thread
            ProcessContent(content);
        }
    }
}

// ✅ Bueno: Usar APIs async
public class GoodAsyncIO
{
    public async Task ProcessFilesAsync(string[] filePaths)
    {
        var tasks = filePaths.Select(async path =>
        {
            var content = await File.ReadAllTextAsync(path); // No bloquea
            ProcessContent(content);
        });
        await Task.WhenAll(tasks);
    }
}

// ✅ Mejor: Usar FileStream async con buffers
public class BestAsyncIO
{
    public async Task ProcessFileAsync(string filePath)
    {
        using (var file = new FileStream(
            filePath, 
            FileMode.Open, 
            FileAccess.Read, 
            FileShare.Read, 
            65536, 
            FileOptions.Asynchronous | FileOptions.SequentialScan))
        {
            var buffer = new byte[65536];
            int bytesRead;
            while ((bytesRead = await file.ReadAsync(buffer, 0, buffer.Length)) > 0)
            {
                await ProcessChunkAsync(buffer, bytesRead);
            }
        }
    }
}
```

---

---

### Avoid frequent open and close operations

**Cómo funciona:**
Abrir y cerrar archivos frecuentemente causa overhead significativo. Reutilizar archivos abiertos mejora el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Menos llamadas al sistema
- Mejor para operaciones frecuentes

**Desventajas:**
- Requiere gestión de recursos
- Puede retener recursos más tiempo
- Requiere cuidado con locks

**Cuándo usar:**
- Cuando se accede a archivos frecuentemente
- Hot paths con acceso a archivos
- Aplicaciones de alto rendimiento
- Operaciones de logging

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al reducir el overhead de abrir/cerrar archivos. El impacto es mayor cuando se hace frecuentemente.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Abrir/cerrar frecuentemente
public class BadOpenClose
{
    public void ProcessItems(List<Item> items)
    {
        foreach (var item in items)
        {
            using (var writer = new StreamWriter("log.txt", append: true))
            {
                writer.WriteLine(item.ToString()); // Abre y cierra cada vez
            }
        }
    }
}

// ✅ Bueno: Reutilizar archivo abierto
public class GoodOpenClose
{
    public void ProcessItems(List<Item> items)
    {
        using (var writer = new StreamWriter("log.txt", append: true))
        {
            foreach (var item in items)
            {
                writer.WriteLine(item.ToString()); // Reutiliza el mismo archivo
            }
        }
    }
}
```

---

### Bulk read and write operations

**Cómo funciona:**
Leer y escribir en bloques grandes reduce el número de llamadas al sistema y mejora el rendimiento al aprovechar mejor el ancho de banda.

**Ventajas:**
- Menos llamadas al sistema
- Mejor rendimiento
- Mejor utilización del ancho de banda
- Menor overhead

**Desventajas:**
- Requiere más memoria
- Puede aumentar latencia
- Requiere gestión de buffers

**Cuándo usar:**
- Siempre que sea posible
- Operaciones de I/O frecuentes
- Cuando el tamaño de datos lo permite
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20x para operaciones de I/O. El impacto es mayor con buffers más grandes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Operaciones pequeñas
public class BadBulk
{
    public void WriteData(List<byte> data)
    {
        using (var file = File.Create("data.bin"))
        {
            foreach (var byte in data)
            {
                file.WriteByte(byte); // Una llamada por byte
            }
        }
    }
}

// ✅ Bueno: Operaciones en bulk
public class GoodBulk
{
    public void WriteData(List<byte> data)
    {
        using (var file = File.Create("data.bin"))
        {
            var buffer = data.ToArray();
            file.Write(buffer, 0, buffer.Length); // Una llamada para todo
        }
    }
}

// ✅ Mejor: Usar buffers grandes
public class BestBulk
{
    private const int BufferSize = 65536;
    
    public void WriteData(Stream source, Stream destination)
    {
        var buffer = new byte[BufferSize];
        int bytesRead;
        while ((bytesRead = source.Read(buffer, 0, BufferSize)) > 0)
        {
            destination.Write(buffer, 0, bytesRead); // Operaciones grandes
        }
    }
}
```

---

### Avoid file locks

**Cómo funciona:**
Los file locks pueden causar contención y bloqueos cuando múltiples procesos intentan acceder al mismo archivo, degradando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menos contención
- Mejor concurrencia
- Menor latencia

**Desventajas:**
- Puede requerir cambios en el diseño
- Puede requerir sincronización alternativa
- Menos seguro en algunos casos

**Cuándo usar:**
- Cuando múltiples procesos acceden a archivos
- Aplicaciones de alto rendimiento
- Cuando se puede evitar locks
- Sistemas con alta concurrencia

**Impacto en performance:**
Evitar file locks puede mejorar el rendimiento en un 10-100x cuando hay contención. El impacto es dramático con muchos procesos.

**Ejemplo en C#:**
```csharp
// ❌ Malo: File locks que causan contención
public class BadFileLock
{
    public void WriteToSharedFile(string filePath, string data)
    {
        // FileShare.None causa locks
        using (var file = File.Open(filePath, FileMode.Append, FileAccess.Write, FileShare.None))
        using (var writer = new StreamWriter(file))
        {
            writer.WriteLine(data); // Bloquea el archivo
        }
    }
}

// ✅ Bueno: Permitir acceso compartido cuando sea posible
public class GoodFileLock
{
    public void WriteToSharedFile(string filePath, string data)
    {
        // FileShare.Read permite lectura concurrente
        using (var file = File.Open(filePath, FileMode.Append, FileAccess.Write, FileShare.Read))
        using (var writer = new StreamWriter(file))
        {
            writer.WriteLine(data); // No bloquea lectores
        }
    }
}

// ✅ Mejor: Usar mecanismos de sincronización más granulares
public class BestFileLock
{
    private readonly SemaphoreSlim _semaphore = new SemaphoreSlim(1, 1);
    
    public async Task WriteToSharedFileAsync(string filePath, string data)
    {
        await _semaphore.WaitAsync();
        try
        {
            await File.AppendAllTextAsync(filePath, data + Environment.NewLine);
        }
        finally
        {
            _semaphore.Release();
        }
    }
}
```

---

### Cache file contents

**Cómo funciona:**
Cachear el contenido de archivos frecuentemente accedidos en memoria evita I/O repetido y mejora el rendimiento significativamente.

**Ventajas:**
- Mucho más rápido que leer del disco
- Reduce I/O
- Mejor rendimiento
- Mejor experiencia de usuario

**Desventajas:**
- Usa memoria
- Puede requerir invalidación
- Puede no estar actualizado

**Cuándo usar:**
- Archivos leídos frecuentemente
- Archivos que no cambian frecuentemente
- Archivos pequeños o medianos
- Cuando la memoria está disponible

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para archivos cacheados comparado con leer del disco cada vez. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Cachear contenido de archivos
public class FileCache
{
    private readonly IMemoryCache _cache;
    private readonly string _filePath;
    
    public FileCache(IMemoryCache cache, string filePath)
    {
        _cache = cache;
        _filePath = filePath;
    }
    
    public async Task<string> GetContentAsync()
    {
        if (!_cache.TryGetValue(_filePath, out string content))
        {
            content = await File.ReadAllTextAsync(_filePath);
            var options = new MemoryCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromMinutes(5),
                SlidingExpiration = TimeSpan.FromMinutes(1)
            };
            _cache.Set(_filePath, content, options);
        }
        return content;
    }
    
    // ✅ Invalidar cache cuando el archivo cambia
    public void InvalidateCache()
    {
        _cache.Remove(_filePath);
    }
}
```

---

### Use RAM disks for temporary files

**Cómo funciona:**
RAM disks almacenan archivos en memoria RAM en lugar de disco, proporcionando acceso extremadamente rápido pero volátil.

**Ventajas:**
- Extremadamente rápido (nanosegundos vs milisegundos)
- Sin latencia de disco
- Mejor para archivos temporales
- Mejor rendimiento

**Desventajas:**
- Volátil (se pierde al reiniciar)
- Usa memoria RAM
- Tamaño limitado
- Requiere configuración del sistema

**Cuándo usar:**
- Archivos temporales frecuentemente accedidos
- Cuando la velocidad es crítica
- Archivos que se pueden regenerar
- Sistemas con suficiente RAM

**Impacto en performance:**
Puede mejorar el rendimiento en un 100-10000x comparado con disco. El impacto es dramático para archivos temporales.

**Nota:** En .NET, esto se configura a nivel del sistema operativo (tmpfs en Linux, RAM disk en Windows). Luego se usa la ruta normalmente.

---

### Avoid synchronous IO APIs

**Cómo funciona:**
Las APIs síncronas de I/O bloquean el thread hasta que se completa la operación, reduciendo el throughput y la escalabilidad.

**Ventajas:**
- Mejor escalabilidad
- Mejor throughput
- No bloquea threads
- Mejor para aplicaciones modernas

**Desventajas:**
- Requiere async/await en todo el stack
- Más complejo
- Puede ser más lento para I/O muy rápido

**Cuándo usar:**
- Siempre que sea posible en aplicaciones async
- Aplicaciones de servidor
- Cuando se necesita alto throughput
- Aplicaciones modernas

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir más operaciones concurrentes. El impacto es mayor con muchas operaciones de I/O.

**Ejemplo en C#:**
```csharp
// ❌ Malo: APIs síncronas
public class BadSyncIO
{
    public void ProcessFile(string filePath)
    {
        var content = File.ReadAllText(filePath); // Bloquea thread
        ProcessContent(content);
    }
}

// ✅ Bueno: APIs async
public class GoodAsyncIO
{
    public async Task ProcessFileAsync(string filePath)
    {
        var content = await File.ReadAllTextAsync(filePath); // No bloquea
        ProcessContent(content);
    }
}
```

---

### Use sendfile() for zero-copy file-to-socket transfers (Linux)

**Cómo funciona:**
sendfile() transfiere datos directamente del archivo al socket sin copiar a user space, eliminando copias innecesarias.

**Ventajas:**
- Elimina copias de memoria
- Mejor rendimiento
- Menor uso de CPU
- Mejor para transferencias grandes

**Desventajas:**
- Solo Linux
- Requiere P/Invoke en .NET
- Limitado a file-to-socket

**Cuándo usar:**
- Servidores de archivos
- Transferencias grandes
- Cuando el I/O es un cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al eliminar copias. En .NET, FileStream y NetworkStream pueden usar esto automáticamente en algunos casos.

**Nota:** En .NET Core/5+, el runtime puede usar sendfile automáticamente para operaciones de red cuando es posible.

---

### Use splice() for zero-copy pipe-to-socket transfers

**Cómo funciona:**
splice() transfiere datos entre file descriptors sin copiar a user space, útil para pipes y sockets.

**Ventajas:**
- Elimina copias
- Mejor rendimiento
- Menor uso de CPU

**Desventajas:**
- Solo Linux
- Requiere P/Invoke
- Más complejo

**Cuándo usar:**
- Procesamiento de datos en pipes
- Cuando se necesita máximo rendimiento
- Sistemas de alto throughput

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-40% al eliminar copias. En .NET, esto se maneja automáticamente por el runtime cuando es posible.

---

### Use vmsplice() for zero-copy user memory to pipe

**Cómo funciona:**
vmsplice() transfiere datos desde user memory a un pipe sin copiar, útil para procesamiento de datos.

**Ventajas:**
- Elimina copias
- Mejor rendimiento
- Menor uso de CPU

**Desventajas:**
- Solo Linux
- Requiere P/Invoke
- Más complejo

**Cuándo usar:**
- Procesamiento de datos en memoria
- Cuando se necesita máximo rendimiento
- Sistemas de alto throughput

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-40% al eliminar copias.

**Nota:** En .NET, estas optimizaciones de bajo nivel se manejan automáticamente por el runtime cuando es posible.

---

### Direct IO (O_DIRECT) to bypass page cache when appropriate

**Cómo funciona:**
O_DIRECT bypassa el page cache del OS, escribiendo directamente al dispositivo. Útil cuando la aplicación maneja su propio caching.

**Ventajas:**
- Control total sobre caching
- Evita doble caching
- Mejor para aplicaciones con caching propio

**Desventajas:**
- Requiere alineamiento específico
- Más complejo
- Puede ser más lento si no se usa bien

**Cuándo usar:**
- Bases de datos con caching propio
- Aplicaciones con caching agresivo
- Cuando se necesita control total

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% cuando se evita doble caching. Puede degradar el rendimiento si no se usa apropiadamente.

**Nota:** En .NET, esto requiere P/Invoke y configuración específica. Generalmente no es necesario ya que .NET maneja el caching eficientemente.

---

### Use io_uring for high-performance async IO (Linux 5.1+)

**Cómo funciona:**
io_uring es una interfaz de I/O asíncrona de Linux que permite submit y completion de operaciones de I/O de manera muy eficiente.

**Ventajas:**
- Muy alto rendimiento
- Bajo overhead
- Mejor para alto throughput
- Soporta batching

**Desventajas:**
- Solo Linux 5.1+
- Requiere P/Invoke
- Más complejo

**Cuándo usar:**
- Sistemas de muy alto rendimiento
- Cuando el I/O es el cuello de botella
- Aplicaciones que hacen mucho I/O

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% comparado con epoll para algunas cargas de trabajo.

**Nota:** En .NET, el runtime puede usar io_uring automáticamente cuando está disponible. Para control directo, se requiere P/Invoke.

---

### Use AIO (Asynchronous IO) for non-blocking file operations

**Cómo funciona:**
AIO permite operaciones de I/O asíncronas sin bloquear threads, mejorando la concurrencia.

**Ventajas:**
- No bloquea threads
- Mejor concurrencia
- Mejor escalabilidad

**Desventajas:**
- Complejidad adicional
- No disponible en todos los sistemas
- Requiere gestión de callbacks

**Cuándo usar:**
- Aplicaciones con mucho I/O
- Cuando se necesita alta concurrencia
- Sistemas de alto throughput

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir más operaciones concurrentes.

**Nota:** En .NET, las APIs async (FileStream, etc.) ya usan I/O asíncrono eficientemente. No es necesario usar AIO directamente.

---

### Preallocate file space to avoid fragmentation

**Cómo funciona:**
Pre-asignar espacio para archivos evita fragmentación al asegurar que el archivo tenga espacio contiguo.

**Ventajas:**
- Evita fragmentación
- Mejor rendimiento de escritura
- Espacio contiguo garantizado

**Desventajas:**
- Usa espacio inmediatamente
- Puede desperdiciar espacio

**Cuándo usar:**
- Archivos de tamaño conocido
- Archivos grandes
- Cuando la fragmentación es un problema

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-30% al evitar fragmentación.

**Ejemplo en C#:**
```csharp
// ✅ Pre-asignar espacio
using (var file = File.Create("large.bin"))
{
    file.SetLength(1024 * 1024 * 100); // Pre-asignar 100MB
    // Ahora el archivo tiene espacio contiguo
}
```

---

### Use fallocate() for file space preallocation

**Cómo funciona:**
fallocate() pre-asigna espacio para archivos de manera eficiente, sin escribir datos reales.

**Ventajas:**
- Muy rápido
- No escribe datos
- Mejor que SetLength en algunos casos

**Desventajas:**
- Solo Linux
- Requiere P/Invoke

**Cuándo usar:**
- Archivos grandes
- Cuando se necesita pre-asignación rápida
- Sistemas Linux

**Impacto en performance:**
Puede ser más rápido que SetLength para archivos muy grandes.

**Nota:** En .NET, FileStream.SetLength() puede usar fallocate automáticamente en Linux cuando es posible.

---

### Optimize filesystem block size for workload

**Cómo funciona:**
Ajustar el block size del filesystem puede mejorar el rendimiento según el patrón de acceso (archivos pequeños vs grandes).

**Ventajas:**
- Mejor rendimiento para patrones específicos
- Mejor utilización del espacio
- Optimizado para carga de trabajo

**Desventajas:**
- Requiere formatear el filesystem
- Requiere conocimiento del patrón de acceso
- No se puede cambiar fácilmente

**Cuándo usar:**
- Cuando se conoce el patrón de acceso
- Archivos principalmente pequeños o grandes
- Optimización de sistemas específicos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para patrones de acceso específicos.

**Nota:** Esto se configura a nivel del filesystem, no desde .NET directamente.

---

### Use noatime mount option to avoid access time updates

**Cómo funciona:**
noatime deshabilita la actualización de access time en archivos, eliminando escrituras innecesarias.

**Ventajas:**
- Elimina escrituras innecesarias
- Mejor rendimiento
- Menor desgaste en SSD

**Desventajas:**
- Pierde información de access time
- Requiere configuración del sistema

**Cuándo usar:**
- Sistemas de alto rendimiento
- Cuando access time no es necesario
- Sistemas con mucho acceso a archivos

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-15% al eliminar escrituras de metadatos.

**Nota:** Esto se configura a nivel del sistema operativo en /etc/fstab (Linux).

---

### Disable file system journaling for read-heavy workloads (with caution)

**Cómo funciona:**
Deshabilitar journaling elimina el overhead de escribir logs de transacciones, mejorando el rendimiento pero aumentando el riesgo de corrupción.

**Ventajas:**
- Mejor rendimiento de escritura
- Menos overhead
- Mejor para read-heavy

**Desventajas:**
- Mayor riesgo de corrupción
- Requiere extrema precaución
- No recomendado para producción

**Cuándo usar:**
- Solo para workloads read-heavy específicos
- Datos que se pueden regenerar
- Con extrema precaución

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-30% pero aumenta significativamente el riesgo.

**Nota:** Generalmente NO recomendado. El journaling es importante para la integridad de datos.

---

### Use tmpfs for temporary files

**Cómo funciona:**
tmpfs es un filesystem en RAM que almacena archivos en memoria, proporcionando acceso extremadamente rápido.

**Ventajas:**
- Extremadamente rápido
- Sin latencia de disco
- Mejor para archivos temporales

**Desventajas:**
- Volátil (se pierde al reiniciar)
- Limitado por RAM disponible
- Requiere configuración

**Cuándo usar:**
- Archivos temporales frecuentemente accedidos
- Cuando la velocidad es crítica
- Archivos que se pueden regenerar

**Impacto en performance:**
Puede mejorar el rendimiento en un 100-10000x comparado con disco para archivos temporales.

**Nota:** En Linux, /tmp a menudo está montado como tmpfs. Se puede configurar en /etc/fstab.

---

### Optimize directory structure to reduce path lookups

**Cómo funciona:**
Estructuras de directorios planas o bien organizadas reducen el número de lookups necesarios para acceder a archivos.

**Ventajas:**
- Menos lookups
- Mejor rendimiento
- Menor latencia

**Desventajas:**
- Puede requerir reorganización
- Menos organización lógica

**Cuándo usar:**
- Sistemas con muchos archivos
- Cuando el acceso a archivos es frecuente
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al reducir lookups de directorios.

**Ejemplo:**
```csharp
// ❌ Malo: Estructura profunda
var path = "a/b/c/d/e/f/file.txt"; // 6 lookups

// ✅ Bueno: Estructura plana o menos profunda
var path = "files/file.txt"; // 1 lookup
```

---

### Use hard links instead of copying when possible

**Cómo funciona:**
Hard links permiten múltiples nombres para el mismo archivo sin duplicar datos, ahorrando espacio y tiempo.

**Ventajas:**
- No duplica datos
- Más rápido que copiar
- Ahorra espacio

**Desventajas:**
- Solo mismo filesystem
- Requiere gestión cuidadosa
- Menos flexible

**Cuándo usar:**
- Cuando se necesita el mismo archivo en múltiples lugares
- Backup y versionado
- Cuando se puede evitar copiar

**Impacto en performance:**
Puede ser más rápido que copiar y ahorra espacio significativo.

**Ejemplo en C#:**
```csharp
// ✅ Crear hard link (requiere P/Invoke en Windows, nativo en Linux)
// En Linux, usar link() system call
// En .NET, no hay API directa, requiere P/Invoke
```

---

## .NET and C# Performance

Esta sección cubre optimizaciones específicas de .NET y C# que pueden mejorar significativamente el rendimiento de aplicaciones .NET.

### Use Server GC

**Cómo funciona:**
Server GC usa múltiples heaps de GC (uno por CPU lógico) y threads dedicados para recolección, optimizado para throughput en servidores. Workstation GC usa un solo heap y está optimizado para aplicaciones de escritorio con menor latencia.

**Ventajas:**
- Mejor throughput en servidores
- Mejor escalabilidad multi-core
- Mejor para aplicaciones con muchos threads
- Mejor utilización de recursos en servidores

**Desventajas:**
- Mayor uso de memoria
- Puede tener mayor latencia de GC
- No es ideal para aplicaciones de escritorio
- Requiere configuración

**Cuándo usar:**
- Aplicaciones de servidor
- Aplicaciones con muchos threads
- Aplicaciones de alto throughput
- Cuando el throughput es más importante que la latencia

**Impacto en performance:**
Puede mejorar el throughput en un 20-50% en aplicaciones de servidor multi-threaded. El impacto es mayor con más CPUs.

**Ejemplo en C#:**
```csharp
// Configurar en runtimeconfig.json o app.config
// runtimeconfig.json:
{
  "runtimeOptions": {
    "configProperties": {
      "System.GC.Server": true,
      "System.GC.Concurrent": false  // Para mejor compactación
    }
  }
}

// O en código (solo funciona antes del primer GC):
// GCSettings.IsServerGC = true; // No funciona en runtime

// Para .NET Core, usar variable de entorno o runtimeconfig.json
// DOTNET_gcServer=1
```

---

### Tune GC heaps

**Cómo funciona:**
Ajustar el tamaño de los heaps de GC puede mejorar el rendimiento. Heaps más grandes reducen la frecuencia de collections pero usan más memoria.

**Ventajas:**
- Menos frecuencia de GC
- Mejor rendimiento
- Más control sobre el GC

**Desventajas:**
- Más uso de memoria
- Requiere experimentación
- Puede no ser necesario

**Cuándo usar:**
- Cuando el GC es un cuello de botella
- Aplicaciones con patrones de memoria conocidos
- Cuando hay memoria disponible
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede reducir la frecuencia de GC en un 30-50% y mejorar el rendimiento general en un 10-20%.

**Ejemplo en C#:**
```csharp
// Configurar límites de GC heap en runtimeconfig.json
{
  "runtimeOptions": {
    "configProperties": {
      "System.GC.HeapHardLimit": "2000000000",  // 2GB
      "System.GC.HeapHardLimitPercent": 50
    }
  }
}

// Forzar GC cuando sea apropiado (usar con cuidado)
public class GCTuning
{
    public void OptimizeMemory()
    {
        // Forzar compactación
        GC.Collect(2, GCCollectionMode.Forced, true, true);
        GC.WaitForPendingFinalizers();
        GC.Collect(2, GCCollectionMode.Forced, true, true);
    }
}
```

---

### Avoid allocations in hot paths

**Cómo funciona:**
Cada allocation en el heap requiere trabajo del GC. Evitar allocations en hot paths (código ejecutado frecuentemente) reduce la presión en el GC y mejora el rendimiento.

**Ventajas:**
- Menos presión en GC
- Mejor rendimiento
- Menor latencia
- Menor uso de memoria

**Desventajas:**
- Puede requerir cambios en el código
- Puede requerir pooling
- Puede reducir legibilidad

**Cuándo usar:**
- Siempre en hot paths
- Código ejecutado millones de veces
- Aplicaciones de alto rendimiento
- Cuando el profiling muestra muchas allocations

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% en hot paths con muchas allocations. El impacto es dramático cuando se eliminan allocations innecesarias.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Allocation en hot path
public class BadHotPath
{
    public int ProcessItem(int value)
    {
        var list = new List<int>(); // Allocation cada vez
        list.Add(value);
        return list.Count;
    }
}

// ✅ Bueno: Evitar allocation
public class GoodHotPath
{
    public int ProcessItem(int value)
    {
        return 1; // Sin allocation
    }
    
    // Si necesitas colección, usar ArrayPool o reutilizar
    private readonly List<int> _reusableList = new List<int>();
    
    public int ProcessItemWithCollection(int value)
    {
        _reusableList.Clear(); // Reutilizar
        _reusableList.Add(value);
        return _reusableList.Count;
    }
}

// ✅ Mejor: Usar stackalloc para datos temporales pequeños
public class BestHotPath
{
    public unsafe int ProcessItems(Span<int> values)
    {
        Span<int> temp = stackalloc int[100]; // Stack, no heap
        // Procesar sin allocations
        return temp.Length;
    }
}
```

---

### Object pooling

**Cómo funciona:**
Object pooling reutiliza objetos en lugar de crearlos y destruirlos constantemente, reduciendo allocations y presión en el GC.

**Ventajas:**
- Reduce allocations dramáticamente
- Menos presión en GC
- Mejor rendimiento
- Menor fragmentación

**Desventajas:**
- Requiere gestión manual
- Puede usar más memoria
- Más complejo
- Requiere limpieza entre usos

**Cuándo usar:**
- Objetos creados frecuentemente
- Hot paths con muchas allocations
- Cuando el GC es un cuello de botella
- Objetos costosos de crear

**Impacto en performance:**
Puede reducir allocations en un 50-90% y mejorar el rendimiento en un 20-50% en código con muchas allocations temporales.

**Ejemplo en C#:**
```csharp
// ✅ Usar ArrayPool para arrays
public class ArrayPoolExample
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData(int size)
    {
        var buffer = _pool.Rent(size);
        try
        {
            // Usar buffer
            ProcessBuffer(buffer);
        }
        finally
        {
            _pool.Return(buffer);
        }
    }
}

// ✅ Pool personalizado para objetos
public class ObjectPool<T> where T : class, new()
{
    private readonly ConcurrentQueue<T> _pool = new ConcurrentQueue<T>();
    
    public T Rent()
    {
        if (_pool.TryDequeue(out var item))
        {
            return item;
        }
        return new T();
    }
    
    public void Return(T item)
    {
        // Resetear si es necesario
        if (item is IResettable resettable)
        {
            resettable.Reset();
        }
        _pool.Enqueue(item);
    }
}

// ✅ Usar Microsoft.Extensions.ObjectPool
using Microsoft.Extensions.ObjectPool;

public class ServiceWithPooling
{
    private readonly ObjectPool<StringBuilder> _pool;
    
    public ServiceWithPooling(ObjectPoolProvider provider)
    {
        _pool = provider.CreateStringBuilderPool();
    }
    
    public string BuildString()
    {
        var sb = _pool.Get();
        try
        {
            sb.Append("Hello");
            return sb.ToString();
        }
        finally
        {
            _pool.Return(sb);
        }
    }
}
```

---

### Prefer structs for small data

**Cómo funciona:**
Los structs en C# son tipos de valor que se almacenan en el stack o inline, evitando allocations del heap y mejorando la localidad de caché.

**Ventajas:**
- Sin allocations del heap
- Mejor localidad de caché
- Mejor rendimiento
- Menos presión en GC

**Desventajas:**
- Se copian al pasar como parámetros (a menos que se use in/ref)
- Tamaño limitado (típicamente <16 bytes recomendado)
- No pueden ser null (a menos que Nullable<T>)
- No soportan herencia

**Cuándo usar:**
- Tipos de datos pequeños (<16 bytes)
- Tipos inmutables
- Cuando se crean muchos objetos
- Hot paths con muchas instancias

**Impacto en performance:**
Puede eliminar allocations completamente y mejorar el rendimiento en un 20-50% para tipos pequeños creados frecuentemente.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Class para datos pequeños
public class PointClass
{
    public int X { get; set; }
    public int Y { get; set; }
}

// ✅ Bueno: Struct para datos pequeños
public struct PointStruct
{
    public int X { get; set; }
    public int Y { get; set; }
    
    public PointStruct(int x, int y)
    {
        X = x;
        Y = y;
    }
}

// ✅ Mejor: Struct readonly para inmutabilidad
public readonly struct ImmutablePoint
{
    public int X { get; }
    public int Y { get; }
    
    public ImmutablePoint(int x, int y)
    {
        X = x;
        Y = y;
    }
}

// ✅ Usar in para evitar copias grandes
public void ProcessPoint(in PointStruct point) // No copia
{
    // Usar point
}

// ⚠️ Evitar structs grandes (se copian completamente)
public struct LargeStruct // ❌ Malo si es >16 bytes
{
    public long Field1, Field2, Field3, Field4; // 32 bytes - se copia completo
}
```

---

### Avoid boxing and unboxing

**Cómo funciona:**
Boxing convierte un tipo de valor a object (heap allocation), y unboxing lo convierte de vuelta. Esto causa allocations innecesarias y degrada el rendimiento.

**Ventajas:**
- Evita allocations innecesarias
- Mejor rendimiento
- Menos presión en GC
- Código más eficiente

**Desventajas:**
- Requiere atención al código
- Puede requerir cambios en APIs

**Cuándo usar:**
- Siempre que sea posible
- Hot paths
- Cuando se usan tipos de valor con object
- Código genérico

**Impacto en performance:**
Puede eliminar allocations y mejorar el rendimiento en un 10-30% en código con mucho boxing/unboxing.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Boxing innecesario
public class BadBoxing
{
    public void ProcessValue(int value)
    {
        object boxed = value; // Boxing - allocation
        ProcessObject(boxed);
        int unboxed = (int)boxed; // Unboxing
    }
    
    public void AddToList()
    {
        var list = new ArrayList(); // ❌ Boxes todos los valores
        list.Add(42); // Boxing
        list.Add(100); // Boxing
    }
}

// ✅ Bueno: Evitar boxing
public class GoodBoxing
{
    public void ProcessValue(int value)
    {
        ProcessInt(value); // Sin boxing
    }
    
    public void AddToList()
    {
        var list = new List<int>(); // ✅ No boxea
        list.Add(42); // Sin boxing
        list.Add(100); // Sin boxing
    }
}

// ✅ Usar genéricos para evitar boxing
public class GenericExample<T> where T : struct
{
    public void ProcessValue(T value) // Sin boxing
    {
        // Procesar value directamente
    }
}

// ✅ Usar interfaces genéricas
public interface IProcessor<T>
{
    void Process(T value); // Sin boxing si T es struct
}
```

---

### Use Span and Memory

**Cómo funciona:**
Span<T> y Memory<T> proporcionan una vista type-safe y memory-safe sobre buffers de memoria, permitiendo trabajar con arrays, stackalloc, y memoria nativa sin allocations adicionales.

**Ventajas:**
- Sin allocations adicionales
- Type-safe
- Memory-safe
- Mejor rendimiento
- Permite zero-copy

**Desventajas:**
- Requiere .NET Core 2.1+
- Puede requerir código unsafe para algunos casos
- Curva de aprendizaje

**Cuándo usar:**
- Trabajar con buffers
- APIs que aceptan arrays
- Operaciones de I/O
- Hot paths con manipulación de arrays
- Cuando se necesita zero-copy

**Impacto en performance:**
Puede eliminar allocations y mejorar el rendimiento en un 20-50% en código que manipula arrays frecuentemente.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear nuevos arrays
public class BadSpan
{
    public int[] ProcessArray(int[] input)
    {
        var result = new int[input.Length]; // Allocation
        for (int i = 0; i < input.Length; i++)
        {
            result[i] = input[i] * 2;
        }
        return result;
    }
}

// ✅ Bueno: Usar Span<T>
public class GoodSpan
{
    public void ProcessArray(Span<int> input, Span<int> output)
    {
        // Sin allocations, trabaja directamente con memoria
        for (int i = 0; i < input.Length; i++)
        {
            output[i] = input[i] * 2;
        }
    }
    
    // ✅ Usar stackalloc para buffers temporales
    public int SumFirstN(int[] array, int n)
    {
        Span<int> slice = array.AsSpan(0, n); // Sin copia
        int sum = 0;
        foreach (var value in slice)
        {
            sum += value;
        }
        return sum;
    }
}

// ✅ Memory<T> para casos async
public async Task ProcessMemoryAsync(Memory<byte> buffer)
{
    // Memory<T> puede ser almacenado y pasado a async methods
    await ProcessBufferAsync(buffer);
}

// ✅ Slice para vistas sin copia
public void ProcessChunks(Span<int> data)
{
    for (int i = 0; i < data.Length; i += 100)
    {
        var chunk = data.Slice(i, Math.Min(100, data.Length - i));
        ProcessChunk(chunk); // Sin copia
    }
}
```

---

### Use ArrayPool

**Cómo funciona:**
ArrayPool<T> proporciona un pool de arrays reutilizables, eliminando allocations frecuentes de arrays temporales.

**Ventajas:**
- Elimina allocations de arrays temporales
- Mejor rendimiento
- Menos presión en GC
- Thread-safe

**Desventajas:**
- Requiere devolver arrays al pool
- Los arrays pueden ser más grandes de lo solicitado
- Requiere limpieza si es necesario

**Cuándo usar:**
- Arrays temporales frecuentes
- Operaciones de I/O
- Hot paths con arrays
- Cuando se necesitan buffers temporales

**Impacto en performance:**
Puede eliminar allocations de arrays y mejorar el rendimiento en un 20-40% en código con muchos arrays temporales.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear arrays constantemente
public class BadArrayPool
{
    public void ProcessData(Stream stream)
    {
        var buffer = new byte[4096]; // Allocation cada vez
        int bytesRead;
        while ((bytesRead = stream.Read(buffer, 0, buffer.Length)) > 0)
        {
            ProcessBuffer(buffer, bytesRead);
        }
    }
}

// ✅ Bueno: Usar ArrayPool
public class GoodArrayPool
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData(Stream stream)
    {
        var buffer = _pool.Rent(4096); // Reutiliza array
        try
        {
            int bytesRead;
            while ((bytesRead = stream.Read(buffer, 0, buffer.Length)) > 0)
            {
                ProcessBuffer(buffer, bytesRead);
            }
        }
        finally
        {
            _pool.Return(buffer); // Devuelve al pool
        }
    }
}

// ✅ Para diferentes tamaños
public class FlexibleArrayPool
{
    private readonly ArrayPool<byte> _pool = ArrayPool<byte>.Shared;
    
    public void ProcessData(Stream stream, int bufferSize)
    {
        var buffer = _pool.Rent(bufferSize); // Puede ser más grande
        try
        {
            // Usar solo la parte que necesitas
            var actualBuffer = buffer.AsSpan(0, bufferSize);
            // ...
        }
        finally
        {
            _pool.Return(buffer);
        }
    }
}
```

---

### Avoid LINQ in hot paths

**Cómo funciona:**
LINQ es conveniente pero crea allocations (delegates, iterators, closures) y overhead. En hot paths, el código manual puede ser mucho más rápido.

**Ventajas:**
- Mejor rendimiento
- Menos allocations
- Menos overhead
- Control total

**Desventajas:**
- Más código
- Menos legible
- Más propenso a errores

**Cuándo usar:**
- Hot paths ejecutados millones de veces
- Cuando el profiling muestra LINQ como cuello de botella
- Código crítico de rendimiento
- Cuando las allocations importan

**Impacto en performance:**
Puede mejorar el rendimiento en un 50-500% en hot paths. El impacto es dramático cuando LINQ crea muchas allocations.

**Ejemplo en C#:**
```csharp
// ❌ Malo: LINQ en hot path
public class BadLINQ
{
    public int SumEvenNumbers(int[] numbers)
    {
        return numbers.Where(x => x % 2 == 0).Sum(); // Muchas allocations
    }
}

// ✅ Bueno: Código manual
public class GoodLINQ
{
    public int SumEvenNumbers(int[] numbers)
    {
        int sum = 0;
        foreach (var num in numbers)
        {
            if (num % 2 == 0)
                sum += num;
        }
        return sum;
    }
}

// ✅ Mejor: Usar Span<T> para mejor rendimiento
public class BestLINQ
{
    public int SumEvenNumbers(Span<int> numbers)
    {
        int sum = 0;
        foreach (var num in numbers)
        {
            if ((num & 1) == 0) // Bit check más rápido
                sum += num;
        }
        return sum;
    }
}
```

---

### Avoid closures

**Cómo funciona:**
Las closures capturan variables del scope externo, creando objetos (allocations) para mantener el estado. Esto puede ser costoso en hot paths.

**Ventajas:**
- Menos allocations
- Mejor rendimiento
- Menos presión en GC

**Desventajas:**
- Puede requerir pasar parámetros explícitamente
- Menos conveniente

**Cuándo usar:**
- Hot paths
- Cuando se usan lambdas frecuentemente
- Código crítico de rendimiento

**Impacto en performance:**
Puede eliminar allocations y mejorar el rendimiento en un 10-30% en código con muchas closures.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Closure captura variable
public class BadClosure
{
    public void ProcessItems(List<Item> items, int threshold)
    {
        items.Where(x => x.Value > threshold).ToList(); // Closure captura threshold
    }
}

// ✅ Bueno: Pasar como parámetro
public class GoodClosure
{
    public void ProcessItems(List<Item> items, int threshold)
    {
        var filtered = new List<Item>();
        foreach (var item in items)
        {
            if (item.Value > threshold)
                filtered.Add(item);
        }
    }
}

// ✅ Mejor: Método estático para evitar closure
public class BestClosure
{
    private static bool IsAboveThreshold(Item item, int threshold)
    {
        return item.Value > threshold;
    }
    
    public void ProcessItems(List<Item> items, int threshold)
    {
        var filtered = new List<Item>();
        foreach (var item in items)
        {
            if (IsAboveThreshold(item, threshold))
                filtered.Add(item);
        }
    }
}
```

---

### Reuse StringBuilder

**Cómo funciona:**
StringBuilder es eficiente para construir strings, pero crear uno nuevo cada vez causa allocations. Reutilizar un StringBuilder reduce allocations.

**Ventajas:**
- Menos allocations
- Mejor rendimiento
- Menos presión en GC

**Desventajas:**
- Requiere limpieza (Clear())
- Puede retener memoria
- Requiere gestión manual

**Cuándo usar:**
- Cuando se construyen strings frecuentemente
- Hot paths con concatenación de strings
- Cuando se construyen muchos strings

**Impacto en performance:**
Puede reducir allocations y mejorar el rendimiento en un 20-50% en código que construye strings frecuentemente.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear StringBuilder cada vez
public class BadStringBuilder
{
    public string BuildString(IEnumerable<string> parts)
    {
        var sb = new StringBuilder(); // Allocation cada vez
        foreach (var part in parts)
        {
            sb.Append(part);
        }
        return sb.ToString();
    }
}

// ✅ Bueno: Reutilizar StringBuilder
public class GoodStringBuilder
{
    private readonly StringBuilder _sb = new StringBuilder();
    
    public string BuildString(IEnumerable<string> parts)
    {
        _sb.Clear(); // Reutilizar
        foreach (var part in parts)
        {
            _sb.Append(part);
        }
        return _sb.ToString();
    }
}

// ✅ Mejor: Usar ObjectPool para StringBuilder
public class BestStringBuilder
{
    private readonly ObjectPool<StringBuilder> _pool;
    
    public BestStringBuilder(ObjectPoolProvider provider)
    {
        _pool = provider.CreateStringBuilderPool();
    }
    
    public string BuildString(IEnumerable<string> parts)
    {
        var sb = _pool.Get();
        try
        {
            foreach (var part in parts)
            {
                sb.Append(part);
            }
            return sb.ToString();
        }
        finally
        {
            _pool.Return(sb);
        }
    }
}
```

---

### Avoid exceptions for control flow

**Cómo funciona:**
Las excepciones son costosas (stack unwinding, creación de objetos). Usarlas para control de flujo normal es muy ineficiente.

**Ventajas:**
- Mucho mejor rendimiento
- Código más claro
- Menos overhead

**Desventajas:**
- Requiere cambios en el diseño
- Puede requerir valores de retorno o out parameters

**Cuándo usar:**
- Siempre evitar excepciones para control de flujo normal
- Usar excepciones solo para casos excepcionales
- Hot paths especialmente

**Impacto en performance:**
Puede mejorar el rendimiento en un 100-10000x al evitar excepciones en código ejecutado frecuentemente. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Excepciones para control de flujo
public class BadExceptions
{
    public int ParseInt(string value)
    {
        try
        {
            return int.Parse(value); // Lanza excepción si falla
        }
        catch (FormatException)
        {
            return -1; // Control de flujo con excepción
        }
    }
}

// ✅ Bueno: Usar TryParse
public class GoodExceptions
{
    public int ParseInt(string value)
    {
        if (int.TryParse(value, out int result))
        {
            return result;
        }
        return -1; // Sin excepción
    }
}

// ✅ Mejor: Valores de retorno o Result pattern
public class BestExceptions
{
    public (bool Success, int Value) TryParseInt(string value)
    {
        if (int.TryParse(value, out int result))
        {
            return (true, result);
        }
        return (false, 0);
    }
}
```

---

### Avoid reflection

**Cómo funciona:**
Reflection es muy lento (100-1000x más lento que acceso directo) porque requiere búsqueda de metadatos, validación, y no puede ser optimizado por el JIT.

**Ventajas:**
- Mucho mejor rendimiento
- Mejor optimización por JIT
- Type-safe en compile-time

**Desventajas:**
- Menos flexible
- Puede requerir código generado
- Menos dinámico

**Cuándo usar:**
- Evitar en hot paths
- Usar source generators o código generado cuando sea posible
- Usar delegates o expresiones cuando se necesite dinamismo

**Impacto en performance:**
Puede mejorar el rendimiento en un 100-1000x al evitar reflection en hot paths. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Reflection en hot path
public class BadReflection
{
    public void SetProperty(object obj, string propertyName, object value)
    {
        var prop = obj.GetType().GetProperty(propertyName);
        prop.SetValue(obj, value); // Muy lento
    }
}

// ✅ Bueno: Usar expresiones compiladas
public class GoodReflection
{
    private static Dictionary<Type, Dictionary<string, Action<object, object>>> _setters 
        = new Dictionary<Type, Dictionary<string, Action<object, object>>>();
    
    public void SetProperty(object obj, string propertyName, object value)
    {
        var type = obj.GetType();
        if (!_setters.TryGetValue(type, out var typeSetters))
        {
            typeSetters = new Dictionary<string, Action<object, object>>();
            _setters[type] = typeSetters;
        }
        
        if (!typeSetters.TryGetValue(propertyName, out var setter))
        {
            var param = Expression.Parameter(typeof(object));
            var prop = Expression.Property(Expression.Convert(param, type), propertyName);
            var valueParam = Expression.Parameter(typeof(object));
            var assign = Expression.Assign(prop, Expression.Convert(valueParam, prop.Type));
            setter = Expression.Lambda<Action<object, object>>(assign, param, valueParam).Compile();
            typeSetters[propertyName] = setter;
        }
        
        setter(obj, value); // Mucho más rápido que reflection
    }
}

// ✅ Mejor: Usar source generators (C# 9+)
// Los source generators generan código en compile-time, eliminando reflection
```

---

### Use source generators

**Cómo funciona:**
Source generators generan código en compile-time, eliminando la necesidad de reflection en runtime y proporcionando código type-safe y optimizado.

**Ventajas:**
- Sin overhead de reflection
- Type-safe
- Mejor rendimiento
- Código generado optimizado

**Desventajas:**
- Requiere .NET 5+
- Curva de aprendizaje
- Más complejo de implementar

**Cuándo usar:**
- Cuando se necesita código dinámico pero con buen rendimiento
- Reemplazo de reflection
- Generación de código repetitivo
- Serialización, mappers, etc.

**Impacto en performance:**
Puede mejorar el rendimiento en un 100-1000x comparado con reflection. El código generado es tan rápido como código escrito manualmente.

**Ejemplo en C#:**
```csharp
// Source generators se implementan como analizadores
// Ejemplo conceptual de uso:

[AutoMapper]
public partial class UserDto
{
    public int Id { get; set; }
    public string Name { get; set; }
}

[AutoMapper]
public partial class User
{
    public int Id { get; set; }
    public string Name { get; set; }
}

// El source generator genera:
// public partial class UserDto
// {
//     public User ToUser() => new User { Id = this.Id, Name = this.Name };
// }
```

---

### Use ConfigureAwait false

**Cómo funciona:**
ConfigureAwait(false) evita capturar el SynchronizationContext, permitiendo que la continuación se ejecute en cualquier thread del thread pool, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menos allocations
- Mejor escalabilidad
- Evita deadlocks en algunos casos

**Desventajas:**
- No se puede acceder a HttpContext, etc. después
- Requiere atención al código
- Solo en librerías (no en código de aplicación)

**Cuándo usar:**
- En librerías y código reutilizable
- Cuando no se necesita el contexto original
- Código que puede ejecutarse en cualquier thread
- Hot paths async

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% en código async frecuente. También puede prevenir deadlocks.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Captura SynchronizationContext innecesariamente
public class BadConfigureAwait
{
    public async Task<string> GetDataAsync()
    {
        var data = await FetchDataAsync(); // Captura contexto
        return ProcessData(data);
    }
}

// ✅ Bueno: ConfigureAwait(false) en librerías
public class GoodConfigureAwait
{
    public async Task<string> GetDataAsync()
    {
        var data = await FetchDataAsync().ConfigureAwait(false); // No captura contexto
        return ProcessData(data);
    }
}

// ⚠️ Nota: Solo usar en librerías, no en código de aplicación
// donde se necesita el contexto (HttpContext, UI thread, etc.)
```

---

### Use System.Text.Json over Newtonsoft.Json

**Cómo funciona:**
System.Text.Json es la nueva biblioteca de serialización JSON de .NET, optimizada para rendimiento y con menos allocations que Newtonsoft.Json.

**Ventajas:**
- Mucho mejor rendimiento (2-3x más rápido)
- Menos allocations
- Mejor integración con Span<T>
- Parte del runtime (.NET Core 3.0+)

**Desventajas:**
- Menos features que Newtonsoft.Json
- API diferente
- Requiere migración

**Cuándo usar:**
- Nuevas aplicaciones .NET Core 3.0+
- Cuando el rendimiento de JSON es crítico
- Cuando se puede migrar de Newtonsoft.Json

**Impacto en performance:**
Puede mejorar el rendimiento de serialización/deserialización en un 2-3x y reducir allocations significativamente.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Newtonsoft.Json (más lento)
using Newtonsoft.Json;

public class BadJson
{
    public string Serialize(object obj)
    {
        return JsonConvert.SerializeObject(obj); // Más lento, más allocations
    }
}

// ✅ Bueno: System.Text.Json
using System.Text.Json;

public class GoodJson
{
    private static readonly JsonSerializerOptions Options = new JsonSerializerOptions
    {
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase
    };
    
    public string Serialize(object obj)
    {
        return JsonSerializer.Serialize(obj, Options); // Más rápido
    }
    
    // ✅ Mejor: Usar Utf8JsonWriter para máximo rendimiento
    public void SerializeToStream(object obj, Stream stream)
    {
        using (var writer = new Utf8JsonWriter(stream))
        {
            JsonSerializer.Serialize(writer, obj, Options);
        }
    }
}
```

---

### Use System.IO.Pipelines for high-performance IO

**Cómo funciona:**
System.IO.Pipelines proporciona una API de alto nivel para I/O asíncrono con backpressure, batching, y zero-copy cuando es posible.

**Ventajas:**
- Mejor rendimiento que Stream tradicional
- Backpressure automático
- Batching automático
- Zero-copy cuando es posible
- Mejor para protocolos

**Desventajas:**
- Curva de aprendizaje
- Más complejo que Stream
- Requiere .NET Core 2.1+

**Cuándo usar:**
- I/O de alto rendimiento
- Protocolos de red
- Parsing de streams
- Cuando se necesita máximo rendimiento de I/O

**Impacto en performance:**
Puede mejorar el rendimiento de I/O en un 20-50% comparado con Stream tradicional, especialmente para protocolos.

**Ejemplo en C#:**
```csharp
using System.IO.Pipelines;

// ✅ Usar Pipelines para I/O de alto rendimiento
public class PipelineExample
{
    public async Task ProcessStreamAsync(Stream stream)
    {
        var pipe = new Pipe();
        var writing = FillPipeAsync(stream, pipe.Writer);
        var reading = ReadPipeAsync(pipe.Reader);
        
        await Task.WhenAll(writing, reading);
    }
    
    private async Task FillPipeAsync(Stream stream, PipeWriter writer)
    {
        const int minimumBufferSize = 512;
        
        while (true)
        {
            var memory = writer.GetMemory(minimumBufferSize);
            int bytesRead = await stream.ReadAsync(memory);
            if (bytesRead == 0) break;
            
            writer.Advance(bytesRead);
            var result = await writer.FlushAsync();
            
            if (result.IsCompleted) break;
        }
        
        await writer.CompleteAsync();
    }
    
    private async Task ReadPipeAsync(PipeReader reader)
    {
        while (true)
        {
            var result = await reader.ReadAsync();
            var buffer = result.Buffer;
            
            ProcessBuffer(buffer);
            
            reader.AdvanceTo(buffer.End);
            
            if (result.IsCompleted) break;
        }
        
        await reader.CompleteAsync();
    }
}
```

---

---

### Use stackalloc

**Cómo funciona:**
stackalloc asigna memoria en el stack en lugar del heap, evitando allocations del heap y el overhead del GC.

**Ventajas:**
- Sin allocations del heap
- Muy rápido
- Sin overhead de GC
- Automático (se libera al salir del scope)

**Desventajas:**
- Tamaño limitado (típicamente 1-8MB)
- Solo para variables locales
- No puede retornarse de la función

**Cuándo usar:**
- Arrays temporales pequeños
- Buffers temporales
- Hot paths con allocations temporales
- Cuando el tamaño es conocido y pequeño

**Impacto en performance:**
Puede eliminar allocations y mejorar el rendimiento en un 10-50% en hot paths con allocations temporales.

**Ejemplo en C#:**
```csharp
// ✅ Usar stackalloc para arrays temporales pequeños
public unsafe int ProcessData(int[] data)
{
    Span<int> temp = stackalloc int[100]; // Stack allocation
    // Procesar datos
    return temp[0];
}

// ✅ Type-safe con Span<T>
public int ProcessDataSafe(int[] data)
{
    Span<int> temp = stackalloc int[100]; // Stack allocation, type-safe
    // Procesar datos
    return temp[0];
}
```

---

### Avoid allocations inside loops

**Cómo funciona:**
Crear objetos dentro de loops causa muchas allocations, aumentando la presión en el GC y degradando el rendimiento.

**Ventajas:**
- Menos allocations
- Menos presión en GC
- Mejor rendimiento
- Menor latencia

**Desventajas:**
- Requiere mover allocations fuera del loop
- Puede requerir reutilización de objetos

**Cuándo usar:**
- Siempre en loops ejecutados frecuentemente
- Hot paths con loops
- Cuando el profiling muestra muchas allocations en loops

**Impacto en performance:**
Puede reducir allocations en un 90-99% y mejorar el rendimiento en un 20-100% en loops con muchas allocations.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Allocation dentro del loop
public class BadLoopAllocation
{
    public void ProcessItems(List<Item> items)
    {
        foreach (var item in items)
        {
            var processor = new ItemProcessor(); // Allocation cada iteración
            processor.Process(item);
        }
    }
}

// ✅ Bueno: Allocation fuera del loop
public class GoodLoopAllocation
{
    public void ProcessItems(List<Item> items)
    {
        var processor = new ItemProcessor(); // Una allocation
        foreach (var item in items)
        {
            processor.Process(item); // Reutilizar
        }
    }
}

// ✅ Mejor: Usar struct si es posible
public struct ItemProcessor
{
    public void Process(Item item) { /* ... */ }
}

public void ProcessItems(List<Item> items)
{
    var processor = new ItemProcessor(); // Struct, sin allocation
    foreach (var item in items)
    {
        processor.Process(item);
    }
}
```

---

### Seal classes when possible

**Cómo funciona:**
Sealing clases evita que otras clases hereden de ellas, permitiendo optimizaciones del compilador y JIT como inlining y devirtualización.

**Ventajas:**
- Mejor optimización por el compilador
- Mejor rendimiento
- Menos overhead de virtual calls
- Código más claro (intención explícita)

**Desventajas:**
- Menos flexible
- No permite herencia
- Puede requerir cambios en el diseño

**Cuándo usar:**
- Cuando la clase no necesita ser heredada
- Hot paths con llamadas a métodos
- Clases que no son parte de una jerarquía
- Siempre que sea posible

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% al permitir mejor optimización. El impacto es mayor en hot paths.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Clase no sellada
public class ItemProcessor
{
    public virtual void Process(Item item) { /* ... */ }
}

// ✅ Bueno: Clase sellada
public sealed class ItemProcessor
{
    public void Process(Item item) { /* ... */ } // Puede ser inlined
}

// ✅ Mejor: Métodos también no virtuales
public sealed class ItemProcessor
{
    public void Process(Item item) { /* ... */ } // Optimizado
}
```

---

### Avoid virtual calls in hot paths

**Cómo funciona:**
Las llamadas virtuales requieren lookup de vtable, evitando optimizaciones como inlining. Evitarlas en hot paths mejora el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Permite inlining
- Menos overhead
- Mejor optimización

**Desventajas:**
- Menos flexible
- Requiere diseño diferente
- Puede requerir otros patrones

**Cuándo usar:**
- Hot paths ejecutados frecuentemente
- Código crítico de rendimiento
- Cuando el polimorfismo no es necesario en hot paths

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% en hot paths con muchas llamadas virtuales. El impacto es mayor cuando se permite inlining.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Virtual calls en hot path
public class BadVirtual
{
    public virtual void Process(Item item) { /* ... */ }
}

public void ProcessManyItems(List<Item> items, BadVirtual processor)
{
    foreach (var item in items) // Hot path
    {
        processor.Process(item); // Virtual call, no puede ser inlined
    }
}

// ✅ Bueno: Evitar virtual calls en hot paths
public sealed class GoodNonVirtual
{
    public void Process(Item item) { /* ... */ } // No virtual
}

public void ProcessManyItems(List<Item> items, GoodNonVirtual processor)
{
    foreach (var item in items) // Hot path
    {
        processor.Process(item); // Direct call, puede ser inlined
    }
}

// ✅ Mejor: Usar generics para polimorfismo sin virtual calls
public void ProcessManyItems<T>(List<Item> items, T processor) where T : IProcessor
{
    foreach (var item in items)
    {
        processor.Process(item); // Puede ser inlined si T es conocido
    }
}
```

---

### Use IHttpClientFactory for HTTP clients

**Cómo funciona:**
IHttpClientFactory gestiona el ciclo de vida de HttpClient instances, evitando socket exhaustion y mejorando el rendimiento mediante connection pooling.

**Ventajas:**
- Evita socket exhaustion
- Mejor rendimiento mediante pooling
- Gestión automática del ciclo de vida
- Mejor para aplicaciones de alto rendimiento

**Desventajas:**
- Requiere configuración
- Curva de aprendizaje

**Cuándo usar:**
- Siempre en aplicaciones ASP.NET Core
- Aplicaciones que hacen muchas llamadas HTTP
- Servicios que consumen APIs externas
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% y prevenir problemas de socket exhaustion que pueden degradar el rendimiento dramáticamente.

**Ejemplo en C#:**
```csharp
// ✅ Configurar IHttpClientFactory en Startup
services.AddHttpClient("MyApi", client =>
{
    client.BaseAddress = new Uri("https://api.example.com");
    client.Timeout = TimeSpan.FromSeconds(30);
});

// ✅ Usar en servicios
public class ApiService
{
    private readonly IHttpClientFactory _httpClientFactory;
    
    public ApiService(IHttpClientFactory httpClientFactory)
    {
        _httpClientFactory = httpClientFactory;
    }
    
    public async Task<string> GetDataAsync()
    {
        var client = _httpClientFactory.CreateClient("MyApi");
        return await client.GetStringAsync("/data");
    }
}
```

---

### Use unsafe code only when justified

**Cómo funciona:**
Unsafe code permite acceso directo a memoria y punteros, proporcionando máximo rendimiento pero con riesgos de seguridad y estabilidad.

**Ventajas:**
- Máximo rendimiento
- Control total sobre memoria
- Acceso directo a punteros
- Útil para operaciones de bajo nivel

**Desventajas:**
- Riesgos de seguridad
- Puede causar crashes
- Más difícil de depurar
- Requiere permisos especiales

**Cuándo usar:**
- Solo cuando es absolutamente necesario
- Operaciones de muy bajo nivel
- Cuando Span<T> y Memory<T> no son suficientes
- Código crítico de rendimiento validado

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% para operaciones específicas, pero generalmente Span<T> y Memory<T> son suficientes y más seguros.

**Ejemplo en C#:**
```csharp
// ⚠️ Unsafe code - usar solo cuando sea absolutamente necesario
public unsafe void ProcessUnsafe(byte[] data)
{
    fixed (byte* ptr = data)
    {
        // Acceso directo a punteros
        for (int i = 0; i < data.Length; i++)
        {
            ptr[i] = (byte)(ptr[i] * 2);
        }
    }
}

// ✅ Preferir Span<T> cuando sea posible (más seguro)
public void ProcessSafe(Span<byte> data)
{
    for (int i = 0; i < data.Length; i++)
    {
        data[i] = (byte)(data[i] * 2); // Type-safe, memory-safe
    }
}
```

---

## Concurrency and Threading

Esta sección cubre optimizaciones relacionadas con concurrencia y threading en .NET, críticas para aplicaciones multi-threaded de alto rendimiento.

### Use async and await correctly

**Cómo funciona:**
async/await permite que los threads se liberen durante operaciones asíncronas, mejorando el throughput y escalabilidad sin bloquear threads.

**Ventajas:**
- Mejor escalabilidad
- No bloquea threads
- Mejor utilización de recursos
- Código más legible

**Desventajas:**
- Overhead de state machine
- Puede ser más lento para operaciones muy rápidas
- Requiere async en todo el stack

**Cuándo usar:**
- Operaciones de I/O
- Operaciones que pueden tomar tiempo
- Aplicaciones de servidor
- Siempre que sea posible en lugar de bloquear

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir más operaciones concurrentes. El impacto es mayor con muchas operaciones de I/O.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Bloquear threads
public class BadAsync
{
    public string GetData(string url)
    {
        var client = new HttpClient();
        return client.GetStringAsync(url).Result; // Bloquea thread
    }
}

// ✅ Bueno: Usar async/await
public class GoodAsync
{
    public async Task<string> GetDataAsync(string url)
    {
        var client = new HttpClient();
        return await client.GetStringAsync(url); // No bloquea
    }
}

// ✅ Mejor: Usar IHttpClientFactory
public class BestAsync
{
    private readonly IHttpClientFactory _httpClientFactory;
    
    public BestAsync(IHttpClientFactory httpClientFactory)
    {
        _httpClientFactory = httpClientFactory;
    }
    
    public async Task<string> GetDataAsync(string url)
    {
        var client = _httpClientFactory.CreateClient();
        return await client.GetStringAsync(url);
    }
}
```

---

### Use Channels for producer-consumer scenarios

**Cómo funciona:**
Channels proporcionan una forma eficiente y thread-safe de comunicación entre productores y consumidores, con backpressure automático.

**Ventajas:**
- Thread-safe
- Backpressure automático
- Muy eficiente
- Mejor que BlockingCollection

**Desventajas:**
- Requiere .NET Core 3.0+
- Curva de aprendizaje

**Cuándo usar:**
- Patrones producer-consumer
- Pipelines de procesamiento
- Cuando se necesita backpressure
- Comunicación entre threads/tasks

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% comparado con BlockingCollection y proporciona mejor escalabilidad.

**Ejemplo en C#:**
```csharp
using System.Threading.Channels;

public class ChannelExample
{
    private readonly Channel<int> _channel;
    
    public ChannelExample()
    {
        var options = new BoundedChannelOptions(1000)
        {
            FullMode = BoundedChannelFullMode.Wait // Backpressure
        };
        _channel = Channel.CreateBounded<int>(options);
    }
    
    public async Task ProduceAsync()
    {
        var writer = _channel.Writer;
        for (int i = 0; i < 1000; i++)
        {
            await writer.WriteAsync(i);
        }
        writer.Complete();
    }
    
    public async Task ConsumeAsync()
    {
        var reader = _channel.Reader;
        await foreach (var item in reader.ReadAllAsync())
        {
            ProcessItem(item);
        }
    }
}
```

---

### Use ValueTask for async operations

**Cómo funciona:**
ValueTask puede evitar allocations cuando una operación async se completa sincrónicamente, mejorando el rendimiento en hot paths.

**Ventajas:**
- Menos allocations cuando se completa sincrónicamente
- Mejor rendimiento en hot paths
- Compatible con Task

**Desventajas:**
- Solo beneficioso cuando muchas operaciones son sincrónicas
- Requiere atención al uso

**Cuándo usar:**
- Hot paths async
- Cuando muchas operaciones se completan sincrónicamente
- APIs de librerías
- Código crítico de rendimiento

**Impacto en performance:**
Puede eliminar allocations y mejorar el rendimiento en un 10-30% en hot paths con muchas operaciones async que se completan sincrónicamente.

**Ejemplo en C#:**
```csharp
// ✅ Usar ValueTask cuando muchas operaciones son sincrónicas
public class ValueTaskExample
{
    private readonly Dictionary<string, string> _cache = new();
    
    public ValueTask<string> GetValueAsync(string key)
    {
        if (_cache.TryGetValue(key, out var value))
        {
            return new ValueTask<string>(value); // Sin allocation
        }
        return new ValueTask<string>(LoadValueAsync(key)); // Allocation solo si es async
    }
    
    private async Task<string> LoadValueAsync(string key)
    {
        // Operación async
        await Task.Delay(100);
        return "value";
    }
}
```

---

### Avoid global locks

**Cómo funciona:**
Los locks globales serializan todo el acceso, eliminando la paralelización y degradando el rendimiento significativamente.

**Ventajas:**
- Mejor paralelización
- Mejor rendimiento
- Mejor escalabilidad
- Menos contención

**Desventajas:**
- Requiere diseño más cuidadoso
- Puede ser más complejo
- Requiere sincronización más granular

**Cuándo usar:**
- Siempre evitar locks globales
- Aplicaciones multi-threaded
- Cuando se necesita paralelización
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al permitir paralelización real. El impacto es dramático con muchos threads.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Lock global
public class BadGlobalLock
{
    private static readonly object _globalLock = new object();
    private int _counter = 0;
    
    public void Increment()
    {
        lock (_globalLock) // Serializa todo
        {
            _counter++;
        }
    }
}

// ✅ Bueno: Lock granular o lock-free
public class GoodFineGrained
{
    private int _counter = 0;
    
    public void Increment()
    {
        Interlocked.Increment(ref _counter); // Lock-free, mejor rendimiento
    }
}

// ✅ Mejor: Lock por instancia si es necesario
public class BestFineGrained
{
    private readonly object _instanceLock = new object();
    private int _counter = 0;
    
    public void Increment()
    {
        lock (_instanceLock) // Lock por instancia, no global
        {
            _counter++;
        }
    }
}
```

---

### Use fine-grained locking

**Cómo funciona:**
Fine-grained locking usa múltiples locks para diferentes recursos, permitiendo más paralelización que un lock global.

**Ventajas:**
- Mejor paralelización
- Menos contención
- Mejor rendimiento
- Mejor escalabilidad

**Desventajas:**
- Más complejo
- Puede causar deadlocks si no se diseña bien
- Requiere más gestión

**Cuándo usar:**
- Cuando diferentes recursos pueden ser accedidos independientemente
- Aplicaciones multi-threaded
- Cuando hay contención en locks globales
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x comparado con locks globales. El impacto es mayor con más threads y menos contención.

**Ejemplo en C#:**
```csharp
// ✅ Fine-grained locking
public class FineGrainedLock
{
    private readonly Dictionary<int, object> _locks = new Dictionary<int, object>();
    private readonly Dictionary<int, int> _counters = new Dictionary<int, int>();
    private readonly object _lockDictLock = new object();
    
    public void Increment(int key)
    {
        object keyLock;
        lock (_lockDictLock)
        {
            if (!_locks.TryGetValue(key, out keyLock))
            {
                keyLock = new object();
                _locks[key] = keyLock;
            }
        }
        
        lock (keyLock) // Lock solo para este key
        {
            if (!_counters.TryGetValue(key, out int value))
                value = 0;
            _counters[key] = value + 1;
        }
    }
}

// ✅ Mejor: Usar ConcurrentDictionary que maneja esto internamente
public class BetterFineGrained
{
    private readonly ConcurrentDictionary<int, int> _counters = new();
    
    public void Increment(int key)
    {
        _counters.AddOrUpdate(key, 1, (k, v) => v + 1); // Lock-free internamente
    }
}
```

---

### Prefer lock-free algorithms

**Cómo funciona:**
Los algoritmos lock-free usan operaciones atómicas (como Compare-And-Swap) en lugar de locks, eliminando bloqueos y mejorando el rendimiento.

**Ventajas:**
- Sin bloqueos
- Mejor rendimiento
- Mejor escalabilidad
- Sin deadlocks

**Desventajas:**
- Muy complejo de implementar correctamente
- Difícil de depurar
- Puede tener bugs sutiles
- Requiere conocimiento profundo

**Cuándo usar:**
- Estructuras de datos lock-free
- Cuando los locks son un cuello de botella
- Sistemas de muy alto rendimiento
- Cuando se necesita máxima escalabilidad

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% comparado con locks en casos de alta contención. El impacto es mayor con muchos threads.

**Ejemplo en C#:**
```csharp
// ✅ Usar Interlocked para operaciones lock-free
public class LockFreeCounter
{
    private long _value = 0;
    
    public void Increment()
    {
        Interlocked.Increment(ref _value); // Lock-free
    }
    
    public long Read()
    {
        return Interlocked.Read(ref _value); // Lock-free
    }
}

// ✅ Lock-free stack (ejemplo simplificado)
public class LockFreeStack<T>
{
    private volatile Node _head;
    
    public void Push(T item)
    {
        var newNode = new Node { Value = item, Next = _head };
        while (Interlocked.CompareExchange(ref _head, newNode, newNode.Next) != newNode.Next)
        {
            newNode.Next = _head;
        }
    }
    
    private class Node
    {
        public T Value;
        public Node Next;
    }
}
```

---

### Use compare-and-swap operations

**Cómo funciona:**
Compare-And-Swap (CAS) es una operación atómica que actualiza un valor solo si tiene el valor esperado, permitiendo algoritmos lock-free.

**Ventajas:**
- Permite algoritmos lock-free
- Mejor rendimiento que locks
- Sin bloqueos
- Mejor escalabilidad

**Desventajas:**
- Puede tener ABA problem
- Puede requerir retry loops
- Más complejo que locks simples

**Cuándo usar:**
- Algoritmos lock-free
- Cuando se necesita máximo rendimiento
- Estructuras de datos concurrentes
- Cuando los locks son un cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% comparado con locks en casos de alta contención.

**Ejemplo en C#:**
```csharp
// ✅ Usar Interlocked.CompareExchange para CAS
public class CASExample
{
    private int _value = 0;
    
    public bool TryUpdate(int expected, int newValue)
    {
        int original = Interlocked.CompareExchange(ref _value, newValue, expected);
        return original == expected; // Éxito si el valor era el esperado
    }
    
    // ✅ CAS con retry loop
    public void UpdateWithRetry(int newValue)
    {
        int current;
        do
        {
            current = _value;
        } while (Interlocked.CompareExchange(ref _value, newValue, current) != current);
    }
}
```

---

### Avoid deadlocks

**Cómo funciona:**
Los deadlocks ocurren cuando múltiples threads esperan recursos en orden diferente, causando que todos se bloqueen indefinidamente.

**Ventajas:**
- Evita bloqueos indefinidos
- Mejor rendimiento
- Mejor experiencia de usuario
- Sistema más estable

**Desventajas:**
- Requiere diseño cuidadoso
- Puede requerir ordenamiento de locks
- Puede requerir timeouts

**Cuándo usar:**
- Siempre evitar deadlocks
- Aplicaciones multi-threaded
- Cuando se usan múltiples locks
- Sistemas críticos

**Impacto en performance:**
Evitar deadlocks previene bloqueos que pueden degradar el rendimiento completamente. El impacto es crítico para la estabilidad.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Orden de locks inconsistente puede causar deadlock
public class BadDeadlock
{
    private readonly object _lock1 = new object();
    private readonly object _lock2 = new object();
    
    public void Method1()
    {
        lock (_lock1)
        {
            lock (_lock2) // Orden: lock1, lock2
            {
                // ...
            }
        }
    }
    
    public void Method2()
    {
        lock (_lock2)
        {
            lock (_lock1) // Orden: lock2, lock1 - puede causar deadlock!
            {
                // ...
            }
        }
    }
}

// ✅ Bueno: Orden consistente de locks
public class GoodDeadlock
{
    private readonly object _lock1 = new object();
    private readonly object _lock2 = new object();
    
    // Siempre adquirir locks en el mismo orden
    private void AcquireLocks(Action action)
    {
        lock (_lock1)
        {
            lock (_lock2)
            {
                action();
            }
        }
    }
    
    public void Method1()
    {
        AcquireLocks(() => { /* ... */ });
    }
    
    public void Method2()
    {
        AcquireLocks(() => { /* ... */ }); // Mismo orden
    }
}

// ✅ Mejor: Usar Monitor.TryEnter con timeout
public class BestDeadlock
{
    private readonly object _lock1 = new object();
    private readonly object _lock2 = new object();
    
    public void MethodWithTimeout()
    {
        if (Monitor.TryEnter(_lock1, TimeSpan.FromSeconds(5)))
        {
            try
            {
                if (Monitor.TryEnter(_lock2, TimeSpan.FromSeconds(5)))
                {
                    try
                    {
                        // ...
                    }
                    finally
                    {
                        Monitor.Exit(_lock2);
                    }
                }
            }
            finally
            {
                Monitor.Exit(_lock1);
            }
        }
    }
}
```

---

### Use thread pools

**Cómo funciona:**
Thread pools reutilizan threads en lugar de crear nuevos, reduciendo el overhead de creación/destrucción de threads y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Mejor utilización de recursos
- Escalabilidad automática

**Desventajas:**
- Menos control sobre threads individuales
- Puede requerir configuración

**Cuándo usar:**
- Siempre en aplicaciones .NET modernas
- Tareas de corta duración
- Operaciones async
- Aplicaciones de servidor

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con crear threads manualmente. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear threads manualmente
public class BadThreadCreation
{
    public void ProcessItems(List<Item> items)
    {
        foreach (var item in items)
        {
            var thread = new Thread(() => ProcessItem(item));
            thread.Start(); // Overhead de crear thread
        }
    }
}

// ✅ Bueno: Usar ThreadPool
public class GoodThreadPool
{
    public void ProcessItems(List<Item> items)
    {
        foreach (var item in items)
        {
            ThreadPool.QueueUserWorkItem(_ => ProcessItem(item)); // Reutiliza threads
        }
    }
}

// ✅ Mejor: Usar Task (usa ThreadPool internamente)
public class BestThreadPool
{
    public async Task ProcessItemsAsync(List<Item> items)
    {
        await Task.WhenAll(items.Select(item => Task.Run(() => ProcessItem(item))));
    }
}
```

---

### Proper thread pool sizing

**Cómo funciona:**
El tamaño del thread pool afecta el rendimiento. Muy pocos threads pueden limitar el paralelismo, mientras que demasiados pueden causar overhead.

**Ventajas:**
- Mejor rendimiento
- Mejor utilización de recursos
- Mejor balanceo

**Desventajas:**
- Requiere tuning
- Puede variar según la carga
- Requiere monitoreo

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando hay problemas de rendimiento
- Sistemas con cargas conocidas
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al optimizar el número de threads. El impacto depende de la carga de trabajo.

**Ejemplo en C#:**
```csharp
// ✅ Configurar ThreadPool
public class ThreadPoolConfig
{
    public void ConfigureThreadPool()
    {
        ThreadPool.GetMinThreads(out int minWorker, out int minIO);
        ThreadPool.GetMaxThreads(out int maxWorker, out int maxIO);
        
        // Ajustar según necesidades
        // Para I/O bound: más threads de I/O
        // Para CPU bound: más threads de worker
        ThreadPool.SetMinThreads(Environment.ProcessorCount, 100);
        ThreadPool.SetMaxThreads(Environment.ProcessorCount * 2, 200);
    }
}
```

---

### Use cancellation tokens

**Cómo funciona:**
Cancellation tokens permiten cancelar operaciones de manera cooperativa, evitando desperdiciar recursos en operaciones que ya no son necesarias.

**Ventajas:**
- Mejor utilización de recursos
- Respuesta más rápida a cancelaciones
- Mejor experiencia de usuario
- Evita trabajo innecesario

**Desventajas:**
- Requiere verificar tokens
- Puede requerir cambios en el código

**Cuándo usar:**
- Operaciones de larga duración
- Operaciones que pueden ser canceladas
- Aplicaciones que responden a cancelaciones
- Siempre que sea posible en operaciones async

**Impacto en performance:**
Puede mejorar la eficiencia al evitar trabajo innecesario. El impacto es mayor cuando hay muchas cancelaciones.

**Ejemplo en C#:**
```csharp
// ✅ Usar CancellationToken
public async Task ProcessItemsAsync(List<Item> items, CancellationToken cancellationToken)
{
    foreach (var item in items)
    {
        cancellationToken.ThrowIfCancellationRequested(); // Verificar cancelación
        await ProcessItemAsync(item, cancellationToken);
    }
}

// ✅ Pasar token a operaciones async
public async Task ProcessItemAsync(Item item, CancellationToken cancellationToken)
{
    await SomeAsyncOperation(item, cancellationToken);
    cancellationToken.ThrowIfCancellationRequested();
    await AnotherAsyncOperation(item, cancellationToken);
}
```

---

### Throttling and rate limiting

**Cómo funciona:**
Throttling y rate limiting controlan la tasa de operaciones, previniendo sobrecarga y mejorando la estabilidad del sistema.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Protege recursos
- Mejor experiencia de usuario

**Desventajas:**
- Puede limitar el throughput
- Requiere configuración
- Puede rechazar requests válidos

**Cuándo usar:**
- APIs públicas
- Sistemas con límites de recursos
- Cuando se necesita proteger backends
- Aplicaciones que consumen APIs externas

**Impacto en performance:**
Puede prevenir degradación del rendimiento al limitar la carga. El impacto es en estabilidad más que en velocidad máxima.

**Ejemplo en C#:**
```csharp
using System.Threading.RateLimiting;

// ✅ Usar RateLimiter
public class RateLimitedService
{
    private readonly RateLimiter _rateLimiter = new TokenBucketRateLimiter(
        new TokenBucketRateLimiterOptions
        {
            TokenLimit = 100,
            ReplenishmentPeriod = TimeSpan.FromSeconds(1),
            TokensPerPeriod = 10
        });
    
    public async Task ProcessRequestAsync()
    {
        using (var lease = await _rateLimiter.AcquireAsync())
        {
            if (lease.IsAcquired)
            {
                // Procesar request
            }
            else
            {
                throw new RateLimitExceededException();
            }
        }
    }
}
```

---

## Databases

Esta sección cubre optimizaciones para acceso a bases de datos desde aplicaciones .NET, críticas para el rendimiento de aplicaciones data-driven.

### Use connection pooling

**Cómo funciona:**
Connection pooling reutiliza conexiones de base de datos en lugar de crear nuevas cada vez, reduciendo el overhead de establecer conexiones.

**Ventajas:**
- Mucho más rápido que crear conexiones nuevas
- Mejor utilización de recursos
- Mejor escalabilidad
- Automático en .NET (ADO.NET, EF Core)

**Desventajas:**
- Requiere configuración adecuada del pool size
- Puede retener conexiones si no se configuran timeouts

**Cuándo usar:**
- Siempre (está habilitado por defecto)
- Aplicaciones con mucho acceso a base de datos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con crear conexiones nuevas cada vez. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Connection pooling está habilitado por defecto en .NET
// Solo asegúrate de cerrar conexiones correctamente

// ❌ Malo: No cerrar conexiones
public class BadConnection
{
    public void QueryData()
    {
        var connection = new SqlConnection(connectionString);
        // Olvidar cerrar agota el pool
    }
}

// ✅ Bueno: Usar using para cerrar automáticamente
public class GoodConnection
{
    public void QueryData()
    {
        using (var connection = new SqlConnection(connectionString))
        {
            connection.Open();
            // Usar conexión
        } // Se cierra y devuelve al pool automáticamente
    }
}

// ✅ Mejor: Configurar connection string para pooling
// "Server=...;Database=...;Pooling=true;Min Pool Size=5;Max Pool Size=100;"
```

---

### Avoid N plus 1 queries

**Cómo funciona:**
N+1 queries ocurre cuando se hace 1 query para obtener una lista y luego N queries adicionales (una por cada item) para obtener datos relacionados, causando muchas queries innecesarias.

**Ventajas:**
- Mucho menos queries a la base de datos
- Mejor rendimiento
- Menor carga en la base de datos
- Mejor escalabilidad

**Desventajas:**
- Requiere cambios en el código
- Puede requerir eager loading o joins

**Cuándo usar:**
- Siempre evitar N+1 queries
- Cuando se cargan relaciones
- Aplicaciones con Entity Framework
- Cualquier ORM

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x dependiendo de cuántas queries se eliminen. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: N+1 queries
public class BadNPlusOne
{
    public async Task<List<Order>> GetOrdersBad(int customerId)
    {
        var orders = await _context.Orders
            .Where(o => o.CustomerId == customerId)
            .ToListAsync();
        
        foreach (var order in orders)
        {
            // Query adicional por cada order - N+1!
            order.Items = await _context.OrderItems
                .Where(i => i.OrderId == order.Id)
                .ToListAsync();
        }
        return orders;
    }
}

// ✅ Bueno: Eager loading
public class GoodNPlusOne
{
    public async Task<List<Order>> GetOrdersGood(int customerId)
    {
        return await _context.Orders
            .Where(o => o.CustomerId == customerId)
            .Include(o => o.Items) // Carga todo en una query
            .ToListAsync();
    }
}

// ✅ Mejor: Usar projection cuando solo se necesitan algunos campos
public async Task<List<OrderDto>> GetOrdersBest(int customerId)
    {
        return await _context.Orders
            .Where(o => o.CustomerId == customerId)
            .Select(o => new OrderDto
            {
                Id = o.Id,
                Items = o.Items.Select(i => new ItemDto { Id = i.Id }).ToList()
            })
            .ToListAsync(); // Una sola query optimizada
    }
```

---

### Use Dapper for high-performance database access

**Cómo funciona:**
Dapper es un micro-ORM que mapea resultados de queries a objetos con overhead mínimo, proporcionando mejor rendimiento que Entity Framework para casos de uso simples.

**Ventajas:**
- Mucho más rápido que EF Core
- Menos overhead
- Control total sobre SQL
- Menos allocations

**Desventajas:**
- Requiere escribir SQL manualmente
- Menos features que EF Core
- No tiene change tracking

**Cuándo usar:**
- Cuando el rendimiento es crítico
- Queries simples o bien definidas
- Cuando se necesita control total sobre SQL
- Hot paths con acceso a base de datos

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-5x comparado con EF Core para queries simples. El impacto es mayor en hot paths.

**Ejemplo en C#:**
```csharp
using Dapper;

// ✅ Dapper es mucho más rápido para queries simples
public class DapperExample
{
    private readonly IDbConnection _connection;
    
    public async Task<List<Order>> GetOrdersAsync(int customerId)
    {
        var sql = @"
            SELECT o.*, i.* 
            FROM Orders o
            LEFT JOIN OrderItems i ON i.OrderId = o.Id
            WHERE o.CustomerId = @CustomerId";
        
        var orderDict = new Dictionary<int, Order>();
        
        var orders = await _connection.QueryAsync<Order, OrderItem, Order>(
            sql,
            (order, item) =>
            {
                if (!orderDict.TryGetValue(order.Id, out var orderEntry))
                {
                    orderEntry = order;
                    orderEntry.Items = new List<OrderItem>();
                    orderDict.Add(order.Id, orderEntry);
                }
                if (item != null)
                    orderEntry.Items.Add(item);
                return orderEntry;
            },
            new { CustomerId = customerId },
            splitOn: "Id");
        
        return orderDict.Values.ToList();
    }
}
```

---

### Use Entity Framework compiled queries

**Cómo funciona:**
Compiled queries pre-compilan expresiones LINQ a SQL, evitando el overhead de compilación en cada ejecución.

**Ventajas:**
- Mejor rendimiento que queries normales
- Menos overhead de compilación
- Útil para queries ejecutadas frecuentemente

**Desventajas:**
- Más complejo de usar
- Requiere gestión manual
- No siempre es necesario

**Cuándo usar:**
- Queries ejecutadas muy frecuentemente
- Hot paths con EF Core
- Cuando la compilación de queries es un cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% para queries ejecutadas frecuentemente.

**Ejemplo en C#:**
```csharp
// ✅ Compiled query para queries frecuentes
public class CompiledQueryExample
{
    private static readonly Func<MyDbContext, int, Task<Order>> GetOrderById =
        EF.CompileAsyncQuery((MyDbContext context, int id) =>
            context.Orders.FirstOrDefault(o => o.Id == id));
    
    public async Task<Order> GetOrderAsync(int id)
    {
        return await GetOrderById(_context, id); // Pre-compilado, más rápido
    }
}
```

---

### Proper indexing strategy

**Cómo funciona:**
Los índices aceleran las queries al permitir acceso rápido a filas específicas sin escanear toda la tabla. Una estrategia adecuada balancea velocidad de lectura con overhead de escritura.

**Ventajas:**
- Queries mucho más rápidas
- Mejor rendimiento de lectura
- Reduce escaneo de tablas
- Mejor experiencia de usuario

**Desventajas:**
- Overhead en escrituras (INSERT, UPDATE, DELETE)
- Usa espacio adicional
- Requiere mantenimiento
- Puede ralentizar escrituras si hay demasiados índices

**Cuándo usar:**
- Columnas usadas frecuentemente en WHERE
- Columnas usadas en JOINs
- Columnas usadas para ordenamiento
- Columnas usadas en filtros

**Impacto en performance:**
Puede mejorar el rendimiento de queries en un 10-1000x. El impacto es dramático para queries que de otra manera escanearían tablas completas.

**Ejemplo en C# (Entity Framework):**
```csharp
// ✅ Definir índices en Entity Framework
public class ApplicationDbContext : DbContext
{
    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        // Índice simple
        modelBuilder.Entity<User>()
            .HasIndex(u => u.Email)
            .IsUnique();
        
        // Índice compuesto
        modelBuilder.Entity<Order>()
            .HasIndex(o => new { o.CustomerId, o.OrderDate });
        
        // Índice con ordenamiento
        modelBuilder.Entity<Product>()
            .HasIndex(p => p.Name)
            .HasDatabaseName("IX_Product_Name");
    }
}
```

---

### Composite indexes

**Cómo funciona:**
Los índices compuestos incluyen múltiples columnas, optimizando queries que filtran por múltiples columnas en orden específico.

**Ventajas:**
- Optimiza queries con múltiples filtros
- Mejor rendimiento para queries complejas
- Puede cubrir queries completamente (covering index)

**Desventajas:**
- Solo útil si se usan columnas en orden
- Más overhead que índices simples
- Requiere más espacio

**Cuándo usar:**
- Queries que filtran por múltiples columnas
- Cuando el orden de columnas en WHERE coincide con el índice
- Queries frecuentes con el mismo patrón

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x para queries con múltiples filtros. El impacto es mayor cuando el índice cubre completamente la query.

**Ejemplo en C# (Entity Framework):**
```csharp
// ✅ Índice compuesto
modelBuilder.Entity<Order>()
    .HasIndex(o => new { o.CustomerId, o.Status, o.OrderDate })
    .HasDatabaseName("IX_Order_Customer_Status_Date");

// Query que usa el índice eficientemente
var orders = context.Orders
    .Where(o => o.CustomerId == customerId && o.Status == OrderStatus.Pending)
    .OrderBy(o => o.OrderDate)
    .ToList(); // Usa el índice compuesto
```

---

### Covering indexes

**Cómo funciona:**
Un covering index incluye todas las columnas necesarias para una query, permitiendo que la query se complete sin acceder a la tabla (index-only scan).

**Ventajas:**
- Muy rápido (no accede a la tabla)
- Mejor rendimiento
- Reduce I/O
- Ideal para queries frecuentes

**Desventajas:**
- Requiere más espacio
- Overhead en escrituras
- Solo útil para queries específicas

**Cuándo usar:**
- Queries muy frecuentes con columnas conocidas
- Cuando el tamaño del índice es razonable
- Queries de solo lectura frecuentes

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al evitar acceso a la tabla. El impacto es dramático para queries frecuentes.

**Ejemplo en C# (Entity Framework):**
```csharp
// ✅ Covering index (incluye columnas usadas en SELECT)
modelBuilder.Entity<Order>()
    .HasIndex(o => new { o.CustomerId, o.OrderDate })
    .IncludeProperties(o => new { o.Total, o.Status }); // Incluir columnas adicionales

// Query que puede usar covering index
var orders = context.Orders
    .Where(o => o.CustomerId == customerId)
    .Select(o => new { o.OrderDate, o.Total, o.Status })
    .ToList(); // Puede completarse solo con el índice
```

---

### Avoid over-indexing

**Cómo funciona:**
Demasiados índices ralentizan las escrituras porque cada INSERT/UPDATE/DELETE debe actualizar múltiples índices, degradando el rendimiento.

**Ventajas:**
- Mejor rendimiento de escritura
- Menos espacio usado
- Menos overhead de mantenimiento
- Mejor balance general

**Desventajas:**
- Puede ralentizar algunas queries
- Requiere análisis cuidadoso

**Cuándo usar:**
- Cuando las escrituras son frecuentes
- Cuando hay demasiados índices
- Después de analizar queries reales
- Cuando el rendimiento de escritura es crítico

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 20-100% al reducir el número de índices. El impacto es mayor con muchas escrituras.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Demasiados índices
modelBuilder.Entity<Product>()
    .HasIndex(p => p.Name)
    .HasIndex(p => p.Category)
    .HasIndex(p => p.Price)
    .HasIndex(p => p.Stock)
    .HasIndex(p => p.CreatedDate)
    .HasIndex(p => p.UpdatedDate); // Demasiados índices para una tabla con muchas escrituras

// ✅ Bueno: Índices estratégicos
modelBuilder.Entity<Product>()
    .HasIndex(p => new { p.Category, p.Price }) // Índice compuesto para queries comunes
    .HasIndex(p => p.Name); // Solo si se busca frecuentemente por nombre
```

---

### Small primary keys

**Cómo funciona:**
Primary keys pequeños reducen el tamaño de índices y mejoran el rendimiento de JOINs y operaciones de base de datos.

**Ventajas:**
- Índices más pequeños
- Mejor rendimiento de JOINs
- Menos I/O
- Mejor utilización de caché

**Desventajas:**
- Puede requerir cambios en el diseño
- Puede ser menos legible

**Cuándo usar:**
- Siempre que sea posible
- Tablas grandes
- Cuando hay muchos JOINs
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al reducir el tamaño de índices. El impacto es mayor en tablas grandes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Primary key grande (string)
public class BadPrimaryKey
{
    public string Id { get; set; } // GUID o string largo
    // ...
}

// ✅ Bueno: Primary key pequeño (int o long)
public class GoodPrimaryKey
{
    public int Id { get; set; } // o long para tablas muy grandes
    // ...
}

// ✅ Mejor: Usar IDENTITY para auto-incremento
public class BestPrimaryKey
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int Id { get; set; }
    // ...
}
```

---

### Avoid random UUIDs as primary keys

**Cómo funciona:**
UUIDs aleatorios causan fragmentación en índices porque las inserciones no son secuenciales, degradando el rendimiento.

**Ventajas:**
- Mejor rendimiento de inserción
- Menos fragmentación
- Mejor utilización de caché
- Mejor para índices clustered

**Desventajas:**
- Puede exponer información sobre el número de registros
- Menos seguro para URLs públicas
- Puede requerir otros identificadores

**Cuándo usar:**
- Cuando la seguridad no requiere UUIDs
- Tablas con muchas inserciones
- Cuando el rendimiento es crítico
- Índices clustered

**Impacto en performance:**
Puede mejorar el rendimiento de inserción en un 20-50% al evitar fragmentación. El impacto es mayor en tablas con muchas inserciones.

**Ejemplo en C#:**
```csharp
// ❌ Malo: UUID aleatorio como primary key
public class BadUUID
{
    public Guid Id { get; set; } = Guid.NewGuid(); // Aleatorio, causa fragmentación
    // ...
}

// ✅ Bueno: INT IDENTITY o UUID secuencial
public class GoodId
{
    [Key]
    [DatabaseGenerated(DatabaseGeneratedOption.Identity)]
    public int Id { get; set; } // Secuencial, mejor rendimiento
    // ...
}

// ✅ Alternativa: UUID secuencial si se necesita UUID
public class SequentialUUID
{
    public Guid Id { get; set; } = SequentialGuid.NewGuid(); // Secuencial
    // ...
}
```

---

### Batch inserts

**Cómo funciona:**
Agrupar múltiples INSERTs en un solo comando reduce el número de round-trips a la base de datos y mejora el rendimiento significativamente.

**Ventajas:**
- Mucho más rápido que inserts individuales
- Menos round-trips
- Mejor rendimiento
- Menor latencia de red

**Desventajas:**
- Requiere batching lógico
- Puede requerir más memoria
- Puede requerir manejo de errores más complejo

**Cuándo usar:**
- Siempre que se insertan múltiples registros
- Operaciones de importación
- Migraciones de datos
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con inserts individuales. El impacto es dramático.

**Ejemplo en C# (Entity Framework):**
```csharp
// ❌ Malo: Inserts individuales
public class BadBatchInsert
{
    public void InsertItems(List<Item> items)
    {
        foreach (var item in items)
        {
            context.Items.Add(item);
            context.SaveChanges(); // Round-trip por cada item
        }
    }
}

// ✅ Bueno: Batch insert
public class GoodBatchInsert
{
    public void InsertItems(List<Item> items)
    {
        context.Items.AddRange(items);
        context.SaveChanges(); // Un round-trip para todos
    }
}

// ✅ Mejor: Bulk insert para grandes volúmenes (usar bibliotecas como Z.EntityFramework.Extensions o Dapper)
public class BestBatchInsert
{
    public void BulkInsertItems(List<Item> items)
    {
        // Usar biblioteca de bulk insert
        context.BulkInsert(items); // Muy rápido para grandes volúmenes
    }
}
```

---

### Bulk updates

**Cómo funciona:**
Actualizar múltiples registros en una sola operación es mucho más eficiente que actualizaciones individuales.

**Ventajas:**
- Mucho más rápido
- Menos round-trips
- Mejor rendimiento
- Menor latencia

**Desventajas:**
- Puede requerir SQL directo
- Menos type-safe
- Puede requerir validación manual

**Cuándo usar:**
- Actualizaciones masivas
- Operaciones de mantenimiento
- Cuando se actualizan muchos registros
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con actualizaciones individuales.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Updates individuales
public class BadBulkUpdate
{
    public void UpdateItems(List<Item> items)
    {
        foreach (var item in items)
        {
            context.Items.Update(item);
            context.SaveChanges(); // Round-trip por cada item
        }
    }
}

// ✅ Bueno: Bulk update con SQL
public class GoodBulkUpdate
{
    public void UpdateItems(int categoryId, string newStatus)
    {
        context.Database.ExecuteSqlRaw(
            "UPDATE Items SET Status = {0} WHERE CategoryId = {1}",
            newStatus, categoryId); // Una operación para todos
    }
}

// ✅ Mejor: Usar biblioteca de bulk operations
public class BestBulkUpdate
{
    public void BulkUpdateItems(List<Item> items)
    {
        context.BulkUpdate(items); // Muy rápido
    }
}
```

---

### Use prepared statements

**Cómo funciona:**
Prepared statements pre-compilan queries SQL, permitiendo reutilización y mejorando el rendimiento al evitar re-parsing.

**Ventajas:**
- Mejor rendimiento (no re-parse)
- Protección contra SQL injection
- Reutilización eficiente
- Mejor para queries repetidas

**Desventajas:**
- Requiere gestión de parámetros
- Puede requerir más código

**Cuándo usar:**
- Siempre para queries repetidas
- Queries con parámetros
- Aplicaciones de alto rendimiento
- Cuando la seguridad es importante

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para queries repetidas. El impacto es mayor con muchas ejecuciones.

**Ejemplo en C# (Dapper):**
```csharp
// ✅ Dapper usa prepared statements automáticamente
public class PreparedStatements
{
    public User GetUser(int id)
    {
        return connection.QueryFirstOrDefault<User>(
            "SELECT * FROM Users WHERE Id = @Id",
            new { Id = id }); // Prepared statement automático
    }
    
    // ✅ Reutilizar query preparada
    private static readonly string GetUserSql = "SELECT * FROM Users WHERE Id = @Id";
    
    public User GetUserOptimized(int id)
    {
        return connection.QueryFirstOrDefault<User>(GetUserSql, new { Id = id });
    }
}
```

---

### Analyze query plans

**Cómo funciona:**
Analizar query plans permite identificar problemas de rendimiento como table scans, índices faltantes, o estimaciones incorrectas.

**Ventajas:**
- Identifica problemas de rendimiento
- Ayuda a optimizar queries
- Detecta índices faltantes
- Mejora el rendimiento

**Desventajas:**
- Requiere conocimiento de SQL
- Puede tomar tiempo
- Requiere herramientas

**Cuándo usar:**
- Cuando hay queries lentas
- Después de cambios en esquema
- Optimización de rendimiento
- Análisis periódico

**Impacto en performance:**
Puede identificar optimizaciones que mejoran el rendimiento en un 10-1000x. El impacto depende de los problemas encontrados.

**Ejemplo en C# (Entity Framework):**
```csharp
// ✅ Habilitar logging de queries en Entity Framework
public class QueryPlanAnalysis
{
    protected override void OnConfiguring(DbContextOptionsBuilder optionsBuilder)
    {
        optionsBuilder
            .UseSqlServer(connectionString)
            .LogTo(Console.WriteLine, LogLevel.Information) // Ver queries generadas
            .EnableSensitiveDataLogging(); // Para desarrollo
    }
    
    // ✅ Usar EXPLAIN en SQL directo
    public void AnalyzeQuery()
    {
        var sql = "EXPLAIN SELECT * FROM Users WHERE Email = @email";
        var plan = connection.Query(sql, new { email = "test@example.com" });
        // Analizar plan de ejecución
    }
}
```

---

### Avoid functions in WHERE clauses

**Cómo funciona:**
Usar funciones en WHERE clauses previene el uso de índices, forzando table scans y degradando el rendimiento.

**Ventajas:**
- Permite uso de índices
- Mejor rendimiento
- Evita table scans
- Mejor para queries grandes

**Desventajas:**
- Puede requerir cambios en queries
- Puede requerir índices funcionales

**Cuándo usar:**
- Siempre evitar funciones en WHERE cuando sea posible
- Cuando se necesita máximo rendimiento
- Queries sobre tablas grandes

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al permitir uso de índices. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Función en WHERE (previene uso de índice)
var users = context.Users
    .Where(u => u.Email.ToLower() == email.ToLower()) // No puede usar índice
    .ToList();

// ✅ Bueno: Comparar directamente (permite uso de índice)
var users = context.Users
    .Where(u => u.Email == email) // Puede usar índice si Email está indexado
    .ToList();

// ✅ Alternativa: Índice funcional o columna calculada
// En SQL Server:
// CREATE INDEX IX_Users_Email_Lower ON Users (LOWER(Email))
```

---

### Avoid leading wildcard LIKE queries

**Cómo funciona:**
LIKE queries con wildcards al inicio (LIKE '%pattern') no pueden usar índices, forzando table scans.

**Ventajas:**
- Permite uso de índices
- Mejor rendimiento
- Evita table scans

**Desventajas:**
- Puede requerir cambios en el diseño
- Puede requerir full-text search

**Cuándo usar:**
- Siempre evitar cuando sea posible
- Cuando se necesita búsqueda de texto, usar full-text search
- Queries sobre tablas grandes

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al permitir uso de índices o full-text search.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Wildcard al inicio (no puede usar índice)
var products = context.Products
    .Where(p => p.Name.Contains("pattern")) // LIKE '%pattern%'
    .ToList();

// ✅ Bueno: Wildcard al final (puede usar índice)
var products = context.Products
    .Where(p => p.Name.StartsWith("pattern")) // LIKE 'pattern%'
    .ToList();

// ✅ Mejor: Full-text search para búsquedas complejas
var products = context.Products
    .Where(p => EF.Functions.Contains(p.Name, "pattern")) // Full-text search
    .ToList();
```

---

### Use keyset pagination

**Cómo funciona:**
Keyset pagination usa el valor de la última fila vista en lugar de OFFSET, evitando escanear filas anteriores y mejorando el rendimiento.

**Ventajas:**
- Mucho más rápido que OFFSET
- Rendimiento constante
- Mejor para grandes datasets
- No se degrada con páginas grandes

**Desventajas:**
- No permite saltar a página específica
- Requiere cambios en la UI
- Más complejo de implementar

**Cuándo usar:**
- Paginación de grandes datasets
- Cuando OFFSET es lento
- Aplicaciones de alto rendimiento
- Infinite scroll

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para páginas grandes. El impacto es dramático con OFFSET grande.

**Ejemplo en C#:**
```csharp
// ❌ Malo: OFFSET pagination (lento para páginas grandes)
public List<Order> GetOrders(int page, int pageSize)
{
    return context.Orders
        .OrderBy(o => o.Id)
        .Skip(page * pageSize) // Escanea todas las filas anteriores
        .Take(pageSize)
        .ToList();
}

// ✅ Bueno: Keyset pagination
public List<Order> GetOrders(int? lastSeenId, int pageSize)
{
    var query = context.Orders.OrderBy(o => o.Id);
    
    if (lastSeenId.HasValue)
    {
        query = query.Where(o => o.Id > lastSeenId.Value); // Muy rápido con índice
    }
    
    return query.Take(pageSize).ToList();
}
```

---

### Tune pool sizes

**Cómo funciona:**
Ajustar el tamaño del connection pool balancea la utilización de recursos con la capacidad de manejar picos de carga.

**Ventajas:**
- Mejor rendimiento
- Mejor utilización de recursos
- Previene agotamiento de conexiones
- Mejor balanceo

**Desventajas:**
- Requiere tuning
- Puede variar según la carga
- Requiere monitoreo

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando hay problemas de conexiones
- Después de análisis de carga
- Sistemas con cargas conocidas

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al optimizar el uso de conexiones. El impacto es mayor cuando el pool está mal configurado.

**Ejemplo en C#:**
```csharp
// ✅ Configurar connection pool en connection string
var connectionString = "Server=...;Database=...;Min Pool Size=5;Max Pool Size=100;";

// ✅ Fórmula común: connections = ((core_count * 2) + effective_spindle_count)
// Para aplicaciones típicas: 20-100 conexiones es común
// Para aplicaciones de alto rendimiento: puede necesitar más

// ✅ Monitorear uso del pool
public class ConnectionPoolMonitor
{
    public void MonitorPool()
    {
        // En SQL Server, consultar sys.dm_exec_connections
        var activeConnections = GetActiveConnections();
        var poolSize = GetPoolSize();
        // Ajustar según métricas
    }
}
```

---

### Partitioning

**Cómo funciona:**
Partitioning divide tablas grandes en particiones más pequeñas basándose en criterios (rango, hash, lista), mejorando el rendimiento de queries y mantenimiento.

**Ventajas:**
- Mejor rendimiento de queries
- Mejor mantenimiento
- Mejor para datos grandes
- Permite eliminación rápida de particiones antiguas

**Desventajas:**
- Más complejo
- Requiere diseño cuidadoso
- Puede requerir queries cross-partition

**Cuándo usar:**
- Tablas muy grandes
- Cuando se puede particionar por criterio claro
- Datos con patrones de acceso por partición
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir el tamaño de datos escaneados. El impacto es dramático para tablas grandes.

---

### Sharding

**Cómo funciona:**
Sharding distribuye datos entre múltiples bases de datos o servidores, permitiendo escalabilidad horizontal y mejor rendimiento.

**Ventajas:**
- Escalabilidad horizontal
- Mejor rendimiento
- Mejor para datos muy grandes
- Permite distribución geográfica

**Desventajas:**
- Muy complejo
- Requiere gestión de múltiples shards
- Puede requerir queries cross-shard
- Consistencia más difícil

**Cuándo usar:**
- Datos muy grandes que no caben en un servidor
- Cuando se necesita escalabilidad horizontal
- Sistemas de muy alto rendimiento
- Cuando se puede shardear por criterio claro

**Impacto en performance:**
Puede mejorar el throughput en un Nx donde N es el número de shards (hasta cierto punto). El impacto es dramático.

---

### Read replicas

**Cómo funciona:**
Read replicas son copias de solo lectura de la base de datos principal, distribuyendo la carga de lectura y mejorando el rendimiento.

**Ventajas:**
- Distribuye carga de lectura
- Mejor rendimiento de lectura
- Mejor escalabilidad
- Alta disponibilidad

**Desventajas:**
- Replicación lag
- Consistencia eventual
- Requiere más recursos
- Más complejo

**Cuándo usar:**
- Cargas de trabajo read-heavy
- Cuando se puede aceptar consistencia eventual
- Sistemas que requieren alta disponibilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un Nx donde N es el número de replicas (hasta cierto punto).

---

### Avoid SELECT star

**Cómo funciona:**
Evitar SELECT * y seleccionar solo columnas necesarias reduce el tamaño de datos transferidos y mejora el rendimiento.

**Ventajas:**
- Menos datos transferidos
- Mejor rendimiento
- Menor uso de memoria
- Mejor para índices covering

**Desventajas:**
- Requiere especificar columnas
- Menos flexible

**Cuándo usar:**
- Siempre cuando sea posible
- Queries frecuentes
- Cuando el rendimiento es crítico
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-80% al reducir la cantidad de datos transferidos. El impacto es mayor con muchas columnas.

**Ejemplo en C#:**
```csharp
// ❌ Malo: SELECT *
var users = context.Users.ToList(); // Trae todas las columnas

// ✅ Bueno: Solo columnas necesarias
var users = context.Users.Select(u => new { u.Id, u.Name }).ToList();
```

---

### Limit selected columns and rows

**Cómo funciona:**
Limitar columnas y filas seleccionadas reduce el tamaño de datos y mejora el rendimiento significativamente.

**Ventajas:**
- Menos datos transferidos
- Mejor rendimiento
- Menor uso de memoria
- Mejor experiencia de usuario

**Desventajas:**
- Puede requerir paginación
- Menos datos por query

**Cuándo usar:**
- Siempre cuando sea posible
- Queries que retornan muchos datos
- Cuando se necesita paginación
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir la cantidad de datos. El impacto es dramático con muchos datos.

**Ejemplo en C#:**
```csharp
// ✅ Limitar filas y columnas
var users = context.Users
    .Select(u => new { u.Id, u.Name })
    .Take(100)
    .ToList();
```

---

### Database query plan caching

**Cómo funciona:**
Cachear query plans evita re-compilar queries, mejorando el rendimiento de queries repetidas.

**Ventajas:**
- Evita re-compilación
- Mejor rendimiento
- Menor uso de CPU
- Automático en la mayoría de bases de datos

**Desventajas:**
- Puede cachear planes subóptimos
- Requiere invalidación cuando cambia el esquema

**Cuándo usar:**
- Siempre cuando sea posible (automático en la mayoría de casos)
- Queries repetidas
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para queries repetidas al evitar re-compilación.

**Nota:** La mayoría de bases de datos cachean query plans automáticamente. Entity Framework también cachea queries compiladas.

---

### Prepared statement caching

**Cómo funciona:**
Cachear prepared statements evita re-parse de queries, mejorando el rendimiento de queries repetidas.

**Ventajas:**
- Evita re-parse
- Mejor rendimiento
- Menor uso de CPU
- Mejor para queries repetidas

**Desventajas:**
- Requiere gestión de cache
- Puede usar memoria

**Cuándo usar:**
- Siempre cuando sea posible
- Queries repetidas
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para queries repetidas al evitar re-parse.

**Nota:** Dapper y Entity Framework usan prepared statements automáticamente cuando es posible.

---

### Materialized views for expensive queries

**Cómo funciona:**
Materialized views pre-computan resultados de queries costosas, almacenándolos como tablas físicas para acceso rápido.

**Ventajas:**
- Muy rápido para queries costosas
- Mejor rendimiento
- Reduce carga en tablas base
- Mejor para analytics

**Desventajas:**
- Requiere actualización periódica
- Usa almacenamiento adicional
- Puede tener datos obsoletos

**Cuándo usar:**
- Queries muy costosas ejecutadas frecuentemente
- Analytics y reporting
- Cuando se puede aceptar datos ligeramente obsoletos
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para queries costosas. El impacto es dramático.

---

### Database views optimization

**Cómo funciona:**
Optimizar views (usar índices, evitar funciones costosas) mejora el rendimiento de queries que usan views.

**Ventajas:**
- Mejor rendimiento de queries
- Mejor utilización de índices
- Optimización según necesidades

**Desventajas:**
- Requiere conocimiento de SQL
- Requiere tuning

**Cuándo usar:**
- Views frecuentemente usadas
- Cuando el rendimiento es crítico
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x según la optimización. El impacto es mayor con views mal optimizadas.

---

### Full-text search indexes

**Cómo funciona:**
Índices de búsqueda full-text optimizan búsquedas de texto, proporcionando mejor rendimiento que LIKE queries.

**Ventajas:**
- Mucho más rápido que LIKE
- Mejor para búsquedas de texto
- Mejor relevancia
- Optimizado para texto

**Desventajas:**
- Requiere configuración
- Usa más almacenamiento
- Específico para texto

**Cuándo usar:**
- Búsquedas de texto frecuentes
- Cuando LIKE es lento
- Sistemas de búsqueda
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x comparado con LIKE queries. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Full-text search en Entity Framework
var products = context.Products
    .Where(p => EF.Functions.Contains(p.Description, "search term"))
    .ToList();
```

---

### Time-series database optimization

**Cómo funciona:**
Bases de datos time-series (InfluxDB, TimescaleDB) están optimizadas para datos temporales, proporcionando mejor rendimiento que bases de datos relacionales tradicionales.

**Ventajas:**
- Optimizado para datos temporales
- Mejor compresión
- Mejor rendimiento de queries temporales
- Mejor para métricas y logging

**Desventajas:**
- Requiere infraestructura adicional
- Menos flexible que bases de datos relacionales
- Específico para casos de uso temporales

**Cuándo usar:**
- Datos temporales (métricas, logs, IoT)
- Cuando se necesita alto rendimiento para queries temporales
- Sistemas de monitoreo
- Aplicaciones de alto rendimiento con datos temporales

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para datos temporales comparado con bases de datos relacionales tradicionales.

---

### Columnar databases for analytics

**Cómo funciona:**
Bases de datos columnares (ClickHouse, BigQuery) almacenan datos por columnas en lugar de filas, optimizando queries analíticas.

**Ventajas:**
- Muy rápido para analytics
- Mejor compresión
- Mejor para agregaciones
- Optimizado para lectura

**Desventajas:**
- Más lento para escrituras
- Menos flexible
- Específico para analytics

**Cuándo usar:**
- Analytics y reporting
- Data warehouses
- Cuando se necesita alto rendimiento para queries analíticas
- Sistemas de business intelligence

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para queries analíticas comparado con bases de datos relacionales tradicionales.

---

### Graph database optimization

**Cómo funciona:**
Bases de datos de grafos (Neo4j) están optimizadas para relaciones complejas, proporcionando mejor rendimiento que bases de datos relacionales para queries de grafos.

**Ventajas:**
- Optimizado para relaciones complejas
- Mejor rendimiento para queries de grafos
- Mejor para redes sociales, recomendaciones
- Mejor para análisis de relaciones

**Desventajas:**
- Requiere infraestructura adicional
- Menos flexible para otros casos de uso
- Específico para casos de uso de grafos

**Cuándo usar:**
- Relaciones complejas
- Redes sociales
- Sistemas de recomendación
- Análisis de relaciones

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para queries de grafos comparado con bases de datos relacionales tradicionales.

---

### Database connection multiplexing

**Cómo funciona:**
Connection multiplexing (PgBouncer, ProxySQL) permite que múltiples conexiones de aplicación compartan un pool más pequeño de conexiones de base de datos, mejorando la escalabilidad.

**Ventajas:**
- Mejor escalabilidad
- Menos conexiones de base de datos
- Mejor utilización de recursos
- Mejor para muchas conexiones de aplicación

**Desventajas:**
- Requiere infraestructura adicional
- Puede introducir latencia
- Más complejo

**Cuándo usar:**
- Aplicaciones con muchas conexiones
- Cuando se necesita mejor escalabilidad
- Sistemas de alto rendimiento
- Cuando el número de conexiones es un problema

**Impacto en performance:**
Puede mejorar la escalabilidad significativamente al reducir el número de conexiones de base de datos necesarias.

---

### Read/write splitting at application level

**Cómo funciona:**
Separar lecturas y escrituras a nivel de aplicación permite dirigir lecturas a replicas y escrituras al servidor principal, mejorando el rendimiento.

**Ventajas:**
- Distribuye carga de lectura
- Mejor rendimiento
- Mejor escalabilidad
- Mejor utilización de recursos

**Desventajas:**
- Más complejo
- Consistencia eventual
- Requiere gestión

**Cuándo usar:**
- Cargas de trabajo read-heavy
- Cuando se puede aceptar consistencia eventual
- Sistemas que requieren alta disponibilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un Nx donde N es el número de replicas (hasta cierto punto).

**Ejemplo en C#:**
```csharp
// ✅ Read/write splitting
public class UserService
{
    private readonly DbContext _writeContext;
    private readonly DbContext _readContext;
    
    public async Task<User> GetUserAsync(int id)
    {
        return await _readContext.Users.FindAsync(id); // Read replica
    }
    
    public async Task CreateUserAsync(User user)
    {
        _writeContext.Users.Add(user);
        await _writeContext.SaveChangesAsync(); // Primary
    }
}
```

---

### Database query hints when necessary

**Cómo funciona:**
Query hints permiten forzar el optimizador a usar índices o estrategias específicas, útil cuando el optimizador no elige la mejor estrategia.

**Ventajas:**
- Control sobre plan de ejecución
- Puede mejorar rendimiento
- Útil cuando el optimizador falla

**Desventajas:**
- Puede degradar rendimiento si se usa mal
- Menos flexible
- Requiere conocimiento profundo

**Cuándo usar:**
- Solo cuando el optimizador no elige la mejor estrategia
- Después de análisis cuidadoso
- Cuando se conoce mejor que el optimizador
- Con extrema precaución

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x cuando se usa correctamente, pero puede degradar el rendimiento si se usa mal.

**Nota:** Usar query hints solo como último recurso después de análisis cuidadoso.

---

### Database statistics updates

**Cómo funciona:**
Actualizar estadísticas de base de datos ayuda al optimizador a elegir mejores planes de ejecución, mejorando el rendimiento de queries.

**Ventajas:**
- Mejores planes de ejecución
- Mejor rendimiento
- Automático en la mayoría de bases de datos
- Mejor para queries complejas

**Desventajas:**
- Puede requerir recursos
- Puede requerir configuración

**Cuándo usar:**
- Siempre mantener actualizadas (automático en la mayoría de casos)
- Después de cambios significativos en datos
- Cuando el rendimiento de queries degrada
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x cuando las estadísticas están desactualizadas. El impacto es dramático.

**Nota:** La mayoría de bases de datos actualizan estadísticas automáticamente. Verificar configuración si hay problemas.

---

### Database vacuum/optimize operations

**Cómo funciona:**
Vacuum y optimize operations limpian y optimizan bases de datos, mejorando el rendimiento y recuperando espacio.

**Ventajas:**
- Mejor rendimiento
- Recupera espacio
- Reduce fragmentación
- Mejor utilización de recursos

**Desventajas:**
- Puede requerir recursos
- Puede bloquear operaciones
- Requiere tiempo

**Cuándo usar:**
- Periódicamente (automático en la mayoría de casos)
- Después de muchas eliminaciones
- Cuando el rendimiento degrada
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% cuando hay fragmentación o espacio no utilizado. El impacto es mayor con mucho espacio no utilizado.

**Nota:** La mayoría de bases de datos tienen operaciones automáticas de mantenimiento. Verificar configuración.

---

### Database connection failover

**Cómo funciona:**
Connection failover permite cambiar automáticamente a un servidor de respaldo cuando el servidor principal falla, mejorando la disponibilidad.

**Ventajas:**
- Alta disponibilidad
- Mejor resiliencia
- Automático
- Mejor experiencia de usuario

**Desventajas:**
- Requiere configuración
- Puede tener latencia durante failover
- Requiere servidores de respaldo

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Aplicaciones de producción
- Sistemas que requieren resiliencia

**Impacto en performance:**
No afecta el rendimiento directamente pero previene indisponibilidad que puede ser crítica.

---

### Read consistency levels

**Cómo funciona:**
Diferentes niveles de consistencia (read uncommitted, read committed, repeatable read, serializable) balancean consistencia y rendimiento.

**Ventajas:**
- Control sobre consistencia vs rendimiento
- Mejor rendimiento con niveles más bajos
- Mejor consistencia con niveles más altos

**Desventajas:**
- Niveles más bajos pueden tener problemas de consistencia
- Niveles más altos pueden tener peor rendimiento

**Cuándo usar:**
- Read uncommitted: nunca (riesgo de dirty reads)
- Read committed: mayoría de casos (balance)
- Repeatable read: cuando se necesita consistencia
- Serializable: cuando se necesita máxima consistencia

**Impacto en performance:**
Niveles más bajos pueden mejorar el rendimiento en un 10-50% pero con riesgo de problemas de consistencia.

---

### Write consistency levels

**Cómo funciona:**
Niveles de consistencia de escritura determinan cuántas replicas deben confirmar antes de considerar la escritura exitosa.

**Ventajas:**
- Control sobre durabilidad vs rendimiento
- Mejor rendimiento con niveles más bajos
- Mejor durabilidad con niveles más altos

**Desventajas:**
- Niveles más bajos pueden tener riesgo de pérdida de datos
- Niveles más altos pueden tener peor rendimiento

**Cuándo usar:**
- Según requisitos de durabilidad
- Sistemas críticos: niveles más altos
- Sistemas de alto rendimiento: niveles más bajos (con precaución)

**Impacto en performance:**
Niveles más bajos pueden mejorar el rendimiento en un 20-50% pero con riesgo de pérdida de datos.

---

### Database replication lag monitoring

**Cómo funciona:**
Monitorear replication lag ayuda a identificar problemas de replicación y asegurar que las replicas estén sincronizadas.

**Ventajas:**
- Identifica problemas de replicación
- Asegura sincronización
- Mejor para sistemas críticos
- Previene problemas de consistencia

**Desventajas:**
- Requiere monitoreo
- Requiere alertas

**Cuándo usar:**
- Siempre cuando se usan replicas
- Sistemas críticos
- Aplicaciones de producción
- Cuando la consistencia es importante

**Impacto en performance:**
No afecta el rendimiento directamente pero previene problemas de consistencia que pueden afectar la experiencia de usuario.

---

### Hot and cold data separation

**Cómo funciona:**
Separar datos calientes (accedidos frecuentemente) y fríos (accedidos raramente) permite optimización independiente y mejor rendimiento.

**Ventajas:**
- Optimización independiente
- Mejor rendimiento
- Mejor costo-efectividad
- Mejor para datos grandes

**Desventajas:**
- Más complejo
- Requiere gestión
- Puede requerir migración de datos

**Cuándo usar:**
- Datos con patrones de acceso diferentes
- Datos grandes con datos antiguos
- Sistemas de alto rendimiento
- Cuando se necesita optimizar costo

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al optimizar datos calientes independientemente.

---

### Data archiving strategies

**Cómo funciona:**
Archivar datos antiguos a almacenamiento más barato libera recursos y mejora el rendimiento de datos activos.

**Ventajas:**
- Libera recursos
- Mejor rendimiento de datos activos
- Mejor costo-efectividad
- Mejor para datos grandes

**Desventajas:**
- Más complejo
- Requiere gestión
- Puede requerir acceso a datos archivados

**Cuándo usar:**
- Datos antiguos accedidos raramente
- Sistemas con mucho crecimiento de datos
- Cuando se necesita optimizar costo
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al reducir el tamaño de datos activos.

---

### Database partitioning by time

**Cómo funciona:**
Partitioning por tiempo divide datos en particiones basadas en tiempo, permitiendo eliminación rápida de datos antiguos y mejor rendimiento.

**Ventajas:**
- Eliminación rápida de datos antiguos
- Mejor rendimiento de queries temporales
- Mejor mantenimiento
- Mejor para datos temporales

**Desventajas:**
- Requiere diseño cuidadoso
- Puede requerir queries cross-partition

**Cuándo usar:**
- Datos temporales (logs, métricas, eventos)
- Cuando se necesita eliminar datos antiguos frecuentemente
- Sistemas de alto rendimiento con datos temporales

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para queries temporales y eliminación de datos antiguos.

---

### Indexing strategies for time-series data

**Cómo funciona:**
Estrategias de indexación para datos time-series optimizan queries temporales, proporcionando mejor rendimiento que índices tradicionales.

**Ventajas:**
- Optimizado para queries temporales
- Mejor rendimiento
- Mejor compresión
- Mejor para datos temporales

**Desventajas:**
- Específico para datos temporales
- Requiere diseño cuidadoso

**Cuándo usar:**
- Datos time-series
- Métricas y logging
- Cuando se hacen queries temporales frecuentemente
- Sistemas de alto rendimiento con datos temporales

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para queries temporales comparado con índices tradicionales.

---

### Query result streaming for large datasets

**Cómo funciona:**
Streaming de resultados permite procesar datos mientras se reciben, evitando cargar todo en memoria y mejorando el rendimiento.

**Ventajas:**
- No carga todo en memoria
- Mejor rendimiento
- Mejor para datasets grandes
- Procesamiento incremental

**Desventajas:**
- Más complejo
- Requiere procesamiento incremental

**Cuándo usar:**
- Datasets grandes
- Cuando no se puede cargar todo en memoria
- Procesamiento de datos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede permitir procesar datasets que de otra manera no cabrían en memoria, mejorando el rendimiento dramáticamente.

**Ejemplo en C#:**
```csharp
// ✅ Streaming de resultados
await foreach (var item in context.Items.AsAsyncEnumerable())
{
    ProcessItem(item); // Procesa mientras recibe
}
```

---

### PostgreSQL MVCC (Multi-Version Concurrency Control) optimization

**Cómo funciona:**
PostgreSQL usa MVCC para permitir lecturas concurrentes sin bloquear escrituras. Optimizar MVCC mejora el rendimiento de operaciones concurrentes.

**Ventajas:**
- Mejor concurrencia
- Lecturas no bloquean escrituras
- Mejor rendimiento
- Mejor para cargas de trabajo mixtas

**Desventajas:**
- Requiere espacio para versiones
- Requiere vacuum periódico
- Más complejo

**Cuándo usar:**
- Siempre en PostgreSQL (automático)
- Cargas de trabajo con muchas lecturas concurrentes
- Sistemas de alto rendimiento
- Cuando se necesita alta concurrencia

**Impacto en performance:**
MVCC permite mejor concurrencia que sistemas con locks, mejorando el rendimiento en un 10-100x para cargas de trabajo concurrentes.

**Nota:** MVCC es automático en PostgreSQL. Optimizar mediante autovacuum tuning y configuración apropiada.

---

### PostgreSQL Write-Ahead Logging (WAL) tuning

**Cómo funciona:**
WAL registra cambios antes de escribirlos a las tablas, permitiendo recuperación y replicación. Tuning de WAL mejora el rendimiento de escritura.

**Ventajas:**
- Permite replicación
- Permite recuperación
- Mejor rendimiento de escritura con tuning apropiado
- Mejor durabilidad

**Desventajas:**
- Overhead de escritura
- Requiere espacio
- Requiere configuración

**Cuándo usar:**
- Siempre en PostgreSQL (automático)
- Sistemas que requieren replicación
- Sistemas que requieren durabilidad
- Después de profiling de escritura

**Impacto en performance:**
Tuning apropiado puede mejorar el rendimiento de escritura en un 10-50%. El impacto depende de la configuración.

**Configuración clave:**
- `wal_buffers`: Tamaño de buffer WAL
- `checkpoint_timeout`: Frecuencia de checkpoints
- `max_wal_size`: Tamaño máximo de WAL

---

### PostgreSQL shared_buffers and work_mem tuning

**Cómo funciona:**
shared_buffers es la memoria compartida para cachear datos. work_mem es memoria para operaciones de ordenamiento y hash. Tuning apropiado mejora el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Mejor utilización de memoria
- Optimización según carga de trabajo
- Mejor para queries complejas

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de PostgreSQL

**Cuándo usar:**
- Siempre configurar apropiadamente
- Sistemas de alto rendimiento
- Después de profiling
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la configuración. El impacto es mayor cuando está mal configurado.

**Configuración típica:**
- `shared_buffers`: 25% de RAM para sistemas dedicados
- `work_mem`: 50-200MB por operación, ajustar según queries

---

### PostgreSQL autovacuum tuning

**Cómo funciona:**
Autovacuum limpia versiones antiguas de MVCC y actualiza estadísticas. Tuning apropiado balancea limpieza y rendimiento.

**Ventajas:**
- Limpieza automática
- Mantiene rendimiento
- Actualiza estadísticas
- Mejor para sistemas de producción

**Desventajas:**
- Puede usar recursos
- Requiere configuración
- Puede afectar rendimiento si está mal configurado

**Cuándo usar:**
- Siempre habilitar (por defecto)
- Sistemas de producción
- Después de profiling
- Cuando hay problemas de espacio o rendimiento

**Impacto en performance:**
Tuning apropiado puede mejorar el rendimiento en un 10-50% al mantener la base de datos limpia y estadísticas actualizadas.

**Configuración clave:**
- `autovacuum_vacuum_scale_factor`: Cuándo hacer vacuum
- `autovacuum_analyze_scale_factor`: Cuándo analizar
- `autovacuum_max_workers`: Número de workers

---

### PostgreSQL parallel query execution

**Cómo funciona:**
PostgreSQL puede ejecutar queries en paralelo usando múltiples workers, mejorando el rendimiento de queries grandes.

**Ventajas:**
- Mejor rendimiento para queries grandes
- Mejor utilización de múltiples cores
- Mejor escalabilidad
- Mejor para analytics

**Desventajas:**
- Solo para queries grandes
- Requiere configuración
- Puede usar más recursos

**Cuándo usar:**
- Queries grandes que se benefician de paralelización
- Analytics y reporting
- Cuando se tienen múltiples cores
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x para queries grandes que se benefician de paralelización.

**Configuración:**
- `max_parallel_workers_per_gather`: Número de workers paralelos
- `parallel_tuple_cost`: Costo de paralelización

---

### PostgreSQL connection pooling (PgBouncer)

**Cómo funciona:**
PgBouncer es un connection pooler para PostgreSQL que permite que múltiples conexiones de aplicación compartan un pool más pequeño de conexiones de base de datos.

**Ventajas:**
- Mejor escalabilidad
- Menos conexiones de base de datos
- Mejor utilización de recursos
- Mejor para muchas conexiones de aplicación

**Desventajas:**
- Requiere infraestructura adicional
- Puede introducir latencia mínima
- Más complejo

**Cuándo usar:**
- Aplicaciones con muchas conexiones
- Cuando se necesita mejor escalabilidad
- Sistemas de alto rendimiento
- Cuando el número de conexiones es un problema

**Impacto en performance:**
Puede mejorar la escalabilidad significativamente al reducir el número de conexiones de base de datos necesarias.

---

### LSM trees (Log-Structured Merge) for write-heavy workloads

**Cómo funciona:**
LSM trees (usado en Cassandra, RocksDB) optimizan escrituras usando estructura append-only y merge periódico, mejorando el rendimiento de escritura.

**Ventajas:**
- Muy rápido para escrituras
- Mejor rendimiento de escritura
- Mejor para write-heavy workloads
- Escalabilidad horizontal

**Desventajas:**
- Más lento para lecturas que B-trees
- Requiere compactación
- Más complejo

**Cuándo usar:**
- Write-heavy workloads
- Cuando las escrituras son mucho más frecuentes que las lecturas
- Sistemas de logging y métricas
- Aplicaciones de alto rendimiento con muchas escrituras

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-100x comparado con B-trees para write-heavy workloads. El impacto es dramático.

---

### LSM tree compaction strategies

**Cómo funciona:**
Compaction de LSM trees merge y reorganiza datos, mejorando el rendimiento de lectura. Diferentes estrategias (leveled, tiered, size-tiered) balancean rendimiento y overhead.

**Ventajas:**
- Mejor rendimiento de lectura
- Reduce fragmentación
- Optimización según necesidades
- Mejor para diferentes cargas de trabajo

**Desventajas:**
- Requiere recursos
- Puede afectar rendimiento durante compactación
- Requiere configuración

**Cuándo usar:**
- Sistemas con LSM trees
- Cuando el rendimiento de lectura es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un 10-100x al reducir fragmentación. El impacto es mayor con datos fragmentados.

**Estrategias:**
- **Leveled**: Mejor para lectura, más overhead de escritura
- **Tiered**: Mejor para escritura, más overhead de lectura
- **Size-tiered**: Balance entre lectura y escritura

---

### LSM tree bloom filters for read optimization

**Cómo funciona:**
Bloom filters permiten verificar rápidamente si una clave puede estar en un SSTable, evitando búsquedas costosas en SSTables que no contienen la clave.

**Ventajas:**
- Evita búsquedas costosas
- Mejor rendimiento de lectura
- Muy eficiente en memoria
- Mejor para reads que no existen

**Desventajas:**
- Falsos positivos posibles
- Requiere memoria adicional
- Específico para LSM trees

**Cuándo usar:**
- Siempre cuando sea posible (automático en la mayoría de casos)
- Sistemas con LSM trees
- Cuando hay muchas reads que no existen
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un 10-100x para reads que no existen al evitar búsquedas costosas.

---

### RocksDB block cache and write buffer tuning

**Cómo funciona:**
RocksDB usa block cache para cachear bloques de datos y write buffer para escrituras. Tuning apropiado mejora el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Optimización según necesidades
- Mejor utilización de memoria
- Mejor para diferentes cargas de trabajo

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de RocksDB

**Cuándo usar:**
- Sistemas que usan RocksDB
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la configuración. El impacto es mayor cuando está mal configurado.

**Configuración típica:**
- `block_cache_size`: 30-50% de RAM disponible
- `write_buffer_size`: 64-256MB por memtable

---

### RocksDB compression (snappy, zstd, lz4)

**Cómo funciona:**
RocksDB puede comprimir datos usando diferentes algoritmos (snappy, zstd, lz4), balanceando compresión y rendimiento.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento de I/O
- Menor uso de almacenamiento
- Optimización según necesidades

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración
- Diferentes algoritmos tienen diferentes trade-offs

**Cuándo usar:**
- Siempre cuando sea posible
- Cuando el I/O es un cuello de botella
- Sistemas de alto rendimiento
- Según requisitos de compresión vs CPU

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-5x cuando el I/O es un cuello de botella. zstd generalmente ofrece mejor balance.

**Algoritmos:**
- **snappy**: Rápido, compresión moderada
- **lz4**: Muy rápido, compresión moderada
- **zstd**: Balance, mejor compresión

---

### RocksDB memtable and SSTable optimization

**Cómo funciona:**
RocksDB usa memtables (memoria) y SSTables (disco). Optimizar tamaño y configuración mejora el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Optimización según necesidades
- Mejor balance entre memoria y disco
- Mejor para diferentes cargas de trabajo

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de RocksDB

**Cuándo usar:**
- Sistemas que usan RocksDB
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la configuración. El impacto es mayor cuando está mal configurado.

---

### Cassandra compaction strategies

**Cómo funciona:**
Cassandra usa diferentes estrategias de compactación (SizeTiered, Leveled, TimeWindowed) para merge y reorganizar datos, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento de lectura
- Reduce fragmentación
- Optimización según necesidades
- Mejor para diferentes cargas de trabajo

**Desventajas:**
- Requiere recursos
- Puede afectar rendimiento durante compactación
- Requiere configuración

**Cuándo usar:**
- Sistemas que usan Cassandra
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un 10-100x al reducir fragmentación. El impacto es mayor con datos fragmentados.

**Estrategias:**
- **SizeTiered**: Mejor para escritura, más overhead de lectura
- **Leveled**: Mejor para lectura, más overhead de escritura
- **TimeWindowed**: Mejor para datos temporales

---

### Cassandra read repair and hinted handoff

**Cómo funciona:**
Read repair corrige inconsistencias durante lecturas. Hinted handoff almacena escrituras temporalmente cuando un nodo está caído, replicándolas cuando vuelve.

**Ventajas:**
- Mejor consistencia
- Mejor disponibilidad
- Mejor resiliencia
- Mejor para sistemas distribuidos

**Desventajas:**
- Overhead adicional
- Requiere recursos
- Puede afectar rendimiento

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Sistemas que usan Cassandra
- Cuando la consistencia es importante
- Sistemas de alto rendimiento

**Impacto en performance:**
Tiene overhead mínimo pero proporciona mejor consistencia y disponibilidad, crítico para sistemas distribuidos.

---

### Cassandra consistency levels

**Cómo funciona:**
Niveles de consistencia de Cassandra (ONE, QUORUM, ALL) determinan cuántas replicas deben responder antes de considerar una operación exitosa.

**Ventajas:**
- Control sobre consistencia vs rendimiento
- Mejor rendimiento con niveles más bajos
- Mejor consistencia con niveles más altos
- Flexibilidad

**Desventajas:**
- Niveles más bajos pueden tener problemas de consistencia
- Niveles más altos pueden tener peor rendimiento
- Requiere configuración apropiada

**Cuándo usar:**
- **ONE**: Lecturas rápidas, puede tener datos obsoletos
- **QUORUM**: Balance entre consistencia y rendimiento (recomendado)
- **ALL**: Máxima consistencia, peor rendimiento

**Impacto en performance:**
Niveles más bajos pueden mejorar el rendimiento en un 20-50% pero con riesgo de problemas de consistencia.

---

### Cassandra token-aware routing

**Cómo funciona:**
Token-aware routing dirige requests al nodo que contiene los datos, reduciendo latencia de red y mejorando el rendimiento.

**Ventajas:**
- Menor latencia
- Mejor rendimiento
- Menos overhead de red
- Mejor para sistemas distribuidos

**Desventajas:**
- Requiere configuración
- Puede requerir conocimiento de distribución

**Cuándo usar:**
- Siempre cuando sea posible
- Sistemas que usan Cassandra
- Aplicaciones de alto rendimiento
- Cuando la latencia es crítica

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al reducir latencia de red. El impacto es mayor con alta latencia de red.

---

### MongoDB WiredTiger cache size tuning

**Cómo funciona:**
WiredTiger es el motor de almacenamiento de MongoDB. Ajustar el tamaño de cache mejora el rendimiento al cachear más datos en memoria.

**Ventajas:**
- Mejor rendimiento
- Mejor utilización de memoria
- Optimización según necesidades
- Mejor para diferentes cargas de trabajo

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de MongoDB

**Cuándo usar:**
- Sistemas que usan MongoDB
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la configuración. El impacto es mayor cuando está mal configurado.

**Configuración típica:**
- `wiredTigerCacheSizeGB`: 50% de RAM disponible para sistemas dedicados

---

### MongoDB write concern and read preference

**Cómo funciona:**
Write concern determina cuántas replicas deben confirmar escrituras. Read preference determina de dónde leer (primary, secondary, nearest).

**Ventajas:**
- Control sobre durabilidad vs rendimiento
- Control sobre de dónde leer
- Flexibilidad
- Optimización según necesidades

**Desventajas:**
- Requiere configuración apropiada
- Niveles más altos pueden tener peor rendimiento

**Cuándo usar:**
- **Write concern**: Según requisitos de durabilidad
- **Read preference**: Según necesidades de lectura (primary para consistencia, secondary para distribución de carga)

**Impacto en performance:**
Write concern más bajo puede mejorar el rendimiento en un 20-50% pero con riesgo de pérdida de datos. Read preference secondary puede distribuir carga.

---

### MongoDB index intersection

**Cómo funciona:**
MongoDB puede usar múltiples índices para una query (index intersection), mejorando el rendimiento cuando un solo índice no es suficiente.

**Ventajas:**
- Mejor rendimiento para queries complejas
- Puede usar múltiples índices
- Mejor para queries con múltiples filtros
- Automático cuando es beneficioso

**Desventajas:**
- Puede ser más lento que un índice compuesto
- Requiere múltiples índices
- Puede no ser usado si hay un índice mejor

**Cuándo usar:**
- Siempre cuando sea beneficioso (automático)
- Queries complejas con múltiples filtros
- Cuando no hay índice compuesto apropiado
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para queries complejas cuando no hay índice compuesto apropiado.

---

### MySQL InnoDB buffer pool tuning

**Cómo funciona:**
InnoDB buffer pool cachea datos y índices en memoria. Ajustar el tamaño mejora el rendimiento al cachear más datos.

**Ventajas:**
- Mejor rendimiento
- Mejor utilización de memoria
- Optimización según necesidades
- Mejor para diferentes cargas de trabajo

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de MySQL

**Cuándo usar:**
- Sistemas que usan MySQL InnoDB
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la configuración. El impacto es mayor cuando está mal configurado.

**Configuración típica:**
- `innodb_buffer_pool_size`: 70-80% de RAM disponible para sistemas dedicados

---

### MySQL InnoDB log file size optimization

**Cómo funciona:**
InnoDB log files almacenan cambios antes de escribirlos a las tablas. Tamaño apropiado mejora el rendimiento de escritura.

**Ventajas:**
- Mejor rendimiento de escritura
- Mejor para write-heavy workloads
- Optimización según necesidades
- Mejor durabilidad

**Desventajas:**
- Requiere más espacio
- Requiere configuración
- Requiere reinicio para cambiar

**Cuándo usar:**
- Sistemas que usan MySQL InnoDB
- Write-heavy workloads
- Cuando el rendimiento de escritura es crítico
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-50% según la configuración. El impacto es mayor para write-heavy workloads.

**Configuración típica:**
- `innodb_log_file_size`: 256MB-2GB dependiendo de carga de trabajo

---

### MySQL query cache (deprecated but concepts apply)

**Cómo funciona:**
MySQL query cache (deprecated en MySQL 8.0) cacheaba resultados de queries. Los conceptos aplican a otros sistemas de cache.

**Ventajas:**
- Muy rápido para queries repetidas
- Reduce carga en base de datos
- Mejor rendimiento
- Conceptos aplican a otros sistemas

**Desventajas:**
- Deprecated en MySQL 8.0
- Puede tener resultados obsoletos
- Requiere invalidación

**Cuándo usar:**
- Conceptos aplican a application-level caching
- Sistemas con versiones antiguas de MySQL
- Como referencia para otros sistemas de cache

**Impacto en performance:**
Podía mejorar el rendimiento en un 10-1000x para queries repetidas, pero está deprecated. Usar application-level caching en su lugar.

**Nota:** MySQL query cache está deprecated. Usar application-level caching (Redis, Memcached, IMemoryCache) en su lugar.

---

### Read replicas with eventual consistency

**Cómo funciona:**
Read replicas con consistencia eventual permiten leer de replicas que pueden tener datos ligeramente obsoletos, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento de lectura
- Distribuye carga
- Mejor escalabilidad
- Mejor para cargas de trabajo read-heavy

**Desventajas:**
- Consistencia eventual
- Puede tener datos obsoletos
- Requiere gestión

**Cuándo usar:**
- Cargas de trabajo read-heavy
- Cuando se puede aceptar consistencia eventual
- Sistemas que requieren alta disponibilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un Nx donde N es el número de replicas (hasta cierto punto).

---

### Database sharding strategies (range, hash, directory-based)

**Cómo funciona:**
Diferentes estrategias de sharding (range, hash, directory-based) distribuyen datos de diferentes maneras, cada una con diferentes trade-offs.

**Ventajas:**
- Escalabilidad horizontal
- Mejor rendimiento
- Optimización según necesidades
- Mejor para datos muy grandes

**Desventajas:**
- Muy complejo
- Requiere gestión
- Puede requerir queries cross-shard
- Consistencia más difícil

**Cuándo usar:**
- Datos muy grandes que no caben en un servidor
- Cuando se necesita escalabilidad horizontal
- Sistemas de muy alto rendimiento
- Cuando se puede shardear por criterio claro

**Estrategias:**
- **Range**: Mejor para queries por rango, puede causar hotspots
- **Hash**: Mejor distribución, difícil queries por rango
- **Directory-based**: Más flexible, requiere lookup

**Impacto en performance:**
Puede mejorar el throughput en un Nx donde N es el número de shards (hasta cierto punto). El impacto es dramático.

---

### Database partitioning strategies (horizontal, vertical)

**Cómo funciona:**
Horizontal partitioning divide filas entre particiones. Vertical partitioning divide columnas entre particiones. Cada una tiene diferentes trade-offs.

**Ventajas:**
- Mejor rendimiento
- Mejor mantenimiento
- Optimización según necesidades
- Mejor para datos grandes

**Desventajas:**
- Más complejo
- Requiere diseño cuidadoso
- Puede requerir queries cross-partition

**Cuándo usar:**
- Tablas muy grandes
- Cuando se puede particionar por criterio claro
- Datos con patrones de acceso por partición
- Sistemas de alto rendimiento

**Estrategias:**
- **Horizontal**: Mejor para reducir tamaño de particiones
- **Vertical**: Mejor para separar columnas frecuentemente vs raramente accedidas

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir el tamaño de datos escaneados. El impacto es dramático.

---

### Columnar storage for analytics (Parquet, ORC)

**Cómo funciona:**
Almacenamiento columnares (Parquet, ORC) almacena datos por columnas en lugar de filas, optimizando queries analíticas.

**Ventajas:**
- Muy rápido para analytics
- Mejor compresión
- Mejor para agregaciones
- Optimizado para lectura

**Desventajas:**
- Más lento para escrituras
- Menos flexible
- Específico para analytics

**Cuándo usar:**
- Analytics y reporting
- Data warehouses
- Cuando se necesita alto rendimiento para queries analíticas
- Sistemas de business intelligence

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para queries analíticas comparado con almacenamiento row-based.

---

### Database compression at storage level

**Cómo funciona:**
Compresión a nivel de almacenamiento comprime datos en disco, reduciendo el tamaño y mejorando el rendimiento de I/O.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento de I/O
- Menor uso de almacenamiento
- Mejor para datos grandes

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración
- Puede afectar rendimiento si CPU es cuello de botella

**Cuándo usar:**
- Cuando el I/O es un cuello de botella
- Datos grandes
- Sistemas de alto rendimiento
- Cuando se puede aceptar overhead de CPU

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-5x cuando el I/O es un cuello de botella. Puede degradar si CPU es cuello de botella.

---

### Database write amplification reduction

**Cómo funciona:**
Reducir write amplification (datos escritos vs datos lógicos) mejora el rendimiento de escritura al escribir menos datos físicos.

**Ventajas:**
- Mejor rendimiento de escritura
- Menor uso de almacenamiento
- Menor desgaste en SSD
- Mejor para write-heavy workloads

**Desventajas:**
- Requiere optimización
- Puede requerir cambios en diseño
- Más complejo

**Cuándo usar:**
- Write-heavy workloads
- Sistemas con mucho desgaste en SSD
- Cuando el rendimiento de escritura es crítico
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-50% al reducir la cantidad de datos escritos físicamente.

---

### Database read amplification optimization

**Cómo funciona:**
Optimizar read amplification (datos leídos vs datos lógicos) mejora el rendimiento de lectura al leer menos datos físicos.

**Ventajas:**
- Mejor rendimiento de lectura
- Menor uso de I/O
- Mejor para read-heavy workloads
- Mejor utilización de recursos

**Desventajas:**
- Requiere optimización
- Puede requerir cambios en diseño
- Más complejo

**Cuándo usar:**
- Read-heavy workloads
- Cuando el rendimiento de lectura es crítico
- Sistemas de alto rendimiento
- Cuando el I/O es un cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un 10-50% al reducir la cantidad de datos leídos físicamente.

---

### Database checkpoint tuning

**Cómo funciona:**
Checkpoints escriben datos en memoria a disco periódicamente. Tuning apropiado balancea durabilidad y rendimiento.

**Ventajas:**
- Balance entre durabilidad y rendimiento
- Mejor rendimiento con tuning apropiado
- Mejor para diferentes cargas de trabajo
- Optimización según necesidades

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de la base de datos

**Cuándo usar:**
- Sistemas de alto rendimiento
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas con diferentes cargas de trabajo

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% según la configuración. El impacto es mayor cuando está mal configurado.

---

### Database log file rotation and sizing

**Cómo funciona:**
Rotación y tamaño apropiado de log files previene problemas de espacio y mejora el rendimiento al mantener logs de tamaño manejable.

**Ventajas:**
- Previene problemas de espacio
- Mejor rendimiento
- Mejor mantenimiento
- Mejor para sistemas de producción

**Desventajas:**
- Requiere configuración
- Puede requerir gestión

**Cuándo usar:**
- Siempre configurar apropiadamente
- Sistemas de producción
- Cuando hay problemas de espacio
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede prevenir degradación del rendimiento al mantener logs de tamaño manejable y prevenir problemas de espacio.

---

### Database transaction log optimization

**Cómo funciona:**
Optimizar transaction logs mejora el rendimiento de escritura al optimizar cómo se escriben los logs de transacciones.

**Ventajas:**
- Mejor rendimiento de escritura
- Mejor durabilidad
- Optimización según necesidades
- Mejor para write-heavy workloads

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de la base de datos

**Cuándo usar:**
- Sistemas de alto rendimiento
- Write-heavy workloads
- Cuando el rendimiento de escritura es crítico
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 10-50% según la configuración. El impacto es mayor para write-heavy workloads.

---

### Database lock contention reduction

**Cómo funciona:**
Reducir contención de locks mejora el rendimiento al permitir más operaciones concurrentes sin bloqueos.

**Ventajas:**
- Mejor concurrencia
- Mejor rendimiento
- Menos bloqueos
- Mejor para sistemas concurrentes

**Desventajas:**
- Requiere optimización
- Puede requerir cambios en diseño
- Más complejo

**Cuándo usar:**
- Sistemas con alta contención de locks
- Cuando el rendimiento es crítico
- Sistemas concurrentes
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x cuando hay alta contención de locks. El impacto es dramático.

**Técnicas:**
- Usar índices apropiados
- Reducir tiempo de transacciones
- Usar isolation levels apropiados
- Particionar datos

---

### Database deadlock detection and prevention

**Cómo funciona:**
Detección y prevención de deadlocks identifica y previene situaciones donde múltiples transacciones esperan indefinidamente.

**Ventajas:**
- Previene bloqueos indefinidos
- Mejor estabilidad
- Mejor rendimiento
- Mejor experiencia de usuario

**Desventajas:**
- Requiere diseño cuidadoso
- Puede requerir timeouts
- Más complejo

**Cuándo usar:**
- Siempre prevenir deadlocks
- Sistemas con múltiples transacciones concurrentes
- Cuando la estabilidad es crítica
- Aplicaciones de producción

**Impacto en performance:**
Previene bloqueos que pueden degradar el rendimiento completamente. El impacto es crítico para estabilidad.

**Técnicas:**
- Ordenar locks consistentemente
- Usar timeouts
- Detectar y abortar deadlocks
- Reducir tiempo de transacciones

---

### Database connection pool sizing formula

**Cómo funciona:**
Fórmula común para sizing de connection pool: `connections = ((core_count * 2) + effective_spindle_count)`. Ajustar según carga de trabajo.

**Ventajas:**
- Mejor utilización de recursos
- Previene agotamiento de conexiones
- Mejor rendimiento
- Punto de partida para tuning

**Desventajas:**
- Requiere ajuste según carga
- Puede variar según aplicación
- Requiere monitoreo

**Cuándo usar:**
- Punto de partida para sizing
- Sistemas de alto rendimiento
- Cuando se necesita optimizar conexiones
- Después de análisis de carga

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al optimizar el uso de conexiones. El impacto es mayor cuando el pool está mal configurado.

**Fórmula:**
- `connections = ((core_count * 2) + effective_spindle_count)`
- Ajustar según carga de trabajo y profiling

---

### Database connection string pooling

**Cómo funciona:**
Connection string pooling reutiliza connection strings, reduciendo overhead de parsing y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Automático en la mayoría de casos
- Mejor para muchas conexiones

**Desventajas:**
- Requiere gestión
- Puede retener recursos

**Cuándo usar:**
- Siempre cuando sea posible (automático en la mayoría de casos)
- Aplicaciones de alto rendimiento
- Cuando se crean muchas conexiones

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al reducir overhead de parsing. El impacto es mayor con muchas conexiones.

---

### Database prepared statement pooling

**Cómo funciona:**
Prepared statement pooling reutiliza prepared statements, reduciendo overhead de preparación y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Automático en la mayoría de casos
- Mejor para queries repetidas

**Desventajas:**
- Requiere gestión
- Puede usar memoria

**Cuándo usar:**
- Siempre cuando sea posible (automático en la mayoría de casos)
- Aplicaciones de alto rendimiento
- Cuando se ejecutan queries repetidas

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al reducir overhead de preparación. El impacto es mayor con queries repetidas.

---

### Database connection keep-alive tuning

**Cómo funciona:**
Keep-alive mantiene conexiones abiertas entre requests, evitando overhead de establecer nuevas conexiones.

**Ventajas:**
- Evita overhead de conexión
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples requests

**Desventajas:**
- Usa recursos mientras está abierto
- Requiere gestión de timeouts

**Cuándo usar:**
- Siempre cuando sea posible
- Aplicaciones de alto rendimiento
- Cuando se hacen múltiples requests
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar overhead de conexión repetido.

---

### Database connection timeout optimization

**Cómo funciona:**
Optimizar timeouts de conexión previene que conexiones esperen indefinidamente, mejorando el rendimiento y la estabilidad.

**Ventajas:**
- Previene esperas indefinidas
- Mejor utilización de recursos
- Mejor estabilidad
- Mejor experiencia de usuario

**Desventajas:**
- Puede terminar conexiones válidas si son muy cortos
- Requiere tuning

**Cuándo usar:**
- Siempre configurar apropiadamente
- Sistemas de alto rendimiento
- Cuando hay problemas de conexión
- Aplicaciones de producción

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% cuando hay problemas de red. El impacto es crítico para estabilidad.

---

### Database connection retry strategies

**Cómo funciona:**
Estrategias de retry reintentan conexiones fallidas con diferentes estrategias (exponential backoff, fixed delay), mejorando la resiliencia.

**Ventajas:**
- Mejor resiliencia
- Mejor probabilidad de éxito
- Mejor para fallos temporales
- Mejor estabilidad

**Desventajas:**
- Aumenta latencia en fallos
- Requiere implementación

**Cuándo usar:**
- Siempre cuando sea posible
- Sistemas distribuidos
- Cuando hay fallos temporales
- Aplicaciones de producción

**Impacto en performance:**
Puede mejorar la tasa de éxito en un 50-90% para fallos temporales sin sobrecargar la base de datos.

---

### Database connection failover with health checks

**Cómo funciona:**
Failover con health checks cambia automáticamente a servidores de respaldo cuando se detectan problemas, mejorando la disponibilidad.

**Ventajas:**
- Alta disponibilidad
- Mejor resiliencia
- Automático
- Mejor experiencia de usuario

**Desventajas:**
- Requiere configuración
- Puede tener latencia durante failover
- Requiere servidores de respaldo

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Aplicaciones de producción
- Sistemas que requieren resiliencia

**Impacto en performance:**
No afecta el rendimiento directamente pero previene indisponibilidad que puede ser crítica.

---

### Database read/write splitting middleware

**Cómo funciona:**
Middleware de read/write splitting dirige automáticamente lecturas a replicas y escrituras al servidor principal, mejorando el rendimiento.

**Ventajas:**
- Automático
- Mejor rendimiento
- Distribuye carga
- Mejor escalabilidad

**Desventajas:**
- Requiere middleware
- Más complejo
- Puede tener latencia adicional

**Cuándo usar:**
- Aplicaciones con read/write splitting
- Cuando se necesita distribución automática
- Sistemas de alto rendimiento
- Aplicaciones distribuidas

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un Nx donde N es el número de replicas (hasta cierto punto).

---

### Database query routing based on read/write

**Cómo funciona:**
Routing de queries basado en read/write dirige queries de lectura a replicas y queries de escritura al servidor principal.

**Ventajas:**
- Mejor rendimiento
- Distribuye carga
- Mejor escalabilidad
- Optimización automática

**Desventajas:**
- Requiere implementación
- Más complejo
- Puede requerir detección de tipo de query

**Cuándo usar:**
- Aplicaciones con read/write splitting
- Cuando se necesita distribución
- Sistemas de alto rendimiento
- Aplicaciones distribuidas

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un Nx donde N es el número de replicas (hasta cierto punto).

---

### Database connection draining for maintenance

**Cómo funciona:**
Connection draining cierra gradualmente conexiones existentes antes de mantenimiento, permitiendo que requests en curso se completen.

**Ventajas:**
- Permite mantenimiento sin pérdida de requests
- Mejor experiencia de usuario
- Mejor para sistemas de producción
- Permite actualizaciones sin downtime

**Desventajas:**
- Requiere implementación
- Más complejo
- Puede tomar tiempo

**Cuándo usar:**
- Mantenimiento de sistemas de producción
- Actualizaciones sin downtime
- Sistemas críticos
- Cuando se necesita alta disponibilidad

**Impacto en performance:**
Permite mantenimiento sin afectar el rendimiento de requests en curso, mejorando la disponibilidad.

---

### Database connection warmup strategies

**Cómo funciona:**
Connection warmup pre-establece conexiones antes de que se necesiten, mejorando el rendimiento inicial y reduciendo latencia.

**Ventajas:**
- Mejor rendimiento inicial
- Reduce latencia de primera conexión
- Mejor experiencia de usuario
- Reduce overhead de establecimiento de conexión

**Desventajas:**
- Requiere recursos iniciales
- Puede establecer conexiones no usadas
- Requiere implementación

**Cuándo usar:**
- Al iniciar la aplicación
- Después de reinicios
- Cuando la latencia inicial es importante
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al tener conexiones ya establecidas. El impacto es mayor con alta latencia de red.

---

## Caching

Esta sección cubre estrategias de caching para mejorar el rendimiento de aplicaciones .NET.

### Use IMemoryCache for in-memory caching

**Cómo funciona:**
IMemoryCache proporciona caching en memoria dentro del proceso, muy rápido pero limitado a un solo proceso.

**Ventajas:**
- Muy rápido (nanosegundos)
- Sin overhead de red
- Fácil de usar
- Integrado en ASP.NET Core

**Desventajas:**
- Limitado a un solo proceso
- Se pierde al reiniciar
- Usa memoria del proceso

**Cuándo usar:**
- Datos que no cambian frecuentemente
- Datos que se leen frecuentemente
- Aplicaciones single-instance
- Datos que pueden regenerarse

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para datos cacheados comparado con obtenerlos de la fuente original. El impacto es dramático.

**Ejemplo en C#:**
```csharp
using Microsoft.Extensions.Caching.Memory;

public class CacheExample
{
    private readonly IMemoryCache _cache;
    
    public CacheExample(IMemoryCache cache)
    {
        _cache = cache;
    }
    
    public async Task<string> GetDataAsync(string key)
    {
        if (!_cache.TryGetValue(key, out string cachedValue))
        {
            cachedValue = await LoadDataAsync(key);
            
            var options = new MemoryCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromMinutes(5),
                SlidingExpiration = TimeSpan.FromMinutes(1),
                Priority = CacheItemPriority.Normal
            };
            
            _cache.Set(key, cachedValue, options);
        }
        return cachedValue;
    }
}
```

---

### Use distributed caching like Redis

**Cómo funciona:**
Redis es un cache distribuido en memoria que puede ser compartido entre múltiples instancias de aplicación, proporcionando caching consistente en un cluster.

**Ventajas:**
- Compartido entre múltiples instancias
- Muy rápido
- Persistencia opcional
- Muchas features (pub/sub, streams, etc.)

**Desventajas:**
- Requiere infraestructura adicional
- Latencia de red (aunque mínima)
- Requiere gestión

**Cuándo usar:**
- Aplicaciones distribuidas
- Múltiples instancias que necesitan cache compartido
- Cuando se necesita cache distribuido
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para datos cacheados y proporcionar consistencia en aplicaciones distribuidas.

**Ejemplo en C#:**
```csharp
using StackExchange.Redis;
using Microsoft.Extensions.Caching.StackExchangeRedis;

// ✅ Configurar Redis en Startup
services.AddStackExchangeRedisCache(options =>
{
    options.Configuration = "localhost:6379";
});

// ✅ Usar IDistributedCache
public class RedisCacheExample
{
    private readonly IDistributedCache _cache;
    
    public RedisCacheExample(IDistributedCache cache)
    {
        _cache = cache;
    }
    
    public async Task<string> GetDataAsync(string key)
    {
        var cached = await _cache.GetStringAsync(key);
        if (cached == null)
        {
            cached = await LoadDataAsync(key);
            var options = new DistributedCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromMinutes(5)
            };
            await _cache.SetStringAsync(key, cached, options);
        }
        return cached;
    }
}
```

---

### Multi-level cache hierarchy

**Cómo funciona:**
Una jerarquía de cache de múltiples niveles (L1: memoria local, L2: cache distribuido, L3: base de datos) optimiza el rendimiento balanceando velocidad y capacidad.

**Ventajas:**
- Mejor rendimiento general
- Balancea velocidad y capacidad
- Reduce carga en niveles inferiores
- Mejor para sistemas complejos

**Desventajas:**
- Más complejo
- Requiere gestión de múltiples niveles
- Puede requerir invalidación más compleja

**Cuándo usar:**
- Sistemas con múltiples fuentes de datos
- Cuando se necesita balancear velocidad y capacidad
- Aplicaciones distribuidas complejas
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al optimizar el acceso a datos en diferentes niveles.

**Ejemplo en C#:**
```csharp
public class MultiLevelCache
{
    private readonly IMemoryCache _localCache; // L1: Muy rápido
    private readonly IDistributedCache _distributedCache; // L2: Compartido
    private readonly IDataService _dataService; // L3: Fuente de datos
    
    public async Task<T> GetDataAsync<T>(string key)
    {
        // L1: Cache local
        if (_localCache.TryGetValue(key, out T localValue))
            return localValue;
        
        // L2: Cache distribuido
        var distributedValue = await _distributedCache.GetStringAsync(key);
        if (distributedValue != null)
        {
            var value = JsonSerializer.Deserialize<T>(distributedValue);
            _localCache.Set(key, value, TimeSpan.FromMinutes(1)); // Cachear localmente
            return value;
        }
        
        // L3: Fuente de datos
        var data = await _dataService.GetDataAsync<T>(key);
        await _distributedCache.SetStringAsync(key, JsonSerializer.Serialize(data));
        _localCache.Set(key, data, TimeSpan.FromMinutes(1));
        return data;
    }
}
```

---

### Correct TTL values

**Cómo funciona:**
Time-To-Live (TTL) determina cuánto tiempo los datos permanecen en cache. Valores correctos balancean frescura de datos con rendimiento.

**Ventajas:**
- Balancea frescura y rendimiento
- Reduce carga en fuentes de datos
- Mejor experiencia de usuario
- Optimiza uso de memoria

**Desventajas:**
- Requiere tuning
- Puede variar según el tipo de datos
- Requiere monitoreo

**Cuándo usar:**
- Siempre configurar TTL apropiado
- Diferentes TTL para diferentes tipos de datos
- Basado en frecuencia de cambio de datos
- Después de análisis de patrones de acceso

**Impacto en performance:**
Puede mejorar el hit ratio del cache en un 20-50% al optimizar TTL. El impacto es mayor cuando los TTL están mal configurados.

**Ejemplo en C#:**
```csharp
// ✅ TTL basado en tipo de datos
public class TTLStrategy
{
    public MemoryCacheEntryOptions GetCacheOptions(string dataType)
    {
        return dataType switch
        {
            "user" => new MemoryCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromMinutes(15) // Cambia ocasionalmente
            },
            "product" => new MemoryCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromHours(1) // Cambia raramente
            },
            "price" => new MemoryCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromMinutes(5) // Cambia frecuentemente
            },
            _ => new MemoryCacheEntryOptions
            {
                AbsoluteExpirationRelativeToNow = TimeSpan.FromMinutes(10)
            }
        };
    }
}
```

---

### Clear cache invalidation strategy

**Cómo funciona:**
Una estrategia clara de invalidación de cache asegura que los datos obsoletos se eliminen cuando cambian, manteniendo la consistencia.

**Ventajas:**
- Mantiene consistencia
- Evita datos obsoletos
- Mejor experiencia de usuario
- Previene bugs

**Desventajas:**
- Requiere diseño cuidadoso
- Puede ser complejo
- Requiere coordinación

**Cuándo usar:**
- Siempre cuando se actualiza datos
- Sistemas con datos que cambian
- Cuando la consistencia es importante
- Aplicaciones críticas

**Impacto en performance:**
Previene problemas de datos obsoletos que pueden causar bugs y degradar la experiencia de usuario.

**Ejemplo en C#:**
```csharp
public class CacheInvalidation
{
    private readonly IMemoryCache _cache;
    
    public async Task UpdateUserAsync(int userId, User user)
    {
        // Actualizar en base de datos
        await _dataService.UpdateUserAsync(userId, user);
        
        // Invalidar cache
        _cache.Remove($"user:{userId}");
        _cache.Remove($"users:list"); // Invalidar listas relacionadas
    }
    
    // ✅ Invalidación por tags/patterns
    public void InvalidateByPattern(string pattern)
    {
        // Implementar invalidación por patrón si es necesario
    }
}
```

---

### Cache warming

**Cómo funciona:**
Cache warming pre-carga datos en cache antes de que se necesiten, mejorando el rendimiento inicial y reduciendo latencia.

**Ventajas:**
- Mejor rendimiento inicial
- Reduce latencia de primera carga
- Mejor experiencia de usuario
- Reduce carga en fuentes de datos

**Desventajas:**
- Requiere recursos iniciales
- Puede cargar datos no usados
- Requiere estrategia de qué pre-cargar

**Cuándo usar:**
- Al iniciar la aplicación
- Después de reinicios
- Datos críticos frecuentemente accedidos
- Cuando la latencia inicial es importante

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al tener datos ya en cache.

**Ejemplo en C#:**
```csharp
public class CacheWarming
{
    private readonly IMemoryCache _cache;
    private readonly IDataService _dataService;
    
    public async Task WarmCacheAsync()
    {
        // Pre-cargar datos críticos
        var criticalData = await _dataService.GetCriticalDataAsync();
        foreach (var item in criticalData)
        {
            _cache.Set($"data:{item.Id}", item, TimeSpan.FromHours(1));
        }
        
        // Pre-cargar datos frecuentemente accedidos
        var popularData = await _dataService.GetPopularDataAsync();
        foreach (var item in popularData)
        {
            _cache.Set($"popular:{item.Id}", item, TimeSpan.FromMinutes(30));
        }
    }
}
```

---

### Avoid cache stampede

**Cómo funciona:**
Cache stampede ocurre cuando múltiples requests intentan cargar el mismo dato al mismo tiempo después de que expira el cache, causando sobrecarga.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Mejor rendimiento
- Protege fuentes de datos

**Desventajas:**
- Requiere sincronización
- Puede requerir locks o semáforos
- Más complejo

**Cuándo usar:**
- Aplicaciones de alto tráfico
- Cuando hay riesgo de stampede
- Datos caros de cargar
- Sistemas críticos

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-90% durante stampedes. El impacto es crítico para estabilidad.

**Ejemplo en C#:**
```csharp
public class CacheStampedePrevention
{
    private readonly IMemoryCache _cache;
    private readonly SemaphoreSlim _semaphore = new SemaphoreSlim(1, 1);
    
    public async Task<T> GetDataAsync<T>(string key, Func<Task<T>> loadFunc)
    {
        if (_cache.TryGetValue(key, out T cachedValue))
            return cachedValue;
        
        // Solo un thread carga, otros esperan
        await _semaphore.WaitAsync();
        try
        {
            // Double-check después de adquirir lock
            if (_cache.TryGetValue(key, out cachedValue))
                return cachedValue;
            
            // Cargar datos
            var data = await loadFunc();
            _cache.Set(key, data, TimeSpan.FromMinutes(5));
            return data;
        }
        finally
        {
            _semaphore.Release();
        }
    }
}
```

---

### Use LRU or LFU eviction

**Cómo funciona:**
LRU (Least Recently Used) y LFU (Least Frequently Used) son políticas de evicción que eliminan datos menos usados cuando el cache está lleno.

**Ventajas:**
- Optimiza uso de memoria
- Mantiene datos más útiles
- Mejor hit ratio
- Automático

**Desventajas:**
- Requiere tracking de uso
- Overhead adicional
- Puede eliminar datos útiles

**Cuándo usar:**
- Cuando el cache tiene límite de tamaño
- Aplicaciones con memoria limitada
- Cuando se necesita optimizar hit ratio
- Sistemas con muchos datos diferentes

**Impacto en performance:**
Puede mejorar el hit ratio del cache en un 10-30% al mantener datos más útiles.

**Nota:** IMemoryCache en .NET usa LRU por defecto. Para control fino, se pueden usar bibliotecas como LazyCache o implementar políticas personalizadas.

---

### Write-through caching

**Cómo funciona:**
Write-through cache escribe simultáneamente en cache y en la fuente de datos, manteniendo consistencia pero con overhead de escritura.

**Ventajas:**
- Consistencia garantizada
- Datos siempre actualizados
- Simple de implementar

**Desventajas:**
- Más lento que write-behind
- Overhead en escrituras
- Puede ser innecesario para algunos casos

**Cuándo usar:**
- Cuando la consistencia es crítica
- Datos que se leen inmediatamente después de escribir
- Sistemas que requieren consistencia fuerte

**Impacto en performance:**
Puede ralentizar escrituras en un 10-50% pero garantiza consistencia.

**Ejemplo en C#:**
```csharp
public class WriteThroughCache
{
    private readonly IMemoryCache _cache;
    private readonly IDataService _dataService;
    
    public async Task SaveDataAsync(string key, object data)
    {
        // Escribir en fuente de datos
        await _dataService.SaveAsync(key, data);
        
        // Escribir en cache simultáneamente
        _cache.Set(key, data, TimeSpan.FromMinutes(5));
    }
}
```

---

### Write-behind caching

**Cómo funciona:**
Write-behind cache escribe primero en cache y luego asíncronamente en la fuente de datos, mejorando el rendimiento de escritura.

**Ventajas:**
- Muy rápido para escrituras
- Mejor rendimiento
- Mejor experiencia de usuario

**Desventajas:**
- Riesgo de pérdida de datos
- Consistencia eventual
- Más complejo de implementar

**Cuándo usar:**
- Cuando el rendimiento de escritura es crítico
- Cuando se puede aceptar consistencia eventual
- Sistemas con alta frecuencia de escritura
- Datos que pueden regenerarse

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 50-90% al hacer escrituras asíncronas.

**Ejemplo en C#:**
```csharp
public class WriteBehindCache
{
    private readonly IMemoryCache _cache;
    private readonly IDataService _dataService;
    private readonly Channel<WriteOperation> _writeQueue;
    
    public WriteBehindCache()
    {
        _writeQueue = Channel.CreateUnbounded<WriteOperation>();
        _ = ProcessWritesAsync(); // Background task
    }
    
    public void SaveDataAsync(string key, object data)
    {
        // Escribir en cache inmediatamente
        _cache.Set(key, data, TimeSpan.FromMinutes(5));
        
        // Encolar escritura asíncrona
        _writeQueue.Writer.TryWrite(new WriteOperation { Key = key, Data = data });
    }
    
    private async Task ProcessWritesAsync()
    {
        await foreach (var operation in _writeQueue.Reader.ReadAllAsync())
        {
            await _dataService.SaveAsync(operation.Key, operation.Data);
        }
    }
}
```

---

## Networking and IO

Esta sección cubre optimizaciones para operaciones de red y comunicación, críticas para aplicaciones distribuidas y servicios web.

### Use IHttpClientFactory for HTTP clients

**Cómo funciona:**
IHttpClientFactory gestiona el ciclo de vida de HttpClient instances, evitando problemas de socket exhaustion y mejorando el rendimiento mediante connection pooling.

**Ventajas:**
- Evita socket exhaustion
- Mejor rendimiento mediante pooling
- Gestión automática del ciclo de vida
- Mejor para aplicaciones de alto rendimiento

**Desventajas:**
- Requiere configuración
- Curva de aprendizaje

**Cuándo usar:**
- Siempre en aplicaciones ASP.NET Core
- Aplicaciones que hacen muchas llamadas HTTP
- Servicios que consumen APIs externas
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% y prevenir problemas de socket exhaustion que pueden degradar el rendimiento dramáticamente.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Crear HttpClient directamente
public class BadHttpClient
{
    public async Task<string> GetDataAsync(string url)
    {
        var client = new HttpClient(); // Puede causar socket exhaustion
        return await client.GetStringAsync(url);
    }
}

// ✅ Bueno: Usar IHttpClientFactory
public class GoodHttpClient
{
    private readonly IHttpClientFactory _httpClientFactory;
    
    public GoodHttpClient(IHttpClientFactory httpClientFactory)
    {
        _httpClientFactory = httpClientFactory;
    }
    
    public async Task<string> GetDataAsync(string url)
    {
        var client = _httpClientFactory.CreateClient(); // Reutiliza conexiones
        return await client.GetStringAsync(url);
    }
}

// ✅ Mejor: Configurar HttpClient con nombre
services.AddHttpClient("MyApi", client =>
{
    client.BaseAddress = new Uri("https://api.example.com");
    client.Timeout = TimeSpan.FromSeconds(30);
});
```

---

### Use gRPC for high-performance RPC

**Cómo funciona:**
gRPC usa HTTP/2 y Protocol Buffers para comunicación RPC de alto rendimiento, proporcionando mejor rendimiento que REST/JSON.

**Ventajas:**
- Mucho más rápido que REST/JSON
- Streaming bidireccional
- Type-safe
- Mejor para microservicios

**Desventajas:**
- Requiere .NET Core 3.0+
- Menos compatible que REST
- Requiere código generado

**Cuándo usar:**
- Comunicación entre microservicios
- Cuando el rendimiento es crítico
- Aplicaciones de alto rendimiento
- Cuando se necesita streaming

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-10x comparado con REST/JSON, especialmente para datos estructurados.

**Ejemplo en C#:**
```csharp
// ✅ gRPC proporciona mejor rendimiento que REST
// Definir servicio en .proto, luego usar cliente generado
var channel = GrpcChannel.ForAddress("https://localhost:5001");
var client = new Greeter.GreeterClient(channel);
var reply = await client.SayHelloAsync(new HelloRequest { Name = "World" });
```

---

### Use Protocol Buffers for serialization

**Cómo funciona:**
Protocol Buffers es un formato de serialización binario muy eficiente, usado por gRPC y proporcionando mejor rendimiento que JSON.

**Ventajas:**
- Muy eficiente (tamaño y velocidad)
- Type-safe
- Compatible entre lenguajes
- Mejor rendimiento que JSON

**Desventajas:**
- Requiere schema (.proto)
- Menos legible que JSON
- Requiere código generado

**Cuándo usar:**
- Comunicación entre servicios
- Cuando el tamaño y velocidad importan
- Aplicaciones de alto rendimiento
- Con gRPC

**Impacto en performance:**
Puede mejorar el rendimiento de serialización en un 3-10x y reducir el tamaño de datos en un 50-80% comparado con JSON.

---

### Use connection pooling

**Cómo funciona:**
Connection pooling reutiliza conexiones de red en lugar de crear nuevas para cada request, reduciendo overhead significativamente.

**Ventajas:**
- Menos overhead de conexión
- Mejor rendimiento
- Menor latencia
- Mejor utilización de recursos

**Desventajas:**
- Requiere gestión del pool
- Puede retener conexiones

**Cuándo usar:**
- Siempre para conexiones de red
- HTTP clients
- Database connections
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con crear conexiones nuevas. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ IHttpClientFactory maneja connection pooling automáticamente
services.AddHttpClient("MyApi"); // Connection pooling automático

// ✅ Database connection pooling automático en .NET
var connectionString = "Server=...;Database=...;"; // Pooling automático
```

---

### Keep-alive connections

**Cómo funciona:**
Keep-alive mantiene conexiones abiertas entre requests, evitando el overhead de establecer nuevas conexiones.

**Ventajas:**
- Evita overhead de conexión
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples requests

**Desventajas:**
- Usa recursos mientras está abierto
- Requiere gestión de timeouts

**Cuándo usar:**
- Siempre cuando se hacen múltiples requests
- HTTP clients
- Database connections
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar overhead de conexión repetido.

**Ejemplo en C#:**
```csharp
// ✅ HttpClient mantiene conexiones keep-alive automáticamente
var client = new HttpClient();
// Conexiones se mantienen abiertas para reutilización
```

---

### Avoid chatty APIs

**Cómo funciona:**
APIs chatty hacen muchos requests pequeños en lugar de pocos requests grandes, aumentando overhead de red.

**Ventajas:**
- Menos overhead de red
- Mejor rendimiento
- Menor latencia total
- Mejor throughput

**Desventajas:**
- Puede requerir cambios en el diseño
- Menos granularidad

**Cuándo usar:**
- Siempre evitar APIs chatty
- Cuando se pueden combinar requests
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir el número de requests. El impacto es dramático con alta latencia.

**Ejemplo en C#:**
```csharp
// ❌ Malo: API chatty
foreach (var id in ids)
{
    var item = await api.GetItemAsync(id); // Un request por item
}

// ✅ Bueno: Batch request
var items = await api.GetItemsAsync(ids); // Un request para todos
```

---

### Batch network requests

**Cómo funciona:**
Agrupar múltiples requests en un solo batch reduce el número de round-trips y mejora el rendimiento.

**Ventajas:**
- Menos round-trips
- Mejor rendimiento
- Menor latencia total
- Mejor throughput

**Desventajas:**
- Requiere lógica de batching
- Puede aumentar latencia del primer item

**Cuándo usar:**
- Cuando se hacen múltiples requests relacionados
- APIs que soportan batching
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x al reducir round-trips. El impacto es mayor con alta latencia.

**Ejemplo en C#:**
```csharp
// ✅ Batch requests
var requests = ids.Select(id => api.GetItemAsync(id));
var items = await Task.WhenAll(requests); // Paralelo

// ✅ O usar API de batch si está disponible
var items = await api.GetItemsBatchAsync(ids); // Un request
```

---

### Use binary protocols

**Cómo funciona:**
Protocolos binarios (protobuf, MessagePack) son más eficientes que texto (JSON, XML) en tamaño y velocidad de serialización.

**Ventajas:**
- Más rápido
- Menor tamaño
- Menor uso de CPU
- Mejor para alto throughput

**Desventajas:**
- Menos legible
- Requiere schema
- Menos compatible

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando el tamaño es importante
- Comunicación interna entre servicios
- Alto throughput

**Impacto en performance:**
Puede mejorar el rendimiento en un 3-10x y reducir el tamaño en un 50-80% comparado con JSON.

**Ejemplo en C#:**
```csharp
// ✅ Usar protobuf
var message = new MyMessage { Id = 1, Name = "Test" };
var bytes = message.ToByteArray(); // Binario, más rápido

// ✅ Usar MessagePack
var bytes = MessagePackSerializer.Serialize(message); // Binario compacto
```

---

### Prefer HTTP/2 or HTTP/3

**Cómo funciona:**
HTTP/2 y HTTP/3 proporcionan multiplexing, compresión de headers, y mejor rendimiento que HTTP/1.1.

**Ventajas:**
- Multiplexing (múltiples requests en una conexión)
- Compresión de headers
- Mejor rendimiento
- Menor latencia

**Desventajas:**
- Requiere soporte del servidor
- Más complejo

**Cuándo usar:**
- Siempre cuando esté disponible
- Aplicaciones modernas
- Cuando se hacen múltiples requests
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% comparado con HTTP/1.1, especialmente con múltiples requests.

**Ejemplo en C#:**
```csharp
// ✅ HttpClient soporta HTTP/2 automáticamente cuando está disponible
var client = new HttpClient();
// Usa HTTP/2 si el servidor lo soporta
```

---

### Avoid head-of-line blocking

**Cómo funciona:**
Head-of-line blocking ocurre cuando un request lento bloquea otros requests en la misma conexión. HTTP/2 y HTTP/3 lo resuelven.

**Ventajas:**
- Mejor rendimiento
- Requests no se bloquean entre sí
- Mejor experiencia de usuario

**Desventajas:**
- Requiere HTTP/2 o HTTP/3
- Más complejo

**Cuándo usar:**
- Siempre evitar con HTTP/2/3
- Aplicaciones con múltiples requests
- Cuando hay requests de diferentes velocidades

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar bloqueos entre requests.

**Nota:** HTTP/2 y HTTP/3 resuelven esto con multiplexing. En .NET, HttpClient lo maneja automáticamente.

---

### Zero-copy networking

**Cómo funciona:**
Zero-copy networking transfiere datos directamente sin copiar entre buffers, mejorando el rendimiento.

**Ventajas:**
- Elimina copias
- Mejor rendimiento
- Menor uso de CPU
- Mejor para alto throughput

**Desventajas:**
- Requiere soporte del sistema
- Más complejo

**Cuándo usar:**
- Sistemas de alto rendimiento
- Cuando el I/O es un cuello de botella
- Transferencias grandes

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al eliminar copias. En .NET, el runtime puede usar esto automáticamente cuando es posible.

**Nota:** En .NET, System.IO.Pipelines y operaciones de red pueden usar zero-copy automáticamente cuando es posible.

---

### Use compression wisely

**Cómo funciona:**
La compresión reduce el tamaño de datos pero aumenta el uso de CPU. El balance depende de si el cuello de botella es ancho de banda o CPU.

**Ventajas:**
- Reduce tamaño de datos
- Menor uso de ancho de banda
- Mejor para transferencia de red

**Desventajas:**
- Aumenta uso de CPU
- Aumenta latencia
- Puede no ser beneficioso si CPU es el cuello de botella

**Cuándo usar:**
- Cuando el ancho de banda es limitado
- Datos que se comprimen bien
- Transferencia de red
- Cuando CPU no es el cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x cuando el ancho de banda es el cuello de botella. Puede degradar si CPU es el cuello de botella.

**Ejemplo en C#:**
```csharp
// ✅ Compresión en HTTP
var client = new HttpClient();
client.DefaultRequestHeaders.Add("Accept-Encoding", "gzip, br");
// El servidor comprime automáticamente
```

---

### Apply backpressure

**Cómo funciona:**
Backpressure controla la tasa de producción cuando el consumidor es más lento, previniendo sobrecarga y mejorando la estabilidad.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Protege recursos
- Mejor rendimiento general

**Desventajas:**
- Puede limitar el throughput
- Requiere implementación

**Cuándo usar:**
- Sistemas con productores y consumidores
- Cuando hay diferencias de velocidad
- Aplicaciones de alto rendimiento
- Sistemas de streaming

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al prevenir sobrecarga. El impacto es en estabilidad.

**Ejemplo en C#:**
```csharp
// ✅ Usar Channels con backpressure
var channel = Channel.CreateBounded<int>(100); // Límite de capacidad
// El productor espera cuando el canal está lleno
```

---

### Cache DNS results

**Cómo funciona:**
Cachear resultados de DNS evita lookups repetidos, reduciendo latencia y mejorando el rendimiento.

**Ventajas:**
- Reduce latencia
- Mejor rendimiento
- Menos lookups DNS
- Mejor para múltiples requests

**Desventajas:**
- Puede tener datos obsoletos
- Requiere invalidación

**Cuándo usar:**
- Siempre cuando se hacen múltiples requests al mismo dominio
- Aplicaciones de alto rendimiento
- Cuando DNS lookup es costoso

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al evitar lookups DNS repetidos. El impacto es mayor con alta latencia DNS.

**Nota:** En .NET, HttpClient cachea DNS automáticamente. El OS también cachea DNS.

---

### Reuse TLS sessions

**Cómo funciona:**
Reutilizar sesiones TLS evita el overhead del handshake completo, mejorando el rendimiento de conexiones TLS.

**Ventajas:**
- Evita overhead de handshake
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples conexiones

**Desventajas:**
- Requiere gestión de sesiones
- Puede tener implicaciones de seguridad

**Cuándo usar:**
- Siempre cuando se hacen múltiples conexiones TLS
- Aplicaciones de alto rendimiento
- Cuando el handshake es costoso

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar handshakes repetidos.

**Nota:** En .NET, HttpClient reutiliza sesiones TLS automáticamente cuando es posible.

---

### Set correct timeouts

**Cómo funciona:**
Configurar timeouts apropiados previene que requests esperen indefinidamente, mejorando el rendimiento y la estabilidad.

**Ventajas:**
- Previene esperas indefinidas
- Mejor utilización de recursos
- Mejor experiencia de usuario
- Previene acumulación de requests

**Desventajas:**
- Puede terminar requests válidos si son muy cortos
- Requiere tuning

**Cuándo usar:**
- Siempre configurar timeouts
- Timeouts apropiados para cada operación
- Timeouts escalonados

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% cuando hay problemas de red. El impacto es crítico para estabilidad.

**Ejemplo en C#:**
```csharp
// ✅ Configurar timeouts
var client = new HttpClient();
client.Timeout = TimeSpan.FromSeconds(30); // Timeout total

// ✅ Timeouts en base de datos
var connectionString = "Server=...;Connection Timeout=30;Command Timeout=60;";
```

---

### Client-side caching

**Cómo funciona:**
Client-side caching almacena datos en el cliente (navegador, aplicación móvil), reduciendo requests al servidor y mejorando el rendimiento.

**Ventajas:**
- Reduce requests al servidor
- Mejor rendimiento
- Mejor experiencia de usuario
- Funciona offline

**Desventajas:**
- Puede tener datos obsoletos
- Requiere invalidación
- Usa almacenamiento del cliente

**Cuándo usar:**
- Datos que no cambian frecuentemente
- Aplicaciones web y móviles
- Cuando se necesita mejor experiencia de usuario
- Datos que se pueden cachear en cliente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al evitar requests al servidor. El impacto es dramático.

---

### HTTP cache headers

**Cómo funciona:**
HTTP cache headers (Cache-Control, ETag, Last-Modified) controlan cómo los navegadores y proxies cachean recursos, mejorando el rendimiento.

**Ventajas:**
- Reduce requests al servidor
- Mejor rendimiento
- Mejor experiencia de usuario
- Automático en navegadores

**Desventajas:**
- Puede tener datos obsoletos
- Requiere configuración apropiada

**Cuándo usar:**
- Siempre para recursos estáticos
- Recursos que no cambian frecuentemente
- Aplicaciones web
- Cuando se necesita mejor rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para recursos estáticos. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Configurar cache headers en ASP.NET Core
app.UseStaticFiles(new StaticFileOptions
{
    OnPrepareResponse = ctx =>
    {
        ctx.Context.Response.Headers.Append("Cache-Control", "public,max-age=31536000");
    }
});
```

---

### CDN caching

**Cómo funciona:**
CDN (Content Delivery Network) cachea contenido en servidores cercanos a usuarios, reduciendo latencia y mejorando el rendimiento.

**Ventajas:**
- Menor latencia
- Mejor rendimiento
- Mejor experiencia de usuario global
- Reduce carga en servidor origen

**Desventajas:**
- Requiere infraestructura adicional
- Costo adicional
- Puede tener datos obsoletos

**Cuándo usar:**
- Contenido estático
- Aplicaciones globales
- Cuando la latencia es crítica
- Aplicaciones de alto tráfico

**Impacto en performance:**
Puede reducir la latencia en un 50-90% para usuarios lejos del servidor origen. El impacto es dramático.

---

### Redis clustering for high availability

**Cómo funciona:**
Redis clustering distribuye datos entre múltiples nodos Redis, proporcionando alta disponibilidad y escalabilidad.

**Ventajas:**
- Alta disponibilidad
- Escalabilidad horizontal
- Tolerancia a fallos
- Mejor rendimiento

**Desventajas:**
- Más complejo
- Requiere más infraestructura
- Requiere gestión

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Datos grandes que no caben en un nodo
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar la disponibilidad y escalabilidad significativamente, permitiendo manejar más datos y tráfico.

---

### Redis persistence strategies (RDB, AOF)

**Cómo funciona:**
Redis puede persistir datos usando RDB (snapshots) o AOF (append-only file), balanceando durabilidad y rendimiento.

**Ventajas:**
- Durabilidad de datos
- Recuperación después de fallos
- Balance entre durabilidad y rendimiento

**Desventajas:**
- Overhead de persistencia
- Puede afectar rendimiento
- Requiere configuración

**Cuándo usar:**
- Cuando se necesita durabilidad
- Sistemas críticos
- Datos importantes
- Según requisitos de durabilidad

**Impacto en performance:**
RDB tiene menos overhead pero puede perder datos recientes. AOF tiene más overhead pero mejor durabilidad.

---

### Redis pipelining for batch operations

**Cómo funciona:**
Redis pipelining agrupa múltiples comandos en un solo request, reduciendo round-trips y mejorando el rendimiento.

**Ventajas:**
- Menos round-trips
- Mejor rendimiento
- Mejor throughput
- Mejor para operaciones batch

**Desventajas:**
- Requiere batching logic
- Más complejo

**Cuándo usar:**
- Operaciones batch
- Cuando se hacen múltiples operaciones
- Aplicaciones de alto rendimiento
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x al reducir round-trips. El impacto es mayor con muchas operaciones.

**Ejemplo en C#:**
```csharp
// ✅ Redis pipelining
var batch = _database.CreateBatch();
var tasks = new List<Task>();
foreach (var key in keys)
{
    tasks.Add(batch.StringGetAsync(key));
}
batch.Execute();
await Task.WhenAll(tasks);
```

---

### Redis pub/sub for real-time updates

**Cómo funciona:**
Redis pub/sub permite publicar y suscribirse a mensajes, útil para actualizaciones en tiempo real y invalidación de cache.

**Ventajas:**
- Actualizaciones en tiempo real
- Útil para invalidación de cache
- Bajo overhead
- Mejor para sistemas distribuidos

**Desventajas:**
- No garantiza entrega
- Requiere gestión de suscripciones

**Cuándo usar:**
- Invalidación de cache distribuido
- Actualizaciones en tiempo real
- Sistemas distribuidos
- Cuando se necesita notificaciones

**Impacto en performance:**
Puede mejorar la efectividad del cache al permitir invalidación distribuida eficiente.

---

### Redis streams for event sourcing

**Cómo funciona:**
Redis streams proporciona funcionalidad de event streaming similar a Kafka, útil para event sourcing y procesamiento de eventos.

**Ventajas:**
- Event streaming
- Útil para event sourcing
- Persistencia
- Mejor para eventos

**Desventajas:**
- Menor throughput que Kafka
- Requiere Redis 5.0+

**Cuándo usar:**
- Event sourcing
- Procesamiento de eventos
- Cuando se necesita event streaming simple
- Sistemas que ya usan Redis

**Impacto en performance:**
Puede manejar miles de eventos por segundo, mejor para casos de uso simples que Kafka.

---

### Redis Lua scripts for atomic operations

**Cómo funciona:**
Redis Lua scripts permiten ejecutar múltiples operaciones atómicamente en el servidor, útil para operaciones complejas.

**Ventajas:**
- Operaciones atómicas
- Mejor rendimiento (menos round-trips)
- Útil para operaciones complejas
- Mejor consistencia

**Desventajas:**
- Más complejo
- Requiere conocimiento de Lua
- Puede bloquear Redis si es lento

**Cuándo usar:**
- Operaciones que requieren atomicidad
- Operaciones complejas
- Cuando se necesita mejor rendimiento
- Sistemas que requieren consistencia

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20x al reducir round-trips y proporcionar atomicidad.

---

### Redis single-threaded model for lock-free operations

**Cómo funciona:**
Redis es single-threaded, ejecutando comandos atómicamente sin necesidad de locks, proporcionando mejor rendimiento.

**Ventajas:**
- Sin locks
- Mejor rendimiento
- Operaciones atómicas automáticas
- Simplicidad

**Desventajas:**
- Limitado a un core
- Puede ser cuello de botella para operaciones costosas

**Cuándo usar:**
- Siempre (es el modelo de Redis)
- Operaciones rápidas
- Cuando se necesita atomicidad

**Impacto en performance:**
Proporciona mejor rendimiento al evitar overhead de locks, aunque está limitado a un core.

---

### Redis memory optimization (ziplist, intset)

**Cómo funciona:**
Redis usa estructuras de datos optimizadas (ziplist, intset) para reducir el uso de memoria, mejorando la eficiencia.

**Ventajas:**
- Menor uso de memoria
- Mejor eficiencia
- Más datos en la misma memoria
- Automático en Redis

**Desventajas:**
- Limitado a ciertos tipos de datos
- Requiere configuración

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Cuando se necesita optimizar memoria
- Datos que se ajustan a las optimizaciones

**Impacto en performance:**
Puede reducir el uso de memoria en un 50-90% para datos que se ajustan a las optimizaciones.

---

### Redis hash slot distribution for clustering

**Cómo funciona:**
Redis clustering distribuye datos usando hash slots, permitiendo distribución eficiente y rebalanceo automático.

**Ventajas:**
- Distribución eficiente
- Rebalanceo automático
- Escalabilidad horizontal
- Mejor rendimiento

**Desventajas:**
- Requiere clustering
- Más complejo

**Cuándo usar:**
- Redis clustering
- Cuando se necesita escalabilidad
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite escalabilidad horizontal, mejorando el throughput total del cluster.

---

### Redis lazy free for non-blocking deletions

**Cómo funciona:**
Redis lazy free elimina objetos grandes de manera asíncrona, evitando bloquear el servidor y mejorando el rendimiento.

**Ventajas:**
- Eliminaciones no bloqueantes
- Mejor rendimiento
- Mejor experiencia de usuario
- Evita bloqueos

**Desventajas:**
- Requiere Redis 4.0+
- Requiere configuración

**Cuándo usar:**
- Siempre cuando sea posible
- Objetos grandes
- Cuando se necesita mejor rendimiento
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para eliminaciones de objetos grandes al evitar bloqueos.

---

### Redis memory eviction policies

**Cómo funciona:**
Políticas de evicción de Redis determinan qué datos eliminar cuando la memoria está llena, balanceando diferentes estrategias.

**Ventajas:**
- Control sobre evicción
- Optimización según necesidades
- Mejor hit ratio
- Automático

**Desventajas:**
- Requiere configuración apropiada
- Puede eliminar datos útiles

**Cuándo usar:**
- Siempre configurar apropiadamente
- Según patrón de acceso
- Cuando se necesita optimizar hit ratio

**Impacto en performance:**
Puede mejorar el hit ratio en un 10-30% al usar la política apropiada.

---

### Redis RDB compression

**Cómo funciona:**
Comprimir snapshots RDB reduce el tamaño de archivos y el tiempo de escritura, mejorando el rendimiento de persistencia.

**Ventajas:**
- Menor tamaño de archivos
- Más rápido de escribir
- Mejor rendimiento
- Menor uso de almacenamiento

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración

**Cuándo usar:**
- Siempre cuando sea posible
- Snapshots grandes
- Cuando se necesita mejor rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de snapshots en un 20-50% y reducir el tamaño en un 50-80%.

---

### Redis AOF rewrite for log compaction

**Cómo funciona:**
AOF rewrite compacta el log AOF eliminando operaciones redundantes, reduciendo el tamaño y mejorando el rendimiento.

**Ventajas:**
- Reduce tamaño del log
- Mejor rendimiento
- Mejor para logs grandes
- Automático en Redis

**Desventajas:**
- Requiere recursos durante rewrite
- Puede causar latencia temporal

**Cuándo usar:**
- Siempre cuando se usa AOF
- Logs grandes
- Cuando se necesita mejor rendimiento

**Impacto en performance:**
Puede reducir el tamaño del log en un 50-90% y mejorar el rendimiento de lectura significativamente.

---

### Redis connection pooling

**Cómo funciona:**
Connection pooling reutiliza conexiones Redis en lugar de crear nuevas, reduciendo overhead y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Mejor utilización de recursos
- Automático en la mayoría de clientes

**Desventajas:**
- Requiere gestión del pool
- Puede retener conexiones

**Cuándo usar:**
- Siempre cuando sea posible
- Aplicaciones de alto rendimiento
- Cuando se hacen muchas operaciones

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con crear conexiones nuevas. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ StackExchange.Redis maneja connection pooling automáticamente
var connection = ConnectionMultiplexer.Connect("localhost:6379");
var database = connection.GetDatabase(); // Reutiliza conexiones
```

---

### Redis transaction batching (MULTI/EXEC)

**Cómo funciona:**
Redis transactions (MULTI/EXEC) permiten ejecutar múltiples comandos atómicamente, útil para operaciones que requieren atomicidad.

**Ventajas:**
- Operaciones atómicas
- Mejor rendimiento (menos round-trips)
- Útil para operaciones complejas
- Mejor consistencia

**Desventajas:**
- Más complejo
- Puede fallar si hay conflictos

**Cuándo usar:**
- Operaciones que requieren atomicidad
- Operaciones complejas
- Cuando se necesita mejor rendimiento
- Sistemas que requieren consistencia

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20x al reducir round-trips y proporcionar atomicidad.

---

### Memcached for simple key-value caching

**Cómo funciona:**
Memcached es un cache distribuido simple de key-value, más simple que Redis pero con menos features.

**Ventajas:**
- Simple
- Muy rápido
- Buen rendimiento
- Menos overhead

**Desventajas:**
- Menos features que Redis
- Sin persistencia
- Menos flexible

**Cuándo usar:**
- Caching simple de key-value
- Cuando no se necesitan features avanzadas
- Sistemas que requieren simplicidad
- Aplicaciones de alto rendimiento con necesidades simples

**Impacto en performance:**
Puede ser más rápido que Redis para casos simples debido a menos overhead, aunque la diferencia es mínima.

---

### Memcached consistent hashing

**Cómo funciona:**
Consistent hashing distribuye datos entre múltiples servidores Memcached de manera eficiente, permitiendo escalabilidad.

**Ventajas:**
- Distribución eficiente
- Escalabilidad horizontal
- Mejor rendimiento
- Rebalanceo mínimo al agregar/remover servidores

**Desventajas:**
- Requiere múltiples servidores
- Más complejo

**Cuándo usar:**
- Memcached clustering
- Cuando se necesita escalabilidad
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite escalabilidad horizontal, mejorando el throughput total del cluster.

---

### Hazelcast for distributed caching

**Cómo funciona:**
Hazelcast es una plataforma de datos en memoria distribuida que proporciona caching distribuido y estructuras de datos distribuidas.

**Ventajas:**
- Caching distribuido
- Estructuras de datos distribuidas
- Alta disponibilidad
- Buen rendimiento

**Desventajas:**
- Requiere infraestructura
- Más complejo que Redis simple
- Requiere gestión

**Cuándo usar:**
- Caching distribuido complejo
- Cuando se necesitan estructuras de datos distribuidas
- Sistemas Java/.NET
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede proporcionar caching distribuido eficiente con estructuras de datos distribuidas.

---

### Hazelcast near-cache for local caching

**Cómo funciona:**
Hazelcast near-cache mantiene una copia local de datos frecuentemente accedidos, reduciendo latencia de red.

**Ventajas:**
- Menor latencia
- Mejor rendimiento
- Reduce carga en red
- Mejor para datos frecuentemente accedidos

**Desventajas:**
- Usa memoria local
- Puede tener datos obsoletos
- Requiere invalidación

**Cuándo usar:**
- Datos frecuentemente accedidos
- Cuando la latencia de red es un problema
- Aplicaciones de alto rendimiento
- Datos que se pueden cachear localmente

**Impacto en performance:**
Puede reducir la latencia en un 50-90% para datos cacheados localmente. El impacto es dramático.

---

### Varnish HTTP accelerator for reverse proxy caching

**Cómo funciona:**
Varnish es un acelerador HTTP que cachea contenido HTTP, proporcionando mejor rendimiento que servidores web tradicionales.

**Ventajas:**
- Muy rápido
- Mejor rendimiento
- Reduce carga en servidor origen
- Mejor para contenido estático

**Desventajas:**
- Requiere infraestructura adicional
- Requiere configuración
- Específico para HTTP

**Cuándo usar:**
- Contenido HTTP estático
- Cuando se necesita mejor rendimiento
- Aplicaciones web de alto tráfico
- Cuando se necesita reducir carga en servidor origen

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para contenido cacheable. El impacto es dramático.

---

### Varnish VCL optimization

**Cómo funciona:**
Varnish Configuration Language (VCL) permite configurar comportamiento de cache, optimizando según necesidades específicas.

**Ventajas:**
- Optimización específica
- Mejor rendimiento
- Control sobre caching
- Flexibilidad

**Desventajas:**
- Requiere conocimiento de VCL
- Requiere configuración

**Cuándo usar:**
- Cuando se necesita optimización específica
- Sistemas de alto rendimiento
- Después de profiling

**Impacto en performance:**
Puede mejorar el hit ratio y rendimiento en un 20-50% según la optimización.

---

### Varnish cache invalidation (PURGE, BAN)

**Cómo funciona:**
Varnish permite invalidar cache usando PURGE (invalidación específica) o BAN (invalidación por patrón), manteniendo consistencia.

**Ventajas:**
- Invalidación eficiente
- Mantiene consistencia
- Mejor para sistemas dinámicos
- Control sobre invalidación

**Desventajas:**
- Requiere implementación
- Puede ser complejo

**Cuándo usar:**
- Sistemas con contenido dinámico
- Cuando se necesita invalidación
- Aplicaciones de alto rendimiento
- Cuando la consistencia es importante

**Impacto en performance:**
Permite mantener consistencia sin degradar el rendimiento significativamente.

---

### Cache-aside pattern

**Cómo funciona:**
Cache-aside (también conocido como lazy loading) carga datos en cache cuando se solicitan, proporcionando flexibilidad.

**Ventajas:**
- Flexible
- Simple de implementar
- Mejor para datos que cambian frecuentemente
- Control sobre qué cachear

**Desventajas:**
- Puede tener cache miss inicial
- Requiere lógica de invalidación

**Cuándo usar:**
- Datos que cambian frecuentemente
- Cuando se necesita flexibilidad
- Sistemas con patrones de acceso impredecibles
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para datos cacheados comparado con obtenerlos de la fuente original.

---

### Cache-through pattern

**Cómo funciona:**
Cache-through (también conocido como write-through) escribe en cache y fuente de datos simultáneamente, manteniendo consistencia.

**Ventajas:**
- Consistencia garantizada
- Datos siempre actualizados
- Simple de implementar

**Desventajas:**
- Más lento que write-behind
- Overhead en escrituras
- Puede ser innecesario para algunos casos

**Cuándo usar:**
- Cuando la consistencia es crítica
- Datos que se leen inmediatamente después de escribir
- Sistemas que requieren consistencia fuerte

**Impacto en performance:**
Puede ralentizar escrituras en un 10-50% pero garantiza consistencia.

---

### Database query result caching

**Cómo funciona:**
Cachear resultados de queries de base de datos evita ejecutar queries repetidas, mejorando el rendimiento significativamente.

**Ventajas:**
- Muy rápido para queries repetidas
- Reduce carga en base de datos
- Mejor rendimiento
- Mejor experiencia de usuario

**Desventajas:**
- Puede tener resultados obsoletos
- Requiere invalidación
- Usa memoria

**Cuándo usar:**
- Queries frecuentes con resultados que no cambian frecuentemente
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar resultados ligeramente obsoletos
- Queries costosas

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para queries cacheadas comparado con ejecutar la query.

---

### Application-level caching strategies

**Cómo funciona:**
Estrategias de caching a nivel de aplicación cachean datos de aplicación, mejorando el rendimiento sin depender de infraestructura externa.

**Ventajas:**
- Control total
- Muy rápido
- Sin overhead de red
- Flexible

**Desventajas:**
- Limitado a un proceso
- Usa memoria del proceso
- Requiere implementación

**Cuándo usar:**
- Datos de aplicación
- Cuando se necesita máximo rendimiento
- Aplicaciones single-instance
- Datos que se pueden cachear en memoria

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para datos cacheados. El impacto es dramático.

---

### Session caching

**Cómo funciona:**
Cachear sesiones en memoria o cache distribuido mejora el rendimiento al evitar acceso a almacenamiento persistente.

**Ventajas:**
- Mejor rendimiento
- Menor latencia
- Mejor experiencia de usuario
- Reduce carga en almacenamiento

**Desventajas:**
- Puede perder sesiones si el servidor falla
- Requiere gestión

**Cuándo usar:**
- Aplicaciones web
- Cuando el rendimiento es crítico
- Sistemas de alto rendimiento
- Cuando se puede aceptar pérdida de sesiones en fallos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con almacenamiento persistente. El impacto es dramático.

---

### Cache compression for large values

**Cómo funciona:**
Comprimir valores grandes en cache reduce el uso de memoria y mejora el rendimiento de transferencia.

**Ventajas:**
- Menor uso de memoria
- Mejor rendimiento de transferencia
- Más datos en la misma memoria
- Mejor para valores grandes

**Desventajas:**
- Aumenta uso de CPU
- Requiere descompresión

**Cuándo usar:**
- Valores grandes
- Cuando la memoria es limitada
- Cuando se puede aceptar overhead de CPU
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede reducir el uso de memoria en un 50-80% pero aumenta el uso de CPU. El balance depende del caso.

---

### Cache partitioning strategies

**Cómo funciona:**
Partitioning de cache divide datos entre múltiples instancias de cache, permitiendo mejor escalabilidad y rendimiento.

**Ventajas:**
- Mejor escalabilidad
- Mejor rendimiento
- Mejor para datos grandes
- Permite distribución

**Desventajas:**
- Más complejo
- Requiere gestión
- Puede requerir routing

**Cuándo usar:**
- Datos grandes que no caben en un cache
- Cuando se necesita escalabilidad
- Sistemas de alto rendimiento
- Cuando se puede particionar datos

**Impacto en performance:**
Puede mejorar la escalabilidad y rendimiento al distribuir datos entre múltiples instancias.

---

### Cache coherency strategies

**Cómo funciona:**
Estrategias de coherencia de cache aseguran que múltiples instancias de cache tengan datos consistentes, mejorando la consistencia.

**Ventajas:**
- Mejor consistencia
- Previene datos obsoletos
- Mejor para sistemas distribuidos
- Mejor experiencia de usuario

**Desventajas:**
- Más complejo
- Puede requerir invalidación distribuida
- Puede afectar rendimiento

**Cuándo usar:**
- Sistemas distribuidos
- Cuando la consistencia es importante
- Aplicaciones de alto rendimiento
- Cuando se necesita evitar datos obsoletos

**Impacto en performance:**
Puede mantener consistencia sin degradar el rendimiento significativamente si se implementa bien.

---

### Distributed cache invalidation

**Cómo funciona:**
Invalidación distribuida de cache asegura que cambios se propaguen a todas las instancias de cache, manteniendo consistencia.

**Ventajas:**
- Mantiene consistencia
- Previene datos obsoletos
- Mejor para sistemas distribuidos
- Mejor experiencia de usuario

**Desventajas:**
- Más complejo
- Requiere comunicación entre instancias
- Puede afectar rendimiento

**Cuándo usar:**
- Sistemas distribuidos
- Cuando la consistencia es importante
- Aplicaciones de alto rendimiento
- Cuando se necesita evitar datos obsoletos

**Impacto en performance:**
Puede mantener consistencia sin degradar el rendimiento significativamente si se implementa bien (pub/sub, etc.).

---

### Cache hit ratio optimization

**Cómo funciona:**
Optimizar cache hit ratio (porcentaje de requests que encuentran datos en cache) mejora la efectividad del cache y el rendimiento.

**Ventajas:**
- Mejor efectividad del cache
- Mejor rendimiento
- Mejor utilización de recursos
- Optimización según necesidades

**Desventajas:**
- Requiere monitoreo
- Requiere ajuste

**Cuándo usar:**
- Siempre monitorear y optimizar
- Sistemas de alto rendimiento
- Cuando se necesita mejor rendimiento
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al mejorar la efectividad del cache. El impacto es mayor cuando el hit ratio es bajo.

---

### Cache preloading

**Cómo funciona:**
Preloading carga datos en cache antes de que se necesiten, mejorando el rendimiento inicial y reduciendo latencia.

**Ventajas:**
- Mejor rendimiento inicial
- Reduce latencia de primera carga
- Mejor experiencia de usuario
- Reduce carga en fuentes de datos

**Desventajas:**
- Requiere recursos iniciales
- Puede cargar datos no usados
- Requiere estrategia de qué pre-cargar

**Cuándo usar:**
- Al iniciar la aplicación
- Después de reinicios
- Datos críticos frecuentemente accedidos
- Cuando la latencia inicial es importante

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al tener datos ya en cache.

---

### Cache versioning for invalidation

**Cómo funciona:**
Versionado de cache usa versiones para invalidar datos, permitiendo invalidación eficiente y manteniendo consistencia.

**Ventajas:**
- Invalidación eficiente
- Mantiene consistencia
- Mejor para sistemas complejos
- Control sobre invalidación

**Desventajas:**
- Más complejo
- Requiere gestión de versiones

**Cuándo usar:**
- Sistemas complejos
- Cuando se necesita invalidación eficiente
- Aplicaciones de alto rendimiento
- Cuando la consistencia es importante

**Impacto en performance:**
Puede mantener consistencia sin degradar el rendimiento significativamente.

---

### Cache stampede prevention (probabilistic early expiration)

**Cómo funciona:**
Prevención de cache stampede usa expiración probabilística temprana para evitar que múltiples requests carguen el mismo dato simultáneamente.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Mejor rendimiento
- Protege fuentes de datos

**Desventajas:**
- Más complejo
- Puede servir datos ligeramente obsoletos

**Cuándo usar:**
- Aplicaciones de alto tráfico
- Cuando hay riesgo de stampede
- Datos caros de cargar
- Sistemas críticos

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-90% durante stampedes. El impacto es crítico para estabilidad.

---

### Cache aside with write-behind

**Cómo funciona:**
Combinar cache-aside con write-behind proporciona flexibilidad en lectura y rendimiento en escritura, balanceando diferentes necesidades.

**Ventajas:**
- Flexibilidad en lectura
- Mejor rendimiento en escritura
- Mejor para sistemas complejos
- Balancea diferentes necesidades

**Desventajas:**
- Más complejo
- Requiere implementación cuidadosa

**Cuándo usar:**
- Sistemas complejos
- Cuando se necesita flexibilidad y rendimiento
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar consistencia eventual

**Impacto en performance:**
Puede mejorar el rendimiento en lectura y escritura al combinar beneficios de ambos patrones.

---

### Cache sharding strategies

**Cómo funciona:**
Sharding de cache divide datos entre múltiples instancias de cache usando estrategias (hash, range, directory), permitiendo escalabilidad.

**Ventajas:**
- Mejor escalabilidad
- Mejor rendimiento
- Mejor para datos grandes
- Permite distribución

**Desventajas:**
- Más complejo
- Requiere gestión
- Puede requerir rebalanceo

**Cuándo usar:**
- Datos grandes que no caben en un cache
- Cuando se necesita escalabilidad
- Sistemas de alto rendimiento
- Cuando se puede shardear datos

**Impacto en performance:**
Puede mejorar la escalabilidad y rendimiento al distribuir datos entre múltiples instancias.

---

## Message Queues and Event Streaming

Esta sección cubre optimizaciones para sistemas de mensajería y procesamiento de eventos, críticos para arquitecturas distribuidas y event-driven.

### Apache Kafka for high-throughput event streaming

**Cómo funciona:**
Kafka es una plataforma de streaming de eventos distribuida que usa zero-copy (sendfile/mmap) y batching para lograr alto throughput.

**Ventajas:**
- Muy alto throughput (millones de mensajes/segundo)
- Zero-copy para mejor rendimiento
- Persistencia y replicación
- Escalabilidad horizontal

**Desventajas:**
- Complejidad operacional
- Requiere infraestructura
- Overhead para casos simples

**Cuándo usar:**
- Event streaming de alto volumen
- Arquitecturas event-driven
- Cuando se necesita alto throughput
- Sistemas de logging y métricas

**Impacto en performance:**
Puede manejar millones de mensajes por segundo con latencia baja. El zero-copy mejora el rendimiento en un 20-50% comparado con copias tradicionales.

---

### Kafka zero-copy using sendfile() and mmap()

**Cómo funciona:**
Kafka usa sendfile() para transferir datos directamente del disco a la red sin copiar a user space, y mmap() para acceso eficiente a archivos de log.

**Ventajas:**
- Elimina copias innecesarias
- Mejor rendimiento
- Menor uso de CPU
- Mejor para alto throughput

**Desventajas:**
- Específico del sistema operativo
- Requiere configuración
- No disponible en todos los sistemas

**Cuándo usar:**
- Sistemas de alto throughput
- Cuando el I/O es un cuello de botella
- Aplicaciones que transfieren muchos datos

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al eliminar copias de memoria. El impacto es mayor para mensajes grandes.

---

### Kafka partitioning strategies

**Cómo funciona:**
Kafka distribuye mensajes entre particiones basándose en la clave del mensaje. Estrategias de particionamiento afectan el throughput y el orden.

**Ventajas:**
- Permite paralelización
- Control sobre ordenamiento
- Mejor throughput
- Escalabilidad

**Desventajas:**
- Requiere diseño cuidadoso
- Puede causar desbalanceo

**Cuándo usar:**
- Event streaming de alto volumen
- Cuando se necesita ordenamiento por clave
- Sistemas que requieren paralelización

**Impacto en performance:**
Puede mejorar el throughput en un 10-100x al permitir procesamiento paralelo. El impacto depende del número de particiones.

---

### Kafka consumer groups for parallel processing

**Cómo funciona:**
Consumer groups permiten que múltiples consumidores procesen diferentes particiones en paralelo, mejorando el throughput.

**Ventajas:**
- Procesamiento paralelo
- Mejor throughput
- Escalabilidad horizontal
- Mejor utilización de recursos

**Desventajas:**
- Requiere gestión de grupos
- Puede requerir rebalanceo

**Cuándo usar:**
- Cuando se necesita alto throughput
- Procesamiento de eventos
- Sistemas que requieren escalabilidad

**Impacto en performance:**
Puede mejorar el throughput en un Nx donde N es el número de consumidores (hasta el número de particiones).

---

### Kafka batching and compression

**Cómo funciona:**
Kafka agrupa mensajes en batches y los comprime, mejorando el throughput y reduciendo el uso de ancho de banda.

**Ventajas:**
- Mejor throughput
- Menor uso de ancho de banda
- Menos overhead por mensaje
- Mejor rendimiento

**Desventajas:**
- Aumenta latencia del primer mensaje
- Requiere configuración

**Cuándo usar:**
- Event streaming de alto volumen
- Cuando el throughput es más importante que la latencia
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput en un 5-20x y reducir el tamaño en un 50-80% con compresión.

---

### Kafka retention policies

**Cómo funciona:**
Kafka retiene mensajes por tiempo o tamaño, permitiendo control sobre cuánto tiempo se mantienen los datos.

**Ventajas:**
- Control sobre almacenamiento
- Permite replay de eventos
- Mejor para event sourcing

**Desventajas:**
- Usa almacenamiento
- Requiere configuración

**Cuándo usar:**
- Event sourcing
- Cuando se necesita replay
- Sistemas que requieren historial

**Impacto en performance:**
Afecta el uso de almacenamiento pero no el rendimiento directamente. Configuración apropiada previene problemas de espacio.

---

### Kafka log segment batching for sequential writes

**Cómo funciona:**
Kafka agrupa mensajes en segmentos de log y los escribe secuencialmente, mejorando el rendimiento de I/O.

**Ventajas:**
- Mejor rendimiento de I/O
- Escrituras secuenciales
- Mejor throughput
- Mejor para alto volumen

**Desventajas:**
- Requiere configuración
- Puede aumentar latencia

**Cuándo usar:**
- Event streaming de alto volumen
- Cuando el throughput es crítico
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput de escritura en un 5-20x al usar escrituras secuenciales en lugar de aleatorias.

---

### Kafka compression (snappy, lz4, gzip, zstd)

**Cómo funciona:**
Kafka comprime mensajes en batches, reduciendo el tamaño y mejorando el throughput de red.

**Ventajas:**
- Reduce tamaño de datos
- Mejor throughput de red
- Menor uso de ancho de banda
- Mejor rendimiento

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración

**Cuándo usar:**
- Event streaming de alto volumen
- Cuando el ancho de banda es limitado
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput en un 2-5x y reducir el tamaño en un 50-80%. zstd generalmente ofrece mejor balance.

---

### Kafka idempotent producers for exactly-once semantics

**Cómo funciona:**
Producers idempotentes aseguran que mensajes no se dupliquen incluso si hay retries, proporcionando exactly-once semantics.

**Ventajas:**
- Exactly-once semantics
- Sin duplicados
- Mejor para sistemas críticos
- Mejor consistencia

**Desventajas:**
- Overhead adicional
- Requiere configuración

**Cuándo usar:**
- Cuando se necesita exactly-once
- Sistemas críticos
- Cuando los duplicados son problemáticos

**Impacto en performance:**
Tiene overhead mínimo (5-10%) pero proporciona garantías importantes para sistemas críticos.

---

### Kafka transactional producers

**Cómo funciona:**
Producers transaccionales permiten enviar múltiples mensajes como una transacción atómica, asegurando all-or-nothing.

**Ventajas:**
- Atomicidad
- Mejor consistencia
- Mejor para sistemas críticos

**Desventajas:**
- Overhead adicional
- Requiere configuración
- Más complejo

**Cuándo usar:**
- Cuando se necesita atomicidad
- Sistemas críticos
- Cuando múltiples mensajes deben ser atómicos

**Impacto en performance:**
Tiene overhead adicional (10-20%) pero proporciona garantías importantes para sistemas críticos.

---

### Kafka log compaction for key-value topics

**Cómo funciona:**
Log compaction mantiene solo el último valor para cada clave, reduciendo el tamaño del log mientras mantiene el estado actual.

**Ventajas:**
- Reduce tamaño del log
- Mantiene estado actual
- Mejor para key-value stores
- Mejor rendimiento de lectura

**Desventajas:**
- Pierde historial
- Requiere configuración

**Cuándo usar:**
- Key-value topics
- Cuando solo se necesita el estado actual
- Sistemas que requieren estado actualizado

**Impacto en performance:**
Puede reducir el tamaño del log en un 50-90% y mejorar el rendimiento de lectura significativamente.

---

### Kafka consumer fetch size tuning

**Cómo funciona:**
Ajustar el tamaño de fetch del consumidor balancea latencia y throughput, afectando cuántos mensajes se obtienen por request.

**Ventajas:**
- Control sobre latencia vs throughput
- Mejor rendimiento
- Optimización según necesidades

**Desventajas:**
- Requiere tuning
- Puede variar según carga

**Cuándo usar:**
- Cuando se necesita optimizar latencia o throughput
- Sistemas de alto rendimiento
- Después de profiling

**Impacto en performance:**
Puede mejorar el throughput en un 20-50% o reducir la latencia según la configuración.

---

### Kafka producer batch size and linger.ms tuning

**Cómo funciona:**
Ajustar batch size y linger.ms balancea latencia y throughput, afectando cuántos mensajes se agrupan antes de enviar.

**Ventajas:**
- Control sobre latencia vs throughput
- Mejor rendimiento
- Optimización según necesidades

**Desventajas:**
- Requiere tuning
- Puede variar según carga

**Cuándo usar:**
- Cuando se necesita optimizar latencia o throughput
- Sistemas de alto rendimiento
- Después de profiling

**Impacto en performance:**
Puede mejorar el throughput en un 20-100% al optimizar batching. El impacto depende de la configuración.

---

### Kafka replication factor and min.insync.replicas

**Cómo funciona:**
Replication factor determina cuántas copias de datos se mantienen. min.insync.replicas determina cuántas deben estar sincronizadas.

**Ventajas:**
- Alta disponibilidad
- Durabilidad
- Tolerancia a fallos
- Mejor confiabilidad

**Desventajas:**
- Usa más almacenamiento
- Overhead de replicación
- Requiere más recursos

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Datos importantes

**Impacto en performance:**
Tiene overhead de replicación (10-30%) pero proporciona alta disponibilidad y durabilidad.

---

### Kafka unclean leader election avoidance

**Cómo funciona:**
Evitar unclean leader election previene pérdida de datos al no permitir que un replica fuera de sync se convierta en leader.

**Ventajas:**
- Previene pérdida de datos
- Mejor durabilidad
- Mejor para sistemas críticos

**Desventajas:**
- Puede causar indisponibilidad temporal
- Requiere configuración

**Cuándo usar:**
- Sistemas críticos
- Cuando la durabilidad es importante
- Cuando se puede aceptar indisponibilidad temporal

**Impacto en performance:**
No afecta el rendimiento directamente pero previene pérdida de datos que puede ser crítica.

---

### RabbitMQ for reliable message queuing

**Cómo funciona:**
RabbitMQ es un message broker que proporciona message queuing confiable con soporte para múltiples patrones de mensajería.

**Ventajas:**
- Confiable
- Múltiples patrones
- Buen rendimiento
- Fácil de usar

**Desventajas:**
- Menor throughput que Kafka
- Requiere infraestructura
- Overhead para casos simples

**Cuándo usar:**
- Message queuing confiable
- Cuando se necesita menor latencia que Kafka
- Sistemas que requieren garantías de entrega

**Impacto en performance:**
Puede manejar miles de mensajes por segundo con latencia baja. Mejor para latencia que para throughput extremo.

---

### RabbitMQ exchanges and routing

**Cómo funciona:**
RabbitMQ usa exchanges para enrutar mensajes a queues basándose en routing keys y tipos de exchange.

**Ventajas:**
- Flexibilidad en routing
- Múltiples patrones
- Mejor organización
- Mejor para sistemas complejos

**Desventajas:**
- Más complejo
- Requiere configuración

**Cuándo usar:**
- Sistemas con routing complejo
- Múltiples consumidores
- Cuando se necesita flexibilidad

**Impacto en performance:**
El overhead de routing es mínimo pero proporciona flexibilidad importante para sistemas complejos.

---

### RabbitMQ message acknowledgments

**Cómo funciona:**
Message acknowledgments aseguran que mensajes se procesen correctamente antes de ser removidos de la queue.

**Ventajas:**
- Garantías de procesamiento
- Mejor confiabilidad
- Previene pérdida de mensajes

**Desventajas:**
- Overhead adicional
- Puede requerir retry logic

**Cuándo usar:**
- Cuando se necesita garantías de procesamiento
- Sistemas críticos
- Cuando la pérdida de mensajes es problemática

**Impacto en performance:**
Tiene overhead mínimo pero proporciona garantías importantes para sistemas críticos.

---

### RabbitMQ publisher confirms

**Cómo funciona:**
Publisher confirms aseguran que mensajes se entreguen a la queue antes de confirmar al publisher.

**Ventajas:**
- Garantías de entrega
- Mejor confiabilidad
- Previene pérdida de mensajes

**Desventajas:**
- Overhead adicional
- Aumenta latencia

**Cuándo usar:**
- Cuando se necesita garantías de entrega
- Sistemas críticos
- Cuando la pérdida de mensajes es problemática

**Impacto en performance:**
Aumenta la latencia ligeramente (10-20%) pero proporciona garantías importantes.

---

### RabbitMQ prefetch count tuning

**Cómo funciona:**
Prefetch count controla cuántos mensajes no confirmados puede tener un consumidor, balanceando distribución y throughput.

**Ventajas:**
- Control sobre distribución
- Mejor balanceo de carga
- Optimización según necesidades

**Desventajas:**
- Requiere tuning
- Puede variar según carga

**Cuándo usar:**
- Cuando se necesita balancear carga entre consumidores
- Sistemas con múltiples consumidores
- Después de profiling

**Impacto en performance:**
Puede mejorar el balanceo de carga y throughput en un 20-50% según la configuración.

---

### RabbitMQ queue mirroring for HA

**Cómo funciona:**
Queue mirroring replica queues en múltiples nodos, proporcionando alta disponibilidad y tolerancia a fallos.

**Ventajas:**
- Alta disponibilidad
- Tolerancia a fallos
- Mejor confiabilidad
- Sin pérdida de datos

**Desventajas:**
- Usa más recursos
- Overhead de replicación
- Requiere configuración

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Datos importantes

**Impacto en performance:**
Tiene overhead de replicación (10-30%) pero proporciona alta disponibilidad crítica.

---

### Amazon SQS for cloud message queuing

**Cómo funciona:**
Amazon SQS es un servicio de message queuing gestionado en AWS, proporcionando message queuing sin gestión de infraestructura.

**Ventajas:**
- Gestionado (sin infraestructura)
- Escalable automáticamente
- Alta disponibilidad
- Integración con otros servicios AWS

**Desventajas:**
- Costo por mensaje
- Latencia de red
- Menor control que self-hosted

**Cuándo usar:**
- Aplicaciones en AWS
- Cuando se necesita message queuing sin gestión
- Sistemas que requieren escalabilidad automática
- Aplicaciones cloud-native

**Impacto en performance:**
Puede manejar millones de mensajes por segundo con escalabilidad automática. Latencia típica de 10-50ms.

---

### Azure Service Bus

**Cómo funciona:**
Azure Service Bus es un servicio de message queuing gestionado en Azure, proporcionando message queuing y pub/sub.

**Ventajas:**
- Gestionado (sin infraestructura)
- Escalable automáticamente
- Alta disponibilidad
- Soporte para pub/sub y queues

**Desventajas:**
- Costo por mensaje
- Latencia de red
- Menor control que self-hosted

**Cuándo usar:**
- Aplicaciones en Azure
- Cuando se necesita message queuing sin gestión
- Sistemas que requieren escalabilidad automática
- Aplicaciones cloud-native

**Impacto en performance:**
Puede manejar miles de mensajes por segundo con escalabilidad automática. Latencia típica de 10-50ms.

---

### Google Cloud Pub/Sub

**Cómo funciona:**
Google Cloud Pub/Sub es un servicio de message queuing gestionado en GCP, proporcionando pub/sub escalable.

**Ventajas:**
- Gestionado (sin infraestructura)
- Escalable automáticamente
- Alta disponibilidad
- Buen rendimiento

**Desventajas:**
- Costo por mensaje
- Latencia de red
- Menor control que self-hosted

**Cuándo usar:**
- Aplicaciones en GCP
- Cuando se necesita pub/sub sin gestión
- Sistemas que requieren escalabilidad automática
- Aplicaciones cloud-native

**Impacto en performance:**
Puede manejar millones de mensajes por segundo con escalabilidad automática. Latencia típica de 10-50ms.

---

### Apache Pulsar for multi-tenancy

**Cómo funciona:**
Apache Pulsar es una plataforma de messaging que soporta multi-tenancy, proporcionando mejor aislamiento y gestión.

**Ventajas:**
- Multi-tenancy
- Mejor aislamiento
- Buen rendimiento
- Escalabilidad horizontal

**Desventajas:**
- Más complejo que Kafka
- Requiere infraestructura
- Curva de aprendizaje

**Cuándo usar:**
- Sistemas multi-tenant
- Cuando se necesita mejor aislamiento
- Sistemas que requieren multi-tenancy
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede manejar millones de mensajes por segundo con mejor aislamiento que Kafka para casos multi-tenant.

---

### Apache Flink for stream processing

**Cómo funciona:**
Apache Flink es una plataforma de stream processing que procesa datos en tiempo real, proporcionando procesamiento de streams de bajo latencia.

**Ventajas:**
- Procesamiento de streams en tiempo real
- Baja latencia
- Buen rendimiento
- Escalabilidad horizontal

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Curva de aprendizaje

**Cuándo usar:**
- Stream processing en tiempo real
- Cuando se necesita baja latencia
- Sistemas de procesamiento de eventos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede procesar millones de eventos por segundo con latencia de milisegundos. Mejor para procesamiento en tiempo real que Kafka Streams en algunos casos.

---

### Kafka Streams for stateful stream processing

**Cómo funciona:**
Kafka Streams permite procesamiento de streams stateful sobre Kafka, proporcionando procesamiento de eventos con estado.

**Ventajas:**
- Procesamiento stateful
- Integrado con Kafka
- Buen rendimiento
- Escalabilidad horizontal

**Desventajas:**
- Requiere Kafka
- Más complejo que procesamiento stateless
- Curva de aprendizaje

**Cuándo usar:**
- Procesamiento de streams stateful
- Cuando se usa Kafka
- Sistemas de procesamiento de eventos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede procesar millones de eventos por segundo con estado. Mejor para casos que requieren estado que procesamiento stateless.

---

### Apache Storm for real-time stream processing

**Cómo funciona:**
Apache Storm es una plataforma de stream processing que procesa datos en tiempo real, proporcionando procesamiento de streams de bajo latencia.

**Ventajas:**
- Procesamiento de streams en tiempo real
- Baja latencia
- Buen rendimiento
- Escalabilidad horizontal

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Menos popular que Flink actualmente

**Cuándo usar:**
- Stream processing en tiempo real
- Cuando se necesita baja latencia
- Sistemas de procesamiento de eventos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede procesar millones de eventos por segundo con latencia de milisegundos. Similar a Flink en rendimiento.

---

### Message queue patterns (pub/sub, point-to-point)

**Cómo funciona:**
Diferentes patrones de message queuing (pub/sub, point-to-point) tienen diferentes características y casos de uso.

**Ventajas:**
- Flexibilidad según necesidades
- Mejor para diferentes casos de uso
- Optimización según patrón

**Desventajas:**
- Requiere conocimiento de patrones
- Puede requerir cambios

**Cuándo usar:**
- **Pub/sub**: Múltiples consumidores, broadcasting
- **Point-to-point**: Un consumidor por mensaje, load balancing

**Impacto en performance:**
Elegir el patrón correcto puede mejorar el rendimiento y escalabilidad según el caso de uso.

---

### Event sourcing with message queues

**Cómo funciona:**
Event sourcing almacena eventos en lugar del estado actual. Message queues pueden usarse para almacenar y procesar eventos.

**Ventajas:**
- Permite replay
- Mejor auditoría
- Mejor para sistemas complejos
- Permite time travel

**Desventajas:**
- Más complejo
- Requiere más almacenamiento
- Consistencia eventual

**Cuándo usar:**
- Sistemas que requieren auditoría
- Cuando se necesita replay
- Sistemas complejos
- Cuando se necesita time travel

**Impacto en performance:**
Puede mejorar el rendimiento de escritura al usar append-only storage, pero puede requerir más procesamiento para lecturas.

---

### Change Data Capture (CDC) for database events

**Cómo funciona:**
CDC captura cambios en bases de datos y los publica como eventos, permitiendo sincronización y procesamiento de cambios.

**Ventajas:**
- Sincronización en tiempo real
- Procesamiento de cambios
- Mejor para sistemas distribuidos
- Mejor para event-driven architecture

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Puede tener overhead

**Cuándo usar:**
- Sincronización de datos
- Event-driven architecture
- Sistemas distribuidos
- Cuando se necesita procesar cambios en tiempo real

**Impacto en performance:**
Puede proporcionar sincronización en tiempo real con overhead mínimo, mejorando la consistencia en sistemas distribuidos.

---

### Message serialization optimization

**Cómo funciona:**
Optimizar serialización de mensajes (usar protobuf, MessagePack en lugar de JSON) mejora el rendimiento y reduce el tamaño.

**Ventajas:**
- Más rápido
- Menor tamaño
- Menor uso de CPU
- Mejor para alto throughput

**Desventajas:**
- Menos legible
- Requiere schema
- Menos compatible

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando el tamaño es importante
- Comunicación interna entre servicios
- Alto throughput

**Impacto en performance:**
Puede mejorar el rendimiento en un 3-10x y reducir el tamaño en un 50-80% comparado con JSON.

---

### Message batching

**Cómo funciona:**
Agrupar múltiples mensajes en un batch reduce el número de requests y mejora el rendimiento.

**Ventajas:**
- Menos requests
- Mejor rendimiento
- Mejor throughput
- Menor overhead

**Desventajas:**
- Aumenta latencia del primer mensaje
- Requiere lógica de batching

**Cuándo usar:**
- Cuando se envían muchos mensajes
- Cuando el throughput es más importante que la latencia
- Aplicaciones de alto rendimiento
- Sistemas de alto volumen

**Impacto en performance:**
Puede mejorar el throughput en un 5-50x al reducir el número de requests. El impacto es mayor con muchos mensajes.

---

### Dead letter queues for error handling

**Cómo funciona:**
Dead letter queues almacenan mensajes que no se pueden procesar, permitiendo análisis y reprocesamiento.

**Ventajas:**
- Mejor manejo de errores
- Permite análisis
- Permite reprocesamiento
- Mejor para sistemas críticos

**Desventajas:**
- Requiere gestión
- Puede acumular mensajes
- Requiere monitoreo

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita manejo de errores robusto
- Aplicaciones de producción
- Sistemas que requieren confiabilidad

**Impacto en performance:**
No afecta el rendimiento directamente pero mejora la confiabilidad y permite recuperación de errores.

---

### Message priority queues

**Cómo funciona:**
Priority queues procesan mensajes por prioridad, permitiendo que mensajes importantes se procesen primero.

**Ventajas:**
- Mensajes importantes primero
- Mejor para sistemas con prioridades
- Mejor experiencia de usuario
- Optimización según necesidades

**Desventajas:**
- Más complejo
- Puede requerir gestión de prioridades
- Puede causar starvation de mensajes de baja prioridad

**Cuándo usar:**
- Sistemas con mensajes de diferentes prioridades
- Cuando algunos mensajes son más importantes
- Aplicaciones que requieren priorización
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar la experiencia de usuario al procesar mensajes importantes primero, aunque puede afectar el throughput total.

---

### Message deduplication

**Cómo funciona:**
Deduplicación de mensajes identifica y elimina mensajes duplicados, previniendo procesamiento duplicado.

**Ventajas:**
- Previene procesamiento duplicado
- Mejor consistencia
- Mejor para sistemas críticos
- Previene problemas de duplicación

**Desventajas:**
- Requiere almacenamiento de IDs
- Puede usar memoria
- Requiere gestión

**Cuándo usar:**
- Sistemas que pueden recibir mensajes duplicados
- Cuando la duplicación es problemática
- Sistemas críticos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Tiene overhead mínimo pero previene procesamiento duplicado que puede causar problemas de consistencia.

---

### Idempotent message processing

**Cómo funciona:**
Procesamiento idempotente asegura que procesar el mismo mensaje múltiples veces produce el mismo resultado, permitiendo retries seguros.

**Ventajas:**
- Permite retries seguros
- Mejor resiliencia
- Mejor para sistemas distribuidos
- Previene problemas de duplicación

**Desventajas:**
- Requiere diseño cuidadoso
- Puede ser más complejo

**Cuándo usar:**
- Siempre cuando sea posible
- Sistemas distribuidos
- Cuando se necesita resiliencia
- Aplicaciones de producción

**Impacto en performance:**
Permite retries que pueden mejorar la confiabilidad sin afectar el rendimiento directamente.

---

### Exactly-once delivery semantics

**Cómo funciona:**
Exactly-once delivery asegura que cada mensaje se entrega exactamente una vez, proporcionando garantías fuertes.

**Ventajas:**
- Garantías fuertes
- Sin duplicados
- Mejor para sistemas críticos
- Mejor consistencia

**Desventajas:**
- Overhead adicional
- Más complejo
- Puede afectar rendimiento

**Cuándo usar:**
- Sistemas críticos
- Cuando los duplicados son problemáticos
- Aplicaciones que requieren garantías fuertes
- Sistemas de alto rendimiento

**Impacto en performance:**
Tiene overhead adicional (10-20%) pero proporciona garantías importantes para sistemas críticos.

---

### At-least-once delivery with idempotency

**Cómo funciona:**
At-least-once delivery con idempotencia permite que mensajes se entreguen múltiples veces pero procesamiento idempotente asegura resultados consistentes.

**Ventajas:**
- Mejor rendimiento que exactly-once
- Garantías de entrega
- Mejor para sistemas críticos
- Balance entre rendimiento y garantías

**Desventajas:**
- Requiere procesamiento idempotente
- Puede requerir más complejidad

**Cuándo usar:**
- Sistemas críticos que requieren garantías
- Cuando se puede implementar idempotencia
- Aplicaciones de alto rendimiento
- Sistemas distribuidos

**Impacto en performance:**
Puede proporcionar mejor rendimiento que exactly-once mientras mantiene garantías importantes mediante idempotencia.

---

### Message queue monitoring and alerting

**Cómo funciona:**
Monitoreo y alertas de message queues identifican problemas (lag, throughput, errores) y alertan cuando hay problemas.

**Ventajas:**
- Identifica problemas temprano
- Mejor visibilidad
- Mejor para sistemas críticos
- Permite acción proactiva

**Desventajas:**
- Requiere infraestructura de monitoreo
- Requiere configuración

**Cuándo usar:**
- Siempre en sistemas de producción
- Sistemas críticos
- Aplicaciones de alto rendimiento
- Cuando se necesita visibilidad

**Impacto en performance:**
No afecta el rendimiento directamente pero permite identificar y resolver problemas que pueden mejorar el rendimiento significativamente.

---

### ZeroMQ for brokerless messaging

**Cómo funciona:**
ZeroMQ proporciona messaging brokerless usando sockets de alto nivel, proporcionando mejor rendimiento que sistemas con broker.

**Ventajas:**
- Sin broker (menor latencia)
- Muy rápido
- Menor overhead
- Mejor para comunicación directa

**Desventajas:**
- Menos features que sistemas con broker
- Requiere gestión de conexiones
- Más complejo

**Cuándo usar:**
- Comunicación directa entre servicios
- Cuando se necesita máximo rendimiento
- Sistemas que no requieren broker
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% comparado con sistemas con broker al eliminar el broker.

---

### ZeroMQ inproc transport for same-process communication

**Cómo funciona:**
ZeroMQ inproc transport permite comunicación entre threads en el mismo proceso sin overhead de red, proporcionando máximo rendimiento.

**Ventajas:**
- Sin overhead de red
- Muy rápido
- Mejor rendimiento
- Mejor para comunicación intra-proceso

**Desventajas:**
- Solo mismo proceso
- Requiere ZeroMQ
- Más complejo

**Cuándo usar:**
- Comunicación entre threads en mismo proceso
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento
- Cuando se puede evitar comunicación de red

**Impacto en performance:**
Puede mejorar el rendimiento en un 100-1000x comparado con comunicación de red. El impacto es dramático.

---

### ZeroMQ message batching (ZMQ_SNDMORE)

**Cómo funciona:**
ZeroMQ message batching permite enviar múltiples mensajes como un batch, reduciendo overhead y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Mejor throughput
- Mejor para muchos mensajes

**Desventajas:**
- Requiere lógica de batching
- Más complejo

**Cuándo usar:**
- Cuando se envían muchos mensajes
- Aplicaciones de alto rendimiento
- Cuando el rendimiento es crítico
- Sistemas de alto volumen

**Impacto en performance:**
Puede mejorar el throughput en un 5-20x al reducir overhead. El impacto es mayor con muchos mensajes.

---

### Message queue backpressure handling

**Cómo funciona:**
Backpressure en message queues controla la tasa de producción cuando el consumidor es más lento, previniendo sobrecarga.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Protege recursos
- Mejor rendimiento general

**Desventajas:**
- Puede limitar el throughput
- Requiere implementación

**Cuándo usar:**
- Sistemas con productores y consumidores
- Cuando hay diferencias de velocidad
- Aplicaciones de alto rendimiento
- Sistemas de streaming

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al prevenir sobrecarga. El impacto es en estabilidad.

---

### Consumer lag monitoring

**Cómo funciona:**
Monitorear consumer lag (diferencia entre mensajes producidos y consumidos) identifica problemas de procesamiento y permite acción proactiva.

**Ventajas:**
- Identifica problemas temprano
- Mejor visibilidad
- Permite acción proactiva
- Mejor para sistemas críticos

**Desventajas:**
- Requiere monitoreo
- Requiere alertas

**Cuándo usar:**
- Siempre en sistemas de producción
- Sistemas críticos
- Aplicaciones de alto rendimiento
- Cuando se necesita visibilidad

**Impacto en performance:**
Permite identificar y resolver problemas que pueden mejorar el rendimiento significativamente.

---

### Message queue partitioning strategies

**Cómo funciona:**
Diferentes estrategias de particionamiento (hash, range, round-robin) distribuyen mensajes de diferentes maneras, cada una con diferentes trade-offs.

**Ventajas:**
- Optimización según necesidades
- Mejor rendimiento
- Mejor distribución
- Flexibilidad

**Desventajas:**
- Requiere conocimiento de estrategias
- Puede requerir configuración

**Cuándo usar:**
- **Hash**: Mejor distribución, difícil mantener orden
- **Range**: Mejor para mantener orden, puede causar hotspots
- **Round-robin**: Mejor distribución simple

**Impacto en performance:**
Elegir la estrategia correcta puede mejorar el rendimiento y distribución según el caso de uso.

---

### Ordered message processing

**Cómo funciona:**
Procesamiento ordenado de mensajes asegura que mensajes se procesen en orden, útil para casos que requieren ordenamiento.

**Ventajas:**
- Mantiene orden
- Mejor para casos que requieren orden
- Mejor consistencia
- Mejor experiencia de usuario

**Desventajas:**
- Puede limitar paralelización
- Puede afectar rendimiento
- Más complejo

**Cuándo usar:**
- Cuando el orden es importante
- Sistemas que requieren ordenamiento
- Aplicaciones que requieren procesamiento ordenado
- Cuando se puede aceptar menor paralelización

**Impacto en performance:**
Puede limitar la paralelización y afectar el rendimiento, pero proporciona ordenamiento crítico para algunos casos.

---

### Message queue compression at transport level

**Cómo funciona:**
Compresión a nivel de transporte comprime mensajes durante la transferencia, reduciendo el tamaño y mejorando el rendimiento de red.

**Ventajas:**
- Reduce tamaño de mensajes
- Mejor rendimiento de red
- Menor uso de ancho de banda
- Mejor para mensajes grandes

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración
- Puede afectar latencia

**Cuándo usar:**
- Mensajes grandes
- Cuando el ancho de banda es limitado
- Sistemas de alto rendimiento
- Cuando se puede aceptar overhead de CPU

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x cuando el ancho de banda es un cuello de botella. Puede degradar si CPU es cuello de botella.

---

## Search Engines

Esta sección cubre optimizaciones para motores de búsqueda como Elasticsearch.

### Elasticsearch bulk API for indexing

**Cómo funciona:**
El bulk API permite indexar múltiples documentos en una sola request, reduciendo el overhead de red y mejorando el throughput.

**Ventajas:**
- Mucho más rápido que indexar individualmente
- Menos overhead de red
- Mejor throughput
- Mejor utilización de recursos

**Desventajas:**
- Requiere batching logic
- Puede aumentar latencia
- Más complejo

**Cuándo usar:**
- Indexación de grandes volúmenes
- Cuando se indexan muchos documentos
- Operaciones de bulk import

**Impacto en performance:**
Puede mejorar el throughput de indexación en un 10-100x comparado con indexar documentos individualmente.

**Ejemplo en C#:**
```csharp
// ✅ Usar bulk API para indexación eficiente
var bulkDescriptor = new BulkDescriptor();
foreach (var document in documents)
{
    bulkDescriptor.Index<MyDocument>(op => op
        .Index("my-index")
        .Document(document));
}
var response = await client.BulkAsync(bulkDescriptor);
```

---

### Elasticsearch index optimization

**Cómo funciona:**
Optimizar índices de Elasticsearch (mapping, settings, sharding) mejora el rendimiento de indexación y búsqueda.

**Ventajas:**
- Mejor rendimiento de indexación
- Mejor rendimiento de búsqueda
- Mejor utilización de recursos
- Optimización según necesidades

**Desventajas:**
- Requiere conocimiento de Elasticsearch
- Requiere tuning
- Puede variar según carga

**Cuándo usar:**
- Sistemas de búsqueda de alto rendimiento
- Cuando el rendimiento de búsqueda es crítico
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la optimización. El impacto depende de la configuración.

---

### Elasticsearch sharding strategies

**Cómo funciona:**
Sharding distribuye datos entre múltiples shards, permitiendo procesamiento paralelo y mejor escalabilidad.

**Ventajas:**
- Procesamiento paralelo
- Mejor escalabilidad
- Mejor throughput
- Mejor utilización de recursos

**Desventajas:**
- Requiere diseño cuidadoso
- Puede causar desbalanceo
- Más complejo

**Cuándo usar:**
- Índices grandes
- Cuando se necesita escalabilidad
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput en un 10-100x al permitir procesamiento paralelo. El impacto depende del número de shards.

---

### Elasticsearch replica configuration

**Cómo funciona:**
Replicas proporcionan copias de datos para alta disponibilidad y mejor rendimiento de lectura.

**Ventajas:**
- Alta disponibilidad
- Mejor rendimiento de lectura
- Tolerancia a fallos
- Mejor escalabilidad de lectura

**Desventajas:**
- Usa más almacenamiento
- Overhead de replicación
- Requiere más recursos

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Sistemas con muchas lecturas

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un Nx donde N es el número de replicas (hasta cierto punto).

---

### Elasticsearch query optimization

**Cómo funciona:**
Optimizar queries de Elasticsearch (usar filtros, evitar scripts, usar índices apropiados) mejora el rendimiento de búsqueda.

**Ventajas:**
- Mejor rendimiento de búsqueda
- Menor uso de CPU
- Mejor experiencia de usuario
- Optimización según necesidades

**Desventajas:**
- Requiere conocimiento de Elasticsearch
- Requiere tuning
- Puede variar según queries

**Cuándo usar:**
- Cuando el rendimiento de búsqueda es crítico
- Queries frecuentes
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x según la optimización. El impacto es dramático con queries mal optimizadas.

---

### Apache Solr for search

**Cómo funciona:**
Apache Solr es una plataforma de búsqueda basada en Lucene que proporciona búsqueda full-text y features avanzadas.

**Ventajas:**
- Búsqueda full-text potente
- Features avanzadas
- Buen rendimiento
- Fácil de usar

**Desventajas:**
- Menor throughput que Elasticsearch en algunos casos
- Requiere infraestructura

**Cuándo usar:**
- Búsqueda full-text
- Cuando se necesitan features avanzadas
- Sistemas de búsqueda

**Impacto en performance:**
Puede manejar miles de queries por segundo con latencia baja. Mejor para búsqueda que para analytics.

---

### Search index warming

**Cómo funciona:**
Index warming pre-carga índices en memoria antes de que se necesiten, mejorando el rendimiento inicial.

**Ventajas:**
- Mejor rendimiento inicial
- Reduce latencia de primera búsqueda
- Mejor experiencia de usuario

**Desventajas:**
- Requiere recursos iniciales
- Puede cargar índices no usados

**Cuándo usar:**
- Al iniciar la aplicación
- Después de reinicios
- Índices críticos

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al tener índices ya en memoria.

---

### Search result caching

**Cómo funciona:**
Cachear resultados de búsqueda evita re-ejecutar queries frecuentes, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Reduce carga en el motor de búsqueda
- Mejor experiencia de usuario

**Desventajas:**
- Puede tener resultados obsoletos
- Requiere invalidación

**Cuándo usar:**
- Queries frecuentes
- Cuando los resultados no cambian frecuentemente
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para queries cacheadas comparado con ejecutar la query.

---

### Faceted search optimization

**Cómo funciona:**
Optimizar faceted search (agregaciones, filtros) mejora el rendimiento de búsquedas con facetas.

**Ventajas:**
- Mejor rendimiento de faceted search
- Menor uso de CPU
- Mejor experiencia de usuario

**Desventajas:**
- Requiere conocimiento de Elasticsearch/Solr
- Requiere tuning

**Cuándo usar:**
- Sistemas con faceted search
- Cuando el rendimiento es crítico
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% según la optimización.

---

### Search relevance tuning

**Cómo funciona:**
Ajustar la relevancia de búsqueda (boost, scoring) mejora la calidad de resultados sin necesariamente afectar el rendimiento directamente.

**Ventajas:**
- Mejor calidad de resultados
- Mejor experiencia de usuario
- Optimización según necesidades

**Desventajas:**
- Requiere conocimiento
- Requiere tuning
- Puede afectar rendimiento si se hace mal

**Cuándo usar:**
- Cuando la calidad de resultados es importante
- Sistemas de búsqueda
- Después de análisis de resultados

**Impacto en performance:**
Generalmente no afecta el rendimiento directamente pero puede mejorar la experiencia de usuario significativamente.

---

### Avoid livelocks

**Cómo funciona:**
Livelocks ocurren cuando threads cambian de estado continuamente sin progresar, similar a deadlocks pero con actividad constante.

**Ventajas de evitarlo:**
- Permite progreso real
- Mejor rendimiento
- Mejor utilización de recursos
- Sistema más estable

**Cuándo evitar:**
- Siempre evitar livelocks
- Sistemas con retry logic
- Cuando hay competencia por recursos
- Aplicaciones multi-threaded

**Impacto en performance:**
Evitar livelocks previene degradación del rendimiento que puede hacer que el sistema no progrese. El impacto es crítico.

**Técnicas:**
- Usar exponential backoff en retries
- Agregar jitter a retries
- Limitar número de retries
- Usar timeouts

---

### Work-stealing schedulers

**Cómo funciona:**
Work-stealing schedulers permiten que threads idle tomen trabajo de otros threads ocupados, mejorando la utilización de recursos.

**Ventajas:**
- Mejor utilización de recursos
- Mejor balanceo de carga
- Mejor rendimiento
- Automático en .NET

**Desventajas:**
- Puede causar overhead de sincronización
- Puede afectar localidad de caché

**Cuándo usar:**
- Siempre en .NET (automático)
- Aplicaciones con trabajo desbalanceado
- Sistemas de alto rendimiento
- Cuando se necesita mejor utilización

**Impacto en performance:**
Puede mejorar la utilización de recursos en un 10-30% al balancear mejor la carga. El impacto es mayor con trabajo desbalanceado.

**Nota:** .NET Task Scheduler usa work-stealing automáticamente. No requiere configuración adicional.

---

### Avoid blocking calls

**Cómo funciona:**
Evitar llamadas bloqueantes permite que threads se liberen para otras operaciones, mejorando el throughput y escalabilidad.

**Ventajas:**
- Mejor escalabilidad
- Mejor throughput
- Mejor utilización de recursos
- No bloquea threads

**Desventajas:**
- Requiere async/await
- Puede ser más complejo

**Cuándo usar:**
- Siempre cuando sea posible
- Operaciones de I/O
- Aplicaciones de servidor
- Cuando se necesita alto throughput

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir más operaciones concurrentes. El impacto es mayor con muchas operaciones.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Bloquear
var result = httpClient.GetStringAsync(url).Result; // Bloquea

// ✅ Bueno: No bloquear
var result = await httpClient.GetStringAsync(url); // No bloquea
```

---

### Avoid sync-over-async

**Cómo funciona:**
Sync-over-async (usar .Result, .Wait() en código async) puede causar deadlocks y degradar el rendimiento significativamente.

**Ventajas de evitarlo:**
- Evita deadlocks
- Mejor escalabilidad
- Mejor rendimiento
- Mejor para aplicaciones async

**Cuándo evitar:**
- Siempre evitar sync-over-async
- Usar async/await correctamente
- En aplicaciones de servidor especialmente
- Cuando se necesita alto throughput

**Impacto en performance:**
Evitar sync-over-async puede prevenir deadlocks y mejorar el throughput en un 2-10x. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Sync-over-async
public string GetData()
{
    return GetDataAsync().Result; // Puede causar deadlock
}

// ✅ Bueno: Async todo el stack
public async Task<string> GetDataAsync()
{
    return await GetDataAsync(); // Correcto
}
```

---

### Producer consumer patterns

**Cómo funciona:**
Patrones producer-consumer separan producción y consumo de datos, permitiendo mejor paralelización y balanceo de carga.

**Ventajas:**
- Mejor paralelización
- Mejor balanceo de carga
- Mejor rendimiento
- Mejor para pipelines de procesamiento

**Desventajas:**
- Más complejo
- Requiere sincronización
- Puede requerir buffering

**Cuándo usar:**
- Pipelines de procesamiento
- Cuando producción y consumo tienen diferentes velocidades
- Aplicaciones de alto rendimiento
- Sistemas de procesamiento de datos

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x al permitir mejor paralelización. El impacto es mayor con diferentes velocidades.

**Ejemplo en C#:**
```csharp
// ✅ Usar Channels para producer-consumer
var channel = Channel.CreateBounded<int>(100);
var producer = Produce(channel.Writer);
var consumer = Consume(channel.Reader);
await Task.WhenAll(producer, consumer);
```

---

### Immutable data structures

**Cómo funciona:**
Estructuras de datos inmutables no pueden modificarse después de crearse, eliminando necesidad de locks y mejorando la concurrencia.

**Ventajas:**
- Sin necesidad de locks
- Thread-safe por naturaleza
- Mejor concurrencia
- Mejor para sistemas concurrentes

**Desventajas:**
- Puede requerir más memoria
- Puede ser menos eficiente para algunas operaciones
- Requiere diseño diferente

**Cuándo usar:**
- Sistemas concurrentes
- Cuando se necesita thread-safety
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar inmutabilidad

**Impacto en performance:**
Puede mejorar la concurrencia al eliminar necesidad de locks, mejorando el rendimiento en un 10-100x en sistemas con alta contención.

**Ejemplo en C#:**
```csharp
// ✅ Estructuras inmutables
var immutableList = ImmutableList<int>.Empty.Add(1).Add(2); // Inmutable
var immutableDict = ImmutableDictionary<string, int>.Empty.Add("key", 1); // Inmutable
```

---

### Actor model (Akka, Orleans)

**Cómo funciona:**
Actor model encapsula estado y comportamiento en actores que se comunican mediante mensajes, proporcionando mejor concurrencia y escalabilidad.

**Ventajas:**
- Mejor concurrencia
- Mejor escalabilidad
- Mejor para sistemas distribuidos
- Mejor aislamiento

**Desventajas:**
- Más complejo
- Curva de aprendizaje
- Requiere infraestructura (para sistemas distribuidos)

**Cuándo usar:**
- Sistemas concurrentes complejos
- Sistemas distribuidos
- Cuando se necesita mejor concurrencia
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar la concurrencia y escalabilidad significativamente al proporcionar mejor aislamiento y comunicación.

**Ejemplo en C# (Orleans):**
```csharp
// ✅ Actor en Orleans
public interface IUserGrain : IGrainWithIntegerKey
{
    Task<User> GetUserAsync();
    Task UpdateUserAsync(User user);
}

public class UserGrain : Grain, IUserGrain
{
    private User _user;
    
    public Task<User> GetUserAsync() => Task.FromResult(_user);
    public Task UpdateUserAsync(User user)
    {
        _user = user;
        return Task.CompletedTask;
    }
}
```

---

### Reactive streams

**Cómo funciona:**
Reactive streams proporcionan un estándar para procesamiento asíncrono de streams con backpressure, mejorando el rendimiento y control.

**Ventajas:**
- Backpressure automático
- Mejor control de flujo
- Mejor rendimiento
- Mejor para streams

**Desventajas:**
- Más complejo
- Curva de aprendizaje
- Requiere bibliotecas (Rx.NET, etc.)

**Cuándo usar:**
- Procesamiento de streams
- Cuando se necesita backpressure
- Aplicaciones de alto rendimiento
- Sistemas de procesamiento de eventos

**Impacto en performance:**
Puede mejorar el rendimiento y control de streams al proporcionar backpressure automático y mejor gestión de flujo.

**Ejemplo en C# (Rx.NET):**
```csharp
// ✅ Reactive streams con Rx.NET
var observable = Observable.Range(1, 100)
    .Where(x => x % 2 == 0)
    .Select(x => x * 2)
    .Buffer(10)
    .Subscribe(
        batch => ProcessBatch(batch),
        error => HandleError(error),
        () => Console.WriteLine("Completed"));
```

---

### Backpressure handling

**Cómo funciona:**
Backpressure controla la tasa de producción cuando el consumidor es más lento, previniendo sobrecarga y mejorando la estabilidad.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Protege recursos
- Mejor rendimiento general

**Desventajas:**
- Puede limitar el throughput
- Requiere implementación

**Cuándo usar:**
- Sistemas con productores y consumidores
- Cuando hay diferencias de velocidad
- Aplicaciones de alto rendimiento
- Sistemas de streaming

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al prevenir sobrecarga. El impacto es en estabilidad.

**Ejemplo en C#:**
```csharp
// ✅ Backpressure con Channels
var channel = Channel.CreateBounded<int>(100); // Límite de capacidad
// El productor espera cuando el canal está lleno
```

---

## Data Structures

Esta sección cubre la elección de estructuras de datos para optimizar el rendimiento.

### Choose data structures based on access patterns

**Cómo funciona:**
Diferentes estructuras de datos tienen diferentes características de rendimiento. Elegir la correcta según el patrón de acceso es crítico.

**Ventajas:**
- Mejor rendimiento
- Menor uso de memoria
- Código más eficiente

**Desventajas:**
- Requiere conocimiento de estructuras de datos
- Puede requerir cambios en el diseño

**Cuándo usar:**
- Siempre al diseñar estructuras de datos
- Hot paths con acceso frecuente
- Cuando el rendimiento es crítico

**Impacto en performance:**
Elegir la estructura correcta puede mejorar el rendimiento en un 10-1000x. El impacto es dramático cuando se usa la estructura incorrecta.

**Ejemplo en C#:**
```csharp
// ✅ Elegir estructura según patrón de acceso
// Búsqueda frecuente: Dictionary/HashSet (O(1))
var lookup = new Dictionary<string, Item>();

// Iteración frecuente: List/Array (mejor localidad)
var items = new List<Item>();

// Inserción/eliminación frecuente: LinkedList (O(1))
var queue = new LinkedList<Item>();

// Ordenamiento necesario: SortedSet/SortedDictionary
var sorted = new SortedSet<Item>();
```

---

### Prefer arrays over lists when size is fixed

**Cómo funciona:**
Los arrays tienen menos overhead que List<T> cuando el tamaño es conocido, proporcionando mejor rendimiento y menos allocations.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Menos allocations
- Mejor localidad de caché

**Desventajas:**
- Tamaño fijo
- Menos flexible

**Cuándo usar:**
- Cuando el tamaño es conocido en compile-time
- Hot paths con colecciones de tamaño fijo
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% y eliminar allocations para colecciones de tamaño fijo.

**Ejemplo en C#:**
```csharp
// ❌ Malo: List cuando el tamaño es fijo
var items = new List<int>(10); // Overhead innecesario

// ✅ Bueno: Array cuando el tamaño es fijo
var items = new int[10]; // Menos overhead
```

---

### Prefer array-backed collections over linked lists

**Cómo funciona:**
Colecciones basadas en arrays (List<T>, Array) tienen mejor localidad de caché y rendimiento que linked lists para la mayoría de casos.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Menos allocations
- Mejor para iteración

**Desventajas:**
- Inserción/eliminación más costosa en medio
- Tamaño fijo para arrays

**Cuándo usar:**
- Siempre cuando sea posible
- Cuando se itera frecuentemente
- Hot paths con colecciones

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al tener mejor localidad de caché. El impacto es mayor con iteraciones frecuentes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: LinkedList para iteración frecuente
var list = new LinkedList<int>(); // Peor localidad de caché

// ✅ Bueno: List para iteración frecuente
var list = new List<int>(); // Mejor localidad de caché
```

---

### Use hash maps for constant-time lookups

**Cómo funciona:**
Hash maps (Dictionary<TKey, TValue>, HashSet<T>) proporcionan búsqueda O(1) promedio, mucho más rápido que búsqueda lineal.

**Ventajas:**
- Búsqueda O(1) promedio
- Muy rápido
- Mejor para lookups frecuentes

**Desventajas:**
- No mantiene orden
- Overhead de hash
- Puede tener colisiones

**Cuándo usar:**
- Cuando se necesita búsqueda frecuente
- Lookups por clave
- Cuando el orden no es importante

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para lookups comparado con búsqueda lineal. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Búsqueda lineal O(n)
var item = items.FirstOrDefault(i => i.Id == id); // O(n)

// ✅ Bueno: Hash map O(1)
var dict = items.ToDictionary(i => i.Id);
var item = dict[id]; // O(1)
```

---

### Pre-size hash tables

**Cómo funciona:**
Pre-dimensionar hash tables evita rehashing y mejora el rendimiento al evitar redimensionamiento.

**Ventajas:**
- Evita rehashing
- Mejor rendimiento
- Menos allocations
- Mejor para tamaños conocidos

**Desventajas:**
- Requiere conocer tamaño aproximado
- Puede desperdiciar espacio

**Cuándo usar:**
- Cuando se conoce el tamaño aproximado
- Hot paths con hash tables
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al evitar rehashing. El impacto es mayor con muchos elementos.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Sin pre-dimensionar
var dict = new Dictionary<int, string>(); // Rehashing cuando crece

// ✅ Bueno: Pre-dimensionar
var dict = new Dictionary<int, string>(expectedSize); // Sin rehashing
```

---

### Avoid hash collisions

**Cómo funciona:**
Colisiones de hash degradan el rendimiento al requerir búsqueda lineal en buckets. Usar buenas funciones de hash y claves apropiadas reduce colisiones.

**Ventajas:**
- Mejor rendimiento
- Búsqueda O(1) real
- Menos overhead

**Desventajas:**
- Requiere buenas funciones de hash
- Puede requerir cambios en claves

**Cuándo usar:**
- Siempre cuando sea posible
- Hot paths con hash tables
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x cuando hay muchas colisiones. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ .NET usa buenas funciones de hash automáticamente
// Para tipos personalizados, implementar GetHashCode() apropiadamente
public class MyKey
{
    public int Id { get; set; }
    public string Name { get; set; }
    
    public override int GetHashCode()
    {
        return HashCode.Combine(Id, Name); // Buena función de hash
    }
}
```

---

### Use struct keys instead of string keys

**Cómo funciona:**
Usar structs como claves en hash tables puede ser más rápido que strings porque las comparaciones y hash son más eficientes.

**Ventajas:**
- Mejor rendimiento
- Menos allocations
- Comparaciones más rápidas

**Desventajas:**
- Menos flexible
- Requiere cambios en el diseño

**Cuándo usar:**
- Hot paths con hash tables
- Cuando se puede usar structs
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al tener comparaciones y hash más eficientes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: String keys
var dict = new Dictionary<string, int>(); // Comparación de strings más lenta

// ✅ Bueno: Struct keys
public struct MyKey { public int Id; public int Type; }
var dict = new Dictionary<MyKey, int>(); // Comparación más rápida
```

---

### Custom equality comparers

**Cómo funciona:**
Comparadores de igualdad personalizados permiten optimizar comparaciones y hash para casos específicos.

**Ventajas:**
- Optimización específica
- Mejor rendimiento
- Control sobre comparaciones

**Desventajas:**
- Más complejo
- Requiere implementación

**Cuándo usar:**
- Cuando se necesita optimización específica
- Hot paths con comparaciones
- Cuando el comparador por defecto no es óptimo

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% según la optimización.

**Ejemplo en C#:**
```csharp
// ✅ Comparador personalizado
public class FastStringComparer : IEqualityComparer<string>
{
    public bool Equals(string x, string y) => x == y;
    public int GetHashCode(string obj) => obj.GetHashCode();
}

var dict = new Dictionary<string, int>(new FastStringComparer());
```

---

### Trees versus hash tables tradeoffs

**Cómo funciona:**
Trees (SortedDictionary, SortedSet) mantienen orden pero son más lentos que hash tables. Hash tables son más rápidos pero no mantienen orden.

**Ventajas:**
- Elección apropiada según necesidades
- Mejor rendimiento
- Optimización según caso de uso

**Desventajas:**
- Requiere conocimiento de estructuras
- Puede requerir cambios

**Cuándo usar:**
- Hash tables: cuando no se necesita orden
- Trees: cuando se necesita orden o range queries
- Según caso de uso específico

**Impacto en performance:**
Elegir la estructura correcta puede mejorar el rendimiento en un 10-100x. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Hash table cuando no se necesita orden
var dict = new Dictionary<int, string>(); // O(1) lookup

// ✅ Tree cuando se necesita orden
var sorted = new SortedDictionary<int, string>(); // O(log n) lookup, ordenado
```

---

### Balanced trees like AVL or Red-Black

**Cómo funciona:**
Árboles balanceados mantienen altura O(log n) garantizando búsqueda, inserción y eliminación en O(log n).

**Ventajas:**
- Búsqueda O(log n) garantizada
- Mantiene orden
- Buen rendimiento

**Desventajas:**
- Más lento que hash tables
- Más complejo
- Overhead de balanceo

**Cuándo usar:**
- Cuando se necesita orden y búsqueda
- Range queries
- Cuando se necesita garantías de rendimiento

**Impacto en performance:**
Proporciona búsqueda O(log n) garantizada. Mejor que O(n) pero peor que O(1) de hash tables.

**Nota:** En .NET, SortedDictionary usa Red-Black tree internamente.

---

### B-trees and B-plus trees

**Cómo funciona:**
B-trees y B-plus trees son árboles balanceados optimizados para I/O de disco, usados en bases de datos.

**Ventajas:**
- Optimizado para I/O
- Mejor para bases de datos
- Búsqueda eficiente

**Desventajas:**
- Más complejo
- Overhead adicional
- Principalmente para bases de datos

**Cuándo usar:**
- Bases de datos
- Sistemas con mucho I/O
- Cuando se necesita búsqueda eficiente en disco

**Impacto en performance:**
Optimizado para I/O de disco, mejor que árboles normales para bases de datos.

**Nota:** Bases de datos como SQL Server, PostgreSQL usan B-trees/B-plus trees para índices.

---

### Tries and prefix trees

**Cómo funciona:**
Tries almacenan strings de manera que permite búsqueda por prefijo eficiente, útil para autocomplete y búsqueda de texto.

**Ventajas:**
- Búsqueda por prefijo eficiente
- Mejor para strings
- Útil para autocomplete

**Desventajas:**
- Usa más memoria
- Más complejo
- Específico para strings

**Cuándo usar:**
- Búsqueda por prefijo
- Autocomplete
- Búsqueda de texto
- Cuando se necesita búsqueda de strings eficiente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para búsqueda por prefijo comparado con búsqueda lineal.

---

### Bloom filters

**Cómo funciona:**
Bloom filters son estructuras probabilísticas que permiten verificar si un elemento está en un conjunto de manera muy eficiente, con posibilidad de falsos positivos pero sin falsos negativos.

**Ventajas:**
- Muy eficiente en memoria
- Búsqueda muy rápida
- Útil para filtrado

**Desventajas:**
- Falsos positivos posibles
- No puede eliminar elementos
- Específico para casos de uso

**Cuándo usar:**
- Filtrado antes de búsqueda costosa
- Cuando se puede aceptar falsos positivos
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al evitar búsquedas costosas para elementos que no están en el conjunto.

---

### Ring buffers

**Cómo funciona:**
Ring buffers (circular buffers) son arrays circulares que permiten inserción y eliminación eficiente en ambos extremos.

**Ventajas:**
- Inserción/eliminación O(1)
- Eficiente en memoria
- Mejor para queues de tamaño fijo

**Desventajas:**
- Tamaño fijo
- Menos flexible

**Cuándo usar:**
- Queues de tamaño fijo
- Buffers circulares
- Cuando se necesita inserción/eliminación eficiente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% comparado con queues dinámicas para casos de tamaño fijo.

---

### Queues and deques

**Cómo funciona:**
Queues (FIFO) y deques (double-ended queues) permiten inserción/eliminación eficiente en uno o ambos extremos.

**Ventajas:**
- Inserción/eliminación eficiente
- Útil para procesamiento en orden
- Mejor para producer-consumer

**Desventajas:**
- Acceso limitado a extremos
- No acceso aleatorio

**Cuándo usar:**
- Producer-consumer patterns
- Procesamiento en orden
- Cuando se necesita FIFO o LIFO

**Impacto en performance:**
Proporciona inserción/eliminación O(1) en extremos, mejor que arrays para estos casos.

**Ejemplo en C#:**
```csharp
// ✅ Queue para FIFO
var queue = new Queue<int>();
queue.Enqueue(1);
var item = queue.Dequeue();

// ✅ Deque para ambos extremos
var deque = new LinkedList<int>(); // O usar biblioteca de deque
```

---

### Priority queues and heaps

**Cómo funciona:**
Priority queues y heaps permiten acceso al elemento de mayor (o menor) prioridad eficientemente.

**Ventajas:**
- Acceso a elemento de mayor prioridad O(1)
- Inserción O(log n)
- Útil para scheduling

**Desventajas:**
- No acceso aleatorio
- Más complejo que queues simples

**Cuándo usar:**
- Scheduling
- Cuando se necesita prioridad
- Algoritmos que requieren elementos de mayor prioridad

**Impacto en performance:**
Proporciona acceso O(1) al elemento de mayor prioridad y inserción O(log n), mejor que ordenar para estos casos.

**Ejemplo en C#:**
```csharp
// ✅ Priority queue (requiere .NET 6+ o biblioteca externa)
var pq = new PriorityQueue<int, int>();
pq.Enqueue(1, 10); // valor, prioridad
var item = pq.Dequeue(); // Obtiene el de mayor prioridad
```

---

### Bitsets and bit arrays

**Cómo funciona:**
Bitsets y bit arrays almacenan bits de manera eficiente, útil para flags y operaciones de bits.

**Ventajas:**
- Muy eficiente en memoria
- Operaciones de bits rápidas
- Mejor para flags

**Desventajas:**
- Limitado a bits
- Menos flexible

**Cuándo usar:**
- Flags
- Operaciones de bits
- Cuando se necesita eficiencia de memoria extrema

**Impacto en performance:**
Puede mejorar el uso de memoria en un 8-32x comparado con bool arrays y proporciona operaciones de bits rápidas.

**Ejemplo en C#:**
```csharp
// ✅ BitArray para flags eficientes
var bits = new BitArray(1000); // 1000 bits, no 1000 bytes
bits[0] = true;
bits[1] = false;
```

---

### Sparse arrays

**Cómo funciona:**
Sparse arrays almacenan solo elementos no nulos, ahorrando memoria cuando hay muchos valores nulos.

**Ventajas:**
- Ahorra memoria
- Mejor para datos sparse
- Eficiente en memoria

**Desventajas:**
- Más complejo
- Overhead adicional
- Acceso más lento

**Cuándo usar:**
- Arrays con muchos valores nulos
- Cuando se necesita ahorrar memoria
- Datos sparse

**Impacto en performance:**
Puede ahorrar memoria significativamente cuando hay muchos valores nulos, aunque el acceso puede ser más lento.

**Ejemplo en C#:**
```csharp
// ✅ Dictionary para sparse array
var sparse = new Dictionary<int, int>(); // Solo almacena valores no nulos
sparse[1000] = 42; // No almacena índices 0-999
```

---

### Avoid pointer-heavy structures

**Cómo funciona:**
Estructuras con muchos punteros tienen peor localidad de caché y requieren más indirecciones, degradando el rendimiento.

**Ventajas:**
- Mejor localidad de caché
- Menos indirecciones
- Mejor rendimiento
- Mejor para iteración

**Desventajas:**
- Puede requerir cambios en el diseño
- Menos flexible

**Cuándo usar:**
- Siempre cuando sea posible
- Hot paths con estructuras
- Cuando se itera frecuentemente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al tener mejor localidad de caché. El impacto es mayor con iteraciones frecuentes.

---

### Flat data structures

**Cómo funciona:**
Estructuras planas (arrays, listas) tienen mejor localidad de caché que estructuras anidadas o con muchos punteros.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Menos indirecciones
- Mejor para iteración

**Desventajas:**
- Puede requerir cambios en el diseño
- Menos flexible

**Cuándo usar:**
- Siempre cuando sea posible
- Hot paths con estructuras
- Cuando se itera frecuentemente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al tener mejor localidad de caché.

---

### Index-based references instead of object references

**Cómo funciona:**
Usar índices en lugar de referencias de objetos puede mejorar la localidad de caché y reducir allocations.

**Ventajas:**
- Mejor localidad de caché
- Menos allocations
- Mejor rendimiento
- Mejor para estructuras grandes

**Desventajas:**
- Menos type-safe
- Requiere gestión de índices
- Más complejo

**Cuándo usar:**
- Estructuras grandes
- Hot paths con muchas referencias
- Cuando se necesita máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al tener mejor localidad de caché y menos allocations.

---

## Algorithms

Esta sección cubre optimizaciones algorítmicas para mejorar el rendimiento.

### Choose optimal Big O complexity

**Cómo funciona:**
La complejidad algorítmica (Big O) determina cómo escala el rendimiento con el tamaño de entrada. Elegir algoritmos con mejor complejidad es fundamental.

**Ventajas:**
- Mejor escalabilidad
- Mejor rendimiento con datos grandes
- Fundamentos sólidos

**Desventajas:**
- Requiere conocimiento de algoritmos
- Puede requerir cambios significativos

**Cuándo usar:**
- Siempre al diseñar algoritmos
- Cuando se procesan grandes volúmenes de datos
- Hot paths con procesamiento de datos

**Impacto en performance:**
Elegir el algoritmo correcto puede mejorar el rendimiento en un 10-10000x dependiendo del tamaño de datos. El impacto es dramático con datos grandes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: O(n²) - búsqueda lineal anidada
public bool ContainsBad(int[] array1, int[] array2)
{
    foreach (var a in array1)
    {
        foreach (var b in array2) // O(n²)
        {
            if (a == b) return true;
        }
    }
    return false;
}

// ✅ Bueno: O(n) - usar HashSet
public bool ContainsGood(int[] array1, int[] array2)
{
    var set = new HashSet<int>(array2); // O(n)
    foreach (var a in array1) // O(n)
    {
        if (set.Contains(a)) return true; // O(1)
    }
    return false;
}
```

---

### Avoid nested loops on large datasets

**Cómo funciona:**
Loops anidados tienen complejidad O(n²) o peor, degradando el rendimiento dramáticamente con datos grandes.

**Ventajas:**
- Mejor escalabilidad
- Mejor rendimiento
- Mejor para datos grandes

**Desventajas:**
- Puede requerir cambios en el algoritmo
- Puede requerir estructuras de datos adicionales

**Cuándo usar:**
- Siempre evitar cuando sea posible
- Cuando se procesan grandes datasets
- Hot paths con loops

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-10000x al reducir complejidad. El impacto es dramático con datos grandes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: O(n²)
for (int i = 0; i < n; i++)
    for (int j = 0; j < n; j++)
        Process(i, j);

// ✅ Bueno: O(n) si es posible
var set = new HashSet<int>(data);
foreach (var item in items)
    if (set.Contains(item)) Process(item);
```

---

### Binary search instead of linear search

**Cómo funciona:**
Binary search tiene complejidad O(log n) comparado con O(n) de búsqueda lineal, mucho más rápido para datos ordenados.

**Ventajas:**
- Mucho más rápido O(log n)
- Mejor escalabilidad
- Mejor para datos grandes

**Desventajas:**
- Requiere datos ordenados
- Más complejo

**Cuándo usar:**
- Datos ordenados
- Búsquedas frecuentes
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para datos grandes comparado con búsqueda lineal.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Búsqueda lineal O(n)
var index = Array.IndexOf(sortedArray, value);

// ✅ Bueno: Binary search O(log n)
var index = Array.BinarySearch(sortedArray, value);
```

---

### Two-pointer technique

**Cómo funciona:**
Two-pointer technique usa dos punteros que se mueven hacia el centro o en la misma dirección, reduciendo complejidad.

**Ventajas:**
- Reduce complejidad
- Mejor rendimiento
- Menos espacio
- Útil para muchos problemas

**Desventajas:**
- Requiere datos ordenados en algunos casos
- Más complejo

**Cuándo usar:**
- Problemas de arrays ordenados
- Búsqueda de pares
- Cuando se puede usar two-pointer

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir complejidad de O(n²) a O(n).

---

### Sliding window

**Cómo funciona:**
Sliding window mantiene una ventana de elementos y la desliza, evitando recalcular todo en cada paso.

**Ventajas:**
- Reduce complejidad
- Mejor rendimiento
- Menos cálculos redundantes

**Desventajas:**
- Requiere diseño cuidadoso
- Más complejo

**Cuándo usar:**
- Problemas de subarrays/substrings
- Cuando se puede usar sliding window
- Optimización de algoritmos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al evitar cálculos redundantes.

---

### Divide and conquer

**Cómo funciona:**
Divide and conquer divide problemas en subproblemas más pequeños, resolviéndolos recursivamente y combinando resultados.

**Ventajas:**
- Reduce complejidad
- Mejor escalabilidad
- Útil para muchos problemas

**Desventajas:**
- Más complejo
- Overhead de recursión

**Cuándo usar:**
- Problemas que se pueden dividir
- Algoritmos como merge sort, quick sort
- Cuando divide and conquer es apropiado

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir complejidad (ej: O(n²) a O(n log n)).

---

### Greedy algorithms

**Cómo funciona:**
Greedy algorithms hacen la elección localmente óptima en cada paso, a menudo proporcionando solución óptima global.

**Ventajas:**
- Generalmente más simple
- Mejor rendimiento
- Útil para muchos problemas

**Desventajas:**
- No siempre óptimo
- Requiere prueba de optimalidad

**Cuándo usar:**
- Cuando greedy es apropiado
- Problemas de optimización
- Cuando se necesita solución rápida

**Impacto en performance:**
Puede mejorar el rendimiento significativamente comparado con algoritmos exhaustivos.

---

### Dynamic programming

**Cómo funciona:**
Dynamic programming almacena resultados de subproblemas para evitar recalcularlos, mejorando el rendimiento.

**Ventajas:**
- Evita recálculo
- Mejor rendimiento
- Reduce complejidad exponencial a polinomial

**Desventajas:**
- Usa más memoria
- Más complejo

**Cuándo usar:**
- Problemas con subproblemas superpuestos
- Cuando se puede usar memoization
- Optimización de algoritmos recursivos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-10000x al evitar recálculo. El impacto es dramático.

---

### Memoization

**Cómo funciona:**
Memoization cachea resultados de funciones para evitar recalcularlos con los mismos parámetros.

**Ventajas:**
- Evita recálculo
- Mejor rendimiento
- Reduce complejidad

**Desventajas:**
- Usa más memoria
- Requiere gestión de cache

**Cuándo usar:**
- Funciones con parámetros repetidos
- Funciones costosas
- Cuando se puede cachear resultados

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al evitar recálculo. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Memoization
private readonly Dictionary<int, long> _cache = new();

public long Fibonacci(int n)
{
    if (_cache.TryGetValue(n, out var result))
        return result;
    
    result = n <= 1 ? n : Fibonacci(n - 1) + Fibonacci(n - 2);
    _cache[n] = result;
    return result;
}
```

---

### Prefer iterative over recursive solutions

**Cómo funciona:**
Soluciones iterativas evitan overhead de llamadas de función y stack, mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Menos uso de stack
- Sin riesgo de stack overflow

**Desventajas:**
- Puede ser menos elegante
- Más complejo en algunos casos

**Cuándo usar:**
- Siempre cuando sea posible
- Hot paths con recursión
- Cuando se puede convertir a iterativo

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% y evitar stack overflow. El impacto es mayor con recursión profunda.

---

### Batch processing

**Cómo funciona:**
Procesar datos en batches en lugar de uno por uno reduce overhead y mejora el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Mejor utilización de recursos
- Mejor throughput

**Desventajas:**
- Puede aumentar latencia del primer item
- Requiere gestión de batches

**Cuándo usar:**
- Cuando se procesan muchos items
- Operaciones que se pueden agrupar
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x al reducir overhead. El impacto es mayor con muchos items.

---

### Parallel algorithms

**Cómo funciona:**
Algoritmos paralelos procesan datos en múltiples threads/cores simultáneamente, mejorando el throughput.

**Ventajas:**
- Mejor throughput
- Mejor utilización de múltiples cores
- Mejor escalabilidad

**Desventajas:**
- Más complejo
- Requiere sincronización
- Puede tener overhead

**Cuándo usar:**
- Datos grandes
- Operaciones independientes
- Cuando se tienen múltiples cores
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput en un Nx donde N es el número de cores (hasta cierto punto).

**Ejemplo en C#:**
```csharp
// ✅ Parallel processing
var results = items.AsParallel()
    .Select(ProcessItem)
    .ToList();
```

---

### SIMD-friendly algorithms

**Cómo funciona:**
Algoritmos SIMD-friendly procesan múltiples elementos simultáneamente usando instrucciones SIMD del CPU.

**Ventajas:**
- Procesamiento paralelo a nivel de CPU
- Mejor rendimiento
- Mejor utilización de CPU

**Desventajas:**
- Requiere datos alineados
- Más complejo
- Específico de CPU

**Cuándo usar:**
- Operaciones vectoriales
- Procesamiento de arrays
- Cuando se puede usar SIMD

**Impacto en performance:**
Puede mejorar el rendimiento en un 4-16x al procesar múltiples elementos simultáneamente.

**Nota:** En .NET, System.Numerics proporciona tipos vectoriales que pueden usar SIMD automáticamente.

---

### Avoid recomputation

**Cómo funciona:**
Evitar recalcular valores que ya se han calculado mejora el rendimiento al reutilizar resultados.

**Ventajas:**
- Evita recálculo
- Mejor rendimiento
- Menos uso de CPU

**Desventajas:**
- Puede requerir almacenamiento
- Requiere gestión

**Cuándo usar:**
- Siempre cuando sea posible
- Cálculos costosos
- Valores que se usan múltiples veces

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al evitar recálculo. El impacto es dramático.

---

### Early exits

**Cómo funciona:**
Salir temprano de loops o funciones cuando se encuentra la respuesta evita procesamiento innecesario.

**Ventajas:**
- Evita procesamiento innecesario
- Mejor rendimiento
- Menos uso de CPU

**Desventajas:**
- Puede requerir cambios en el código
- Menos elegante en algunos casos

**Cuándo usar:**
- Siempre cuando sea posible
- Loops con condiciones de salida
- Funciones que pueden retornar temprano

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x cuando se puede salir temprano. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Procesa todo
foreach (var item in items)
{
    if (item.IsValid) Process(item);
}

// ✅ Bueno: Early exit
foreach (var item in items)
{
    if (!item.IsValid) continue; // Early exit
    Process(item);
    if (found) break; // Early exit cuando se encuentra
}
```

---

### Short-circuit logic

**Cómo funciona:**
Short-circuit logic evalúa solo lo necesario (&&, ||), evitando evaluación innecesaria.

**Ventajas:**
- Evita evaluación innecesaria
- Mejor rendimiento
- Menos uso de CPU

**Desventajas:**
- Requiere cuidado con efectos secundarios
- Puede ser menos obvio

**Cuándo usar:**
- Siempre cuando sea posible
- Condiciones con &&, ||
- Cuando la evaluación es costosa

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x cuando se evita evaluación costosa. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Short-circuit: si IsValid es false, no evalúa ExpensiveCheck
if (item.IsValid && ExpensiveCheck(item))
{
    Process(item);
}
```

---

## System Design

Esta sección cubre principios de diseño de sistemas para alto rendimiento.

### Stateless services

**Cómo funciona:**
Servicios stateless no mantienen estado entre requests, permitiendo mejor escalabilidad horizontal y load balancing.

**Ventajas:**
- Mejor escalabilidad
- Mejor load balancing
- Más fácil de escalar horizontalmente
- Mejor resiliencia

**Desventajas:**
- Requiere almacenar estado externamente
- Puede requerir más I/O

**Cuándo usar:**
- Aplicaciones web y APIs
- Microservicios
- Sistemas que necesitan escalar
- Aplicaciones distribuidas

**Impacto en performance:**
Permite escalabilidad horizontal ilimitada, mejorando el throughput total del sistema. El impacto puede ser 10-100x en capacidad total.

---

### Load balancing

**Cómo funciona:**
Load balancing distribuye requests entre múltiples instancias de un servicio, mejorando el throughput y disponibilidad.

**Ventajas:**
- Mejor throughput total
- Mejor disponibilidad
- Mejor utilización de recursos
- Escalabilidad horizontal

**Desventajas:**
- Requiere infraestructura
- Puede introducir latencia
- Requiere gestión

**Cuándo usar:**
- Aplicaciones de alto tráfico
- Sistemas que necesitan alta disponibilidad
- Cuando una instancia no es suficiente
- Aplicaciones distribuidas

**Impacto en performance:**
Puede mejorar el throughput total en un Nx donde N es el número de instancias (hasta el límite de la base de datos u otros recursos compartidos).

---

### Minimize network hops

**Cómo funciona:**
Reducir el número de saltos de red entre servicios reduce latencia y mejora el rendimiento.

**Ventajas:**
- Menor latencia
- Mejor rendimiento
- Menos puntos de fallo
- Mejor experiencia de usuario

**Desventajas:**
- Puede requerir cambios en arquitectura
- Puede requerir consolidación

**Cuándo usar:**
- Siempre cuando sea posible
- Aplicaciones distribuidas
- Cuando la latencia es crítica
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar la latencia en un 20-50% por cada hop eliminado. El impacto es mayor con alta latencia de red.

---

### Avoid chatty service communication

**Cómo funciona:**
Evitar comunicación chatty entre servicios (muchos requests pequeños) reduce overhead de red y mejora el rendimiento.

**Ventajas:**
- Menos overhead de red
- Mejor rendimiento
- Menor latencia total
- Mejor throughput

**Desventajas:**
- Puede requerir cambios en APIs
- Menos granularidad

**Cuándo usar:**
- Siempre cuando sea posible
- Comunicación entre servicios
- Aplicaciones distribuidas
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al reducir el número de requests. El impacto es dramático con alta latencia.

---

### Prefer asynchronous communication

**Cómo funciona:**
Comunicación asíncrona (message queues, eventos) permite mejor desacoplamiento y mejor rendimiento que comunicación síncrona.

**Ventajas:**
- Mejor desacoplamiento
- Mejor rendimiento
- Mejor escalabilidad
- Mejor resiliencia

**Desventajas:**
- Más complejo
- Consistencia eventual
- Requiere gestión de mensajes

**Cuándo usar:**
- Cuando se puede aceptar consistencia eventual
- Sistemas distribuidos
- Cuando se necesita mejor escalabilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput en un 2-10x al permitir mejor paralelización y desacoplamiento.

---

### Event-driven architecture

**Cómo funciona:**
Arquitectura event-driven usa eventos para comunicación entre servicios, proporcionando mejor desacoplamiento y escalabilidad.

**Ventajas:**
- Mejor desacoplamiento
- Mejor escalabilidad
- Mejor resiliencia
- Mejor para sistemas complejos

**Desventajas:**
- Más complejo
- Consistencia eventual
- Requiere gestión de eventos

**Cuándo usar:**
- Sistemas distribuidos complejos
- Cuando se necesita mejor desacoplamiento
- Sistemas que requieren escalabilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el throughput y escalabilidad significativamente al permitir mejor paralelización.

---

### Design for failure

**Cómo funciona:**
Diseñar sistemas para manejar fallos (timeouts, retries, circuit breakers) mejora la resiliencia y estabilidad.

**Ventajas:**
- Mejor resiliencia
- Mejor estabilidad
- Previene cascading failures
- Mejor experiencia de usuario

**Desventajas:**
- Más complejo
- Requiere implementación

**Cuándo usar:**
- Siempre en sistemas distribuidos
- Sistemas críticos
- Cuando la disponibilidad es importante
- Aplicaciones de producción

**Impacto en performance:**
Previene degradación del rendimiento del 50-100% cuando hay fallos. El impacto es en estabilidad.

---

### Apply backpressure everywhere

**Cómo funciona:**
Aplicar backpressure en todos los niveles previene sobrecarga y mejora la estabilidad del sistema.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Protege recursos
- Mejor rendimiento general

**Desventajas:**
- Puede limitar el throughput
- Requiere implementación

**Cuándo usar:**
- Sistemas con productores y consumidores
- Cuando hay diferencias de velocidad
- Aplicaciones de alto rendimiento
- Sistemas de streaming

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al prevenir sobrecarga. El impacto es en estabilidad.

---

### Idempotent operations

**Cómo funciona:**
Operaciones idempotentes pueden ejecutarse múltiples veces sin efectos secundarios, permitiendo retries seguros.

**Ventajas:**
- Permite retries seguros
- Mejor resiliencia
- Mejor para sistemas distribuidos
- Previene problemas de duplicación

**Desventajas:**
- Requiere diseño cuidadoso
- Puede ser más complejo

**Cuándo usar:**
- Siempre cuando sea posible
- Operaciones que pueden fallar
- Sistemas distribuidos
- Cuando se necesita resiliencia

**Impacto en performance:**
Permite retries que pueden mejorar la confiabilidad sin afectar el rendimiento directamente.

---

### Graceful degradation

**Cómo funciona:**
Degradación graceful proporciona funcionalidad reducida cuando hay problemas, manteniendo el servicio disponible.

**Ventajas:**
- Mantiene servicio disponible
- Mejor experiencia de usuario
- Previene fallos completos
- Mejor resiliencia

**Desventajas:**
- Requiere diseño cuidadoso
- Puede requerir funcionalidad reducida

**Cuándo usar:**
- Sistemas críticos
- Cuando la disponibilidad es importante
- Sistemas que pueden tener problemas
- Aplicaciones de producción

**Impacto en performance:**
Mantiene el servicio disponible incluso cuando hay problemas, mejorando la experiencia de usuario.

---

### Rate limiting

**Cómo funciona:**
Rate limiting controla la tasa de requests, previniendo sobrecarga y mejorando la estabilidad.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Protege recursos
- Mejor experiencia de usuario

**Desventajas:**
- Puede rechazar requests válidos
- Requiere configuración

**Cuándo usar:**
- APIs públicas
- Sistemas con límites de recursos
- Cuando se necesita proteger backends
- Aplicaciones que consumen APIs externas

**Impacto en performance:**
Puede prevenir degradación del rendimiento al limitar la carga. El impacto es en estabilidad.

---

### Traffic shaping

**Cómo funciona:**
Traffic shaping controla la tasa de tráfico, suavizando picos y mejorando la estabilidad.

**Ventajas:**
- Suaviza picos
- Mejor estabilidad
- Mejor utilización de recursos
- Previene sobrecarga

**Desventajas:**
- Puede aumentar latencia
- Requiere configuración

**Cuándo usar:**
- Sistemas con picos de tráfico
- Cuando se necesita suavizar carga
- Aplicaciones de alto rendimiento
- Sistemas que requieren estabilidad

**Impacto en performance:**
Puede prevenir degradación del rendimiento al suavizar picos. El impacto es en estabilidad.

---

### Circuit breakers

**Cómo funciona:**
Circuit breakers previenen llamadas a servicios que están fallando, mejorando la resiliencia y reduciendo latencia.

**Ventajas:**
- Previene llamadas a servicios fallando
- Mejor resiliencia
- Reduce latencia de fallos
- Mejor experiencia de usuario

**Desventajas:**
- Requiere implementación
- Puede causar indisponibilidad temporal

**Cuándo usar:**
- Sistemas distribuidos
- Cuando hay servicios externos
- Sistemas que requieren resiliencia
- Aplicaciones de producción

**Impacto en performance:**
Puede reducir la latencia de fallos en un 50-90% al evitar esperas en servicios fallando.

**Ejemplo en C#:**
```csharp
// ✅ Usar Polly para circuit breakers
var policy = Policy
    .Handle<HttpRequestException>()
    .CircuitBreakerAsync(
        handledEventsAllowedBeforeBreaking: 5,
        durationOfBreak: TimeSpan.FromSeconds(30));
    
await policy.ExecuteAsync(() => httpClient.GetAsync(url));
```

---

### Retry with exponential backoff

**Cómo funciona:**
Retry con exponential backoff reintenta operaciones fallidas con delays crecientes, mejorando la probabilidad de éxito.

**Ventajas:**
- Mejor probabilidad de éxito
- Reduce carga en servicios
- Mejor para fallos temporales
- Mejor resiliencia

**Desventajas:**
- Aumenta latencia en fallos
- Requiere implementación

**Cuándo usar:**
- Operaciones que pueden fallar temporalmente
- Sistemas distribuidos
- Cuando se necesita resiliencia
- Aplicaciones de producción

**Impacto en performance:**
Puede mejorar la tasa de éxito en un 50-90% para fallos temporales sin sobrecargar servicios.

**Ejemplo en C#:**
```csharp
// ✅ Usar Polly para retry con exponential backoff
var policy = Policy
    .Handle<HttpRequestException>()
    .WaitAndRetryAsync(
        retryCount: 3,
        sleepDurationProvider: retryAttempt => TimeSpan.FromSeconds(Math.Pow(2, retryAttempt)));
    
await policy.ExecuteAsync(() => httpClient.GetAsync(url));
```

---

### CQRS

**Cómo funciona:**
CQRS (Command Query Responsibility Segregation) separa comandos (escrituras) de queries (lecturas), permitiendo optimización independiente.

**Ventajas:**
- Optimización independiente
- Mejor escalabilidad
- Mejor rendimiento
- Mejor para sistemas complejos

**Desventajas:**
- Más complejo
- Requiere más infraestructura
- Consistencia eventual

**Cuándo usar:**
- Sistemas con diferentes patrones de lectura/escritura
- Cuando se necesita optimizar lecturas y escrituras independientemente
- Sistemas complejos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al optimizar lecturas y escrituras independientemente.

---

### Event sourcing

**Cómo funciona:**
Event sourcing almacena eventos en lugar del estado actual, permitiendo replay y mejor auditoría.

**Ventajas:**
- Permite replay
- Mejor auditoría
- Mejor para sistemas complejos
- Permite time travel

**Desventajas:**
- Más complejo
- Requiere más almacenamiento
- Consistencia eventual

**Cuándo usar:**
- Sistemas que requieren auditoría
- Cuando se necesita replay
- Sistemas complejos
- Cuando se necesita time travel

**Impacto en performance:**
Puede mejorar el rendimiento de escritura al usar append-only storage, pero puede requerir más procesamiento para lecturas.

---

### Data partitioning strategies

**Cómo funciona:**
Partitioning divide datos en múltiples particiones, permitiendo mejor escalabilidad y rendimiento.

**Ventajas:**
- Mejor escalabilidad
- Mejor rendimiento
- Mejor para datos grandes
- Permite procesamiento paralelo

**Desventajas:**
- Más complejo
- Requiere gestión de particiones
- Puede requerir queries cross-partition

**Cuándo usar:**
- Datos grandes
- Cuando se necesita escalabilidad
- Sistemas de alto rendimiento
- Cuando se puede particionar datos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al permitir procesamiento paralelo y reducir el tamaño de datos por partición.

---

### Polyglot persistence

**Cómo funciona:**
Polyglot persistence usa diferentes tipos de bases de datos para diferentes necesidades, optimizando cada caso de uso.

**Ventajas:**
- Optimización por caso de uso
- Mejor rendimiento
- Mejor para sistemas complejos
- Usa la mejor herramienta para cada trabajo

**Desventajas:**
- Más complejo
- Requiere más infraestructura
- Requiere gestión de múltiples sistemas

**Cuándo usar:**
- Sistemas complejos
- Cuando diferentes datos tienen diferentes necesidades
- Aplicaciones de alto rendimiento
- Cuando se necesita optimización específica

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al usar la base de datos óptima para cada caso de uso.

---

### Append-only logs

**Cómo funciona:**
Append-only logs solo permiten agregar datos, proporcionando mejor rendimiento de escritura y mejor para event sourcing.

**Ventajas:**
- Muy rápido para escrituras
- Mejor rendimiento
- Mejor para event sourcing
- Permite replay

**Desventajas:**
- No permite actualizaciones
- Requiere compactación
- Puede usar más espacio

**Cuándo usar:**
- Event sourcing
- Sistemas de logging
- Cuando las escrituras son mucho más frecuentes
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de escritura en un 5-20x comparado con almacenamiento que permite actualizaciones.

---

### Cache hierarchies

**Cómo funciona:**
Jerarquías de cache (L1, L2, L3) balancean velocidad y capacidad, optimizando el acceso a datos.

**Ventajas:**
- Balancea velocidad y capacidad
- Mejor rendimiento general
- Reduce carga en niveles inferiores
- Mejor para sistemas complejos

**Desventajas:**
- Más complejo
- Requiere gestión de múltiples niveles
- Puede requerir invalidación más compleja

**Cuándo usar:**
- Sistemas con múltiples fuentes de datos
- Cuando se necesita balancear velocidad y capacidad
- Aplicaciones distribuidas complejas
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al optimizar el acceso a datos en diferentes niveles.

---

### Horizontal scaling

**Cómo funciona:**
Escalado horizontal agrega más instancias en lugar de hacer instancias más grandes, proporcionando mejor escalabilidad.

**Ventajas:**
- Mejor escalabilidad
- Mejor para alto tráfico
- Mejor disponibilidad
- Mejor utilización de recursos

**Desventajas:**
- Requiere load balancing
- Puede requerir más gestión
- Requiere stateless services

**Cuándo usar:**
- Aplicaciones de alto tráfico
- Cuando se necesita escalabilidad
- Sistemas que requieren alta disponibilidad
- Aplicaciones distribuidas

**Impacto en performance:**
Puede mejorar el throughput total en un Nx donde N es el número de instancias (hasta el límite de recursos compartidos).

---

### Vertical scaling when necessary

**Cómo funciona:**
Escalado vertical aumenta recursos de una instancia (CPU, memoria), útil cuando horizontal no es posible.

**Ventajas:**
- Más simple
- No requiere cambios en arquitectura
- Mejor para algunos casos

**Desventajas:**
- Límites físicos
- Más costoso
- Menos escalable

**Cuándo usar:**
- Cuando horizontal no es posible
- Aplicaciones single-instance
- Cuando se necesita más recursos rápidamente
- Sistemas con límites de escalado horizontal

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x dependiendo de los recursos agregados.

---

### Auto-scaling rules

**Cómo funciona:**
Auto-scaling ajusta automáticamente el número de instancias basándose en métricas, optimizando recursos y costo.

**Ventajas:**
- Optimiza recursos
- Mejor costo-efectividad
- Mejor para cargas variables
- Automático

**Desventajas:**
- Requiere configuración
- Puede tener latencia en escalado
- Requiere métricas apropiadas

**Cuándo usar:**
- Cargas variables
- Cuando se necesita optimizar costo
- Sistemas en cloud
- Aplicaciones con patrones de tráfico variables

**Impacto en performance:**
Puede optimizar recursos y costo mientras mantiene el rendimiento apropiado.

---

### Multi-region deployments

**Cómo funciona:**
Despliegues multi-región distribuyen instancias en múltiples regiones geográficas, reduciendo latencia y mejorando disponibilidad.

**Ventajas:**
- Menor latencia para usuarios
- Mejor disponibilidad
- Mejor resiliencia
- Mejor experiencia de usuario global

**Desventajas:**
- Más complejo
- Requiere más infraestructura
- Puede requerir replicación de datos
- Más costoso

**Cuándo usar:**
- Aplicaciones globales
- Cuando la latencia es crítica
- Sistemas que requieren alta disponibilidad
- Aplicaciones con usuarios globales

**Impacto en performance:**
Puede reducir la latencia en un 50-90% para usuarios en regiones cercanas. El impacto es dramático.

---

### Active-active versus active-passive

**Cómo funciona:**
Active-active usa todas las instancias activamente, mientras active-passive mantiene instancias en standby.

**Ventajas:**
- Active-active: mejor utilización de recursos, mejor throughput
- Active-passive: más simple, menos recursos activos

**Desventajas:**
- Active-active: más complejo, requiere sincronización
- Active-passive: recursos desperdiciados en standby

**Cuándo usar:**
- Active-active: cuando se necesita máximo throughput
- Active-passive: cuando se necesita simplicidad

**Impacto en performance:**
Active-active puede mejorar el throughput en un 2x comparado con active-passive al usar todos los recursos.

---

### Consensus algorithms like Raft

**Cómo funciona:**
Algoritmos de consenso (Raft, Paxos) aseguran que múltiples nodos acuerden sobre el estado, crítico para sistemas distribuidos.

**Ventajas:**
- Consistencia en sistemas distribuidos
- Alta disponibilidad
- Tolerancia a fallos
- Mejor para sistemas críticos

**Desventajas:**
- Más complejo
- Overhead de consenso
- Requiere mayoría de nodos

**Cuándo usar:**
- Sistemas distribuidos que requieren consistencia
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Bases de datos distribuidas

**Impacto en performance:**
Tiene overhead de consenso (10-30%) pero proporciona consistencia y alta disponibilidad críticas.

---

### Latency budgets

**Cómo funciona:**
Latency budgets asignan tiempo máximo para cada componente, ayudando a identificar y optimizar cuellos de botella.

**Ventajas:**
- Identifica cuellos de botella
- Ayuda a optimizar
- Mejor experiencia de usuario
- Mejor planificación

**Desventajas:**
- Requiere monitoreo
- Requiere ajuste

**Cuándo usar:**
- Sistemas con requisitos de latencia
- Aplicaciones de alto rendimiento
- Cuando la latencia es crítica
- Sistemas distribuidos

**Impacto en performance:**
Puede mejorar la latencia en un 20-50% al identificar y optimizar cuellos de botella.

---

### Tail latency optimization

**Cómo funciona:**
Optimizar tail latency (P95, P99) mejora la experiencia de la mayoría de usuarios, no solo el promedio.

**Ventajas:**
- Mejor experiencia de usuario
- Mejor para la mayoría de usuarios
- Identifica problemas de rendimiento
- Mejor calidad de servicio

**Desventajas:**
- Requiere monitoreo
- Requiere optimización específica

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando la experiencia de usuario es crítica
- Sistemas con requisitos de latencia
- Aplicaciones de producción

**Impacto en performance:**
Puede mejorar la experiencia de usuario en un 20-50% al optimizar tail latency en lugar de solo el promedio.

---

### Hot path isolation

**Cómo funciona:**
Aislar hot paths (código ejecutado frecuentemente) permite optimización específica sin afectar el resto del sistema.

**Ventajas:**
- Permite optimización específica
- Mejor rendimiento
- Mejor para hot paths
- Enfoque en código crítico

**Desventajas:**
- Requiere identificación de hot paths
- Puede requerir refactorización

**Cuándo usar:**
- Hot paths identificados
- Cuando se necesita optimización específica
- Aplicaciones de alto rendimiento
- Después de profiling

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al optimizar código ejecutado frecuentemente. El impacto es dramático.

---

### Design for scalability from the start

**Cómo funciona:**
Diseñar sistemas para escalabilidad desde el inicio evita problemas de escalabilidad más adelante y mejora el rendimiento a largo plazo.

**Ventajas:**
- Evita problemas de escalabilidad
- Mejor rendimiento a largo plazo
- Mejor arquitectura
- Mejor para crecimiento

**Desventajas:**
- Puede requerir más diseño inicial
- Puede ser más complejo

**Cuándo usar:**
- Siempre al diseñar sistemas nuevos
- Sistemas que se espera que crezcan
- Aplicaciones de alto rendimiento
- Sistemas que requieren escalabilidad

**Impacto en performance:**
Puede prevenir problemas de escalabilidad que pueden degradar el rendimiento dramáticamente. El impacto es crítico a largo plazo.

---

### API Gateway for request routing and aggregation

**Cómo funciona:**
API Gateway enruta y agrega requests, proporcionando punto único de entrada y mejor gestión de APIs.

**Ventajas:**
- Punto único de entrada
- Mejor gestión de APIs
- Mejor seguridad
- Mejor para microservicios

**Desventajas:**
- Requiere infraestructura adicional
- Puede introducir latencia
- Más complejo

**Cuándo usar:**
- Microservicios
- Cuando se necesita gestión centralizada de APIs
- Sistemas distribuidos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede introducir latencia mínima (1-5ms) pero proporciona beneficios de gestión y seguridad que pueden mejorar el rendimiento general.

---

### Service mesh (Istio, Linkerd) for microservices communication

**Cómo funciona:**
Service mesh proporciona comunicación entre microservicios con features como load balancing, retry, circuit breaking, mejorando la confiabilidad.

**Ventajas:**
- Mejor comunicación entre microservicios
- Features automáticas (retry, circuit breaking)
- Mejor observabilidad
- Mejor seguridad

**Desventajas:**
- Requiere infraestructura adicional
- Puede introducir latencia
- Más complejo

**Cuándo usar:**
- Microservicios
- Cuando se necesita mejor comunicación
- Sistemas distribuidos complejos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede introducir latencia mínima (1-3ms) pero proporciona features que pueden mejorar la confiabilidad y rendimiento general.

---

### Load balancer algorithms

**Cómo funciona:**
Diferentes algoritmos de load balancing (round-robin, least connections, IP hash) distribuyen carga de diferentes maneras.

**Ventajas:**
- Optimización según necesidades
- Mejor distribución
- Mejor rendimiento
- Flexibilidad

**Desventajas:**
- Requiere conocimiento de algoritmos
- Puede requerir configuración

**Cuándo usar:**
- **Round-robin**: Distribución simple, balanceada
- **Least connections**: Mejor para conexiones de larga duración
- **IP hash**: Mejor para mantener sesiones

**Impacto en performance:**
Elegir el algoritmo correcto puede mejorar la distribución y rendimiento en un 10-30% según el caso de uso.

---

### Health checks and automatic failover

**Cómo funciona:**
Health checks monitorean el estado de servicios y failover automático cambia a servicios saludables cuando hay problemas.

**Ventajas:**
- Alta disponibilidad
- Mejor resiliencia
- Automático
- Mejor experiencia de usuario

**Desventajas:**
- Requiere configuración
- Puede tener latencia durante failover
- Requiere servicios de respaldo

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Aplicaciones de producción
- Sistemas que requieren resiliencia

**Impacto en performance:**
No afecta el rendimiento directamente pero previene indisponibilidad que puede ser crítica.

---

### Bulkhead pattern for fault isolation

**Cómo funciona:**
Bulkhead pattern aísla recursos (threads, conexiones) para prevenir que fallos en un área afecten otras áreas.

**Ventajas:**
- Aislamiento de fallos
- Mejor resiliencia
- Previene cascading failures
- Mejor estabilidad

**Desventajas:**
- Requiere más recursos
- Más complejo
- Puede subutilizar recursos

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita aislamiento de fallos
- Aplicaciones de producción
- Sistemas que requieren alta disponibilidad

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al aislar fallos. El impacto es en estabilidad.

---

### Saga pattern for distributed transactions

**Cómo funciona:**
Saga pattern maneja transacciones distribuidas mediante una serie de transacciones locales con compensación, evitando bloqueos largos.

**Ventajas:**
- Mejor para transacciones distribuidas
- Evita bloqueos largos
- Mejor escalabilidad
- Mejor rendimiento

**Desventajas:**
- Más complejo
- Requiere lógica de compensación
- Consistencia eventual

**Cuándo usar:**
- Transacciones distribuidas
- Cuando se necesita mejor escalabilidad
- Sistemas distribuidos
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con two-phase commit al evitar bloqueos largos.

---

### Two-phase commit for distributed transactions

**Cómo funciona:**
Two-phase commit coordina transacciones distribuidas mediante dos fases (prepare, commit), proporcionando atomicidad pero con bloqueos.

**Ventajas:**
- Atomicidad garantizada
- Mejor consistencia
- Mejor para sistemas críticos

**Desventajas:**
- Bloqueos largos
- Peor escalabilidad
- Puede causar deadlocks
- Más lento

**Cuándo usar:**
- Transacciones distribuidas que requieren atomicidad fuerte
- Sistemas críticos
- Cuando se puede aceptar bloqueos
- Sistemas que requieren consistencia fuerte

**Impacto en performance:**
Puede degradar el rendimiento significativamente debido a bloqueos largos. Preferir Saga pattern cuando sea posible.

---

### Eventual consistency patterns

**Cómo funciona:**
Consistencia eventual permite que sistemas distribuidos tengan datos ligeramente inconsistentes temporalmente, mejorando el rendimiento y escalabilidad.

**Ventajas:**
- Mejor rendimiento
- Mejor escalabilidad
- Mejor disponibilidad
- Mejor para sistemas distribuidos

**Desventajas:**
- Consistencia eventual
- Puede tener datos obsoletos temporalmente
- Requiere diseño cuidadoso

**Cuándo usar:**
- Sistemas distribuidos
- Cuando se puede aceptar consistencia eventual
- Aplicaciones de alto rendimiento
- Sistemas que requieren escalabilidad

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al evitar bloqueos y permitir mejor escalabilidad. El impacto es dramático.

---

### CQRS read models optimization

**Cómo funciona:**
Optimizar read models en CQRS permite optimizar lecturas independientemente de escrituras, mejorando el rendimiento de lectura.

**Ventajas:**
- Optimización independiente
- Mejor rendimiento de lectura
- Mejor escalabilidad
- Mejor para sistemas complejos

**Desventajas:**
- Más complejo
- Requiere sincronización
- Consistencia eventual

**Cuándo usar:**
- Sistemas con CQRS
- Cuando se necesita optimizar lecturas
- Sistemas de alto rendimiento
- Aplicaciones con diferentes patrones de lectura/escritura

**Impacto en performance:**
Puede mejorar el rendimiento de lectura en un 10-100x al optimizar read models independientemente.

---

### Event sourcing snapshots

**Cómo funciona:**
Snapshots en event sourcing almacenan estado en puntos específicos, permitiendo replay más rápido desde snapshots en lugar de desde el inicio.

**Ventajas:**
- Replay más rápido
- Mejor rendimiento
- Mejor para sistemas con muchos eventos
- Mejor para recuperación

**Desventajas:**
- Requiere almacenamiento adicional
- Requiere gestión de snapshots
- Más complejo

**Cuándo usar:**
- Sistemas con event sourcing
- Cuando hay muchos eventos
- Sistemas que requieren replay frecuente
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de replay en un 10-1000x al permitir replay desde snapshots en lugar de desde el inicio.

---

### Blue-green deployments

**Cómo funciona:**
Blue-green deployments mantienen dos entornos idénticos (blue y green), permitiendo cambios sin downtime.

**Ventajas:**
- Sin downtime
- Rollback rápido
- Mejor para sistemas críticos
- Mejor experiencia de usuario

**Desventajas:**
- Requiere más recursos
- Más complejo
- Requiere gestión

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita sin downtime
- Aplicaciones de producción
- Sistemas que requieren alta disponibilidad

**Impacto en performance:**
Permite deployments sin afectar el rendimiento de usuarios, mejorando la disponibilidad.

---

### Canary deployments

**Cómo funciona:**
Canary deployments despliegan cambios a un pequeño porcentaje de usuarios primero, permitiendo validación antes de despliegue completo.

**Ventajas:**
- Validación antes de despliegue completo
- Rollback rápido
- Mejor para sistemas críticos
- Reduce riesgo

**Desventajas:**
- Más complejo
- Requiere gestión
- Puede requerir más tiempo

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita validación
- Aplicaciones de producción
- Sistemas que requieren bajo riesgo

**Impacto en performance:**
Permite validación sin afectar a todos los usuarios, mejorando la confiabilidad.

---

### Feature flags for gradual rollouts

**Cómo funciona:**
Feature flags permiten habilitar/deshabilitar features sin redeploy, permitiendo rollouts graduales y rollback rápido.

**Ventajas:**
- Rollouts graduales
- Rollback rápido
- Mejor control
- Mejor para sistemas críticos

**Desventajas:**
- Requiere gestión
- Puede complicar código
- Requiere infraestructura

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita control sobre features
- Aplicaciones de producción
- Sistemas que requieren rollouts graduales

**Impacto en performance:**
Permite control sobre features sin afectar el rendimiento, mejorando la confiabilidad.

---

### A/B testing infrastructure

**Cómo funciona:**
Infraestructura de A/B testing permite probar diferentes versiones con diferentes usuarios, permitiendo optimización basada en datos.

**Ventajas:**
- Optimización basada en datos
- Mejor experiencia de usuario
- Permite experimentación
- Mejor para optimización

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Requiere análisis

**Cuándo usar:**
- Cuando se necesita optimización basada en datos
- Aplicaciones que requieren optimización
- Sistemas de alto rendimiento
- Cuando se necesita experimentación

**Impacto en performance:**
Permite optimización basada en datos que puede mejorar el rendimiento y experiencia de usuario significativamente.

---

### Distributed tracing (Jaeger, Zipkin)

**Cómo funciona:**
Distributed tracing rastrea requests a través de múltiples servicios, permitiendo identificar cuellos de botella y problemas de rendimiento.

**Ventajas:**
- Identifica cuellos de botella
- Mejor visibilidad
- Mejor para microservicios
- Permite optimización

**Desventajas:**
- Requiere infraestructura
- Overhead adicional
- Más complejo

**Cuándo usar:**
- Microservicios
- Sistemas distribuidos
- Cuando se necesita visibilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Tiene overhead mínimo (1-5%) pero permite identificar problemas que pueden mejorar el rendimiento en un 10-100x.

---

### Service discovery

**Cómo funciona:**
Service discovery permite que servicios encuentren otros servicios dinámicamente, mejorando la flexibilidad y escalabilidad.

**Ventajas:**
- Flexibilidad
- Mejor escalabilidad
- Mejor para microservicios
- Automático

**Desventajas:**
- Requiere infraestructura
- Puede introducir latencia
- Más complejo

**Cuándo usar:**
- Microservicios
- Sistemas distribuidos
- Cuando se necesita flexibilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede introducir latencia mínima (1-5ms) pero proporciona flexibilidad que puede mejorar la escalabilidad.

---

### Configuration management

**Cómo funciona:**
Configuration management centraliza y gestiona configuración, permitiendo cambios sin redeploy y mejor gestión.

**Ventajas:**
- Cambios sin redeploy
- Mejor gestión
- Mejor para sistemas complejos
- Mejor control

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Requiere gestión

**Cuándo usar:**
- Sistemas complejos
- Cuando se necesita cambios sin redeploy
- Aplicaciones de producción
- Sistemas que requieren flexibilidad

**Impacto en performance:**
No afecta el rendimiento directamente pero permite cambios que pueden mejorar el rendimiento sin redeploy.

---

### Secret management

**Cómo funciona:**
Secret management gestiona secretos (passwords, keys) de manera segura, mejorando la seguridad sin afectar el rendimiento.

**Ventajas:**
- Mejor seguridad
- Gestión centralizada
- Mejor para sistemas complejos
- Mejor control

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Requiere gestión

**Cuándo usar:**
- Sistemas que requieren seguridad
- Aplicaciones de producción
- Sistemas complejos
- Cuando se necesita gestión de secretos

**Impacto en performance:**
No afecta el rendimiento directamente pero mejora la seguridad que es crítica para sistemas de producción.

---

### API versioning strategies

**Cómo funciona:**
Estrategias de versionado de APIs permiten cambios en APIs sin romper clientes existentes, mejorando la flexibilidad.

**Ventajas:**
- Cambios sin romper clientes
- Mejor flexibilidad
- Mejor para sistemas complejos
- Mejor control

**Desventajas:**
- Requiere gestión
- Puede complicar APIs
- Requiere mantenimiento

**Cuándo usar:**
- APIs públicas
- Sistemas complejos
- Cuando se necesita cambios sin romper clientes
- Aplicaciones de producción

**Impacto en performance:**
No afecta el rendimiento directamente pero permite cambios que pueden mejorar el rendimiento sin romper clientes.

---

### GraphQL query optimization

**Cómo funciona:**
Optimizar queries de GraphQL (usar DataLoader, limitar profundidad, etc.) mejora el rendimiento de queries GraphQL.

**Ventajas:**
- Mejor rendimiento
- Mejor para queries complejas
- Optimización según necesidades
- Mejor experiencia de usuario

**Desventajas:**
- Requiere conocimiento de GraphQL
- Requiere optimización
- Puede ser más complejo

**Cuándo usar:**
- Sistemas que usan GraphQL
- Cuando el rendimiento es crítico
- Queries complejas
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x para queries complejas. El impacto es dramático con queries mal optimizadas.

---

### GraphQL DataLoader for N+1 prevention

**Cómo funciona:**
DataLoader agrupa y cachea cargas de datos, previniendo problemas N+1 en GraphQL y mejorando el rendimiento.

**Ventajas:**
- Previene N+1
- Mejor rendimiento
- Mejor para GraphQL
- Automático

**Desventajas:**
- Requiere DataLoader
- Puede requerir configuración

**Cuándo usar:**
- Sistemas que usan GraphQL
- Cuando hay riesgo de N+1
- Aplicaciones de alto rendimiento
- Queries con relaciones

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al prevenir N+1. El impacto es dramático.

---

### REST API response compression

**Cómo funciona:**
Comprimir respuestas de REST APIs reduce el tamaño de datos transferidos, mejorando el rendimiento de red.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento de red
- Menor uso de ancho de banda
- Mejor experiencia de usuario

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración

**Cuándo usar:**
- APIs con respuestas grandes
- Cuando el ancho de banda es limitado
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar overhead de CPU

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x cuando el ancho de banda es un cuello de botella. Puede degradar si CPU es cuello de botella.

---

### API rate limiting per user/IP

**Cómo funciona:**
Rate limiting por usuario/IP controla la tasa de requests por usuario o IP, previniendo abuso y mejorando la estabilidad.

**Ventajas:**
- Previene abuso
- Mejor estabilidad
- Protege recursos
- Mejor experiencia de usuario

**Desventajas:**
- Puede rechazar requests válidos
- Requiere configuración
- Requiere gestión

**Cuándo usar:**
- APIs públicas
- Sistemas con riesgo de abuso
- Aplicaciones de alto rendimiento
- Cuando se necesita proteger recursos

**Impacto en performance:**
Puede prevenir degradación del rendimiento al limitar abuso. El impacto es en estabilidad.

---

### Request/response compression

**Cómo funciona:**
Comprimir requests y responses reduce el tamaño de datos transferidos, mejorando el rendimiento de red.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento de red
- Menor uso de ancho de banda
- Mejor experiencia de usuario

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración

**Cuándo usar:**
- Requests/responses grandes
- Cuando el ancho de banda es limitado
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar overhead de CPU

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x cuando el ancho de banda es un cuello de botella. Puede degradar si CPU es cuello de botella.

---

### Object storage (S3, Azure Blob, GCS) for static assets

**Cómo funciona:**
Object storage almacena assets estáticos (imágenes, videos, archivos) de manera escalable y económica.

**Ventajas:**
- Escalable
- Económico
- Mejor para assets estáticos
- Mejor rendimiento

**Desventajas:**
- Latencia de red
- Requiere gestión
- Menos control

**Cuándo usar:**
- Assets estáticos
- Cuando se necesita escalabilidad
- Aplicaciones de alto tráfico
- Sistemas que requieren almacenamiento económico

**Impacto en performance:**
Puede mejorar el rendimiento al liberar recursos del servidor y proporcionar mejor escalabilidad para assets estáticos.

---

### Edge computing for low latency

**Cómo funciona:**
Edge computing procesa datos cerca de usuarios, reduciendo latencia y mejorando el rendimiento.

**Ventajas:**
- Menor latencia
- Mejor rendimiento
- Mejor experiencia de usuario
- Mejor para usuarios globales

**Desventajas:**
- Requiere infraestructura
- Más complejo
- Puede ser más costoso

**Cuándo usar:**
- Aplicaciones que requieren baja latencia
- Usuarios globales
- Aplicaciones de alto rendimiento
- Cuando la latencia es crítica

**Impacto en performance:**
Puede reducir la latencia en un 50-90% para usuarios lejos del servidor central. El impacto es dramático.

---

### WebAssembly for client-side performance

**Cómo funciona:**
WebAssembly permite ejecutar código de alto rendimiento en navegadores, proporcionando mejor rendimiento que JavaScript.

**Ventajas:**
- Mejor rendimiento que JavaScript
- Código de bajo nivel
- Mejor para computación intensiva
- Mejor experiencia de usuario

**Desventajas:**
- Más complejo
- Requiere compilación
- Curva de aprendizaje

**Cuándo usar:**
- Computación intensiva en cliente
- Cuando se necesita máximo rendimiento en cliente
- Aplicaciones que requieren procesamiento en cliente
- Cuando JavaScript no es suficiente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con JavaScript para computación intensiva. El impacto es dramático.

---

### Serverless function optimization

**Cómo funciona:**
Optimizar funciones serverless (cold start reduction, memory tuning, etc.) mejora el rendimiento y reduce costos.

**Ventajas:**
- Mejor rendimiento
- Menor costo
- Optimización según necesidades
- Mejor para cargas variables

**Desventajas:**
- Requiere optimización
- Puede variar según proveedor
- Requiere conocimiento

**Cuándo usar:**
- Funciones serverless
- Cargas variables
- Aplicaciones serverless
- Cuando se necesita optimizar costo y rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-100% y reducir costos significativamente. El impacto depende de la optimización.

---

### Container optimization

**Cómo funciona:**
Optimizar containers (tamaño de imagen, layers, etc.) mejora el rendimiento de despliegue y ejecución.

**Ventajas:**
- Mejor rendimiento de despliegue
- Menor uso de recursos
- Mejor para sistemas de alto rendimiento
- Menor costo

**Desventajas:**
- Requiere optimización
- Puede requerir más tiempo
- Requiere conocimiento

**Cuándo usar:**
- Sistemas con containers
- Cuando el rendimiento es crítico
- Aplicaciones de alto rendimiento
- Cuando se necesita optimizar recursos

**Impacto en performance:**
Puede mejorar el rendimiento de despliegue en un 50-90% y reducir el uso de recursos en un 20-50%.

---

### Kubernetes pod resource limits

**Cómo funciona:**
Configurar resource limits apropiados para pods de Kubernetes previene que pods usen demasiados recursos y mejora la estabilidad.

**Ventajas:**
- Previene uso excesivo de recursos
- Mejor estabilidad
- Mejor utilización de recursos
- Mejor para sistemas compartidos

**Desventajas:**
- Requiere configuración
- Puede limitar pods si están mal configurados

**Cuándo usar:**
- Siempre configurar apropiadamente
- Sistemas con Kubernetes
- Aplicaciones de producción
- Sistemas que requieren estabilidad

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al prevenir uso excesivo de recursos. El impacto es en estabilidad.

---

### Kubernetes horizontal pod autoscaling

**Cómo funciona:**
HPA ajusta automáticamente el número de pods basándose en métricas, optimizando recursos y costo.

**Ventajas:**
- Optimiza recursos
- Mejor costo-efectividad
- Automático
- Mejor para cargas variables

**Desventajas:**
- Requiere configuración
- Puede tener latencia en escalado
- Requiere métricas apropiadas

**Cuándo usar:**
- Cargas variables
- Cuando se necesita optimizar costo
- Sistemas en Kubernetes
- Aplicaciones con patrones de tráfico variables

**Impacto en performance:**
Puede optimizar recursos y costo mientras mantiene el rendimiento apropiado.

---

### Kubernetes vertical pod autoscaling

**Cómo funciona:**
VPA ajusta automáticamente los recursos de pods basándose en uso, optimizando recursos y costo.

**Ventajas:**
- Optimiza recursos
- Mejor costo-efectividad
- Automático
- Mejor utilización de recursos

**Desventajas:**
- Requiere configuración
- Puede requerir reinicio de pods
- Requiere métricas apropiadas

**Cuándo usar:**
- Cuando se necesita optimizar recursos
- Sistemas en Kubernetes
- Aplicaciones con uso variable de recursos
- Cuando se necesita optimizar costo

**Impacto en performance:**
Puede optimizar recursos y costo mientras mantiene el rendimiento apropiado.

---

### Container image optimization

**Cómo funciona:**
Optimizar imágenes de containers (tamaño, layers, etc.) mejora el rendimiento de despliegue y reduce uso de recursos.

**Ventajas:**
- Mejor rendimiento de despliegue
- Menor uso de recursos
- Menor costo
- Mejor para sistemas de alto rendimiento

**Desventajas:**
- Requiere optimización
- Puede requerir más tiempo
- Requiere conocimiento

**Cuándo usar:**
- Sistemas con containers
- Cuando el rendimiento es crítico
- Aplicaciones de alto rendimiento
- Cuando se necesita optimizar recursos

**Impacto en performance:**
Puede mejorar el rendimiento de despliegue en un 50-90% y reducir el uso de recursos en un 20-50%.

---

### Multi-stage Docker builds

**Cómo funciona:**
Multi-stage Docker builds usan múltiples stages para construir imágenes, reduciendo el tamaño final y mejorando el rendimiento.

**Ventajas:**
- Imágenes más pequeñas
- Mejor rendimiento de despliegue
- Menor uso de recursos
- Mejor seguridad

**Desventajas:**
- Más complejo
- Requiere conocimiento de Docker

**Cuándo usar:**
- Siempre cuando sea posible
- Sistemas con containers
- Aplicaciones de alto rendimiento
- Cuando se necesita optimizar tamaño

**Impacto en performance:**
Puede reducir el tamaño de imágenes en un 50-90% y mejorar el rendimiento de despliegue significativamente.

---

### Nginx event-driven architecture (epoll-based)

**Cómo funciona:**
Nginx usa arquitectura event-driven basada en epoll (Linux) que maneja miles de conexiones concurrentes eficientemente.

**Ventajas:**
- Muy eficiente
- Maneja muchas conexiones
- Mejor rendimiento
- Mejor para alto tráfico

**Desventajas:**
- Requiere Nginx
- Requiere configuración

**Cuándo usar:**
- Servidores web de alto tráfico
- Cuando se necesita alto rendimiento
- Aplicaciones de alto tráfico
- Sistemas que requieren muchas conexiones

**Impacto en performance:**
Puede manejar miles de conexiones concurrentes con bajo uso de recursos, mejorando el rendimiento dramáticamente.

---

### Nginx worker processes and worker connections tuning

**Cómo funciona:**
Ajustar el número de worker processes y worker connections en Nginx optimiza el rendimiento según la carga de trabajo.

**Ventajas:**
- Optimización según necesidades
- Mejor rendimiento
- Mejor utilización de recursos
- Mejor para diferentes cargas

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de Nginx

**Cuándo usar:**
- Sistemas con Nginx
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% según la configuración. El impacto es mayor cuando está mal configurado.

**Configuración típica:**
- `worker_processes`: Número de cores
- `worker_connections`: 1024-4096 por worker

---

### Nginx sendfile and tcp_nopush for zero-copy

**Cómo funciona:**
sendfile y tcp_nopush en Nginx permiten zero-copy para transferencias de archivos, mejorando el rendimiento.

**Ventajas:**
- Elimina copias
- Mejor rendimiento
- Menor uso de CPU
- Mejor para archivos grandes

**Desventajas:**
- Requiere configuración
- Específico de Nginx

**Cuándo usar:**
- Servidores de archivos
- Cuando se transfieren archivos grandes
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al eliminar copias. El impacto es mayor para archivos grandes.

---

### Nginx gzip_static for pre-compressed files

**Cómo funciona:**
gzip_static sirve archivos pre-comprimidos en lugar de comprimir sobre la marcha, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menor uso de CPU
- Mejor para archivos estáticos
- Mejor experiencia de usuario

**Desventajas:**
- Requiere pre-compresión
- Requiere más almacenamiento
- Requiere configuración

**Cuándo usar:**
- Archivos estáticos
- Cuando se puede pre-comprimir
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al evitar compresión sobre la marcha. El impacto es mayor con muchos archivos.

---

### Nginx open_file_cache for file descriptor caching

**Cómo funciona:**
open_file_cache cachea file descriptors en Nginx, reduciendo overhead de abrir archivos y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Mejor para archivos frecuentemente accedidos
- Mejor utilización de recursos

**Desventajas:**
- Requiere configuración
- Usa memoria

**Cuándo usar:**
- Archivos frecuentemente accedidos
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento
- Cuando se accede a muchos archivos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al reducir overhead de abrir archivos. El impacto es mayor con muchos archivos.

---

### Nginx proxy_cache for reverse proxy caching

**Cómo funciona:**
proxy_cache en Nginx cachea respuestas de servidores backend, reduciendo carga en backends y mejorando el rendimiento.

**Ventajas:**
- Reduce carga en backends
- Mejor rendimiento
- Mejor experiencia de usuario
- Mejor para contenido cacheable

**Desventajas:**
- Requiere configuración
- Usa almacenamiento
- Puede tener datos obsoletos

**Cuándo usar:**
- Reverse proxy
- Contenido cacheable
- Aplicaciones de alto rendimiento
- Sistemas que requieren reducir carga en backends

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para contenido cacheable. El impacto es dramático.

---

### Nginx upstream keepalive connections

**Cómo funciona:**
upstream keepalive en Nginx mantiene conexiones abiertas a backends, reduciendo overhead de establecer conexiones.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples requests

**Desventajas:**
- Requiere configuración
- Usa recursos mientras está abierto

**Cuándo usar:**
- Reverse proxy con backends
- Cuando se hacen múltiples requests
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar overhead de conexión repetido. El impacto es mayor con muchos requests.

---

### Nginx rate limiting (limit_req, limit_conn)

**Cómo funciona:**
Rate limiting en Nginx controla la tasa de requests y conexiones, previniendo abuso y mejorando la estabilidad.

**Ventajas:**
- Previene abuso
- Mejor estabilidad
- Protege recursos
- Mejor experiencia de usuario

**Desventajas:**
- Puede rechazar requests válidos
- Requiere configuración

**Cuándo usar:**
- APIs públicas
- Sistemas con riesgo de abuso
- Aplicaciones de alto rendimiento
- Cuando se necesita proteger recursos

**Impacto en performance:**
Puede prevenir degradación del rendimiento al limitar abuso. El impacto es en estabilidad.

---

### Nginx SSL session caching

**Cómo funciona:**
SSL session caching en Nginx cachea sesiones TLS, evitando handshakes repetidos y mejorando el rendimiento.

**Ventajas:**
- Evita handshakes repetidos
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples conexiones

**Desventajas:**
- Requiere configuración
- Usa memoria

**Cuándo usar:**
- Siempre cuando se usa SSL/TLS
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento
- Cuando se hacen múltiples conexiones TLS

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar handshakes repetidos. El impacto es mayor con muchas conexiones.

---

### Nginx worker CPU affinity

**Cómo funciona:**
CPU affinity en Nginx asigna workers a cores específicos, mejorando la localidad de caché y el rendimiento.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Mejor utilización de CPU
- Mejor para sistemas multi-core

**Desventajas:**
- Requiere configuración
- Puede no ser beneficioso en todos los casos

**Cuándo usar:**
- Sistemas multi-core
- Cuando se necesita máximo rendimiento
- Aplicaciones de alto rendimiento
- Sistemas que requieren optimización extrema

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la localidad de caché. El impacto es mayor en sistemas multi-core.

---

### Nginx aio (asynchronous I/O) for file operations

**Cómo funciona:**
aio en Nginx permite I/O asíncrono para operaciones de archivo, mejorando el rendimiento para archivos grandes.

**Ventajas:**
- Mejor rendimiento para archivos grandes
- No bloquea workers
- Mejor para I/O intensivo
- Mejor utilización de recursos

**Desventajas:**
- Requiere configuración
- Puede no ser beneficioso para archivos pequeños

**Cuándo usar:**
- Archivos grandes
- I/O intensivo
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% para archivos grandes. El impacto es mayor con archivos muy grandes.

---

### Nginx directio for large file serving

**Cómo funciona:**
directio en Nginx bypassa el page cache para archivos grandes, mejorando el rendimiento para archivos muy grandes.

**Ventajas:**
- Mejor rendimiento para archivos muy grandes
- Evita doble caching
- Mejor para archivos grandes
- Mejor utilización de memoria

**Desventajas:**
- Requiere configuración
- Solo para archivos grandes
- Puede no ser beneficioso para archivos pequeños

**Cuándo usar:**
- Archivos muy grandes
- Cuando se necesita máximo rendimiento
- Aplicaciones de alto rendimiento
- Sistemas que sirven archivos grandes

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para archivos muy grandes. El impacto es mayor con archivos muy grandes.

---

### Nginx sendfile_max_chunk tuning

**Cómo funciona:**
sendfile_max_chunk en Nginx controla el tamaño máximo de chunks para sendfile, optimizando el rendimiento.

**Ventajas:**
- Optimización según necesidades
- Mejor rendimiento
- Mejor para diferentes tamaños de archivo
- Flexibilidad

**Desventajas:**
- Requiere tuning
- Puede variar según carga

**Cuándo usar:**
- Sistemas con Nginx
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% según la configuración. El impacto depende del tamaño de archivos.

---

### Nginx multi_accept for better connection handling

**Cómo funciona:**
multi_accept en Nginx permite que workers acepten múltiples conexiones a la vez, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Mejor para muchas conexiones
- Mejor utilización de recursos
- Mejor para alto tráfico

**Desventajas:**
- Requiere configuración
- Puede no ser beneficioso en todos los casos

**Cuándo usar:**
- Alto tráfico
- Muchas conexiones
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para alto tráfico. El impacto es mayor con muchas conexiones.

---

### Nginx accept_mutex for connection distribution

**Cómo funciona:**
accept_mutex en Nginx distribuye conexiones entre workers, mejorando el balanceo de carga.

**Ventajas:**
- Mejor balanceo de carga
- Mejor distribución
- Mejor rendimiento
- Mejor para sistemas multi-core

**Desventajas:**
- Requiere configuración
- Puede no ser necesario en sistemas modernos

**Cuándo usar:**
- Sistemas multi-core
- Cuando hay desbalanceo de conexiones
- Aplicaciones de alto rendimiento
- Sistemas que requieren mejor distribución

**Impacto en performance:**
Puede mejorar el balanceo de carga en un 10-30%. El impacto es mayor cuando hay desbalanceo.

---

### Nginx deferred accept for reduced CPU usage

**Cómo funciona:**
deferred accept en Nginx difiere la aceptación de conexiones hasta que hay datos, reduciendo el uso de CPU.

**Ventajas:**
- Menor uso de CPU
- Mejor rendimiento
- Mejor para muchas conexiones
- Mejor utilización de recursos

**Desventajas:**
- Requiere configuración
- Puede no ser beneficioso en todos los casos

**Cuándo usar:**
- Muchas conexiones
- Cuando el CPU es un cuello de botella
- Aplicaciones de alto rendimiento
- Sistemas que requieren optimización de CPU

**Impacto en performance:**
Puede reducir el uso de CPU en un 10-30% y mejorar el rendimiento. El impacto es mayor con muchas conexiones.

---

### Apache mod_event for event-driven MPM

**Cómo funciona:**
mod_event en Apache proporciona MPM event-driven que maneja conexiones eficientemente, similar a Nginx.

**Ventajas:**
- Muy eficiente
- Maneja muchas conexiones
- Mejor rendimiento
- Mejor para alto tráfico

**Desventajas:**
- Requiere Apache
- Requiere configuración

**Cuándo usar:**
- Servidores web Apache de alto tráfico
- Cuando se necesita alto rendimiento
- Aplicaciones de alto tráfico
- Sistemas que requieren muchas conexiones

**Impacto en performance:**
Puede manejar miles de conexiones concurrentes con bajo uso de recursos, mejorando el rendimiento dramáticamente.

---

### Apache worker/event MPM tuning

**Cómo funciona:**
Ajustar worker/event MPM en Apache optimiza el rendimiento según la carga de trabajo.

**Ventajas:**
- Optimización según necesidades
- Mejor rendimiento
- Mejor utilización de recursos
- Mejor para diferentes cargas

**Desventajas:**
- Requiere tuning
- Puede variar según carga
- Requiere conocimiento de Apache

**Cuándo usar:**
- Sistemas con Apache
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% según la configuración. El impacto es mayor cuando está mal configurado.

---

### Apache KeepAlive and MaxKeepAliveRequests

**Cómo funciona:**
KeepAlive en Apache mantiene conexiones abiertas entre requests. MaxKeepAliveRequests limita cuántos requests por conexión.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples requests

**Desventajas:**
- Requiere configuración
- Usa recursos mientras está abierto

**Cuándo usar:**
- Siempre cuando sea posible
- Aplicaciones de alto rendimiento
- Cuando se hacen múltiples requests
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar overhead de conexión repetido. El impacto es mayor con muchos requests.

---

### Apache mod_cache for HTTP caching

**Cómo funciona:**
mod_cache en Apache cachea respuestas HTTP, reduciendo carga en backends y mejorando el rendimiento.

**Ventajas:**
- Reduce carga en backends
- Mejor rendimiento
- Mejor experiencia de usuario
- Mejor para contenido cacheable

**Desventajas:**
- Requiere configuración
- Usa almacenamiento
- Puede tener datos obsoletos

**Cuándo usar:**
- Contenido cacheable
- Aplicaciones de alto rendimiento
- Sistemas que requieren reducir carga en backends
- Cuando se puede cachear contenido

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para contenido cacheable. El impacto es dramático.

---

### Apache mod_deflate for compression

**Cómo funciona:**
mod_deflate en Apache comprime respuestas HTTP, reduciendo el tamaño de datos y mejorando el rendimiento de red.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento de red
- Menor uso de ancho de banda
- Mejor experiencia de usuario

**Desventajas:**
- Aumenta uso de CPU
- Requiere configuración

**Cuándo usar:**
- Respuestas grandes
- Cuando el ancho de banda es limitado
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar overhead de CPU

**Impacto en performance:**
Puede mejorar el rendimiento en un 2-10x cuando el ancho de banda es un cuello de botella. Puede degradar si CPU es cuello de botella.

---

### HAProxy connection pooling and keepalive optimization

**Cómo funciona:**
Connection pooling y keepalive en HAProxy reutilizan conexiones, reduciendo overhead y mejorando el rendimiento.

**Ventajas:**
- Menos overhead
- Mejor rendimiento
- Menor latencia
- Mejor para múltiples requests

**Desventajas:**
- Requiere configuración
- Usa recursos mientras está abierto

**Cuándo usar:**
- Siempre cuando sea posible
- Aplicaciones de alto rendimiento
- Cuando se hacen múltiples requests
- Sistemas que requieren máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al evitar overhead de conexión repetido. El impacto es mayor con muchos requests.

---

### HAProxy stick tables for session persistence

**Cómo funciona:**
Stick tables en HAProxy mantienen persistencia de sesión, dirigiendo requests del mismo cliente al mismo servidor.

**Ventajas:**
- Persistencia de sesión
- Mejor para aplicaciones stateful
- Mejor experiencia de usuario
- Mejor para sistemas que requieren sesión

**Desventajas:**
- Requiere configuración
- Usa memoria
- Puede causar desbalanceo

**Cuándo usar:**
- Aplicaciones stateful
- Cuando se necesita persistencia de sesión
- Sistemas que requieren sesión
- Aplicaciones de alto rendimiento

**Impacto en performance:**
No afecta el rendimiento directamente pero proporciona persistencia de sesión que puede ser crítica para algunas aplicaciones.

---

### HAProxy health checks and automatic failover

**Cómo funciona:**
Health checks en HAProxy monitorean el estado de servidores y failover automático cambia a servidores saludables cuando hay problemas.

**Ventajas:**
- Alta disponibilidad
- Mejor resiliencia
- Automático
- Mejor experiencia de usuario

**Desventajas:**
- Requiere configuración
- Puede tener latencia durante failover
- Requiere servidores de respaldo

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alta disponibilidad
- Aplicaciones de producción
- Sistemas que requieren resiliencia

**Impacto en performance:**
No afecta el rendimiento directamente pero previene indisponibilidad que puede ser crítica.

---

### HAProxy ACL optimization for routing decisions

**Cómo funciona:**
Optimizar ACLs (Access Control Lists) en HAProxy mejora el rendimiento de decisiones de routing.

**Ventajas:**
- Mejor rendimiento de routing
- Optimización según necesidades
- Mejor para routing complejo
- Mejor utilización de recursos

**Desventajas:**
- Requiere optimización
- Puede requerir conocimiento de HAProxy

**Cuándo usar:**
- Routing complejo
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento de routing en un 10-50% según la optimización. El impacto es mayor con routing complejo.

---

### HAProxy connection limits and queuing tuning

**Cómo funciona:**
Ajustar límites de conexión y queuing en HAProxy optimiza el rendimiento y previene sobrecarga.

**Ventajas:**
- Previene sobrecarga
- Mejor estabilidad
- Optimización según necesidades
- Mejor rendimiento

**Desventajas:**
- Requiere tuning
- Puede variar según carga

**Cuándo usar:**
- Sistemas con HAProxy
- Cuando el rendimiento es crítico
- Después de profiling
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% al prevenir sobrecarga. El impacto es en estabilidad.

---

### HAProxy SSL termination and session reuse

**Cómo funciona:**
SSL termination y session reuse en HAProxy terminan SSL en HAProxy y reutilizan sesiones, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menor carga en backends
- Mejor para múltiples conexiones
- Mejor utilización de recursos

**Desventajas:**
- Requiere configuración
- Requiere certificados en HAProxy

**Cuándo usar:**
- Cuando se usa SSL/TLS
- Aplicaciones de alto rendimiento
- Sistemas que requieren máximo rendimiento
- Cuando se pueden terminar SSL en HAProxy

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% al terminar SSL en HAProxy y reutilizar sesiones. El impacto es mayor con muchas conexiones.

---

### HAProxy HTTP/2 and WebSocket support

**Cómo funciona:**
Soporte para HTTP/2 y WebSocket en HAProxy permite mejor rendimiento y funcionalidad moderna.

**Ventajas:**
- Mejor rendimiento con HTTP/2
- Soporte para WebSocket
- Mejor para aplicaciones modernas
- Mejor experiencia de usuario

**Desventajas:**
- Requiere configuración
- Requiere HAProxy 1.8+

**Cuándo usar:**
- Aplicaciones modernas
- Cuando se necesita HTTP/2
- Aplicaciones con WebSocket
- Sistemas de alto rendimiento

**Impacto en performance:**
HTTP/2 puede mejorar el rendimiento en un 20-50% comparado con HTTP/1.1. El impacto es mayor con múltiples requests.

---

### HAProxy load balancing algorithms

**Cómo funciona:**
Diferentes algoritmos de load balancing en HAProxy (roundrobin, leastconn, source, etc.) distribuyen carga de diferentes maneras.

**Ventajas:**
- Optimización según necesidades
- Mejor distribución
- Mejor rendimiento
- Flexibilidad

**Desventajas:**
- Requiere conocimiento de algoritmos
- Puede requerir configuración

**Cuándo usar:**
- **roundrobin**: Distribución simple, balanceada
- **leastconn**: Mejor para conexiones de larga duración
- **source**: Mejor para mantener sesiones

**Impacto en performance:**
Elegir el algoritmo correcto puede mejorar la distribución y rendimiento en un 10-30% según el caso de uso.

---

### HAProxy rate limiting

**Cómo funciona:**
Rate limiting en HAProxy controla la tasa de conexiones, requests y ancho de banda, previniendo abuso y mejorando la estabilidad.

**Ventajas:**
- Previene abuso
- Mejor estabilidad
- Protege recursos
- Mejor experiencia de usuario

**Desventajas:**
- Puede rechazar requests válidos
- Requiere configuración

**Cuándo usar:**
- APIs públicas
- Sistemas con riesgo de abuso
- Aplicaciones de alto rendimiento
- Cuando se necesita proteger recursos

**Impacto en performance:**
Puede prevenir degradación del rendimiento al limitar abuso. El impacto es en estabilidad.

---

### HAProxy content switching

**Cómo funciona:**
Content switching en HAProxy enruta requests basándose en contenido (URL, header, path, host), proporcionando flexibilidad.

**Ventajas:**
- Flexibilidad en routing
- Mejor para sistemas complejos
- Optimización según necesidades
- Mejor control

**Desventajas:**
- Más complejo
- Requiere configuración
- Puede afectar rendimiento si es muy complejo

**Cuándo usar:**
- Routing complejo
- Sistemas que requieren routing basado en contenido
- Aplicaciones de alto rendimiento
- Cuando se necesita flexibilidad

**Impacto en performance:**
Puede proporcionar flexibilidad con overhead mínimo si se configura apropiadamente.

---

### HAProxy connection draining and graceful shutdown

**Cómo funciona:**
Connection draining y graceful shutdown en HAProxy permiten cerrar servidores gradualmente, permitiendo que requests en curso se completen.

**Ventajas:**
- Permite mantenimiento sin pérdida de requests
- Mejor experiencia de usuario
- Mejor para sistemas de producción
- Permite actualizaciones sin downtime

**Desventajas:**
- Requiere configuración
- Puede tomar tiempo

**Cuándo usar:**
- Mantenimiento de sistemas de producción
- Actualizaciones sin downtime
- Sistemas críticos
- Cuando se necesita alta disponibilidad

**Impacto en performance:**
Permite mantenimiento sin afectar el rendimiento de requests en curso, mejorando la disponibilidad.

---

### HAProxy dynamic server weight adjustment

**Cómo funciona:**
Ajuste dinámico de peso de servidores en HAProxy permite balancear carga según capacidad de servidores.

**Ventajas:**
- Mejor balanceo de carga
- Optimización según capacidad
- Mejor utilización de recursos
- Mejor rendimiento

**Desventajas:**
- Requiere configuración
- Puede requerir monitoreo

**Cuándo usar:**
- Servidores con diferentes capacidades
- Cuando se necesita mejor balanceo
- Aplicaciones de alto rendimiento
- Sistemas que requieren optimización

**Impacto en performance:**
Puede mejorar el balanceo de carga y rendimiento en un 10-30% al optimizar según capacidad.

---

### HAProxy DNS-based service discovery

**Cómo funciona:**
DNS-based service discovery en HAProxy permite descubrir servidores dinámicamente mediante DNS, mejorando la flexibilidad.

**Ventajas:**
- Flexibilidad
- Descubrimiento dinámico
- Mejor para sistemas dinámicos
- Mejor escalabilidad

**Desventajas:**
- Requiere DNS
- Puede introducir latencia
- Más complejo

**Cuándo usar:**
- Sistemas dinámicos
- Cuando se necesita descubrimiento dinámico
- Aplicaciones de alto rendimiento
- Sistemas que requieren flexibilidad

**Impacto en performance:**
Puede introducir latencia mínima pero proporciona flexibilidad que puede mejorar la escalabilidad.

---

### HAProxy stats and monitoring

**Cómo funciona:**
Stats y monitoreo en HAProxy proporcionan métricas de rendimiento, permitiendo identificación de problemas y optimización.

**Ventajas:**
- Identifica problemas
- Permite optimización
- Mejor visibilidad
- Mejor para sistemas críticos

**Desventajas:**
- Requiere configuración
- Requiere monitoreo

**Cuándo usar:**
- Siempre en sistemas de producción
- Sistemas críticos
- Aplicaciones de alto rendimiento
- Cuando se necesita visibilidad

**Impacto en performance:**
Permite identificar y resolver problemas que pueden mejorar el rendimiento significativamente.

---

## Logging and Observability

Esta sección cubre optimizaciones para logging y observabilidad sin degradar el rendimiento.

### Avoid logging in hot paths

**Cómo funciona:**
Logging puede ser costoso (formateo de strings, I/O). Evitar logging en hot paths mejora el rendimiento significativamente.

**Ventajas:**
- Mejor rendimiento
- Menos I/O
- Menos allocations

**Desventajas:**
- Menos visibilidad
- Requiere logging estratégico

**Cuándo usar:**
- Siempre en hot paths
- Código ejecutado millones de veces
- Cuando el logging es un cuello de botella

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% en hot paths con logging frecuente. El impacto es mayor cuando el logging está habilitado.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Logging en hot path
public void ProcessItem(Item item)
{
    _logger.LogInformation($"Processing item {item.Id}"); // Costoso
    Process(item);
}

// ✅ Bueno: Logging condicional o fuera del hot path
public void ProcessItem(Item item)
{
    if (_logger.IsEnabled(LogLevel.Information))
    {
        _logger.LogInformation("Processing item {ItemId}", item.Id);
    }
    Process(item);
}
```

---

### Structured logging

**Cómo funciona:**
Structured logging usa formatos estructurados (JSON) en lugar de strings formateadas, mejorando el rendimiento y facilitando el análisis.

**Ventajas:**
- Mejor rendimiento (menos formateo)
- Mejor para análisis
- Mejor para agregación
- Compatible con sistemas de logging modernos

**Desventajas:**
- Menos legible directamente
- Requiere herramientas de análisis

**Cuándo usar:**
- Aplicaciones de producción
- Sistemas con mucho logging
- Cuando se necesita análisis de logs
- Aplicaciones distribuidas

**Impacto en performance:**
Puede mejorar el rendimiento de logging en un 20-50% al reducir el formateo de strings.

**Ejemplo en C#:**
```csharp
// ✅ Structured logging con Serilog
Log.Information("Processing item {ItemId} with value {Value}", item.Id, item.Value);
// En lugar de: Log.Information($"Processing item {item.Id} with value {item.Value}");
```

---

### Asynchronous logging

**Cómo funciona:**
Logging asíncrono escribe logs en background sin bloquear el thread principal, mejorando el rendimiento.

**Ventajas:**
- No bloquea threads
- Mejor rendimiento
- Mejor escalabilidad
- Mejor para alto tráfico

**Desventajas:**
- Puede perder logs en caso de crash
- Requiere configuración
- Más complejo

**Cuándo usar:**
- Siempre cuando sea posible
- Aplicaciones de alto tráfico
- Sistemas de alto rendimiento
- Cuando el logging es frecuente

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al no bloquear threads. El impacto es mayor con logging frecuente.

**Ejemplo en C#:**
```csharp
// ✅ Configurar logging asíncrono en Serilog
Log.Logger = new LoggerConfiguration()
    .WriteTo.Async(a => a.Console()) // Asíncrono
    .CreateLogger();
```

---

### Correct log levels

**Cómo funciona:**
Usar niveles de log apropiados (Debug, Info, Warning, Error) permite filtrar logs y mejorar el rendimiento.

**Ventajas:**
- Permite filtrar logs
- Mejor rendimiento
- Mejor para producción
- Mejor gestión

**Desventajas:**
- Requiere configuración apropiada
- Puede requerir ajuste

**Cuándo usar:**
- Siempre usar niveles apropiados
- Debug: desarrollo
- Info: información importante
- Warning: advertencias
- Error: errores

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al filtrar logs innecesarios. El impacto es mayor cuando hay muchos logs.

---

### Log sampling

**Cómo funciona:**
Log sampling registra solo un porcentaje de logs, reduciendo el volumen y mejorando el rendimiento.

**Ventajas:**
- Reduce volumen de logs
- Mejor rendimiento
- Mejor para alto tráfico
- Mejor gestión

**Desventajas:**
- Puede perder información
- Requiere configuración

**Cuándo usar:**
- Alto tráfico
- Cuando hay muchos logs
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar muestreo

**Impacto en performance:**
Puede mejorar el rendimiento en un 50-90% al reducir el volumen de logs. El impacto es mayor con muchos logs.

---

### Structured logging

**Cómo funciona:**
Structured logging registra logs en formato estructurado (JSON), permitiendo mejor análisis y mejor rendimiento.

**Ventajas:**
- Mejor análisis
- Mejor rendimiento
- Mejor para sistemas complejos
- Mejor búsqueda

**Desventajas:**
- Menos legible para humanos
- Requiere herramientas de análisis

**Cuándo usar:**
- Siempre cuando sea posible
- Sistemas complejos
- Aplicaciones de alto rendimiento
- Cuando se necesita análisis

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% y proporcionar mejor análisis. El impacto es mayor con muchos logs.

**Ejemplo en C#:**
```csharp
// ✅ Structured logging con Serilog
_logger.LogInformation("Processing item {ItemId} at {Timestamp}", itemId, DateTime.UtcNow);
```

---

### Avoid string interpolation when logging is disabled

**Cómo funciona:**
Evitar string interpolation cuando el logging está deshabilitado previene formateo innecesario y mejora el rendimiento.

**Ventajas:**
- Evita formateo innecesario
- Mejor rendimiento
- Menos allocations
- Mejor para hot paths

**Desventajas:**
- Requiere verificar si está habilitado
- Puede ser menos conveniente

**Cuándo usar:**
- Siempre cuando sea posible
- Hot paths con logging
- Cuando el logging puede estar deshabilitado
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al evitar formateo innecesario. El impacto es mayor cuando el logging está deshabilitado.

**Ejemplo en C#:**
```csharp
// ❌ Malo: String interpolation siempre
_logger.LogInformation($"Processing item {item.Id}"); // Siempre formatea

// ✅ Bueno: Solo formatea si está habilitado
if (_logger.IsEnabled(LogLevel.Information))
{
    _logger.LogInformation("Processing item {ItemId}", item.Id);
}
```

---

### Batch log writes

**Cómo funciona:**
Agrupar escrituras de logs en batches reduce el número de operaciones de I/O y mejora el rendimiento.

**Ventajas:**
- Menos operaciones de I/O
- Mejor rendimiento
- Mejor throughput
- Menor overhead

**Desventajas:**
- Puede aumentar latencia
- Requiere batching logic

**Cuándo usar:**
- Cuando hay muchos logs
- Aplicaciones de alto rendimiento
- Sistemas de alto tráfico
- Cuando el rendimiento es crítico

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-50x al reducir operaciones de I/O. El impacto es mayor con muchos logs.

---

### Prefer metrics over logs

**Cómo funciona:**
Usar métricas en lugar de logs para monitoreo proporciona mejor rendimiento y mejor análisis.

**Ventajas:**
- Mejor rendimiento
- Mejor análisis
- Mejor para monitoreo
- Menor volumen de datos

**Desventajas:**
- Menos detalle que logs
- Requiere herramientas de métricas

**Cuándo usar:**
- Monitoreo y observabilidad
- Cuando se necesita mejor rendimiento
- Aplicaciones de alto rendimiento
- Sistemas que requieren métricas

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x comparado con logs para monitoreo. El impacto es dramático.

---

### Distributed tracing

**Cómo funciona:**
Distributed tracing rastrea requests a través de múltiples servicios, permitiendo identificar cuellos de botella.

**Ventajas:**
- Identifica cuellos de botella
- Mejor visibilidad
- Mejor para microservicios
- Permite optimización

**Desventajas:**
- Requiere infraestructura
- Overhead adicional
- Más complejo

**Cuándo usar:**
- Microservicios
- Sistemas distribuidos
- Cuando se necesita visibilidad
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Tiene overhead mínimo (1-5%) pero permite identificar problemas que pueden mejorar el rendimiento en un 10-100x.

---

### Continuous profiling

**Cómo funciona:**
Continuous profiling perfila aplicaciones continuamente en producción, permitiendo identificación de problemas de rendimiento.

**Ventajas:**
- Identifica problemas en producción
- Mejor visibilidad
- Permite optimización
- Mejor para sistemas críticos

**Desventajas:**
- Requiere infraestructura
- Overhead adicional
- Más complejo

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita visibilidad continua
- Sistemas de alto rendimiento

**Impacto en performance:**
Tiene overhead mínimo (1-5%) pero permite identificar problemas que pueden mejorar el rendimiento en un 10-100x.

---

### APM tools (Application Performance Monitoring)

**Cómo funciona:**
APM tools monitorean el rendimiento de aplicaciones, proporcionando visibilidad y alertas.

**Ventajas:**
- Mejor visibilidad
- Alertas automáticas
- Mejor para sistemas críticos
- Permite optimización

**Desventajas:**
- Requiere infraestructura
- Costo adicional
- Overhead adicional

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita visibilidad
- Sistemas de alto rendimiento

**Impacto en performance:**
Tiene overhead mínimo (1-5%) pero permite identificar problemas que pueden mejorar el rendimiento en un 10-100x.

---

### Real-time alerting

**Cómo funciona:**
Alertas en tiempo real notifican sobre problemas de rendimiento inmediatamente, permitiendo acción rápida.

**Ventajas:**
- Acción rápida
- Mejor para sistemas críticos
- Previene problemas mayores
- Mejor experiencia de usuario

**Desventajas:**
- Requiere configuración
- Puede causar alert fatigue

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita acción rápida
- Sistemas que requieren alta disponibilidad

**Impacto en performance:**
Permite acción rápida que puede prevenir degradación del rendimiento. El impacto es en estabilidad.

---

### Log aggregation (ELK stack, Splunk)

**Cómo funciona:**
Log aggregation agrega logs de múltiples fuentes, permitiendo análisis centralizado y mejor visibilidad.

**Ventajas:**
- Análisis centralizado
- Mejor visibilidad
- Mejor para sistemas complejos
- Permite búsqueda

**Desventajas:**
- Requiere infraestructura
- Costo adicional
- Más complejo

**Cuándo usar:**
- Sistemas complejos
- Múltiples servicios
- Cuando se necesita análisis centralizado
- Aplicaciones de producción

**Impacto en performance:**
No afecta el rendimiento directamente pero permite análisis que puede identificar problemas de rendimiento.

---

### Metrics collection (Prometheus, Datadog)

**Cómo funciona:**
Collection de métricas agrega métricas de múltiples fuentes, permitiendo análisis y alertas.

**Ventajas:**
- Análisis centralizado
- Mejor visibilidad
- Permite alertas
- Mejor para sistemas complejos

**Desventajas:**
- Requiere infraestructura
- Costo adicional
- Más complejo

**Cuándo usar:**
- Sistemas complejos
- Múltiples servicios
- Cuando se necesita análisis de métricas
- Aplicaciones de producción

**Impacto en performance:**
Tiene overhead mínimo pero permite análisis que puede identificar problemas de rendimiento.

---

### Error tracking (Sentry, Rollbar)

**Cómo funciona:**
Error tracking rastrea y agrega errores, permitiendo identificación y resolución rápida de problemas.

**Ventajas:**
- Identificación rápida de errores
- Mejor visibilidad
- Mejor para sistemas críticos
- Permite acción rápida

**Desventajas:**
- Requiere infraestructura
- Costo adicional
- Overhead adicional

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita tracking de errores
- Sistemas que requieren alta disponibilidad

**Impacto en performance:**
Tiene overhead mínimo pero permite identificación rápida de errores que pueden afectar el rendimiento.

---

### Performance dashboards

**Cómo funciona:**
Dashboards de rendimiento visualizan métricas de rendimiento, permitiendo monitoreo y análisis.

**Ventajas:**
- Mejor visibilidad
- Permite análisis
- Mejor para sistemas críticos
- Permite identificación de problemas

**Desventajas:**
- Requiere configuración
- Requiere herramientas

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita visibilidad
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificación de problemas que pueden mejorar el rendimiento significativamente.

---

### Service level indicators (SLIs)

**Cómo funciona:**
SLIs miden aspectos específicos del servicio (latencia, disponibilidad, etc.), proporcionando métricas objetivas.

**Ventajas:**
- Métricas objetivas
- Mejor para sistemas críticos
- Permite SLOs
- Mejor gestión

**Desventajas:**
- Requiere definición
- Requiere monitoreo

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita métricas objetivas
- Sistemas que requieren SLOs

**Impacto en performance:**
Permite medición objetiva que puede guiar optimizaciones que mejoran el rendimiento.

---

### Service level objectives (SLOs)

**Cómo funciona:**
SLOs definen objetivos de rendimiento (ej: 99.9% de requests bajo 200ms), proporcionando objetivos medibles.

**Ventajas:**
- Objetivos medibles
- Mejor para sistemas críticos
- Permite gestión
- Mejor experiencia de usuario

**Desventajas:**
- Requiere definición
- Requiere monitoreo

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones de producción
- Cuando se necesita objetivos medibles
- Sistemas que requieren garantías

**Impacto en performance:**
Permite objetivos medibles que pueden guiar optimizaciones que mejoran el rendimiento.

---

### Service level agreements (SLAs)

**Cómo funciona:**
SLAs definen acuerdos de nivel de servicio con clientes, proporcionando garantías contractuales.

**Ventajas:**
- Garantías contractuales
- Mejor para sistemas críticos
- Mejor gestión
- Mejor experiencia de usuario

**Desventajas:**
- Requiere definición
- Requiere cumplimiento
- Puede tener consecuencias

**Cuándo usar:**
- Sistemas críticos
- Aplicaciones con clientes
- Cuando se necesita garantías contractuales
- Sistemas que requieren SLAs

**Impacto en performance:**
Proporciona garantías que pueden guiar optimizaciones que mejoran el rendimiento.

---

## Media and Content Optimization

Esta sección cubre optimizaciones para contenido multimedia y assets web.

### Image optimization and compression

**Cómo funciona:**
Comprimir imágenes reduce el tamaño de archivo sin pérdida significativa de calidad visual, mejorando los tiempos de carga.

**Ventajas:**
- Menor tamaño de archivo
- Mejor tiempos de carga
- Menor uso de ancho de banda
- Mejor experiencia de usuario

**Desventajas:**
- Puede requerir procesamiento
- Puede requerir herramientas

**Cuándo usar:**
- Aplicaciones web
- Cuando las imágenes son grandes
- Aplicaciones móviles
- Cuando el ancho de banda es limitado

**Impacto en performance:**
Puede reducir el tamaño de imágenes en un 50-90% y mejorar los tiempos de carga en un 2-10x dependiendo del tamaño original.

---

### Use WebP and AVIF image formats

**Cómo funciona:**
WebP y AVIF son formatos de imagen modernos que proporcionan mejor compresión que JPEG/PNG tradicionales.

**Ventajas:**
- Mejor compresión (30-50% más pequeño)
- Mejor calidad
- Soporte moderno

**Desventajas:**
- Soporte de navegador limitado (aunque mejorando)
- Puede requerir fallbacks

**Cuándo usar:**
- Aplicaciones web modernas
- Cuando el tamaño es crítico
- Aplicaciones que pueden usar formatos modernos

**Impacto en performance:**
Puede reducir el tamaño de imágenes en un 30-50% comparado con JPEG/PNG, mejorando los tiempos de carga significativamente.

---

### Lazy loading for images

**Cómo funciona:**
Lazy loading carga imágenes solo cuando son visibles o cerca de ser visibles, reduciendo carga inicial y mejorando el rendimiento.

**Ventajas:**
- Reduce carga inicial
- Mejor rendimiento inicial
- Mejor experiencia de usuario
- Mejor para páginas con muchas imágenes

**Desventajas:**
- Puede causar carga cuando se hace scroll
- Requiere implementación

**Cuándo usar:**
- Páginas con muchas imágenes
- Cuando se necesita mejor rendimiento inicial
- Aplicaciones web
- Cuando las imágenes no son críticas inicialmente

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al reducir la carga inicial. El impacto es mayor con muchas imágenes.

---

### Responsive images (srcset)

**Cómo funciona:**
Responsive images (srcset) sirven diferentes tamaños de imágenes según el dispositivo, reduciendo el tamaño de datos transferidos.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento
- Mejor experiencia de usuario
- Mejor para dispositivos móviles

**Desventajas:**
- Requiere múltiples versiones de imágenes
- Requiere implementación

**Cuándo usar:**
- Aplicaciones web
- Cuando se necesita optimizar para diferentes dispositivos
- Aplicaciones de alto rendimiento
- Cuando el tamaño de datos es importante

**Impacto en performance:**
Puede reducir el tamaño de datos en un 50-80% para dispositivos móviles, mejorando el rendimiento significativamente.

---

### Image CDN optimization

**Cómo funciona:**
Image CDN optimiza y sirve imágenes desde servidores cercanos a usuarios, reduciendo latencia y mejorando el rendimiento.

**Ventajas:**
- Menor latencia
- Mejor rendimiento
- Optimización automática
- Mejor experiencia de usuario global

**Desventajas:**
- Requiere infraestructura adicional
- Costo adicional

**Cuándo usar:**
- Aplicaciones con muchas imágenes
- Aplicaciones globales
- Cuando la latencia es crítica
- Aplicaciones de alto tráfico

**Impacto en performance:**
Puede reducir la latencia en un 50-90% para usuarios lejos del servidor origen. El impacto es dramático.

---

### Video streaming optimization

**Cómo funciona:**
Optimizar video streaming (codecs, bitrate, etc.) mejora el rendimiento y experiencia de usuario para videos.

**Ventajas:**
- Mejor rendimiento
- Mejor experiencia de usuario
- Menor uso de ancho de banda
- Optimización según necesidades

**Desventajas:**
- Requiere conocimiento de video
- Puede requerir múltiples versiones

**Cuándo usar:**
- Aplicaciones con video
- Cuando se necesita optimizar video
- Aplicaciones de alto rendimiento
- Cuando el ancho de banda es limitado

**Impacto en performance:**
Puede mejorar el rendimiento y reducir el uso de ancho de banda en un 20-50% según la optimización.

---

### Video codec selection

**Cómo funciona:**
Diferentes codecs de video (H.264, H.265, VP9, AV1) tienen diferentes características de compresión y rendimiento.

**Ventajas:**
- Optimización según necesidades
- Mejor compresión
- Mejor rendimiento
- Flexibilidad

**Desventajas:**
- Requiere conocimiento de codecs
- Puede requerir múltiples versiones

**Cuándo usar:**
- **H.264**: Compatibilidad amplia, compresión moderada
- **H.265**: Mejor compresión, requiere más CPU
- **VP9**: Mejor compresión, menos compatible
- **AV1**: Mejor compresión, menos compatible, requiere más CPU

**Impacto en performance:**
Puede reducir el tamaño de video en un 30-50% con codecs más modernos, mejorando el rendimiento de transferencia.

---

### Adaptive bitrate streaming

**Cómo funciona:**
Adaptive bitrate streaming ajusta la calidad de video según el ancho de banda disponible, mejorando la experiencia de usuario.

**Ventajas:**
- Mejor experiencia de usuario
- Mejor para diferentes anchos de banda
- Reduce buffering
- Mejor rendimiento

**Desventajas:**
- Requiere múltiples versiones
- Más complejo

**Cuándo usar:**
- Video streaming
- Cuando se necesita mejor experiencia de usuario
- Aplicaciones de alto rendimiento
- Cuando hay diferentes anchos de banda

**Impacto en performance:**
Puede mejorar la experiencia de usuario significativamente al reducir buffering y adaptarse al ancho de banda.

---

### Video thumbnail generation

**Cómo funciona:**
Generar thumbnails de videos permite mostrar previews sin cargar videos completos, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Mejor experiencia de usuario
- Reduce carga
- Mejor para listas de videos

**Desventajas:**
- Requiere generación
- Requiere almacenamiento

**Cuándo usar:**
- Aplicaciones con videos
- Cuando se muestran listas de videos
- Aplicaciones de alto rendimiento
- Cuando se necesita mejor rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100x al evitar cargar videos completos para previews. El impacto es dramático.

---

### Audio compression

**Cómo funciona:**
Comprimir audio reduce el tamaño de archivos de audio, mejorando el rendimiento de transferencia.

**Ventajas:**
- Reduce tamaño de datos
- Mejor rendimiento de transferencia
- Menor uso de ancho de banda
- Mejor experiencia de usuario

**Desventajas:**
- Puede afectar calidad
- Requiere configuración

**Cuándo usar:**
- Aplicaciones con audio
- Cuando el tamaño es importante
- Aplicaciones de alto rendimiento
- Cuando se puede aceptar compresión

**Impacto en performance:**
Puede reducir el tamaño en un 50-90% según el codec, mejorando el rendimiento de transferencia significativamente.

---

### Font optimization and subsetting

**Cómo funciona:**
Optimizar y subsetting de fuentes reduce el tamaño de archivos de fuentes, mejorando el rendimiento de carga.

**Ventajas:**
- Reduce tamaño de fuentes
- Mejor rendimiento de carga
- Menor uso de ancho de banda
- Mejor experiencia de usuario

**Desventajas:**
- Requiere procesamiento
- Puede limitar caracteres disponibles

**Cuándo usar:**
- Aplicaciones web
- Cuando se usan fuentes personalizadas
- Aplicaciones de alto rendimiento
- Cuando el tamaño es importante

**Impacto en performance:**
Puede reducir el tamaño de fuentes en un 50-90%, mejorando el rendimiento de carga significativamente.

---

### Web font loading strategies

**Cómo funciona:**
Diferentes estrategias de carga de fuentes web (font-display, preload, etc.) mejoran el rendimiento y experiencia de usuario.

**Ventajas:**
- Mejor rendimiento
- Mejor experiencia de usuario
- Optimización según necesidades
- Mejor para diferentes casos

**Desventajas:**
- Requiere configuración
- Puede requerir conocimiento

**Cuándo usar:**
- Aplicaciones web con fuentes personalizadas
- Cuando se necesita mejor rendimiento
- Aplicaciones de alto rendimiento
- Cuando la experiencia de usuario es crítica

**Impacto en performance:**
Puede mejorar el rendimiento y experiencia de usuario significativamente al optimizar la carga de fuentes.

---

### CSS minification

**Cómo funciona:**
Minificar CSS elimina espacios y comentarios, reduciendo el tamaño de archivos CSS y mejorando el rendimiento.

**Ventajas:**
- Reduce tamaño de CSS
- Mejor rendimiento de carga
- Menor uso de ancho de banda
- Automático en la mayoría de casos

**Desventajas:**
- Menos legible
- Requiere herramientas

**Cuándo usar:**
- Siempre en producción
- Aplicaciones web
- Cuando se necesita optimizar tamaño
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede reducir el tamaño de CSS en un 20-50%, mejorando el rendimiento de carga.

---

### JavaScript minification and bundling

**Cómo funciona:**
Minificar y bundling de JavaScript reduce el tamaño y número de archivos, mejorando el rendimiento de carga.

**Ventajas:**
- Reduce tamaño de JavaScript
- Menos requests
- Mejor rendimiento de carga
- Menor uso de ancho de banda

**Desventajas:**
- Menos legible
- Requiere herramientas

**Cuándo usar:**
- Siempre en producción
- Aplicaciones web
- Cuando se necesita optimizar tamaño
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede reducir el tamaño en un 30-70% y el número de requests, mejorando el rendimiento de carga significativamente.

---

### Tree shaking for JavaScript

**Cómo funciona:**
Tree shaking elimina código no usado de bundles de JavaScript, reduciendo el tamaño y mejorando el rendimiento.

**Ventajas:**
- Reduce tamaño de JavaScript
- Mejor rendimiento de carga
- Menor uso de ancho de banda
- Mejor para aplicaciones modernas

**Desventajas:**
- Requiere herramientas modernas
- Puede requerir configuración

**Cuándo usar:**
- Aplicaciones modernas
- Cuando se usan módulos ES6
- Aplicaciones de alto rendimiento
- Cuando se necesita optimizar tamaño

**Impacto en performance:**
Puede reducir el tamaño en un 20-60% al eliminar código no usado, mejorando el rendimiento de carga.

---

### Code splitting for JavaScript

**Cómo funciona:**
Code splitting divide JavaScript en chunks más pequeños cargados bajo demanda, mejorando el rendimiento inicial.

**Ventajas:**
- Mejor rendimiento inicial
- Carga bajo demanda
- Mejor experiencia de usuario
- Mejor para aplicaciones grandes

**Desventajas:**
- Más complejo
- Requiere configuración

**Cuándo usar:**
- Aplicaciones grandes
- Cuando se necesita mejor rendimiento inicial
- Aplicaciones de alto rendimiento
- Cuando se puede cargar código bajo demanda

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al reducir el tamaño inicial. El impacto es mayor con aplicaciones grandes.

---

### Asset versioning and cache busting

**Cómo funciona:**
Versionado de assets y cache busting permite cachear assets mientras se actualizan cuando cambian, mejorando el rendimiento.

**Ventajas:**
- Mejor caching
- Mejor rendimiento
- Mejor experiencia de usuario
- Mejor para assets estáticos

**Desventajas:**
- Requiere gestión de versiones
- Puede requerir configuración

**Cuándo usar:**
- Siempre para assets estáticos
- Aplicaciones web
- Cuando se necesita mejor caching
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x para assets cacheados. El impacto es dramático.

---

## Compilation and Code Generation

Esta sección cubre optimizaciones a nivel de compilación.

### Profile-Guided Optimization (PGO)

**Cómo funciona:**
PGO usa datos de ejecución real para guiar optimizaciones del compilador, mejorando el rendimiento del código generado.

**Ventajas:**
- Mejor optimización basada en datos reales
- Mejor rendimiento del código generado
- Optimizaciones más efectivas

**Desventajas:**
- Requiere ejecución de perfilado
- Proceso de dos pasos
- Puede ser complejo

**Cuándo usar:**
- Aplicaciones de alto rendimiento
- Cuando se necesita máximo rendimiento
- Aplicaciones con patrones de ejecución conocidos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% mediante optimizaciones más efectivas basadas en datos reales.

---

### Ahead-of-Time (AOT) compilation

**Cómo funciona:**
AOT compilation compila código a código nativo antes de ejecución, reduciendo overhead de JIT y mejorando el rendimiento inicial.

**Ventajas:**
- Mejor rendimiento inicial
- Menor overhead
- Mejor para aplicaciones que requieren inicio rápido
- Mejor para algunos casos de uso

**Desventajas:**
- Tamaño de binario mayor
- Menos flexible
- Requiere .NET Native o similar

**Cuándo usar:**
- Aplicaciones que requieren inicio rápido
- Cuando se necesita máximo rendimiento inicial
- Aplicaciones móviles
- Sistemas embebidos

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 20-50% al eliminar overhead de JIT. El impacto es mayor para inicio rápido.

---

### Link-Time Optimization (LTO)

**Cómo funciona:**
LTO optimiza código a través de módulos durante el linking, permitiendo optimizaciones que no son posibles a nivel de módulo individual.

**Ventajas:**
- Mejores optimizaciones
- Mejor rendimiento
- Optimización cross-module
- Mejor para aplicaciones grandes

**Desventajas:**
- Tiempo de compilación mayor
- Requiere configuración

**Cuándo usar:**
- Aplicaciones grandes
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento
- Cuando se puede aceptar tiempo de compilación mayor

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% al permitir optimizaciones cross-module. El impacto depende de la aplicación.

---

### Interprocedural Optimization (IPO)

**Cómo funciona:**
IPO optimiza código a través de boundaries de funciones, permitiendo optimizaciones que no son posibles a nivel de función individual.

**Ventajas:**
- Mejores optimizaciones
- Mejor rendimiento
- Optimización cross-function
- Mejor para aplicaciones grandes

**Desventajas:**
- Tiempo de compilación mayor
- Requiere configuración

**Cuándo usar:**
- Aplicaciones grandes
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento
- Cuando se puede aceptar tiempo de compilación mayor

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% al permitir optimizaciones cross-function. El impacto depende de la aplicación.

---

### Loop unrolling by compiler

**Cómo funciona:**
Loop unrolling duplica el cuerpo de loops para reducir overhead de iteración, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menos overhead de iteración
- Automático en compiladores modernos
- Mejor para loops pequeños

**Desventajas:**
- Aumenta tamaño de código
- Puede no ser beneficioso para loops grandes

**Cuándo usar:**
- Siempre cuando sea beneficioso (automático)
- Loops pequeños ejecutados frecuentemente
- Hot paths con loops
- Cuando el compilador puede optimizar

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para loops pequeños. El impacto es mayor con loops ejecutados frecuentemente.

---

### Function inlining for hot paths

**Cómo funciona:**
Function inlining reemplaza llamadas de función con el cuerpo de la función, eliminando overhead de llamada.

**Ventajas:**
- Elimina overhead de llamada
- Mejor rendimiento
- Permite más optimizaciones
- Automático en compiladores modernos

**Desventajas:**
- Aumenta tamaño de código
- Puede no ser beneficioso para funciones grandes

**Cuándo usar:**
- Siempre cuando sea beneficioso (automático)
- Hot paths con llamadas de función
- Funciones pequeñas ejecutadas frecuentemente
- Cuando el compilador puede optimizar

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% para funciones pequeñas ejecutadas frecuentemente. El impacto es mayor en hot paths.

---

### Dead code elimination

**Cómo funciona:**
Dead code elimination elimina código que nunca se ejecuta, reduciendo el tamaño de binarios y mejorando el rendimiento.

**Ventajas:**
- Reduce tamaño de binarios
- Mejor rendimiento
- Automático en compiladores modernos
- Mejor para aplicaciones

**Desventajas:**
- Requiere análisis estático
- Puede no detectar todo el dead code

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Aplicaciones que requieren tamaño pequeño
- Sistemas embebidos
- Cuando se necesita optimizar tamaño

**Impacto en performance:**
Puede reducir el tamaño en un 10-30% y mejorar el rendimiento al reducir código innecesario.

---

### Constant folding and propagation

**Cómo funciona:**
Constant folding evalúa expresiones constantes en compile-time. Constant propagation reemplaza variables con valores constantes.

**Ventajas:**
- Mejor rendimiento
- Menos cálculos en runtime
- Automático en compiladores modernos
- Mejor para código con constantes

**Desventajas:**
- Requiere análisis estático
- Puede no detectar todas las constantes

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Código con muchas constantes
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% al eliminar cálculos en runtime. El impacto depende del código.

---

### Register allocation optimization

**Cómo funciona:**
Register allocation optimiza qué variables se almacenan en registros vs memoria, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Menos acceso a memoria
- Automático en compiladores modernos
- Mejor para hot paths

**Desventajas:**
- Requiere análisis complejo
- Limitado por número de registros

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Hot paths con muchas variables
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al reducir acceso a memoria. El impacto es mayor en hot paths.

---

### Instruction scheduling optimization

**Cómo funciona:**
Instruction scheduling reordena instrucciones para mejor utilización de pipeline de CPU, mejorando el rendimiento.

**Ventajas:**
- Mejor utilización de CPU
- Mejor rendimiento
- Automático en compiladores modernos
- Mejor para CPUs modernas

**Desventajas:**
- Requiere conocimiento de CPU
- Puede variar según CPU

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento
- CPUs modernas con pipelines complejos

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la utilización de pipeline. El impacto depende de la CPU.

---

### Vectorization by compiler

**Cómo funciona:**
Auto-vectorization convierte loops en instrucciones SIMD, procesando múltiples elementos simultáneamente.

**Ventajas:**
- Procesamiento paralelo a nivel de CPU
- Mejor rendimiento
- Automático en compiladores modernos
- Mejor para operaciones vectoriales

**Desventajas:**
- Requiere datos alineados
- Puede no ser beneficioso para todos los casos

**Cuándo usar:**
- Siempre cuando sea beneficioso (automático)
- Loops con operaciones vectoriales
- Cuando se puede usar SIMD
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 4-16x al procesar múltiples elementos simultáneamente. El impacto es dramático.

---

### Branch prediction hints

**Cómo funciona:**
Branch prediction hints ayudan al CPU a predecir branches, mejorando el rendimiento al reducir pipeline stalls.

**Ventajas:**
- Mejor predicción de branches
- Mejor rendimiento
- Mejor para branches frecuentes
- Mejor utilización de CPU

**Desventajas:**
- Específico de CPU
- Puede no ser beneficioso en todos los casos

**Cuándo usar:**
- Branches con patrones conocidos
- Cuando se conoce el patrón de branches
- Sistemas de alto rendimiento
- Cuando se necesita máximo rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para branches con patrones conocidos. El impacto depende del patrón.

---

### Compiler-specific optimizations

**Cómo funciona:**
Optimizaciones específicas de compilador (-O3, -march=native, etc.) habilitan optimizaciones agresivas, mejorando el rendimiento.

**Ventajas:**
- Mejores optimizaciones
- Mejor rendimiento
- Optimización según CPU
- Mejor para aplicaciones específicas

**Desventajas:**
- Puede aumentar tiempo de compilación
- Puede no ser portable
- Requiere configuración

**Cuándo usar:**
- Cuando se necesita máximo rendimiento
- Aplicaciones específicas para una plataforma
- Sistemas de alto rendimiento
- Cuando se puede aceptar tiempo de compilación mayor

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% según las optimizaciones. El impacto depende de la aplicación.

---

### Whole Program Optimization (WPO)

**Cómo funciona:**
WPO optimiza todo el programa como una unidad, permitiendo optimizaciones que no son posibles a nivel de módulo.

**Ventajas:**
- Mejores optimizaciones
- Mejor rendimiento
- Optimización global
- Mejor para aplicaciones

**Desventajas:**
- Tiempo de compilación mayor
- Requiere recompilar todo

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede aceptar tiempo de compilación mayor
- Sistemas de alto rendimiento
- Release builds

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al permitir optimizaciones globales. El impacto depende de la aplicación.

---

### Cross-module optimization

**Cómo funciona:**
Cross-module optimization optimiza código a través de módulos, permitiendo optimizaciones que no son posibles a nivel de módulo individual.

**Ventajas:**
- Mejores optimizaciones
- Mejor rendimiento
- Optimización cross-module
- Mejor para aplicaciones grandes

**Desventajas:**
- Tiempo de compilación mayor
- Requiere configuración

**Cuándo usar:**
- Aplicaciones grandes
- Cuando se necesita máximo rendimiento
- Sistemas de alto rendimiento
- Cuando se puede aceptar tiempo de compilación mayor

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% al permitir optimizaciones cross-module. El impacto depende de la aplicación.

---

### Binary optimization tools (BOLT)

**Cómo funciona:**
BOLT optimiza binarios basándose en perfiles de ejecución, reordenando código para mejor localidad de caché.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Optimización basada en perfiles
- Mejor para aplicaciones específicas

**Desventajas:**
- Requiere profiling
- Requiere herramientas adicionales
- Más complejo

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede hacer profiling
- Sistemas de alto rendimiento
- Cuando se necesita optimización extrema

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la localidad de caché. El impacto depende de la aplicación.

---

### Code layout optimization

**Cómo funciona:**
Code layout optimization reordena código para mejor localidad de caché de instrucciones, mejorando el rendimiento.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Mejor utilización de instruction cache
- Mejor para hot paths

**Desventajas:**
- Requiere análisis
- Puede requerir herramientas

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede analizar hot paths
- Sistemas de alto rendimiento
- Cuando se necesita optimización extrema

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la localidad de caché. El impacto es mayor en hot paths.

---

### Hot/cold code splitting

**Cómo funciona:**
Hot/cold code splitting separa código frecuentemente ejecutado (hot) de código raramente ejecutado (cold), mejorando la localidad de caché.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Mejor utilización de instruction cache
- Mejor para hot paths

**Desventajas:**
- Requiere análisis
- Puede requerir herramientas

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede identificar hot/cold code
- Sistemas de alto rendimiento
- Cuando se necesita optimización extrema

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la localidad de caché. El impacto es mayor en hot paths.

---

### Function reordering

**Cómo funciona:**
Function reordering reordena funciones basándose en frecuencia de llamadas, mejorando la localidad de caché.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Mejor utilización de instruction cache
- Mejor para funciones frecuentemente llamadas

**Desventajas:**
- Requiere análisis
- Puede requerir herramientas

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede analizar frecuencia de llamadas
- Sistemas de alto rendimiento
- Cuando se necesita optimización extrema

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la localidad de caché. El impacto es mayor con funciones frecuentemente llamadas.

---

### Basic block reordering

**Cómo funciona:**
Basic block reordering reordena bloques básicos dentro de funciones para mejor localidad de caché, mejorando el rendimiento.

**Ventajas:**
- Mejor localidad de caché
- Mejor rendimiento
- Mejor utilización de instruction cache
- Mejor para hot paths

**Desventajas:**
- Requiere análisis
- Puede requerir herramientas

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede analizar hot paths
- Sistemas de alto rendimiento
- Cuando se necesita optimización extrema

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al mejorar la localidad de caché. El impacto es mayor en hot paths.

---

### JIT compilation optimization

**Cómo funciona:**
Optimizar JIT compilation mejora el rendimiento de código compilado just-in-time, mejorando el rendimiento de aplicaciones .NET.

**Ventajas:**
- Mejor rendimiento de código JIT
- Optimización en runtime
- Mejor para aplicaciones .NET
- Automático en .NET

**Desventajas:**
- Overhead de compilación
- Puede requerir warmup

**Cuándo usar:**
- Siempre en .NET (automático)
- Aplicaciones .NET
- Cuando se necesita mejor rendimiento
- Sistemas de alto rendimiento

**Impacto en performance:**
JIT en .NET optimiza código automáticamente, mejorando el rendimiento en un 10-50% comparado con código no optimizado.

---

### JIT warmup strategies

**Cómo funciona:**
JIT warmup pre-compila código crítico antes de que se necesite, mejorando el rendimiento inicial.

**Ventajas:**
- Mejor rendimiento inicial
- Reduce latencia de primera ejecución
- Mejor experiencia de usuario
- Mejor para código crítico

**Desventajas:**
- Requiere recursos iniciales
- Puede compilar código no usado

**Cuándo usar:**
- Aplicaciones que requieren inicio rápido
- Código crítico
- Cuando la latencia inicial es importante
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento inicial en un 50-90% al tener código ya compilado. El impacto es mayor para inicio rápido.

---

### JIT code cache optimization

**Cómo funciona:**
Optimizar JIT code cache mejora el rendimiento al cachear código compilado, evitando re-compilación.

**Ventajas:**
- Evita re-compilación
- Mejor rendimiento
- Mejor para código ejecutado múltiples veces
- Automático en .NET

**Desventajas:**
- Usa memoria
- Puede requerir configuración

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Aplicaciones .NET
- Cuando se ejecuta código múltiples veces
- Sistemas de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-50% al evitar re-compilación. El impacto es mayor con código ejecutado múltiples veces.

---

### Native code generation optimization

**Cómo funciona:**
Optimizar generación de código nativo mejora el rendimiento de código compilado a nativo, mejorando el rendimiento.

**Ventajas:**
- Mejor rendimiento
- Código nativo optimizado
- Mejor para aplicaciones que requieren máximo rendimiento
- Mejor para casos específicos

**Desventajas:**
- Requiere compilación nativa
- Menos flexible
- Puede requerir más tiempo

**Cuándo usar:**
- Aplicaciones que requieren máximo rendimiento
- Cuando se puede compilar a nativo
- Sistemas embebidos
- Aplicaciones específicas

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-50% comparado con código interpretado o JIT. El impacto depende de la aplicación.

---

### Assembly code optimization

**Cómo funciona:**
Optimizar código assembly manualmente puede proporcionar máximo rendimiento para código crítico específico.

**Ventajas:**
- Máximo rendimiento
- Control total
- Mejor para código crítico específico
- Mejor para casos extremos

**Desventajas:**
- Muy complejo
- No portable
- Requiere conocimiento profundo
- Difícil de mantener

**Cuándo usar:**
- Solo para código crítico extremo
- Cuando el compilador no puede optimizar suficientemente
- Sistemas que requieren máximo rendimiento
- Con extrema precaución

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-100% para código crítico específico, pero generalmente no es necesario.

---

### Compiler intrinsics for SIMD

**Cómo funciona:**
Compiler intrinsics permiten usar instrucciones SIMD directamente, proporcionando máximo rendimiento para operaciones vectoriales.

**Ventajas:**
- Máximo rendimiento para SIMD
- Control directo sobre SIMD
- Mejor para operaciones vectoriales
- Mejor para casos específicos

**Desventajas:**
- Específico de CPU
- Más complejo
- Requiere conocimiento de SIMD

**Cuándo usar:**
- Operaciones vectoriales críticas
- Cuando se necesita máximo rendimiento para SIMD
- Sistemas de alto rendimiento
- Cuando el compilador no puede auto-vectorizar

**Impacto en performance:**
Puede mejorar el rendimiento en un 4-16x para operaciones vectoriales. El impacto es dramático.

**Ejemplo en C#:**
```csharp
// ✅ Usar System.Numerics para SIMD
using System.Numerics;

var vector1 = new Vector<int>(data1);
var vector2 = new Vector<int>(data2);
var result = Vector.Add(vector1, vector2); // SIMD automático
```

---

### Compiler-specific pragmas and hints

**Cómo funciona:**
Pragmas y hints específicos de compilador permiten guiar optimizaciones, mejorando el rendimiento para casos específicos.

**Ventajas:**
- Guía optimizaciones
- Mejor rendimiento para casos específicos
- Mejor control
- Mejor para optimización extrema

**Desventajas:**
- Específico de compilador
- Menos portable
- Requiere conocimiento

**Cuándo usar:**
- Casos específicos que requieren optimización extrema
- Cuando se conoce mejor que el compilador
- Sistemas de alto rendimiento
- Con precaución

**Impacto en performance:**
Puede mejorar el rendimiento en un 5-20% para casos específicos. El impacto depende del caso.

---

### Optimization flags per compilation unit

**Cómo funciona:**
Configurar flags de optimización por unidad de compilación permite optimizar diferentes partes de manera diferente.

**Ventajas:**
- Optimización específica
- Mejor rendimiento
- Mejor para aplicaciones grandes
- Flexibilidad

**Desventajas:**
- Requiere configuración
- Puede ser más complejo

**Cuándo usar:**
- Aplicaciones grandes con diferentes necesidades
- Cuando se necesita optimización específica
- Sistemas de alto rendimiento
- Cuando diferentes partes tienen diferentes necesidades

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% al optimizar específicamente. El impacto depende de la aplicación.

---

### Incremental compilation

**Cómo funciona:**
Incremental compilation compila solo partes cambiadas, reduciendo tiempo de compilación y mejorando productividad.

**Ventajas:**
- Tiempo de compilación menor
- Mejor productividad
- Mejor para desarrollo
- Automático en la mayoría de casos

**Desventajas:**
- Puede no detectar todos los cambios
- Puede requerir clean build ocasional

**Cuándo usar:**
- Siempre en desarrollo (automático)
- Cuando se necesita tiempo de compilación menor
- Desarrollo iterativo
- Cuando se necesita mejor productividad

**Impacto en performance:**
Puede reducir el tiempo de compilación en un 50-90% para cambios pequeños. El impacto es en productividad.

---

### Parallel compilation

**Cómo funciona:**
Parallel compilation compila múltiples archivos simultáneamente, reduciendo tiempo de compilación total.

**Ventajas:**
- Tiempo de compilación menor
- Mejor utilización de múltiples cores
- Mejor productividad
- Automático en la mayoría de casos

**Desventajas:**
- Requiere múltiples cores
- Puede usar más recursos

**Cuándo usar:**
- Siempre cuando sea posible (automático)
- Sistemas con múltiples cores
- Cuando se necesita tiempo de compilación menor
- Aplicaciones grandes

**Impacto en performance:**
Puede reducir el tiempo de compilación en un 2-10x dependiendo del número de cores. El impacto es dramático.

---

## Measurement and Optimization

Esta sección cubre herramientas y técnicas para medir y optimizar el rendimiento.

### Measure before optimizing

**Cómo funciona:**
Medir el rendimiento antes de optimizar permite identificar cuellos de botella reales y enfocar esfuerzos donde tendrán mayor impacto.

**Ventajas:**
- Enfoque en optimizaciones reales
- Evita optimizaciones prematuras
- Mejor ROI de optimizaciones
- Datos objetivos

**Desventajas:**
- Requiere herramientas
- Requiere tiempo

**Cuándo usar:**
- Siempre antes de optimizar
- Cuando hay problemas de rendimiento
- Para establecer baselines
- Para validar optimizaciones

**Impacto en performance:**
Medir primero permite identificar optimizaciones que pueden mejorar el rendimiento en un 10-1000x, mientras que optimizar sin medir puede no tener impacto.

---

### Use BenchmarkDotNet for benchmarking

**Cómo funciona:**
BenchmarkDotNet es una herramienta de benchmarking para .NET que proporciona mediciones precisas y reproducibles del rendimiento.

**Ventajas:**
- Mediciones precisas
- Reproducible
- Fácil de usar
- Integración con CI/CD

**Desventajas:**
- Requiere tiempo de ejecución
- Puede ser lento para muchos benchmarks

**Cuándo usar:**
- Comparar implementaciones
- Validar optimizaciones
- Establecer baselines
- CI/CD de rendimiento

**Impacto en performance:**
Permite identificar optimizaciones efectivas que pueden mejorar el rendimiento en un 10-100x.

**Ejemplo en C#:**
```csharp
using BenchmarkDotNet.Attributes;
using BenchmarkDotNet.Running;

[MemoryDiagnoser]
public class MyBenchmark
{
    [Benchmark]
    public void Method1() { /* ... */ }
    
    [Benchmark]
    public void Method2() { /* ... */ }
}

// Ejecutar: BenchmarkRunner.Run<MyBenchmark>();
```

---

### Measure before optimizing

**Cómo funciona:**
Medir antes de optimizar identifica cuellos de botella reales, evitando optimización prematura y mejorando la efectividad.

**Ventajas:**
- Identifica cuellos de botella reales
- Evita optimización prematura
- Mejor efectividad
- Mejor ROI

**Desventajas:**
- Requiere herramientas de medición
- Puede tomar tiempo

**Cuándo usar:**
- Siempre antes de optimizar
- Cuando se necesita identificar problemas
- Sistemas de alto rendimiento
- Cuando se necesita mejor efectividad

**Impacto en performance:**
Permite identificar problemas reales que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### CPU profiling

**Cómo funciona:**
CPU profiling identifica dónde se gasta tiempo de CPU, permitiendo identificar hot paths y cuellos de botella.

**Ventajas:**
- Identifica hot paths
- Identifica cuellos de botella
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Cuando se necesita identificar problemas de CPU
- Sistemas de alto rendimiento
- Después de medir
- Cuando el CPU es un cuello de botella

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Memory profiling

**Cómo funciona:**
Memory profiling identifica uso de memoria y allocations, permitiendo identificar memory leaks y optimizaciones.

**Ventajas:**
- Identifica memory leaks
- Identifica allocations excesivas
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Cuando se necesita identificar problemas de memoria
- Sistemas de alto rendimiento
- Después de medir
- Cuando la memoria es un problema

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### IO profiling

**Cómo funciona:**
IO profiling identifica operaciones de I/O costosas, permitiendo identificar cuellos de botella de I/O.

**Ventajas:**
- Identifica cuellos de botella de I/O
- Permite optimización dirigida
- Mejor efectividad
- Mejor para sistemas I/O intensivos

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Cuando se necesita identificar problemas de I/O
- Sistemas I/O intensivos
- Después de medir
- Cuando el I/O es un cuello de botella

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Benchmarking

**Cómo funciona:**
Benchmarking mide el rendimiento de código específico, permitiendo comparar diferentes implementaciones.

**Ventajas:**
- Compara implementaciones
- Mide mejoras
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere implementación
- Puede tomar tiempo

**Cuándo usar:**
- Cuando se necesita comparar implementaciones
- Sistemas de alto rendimiento
- Después de optimizar
- Cuando se necesita medir mejoras

**Impacto en performance:**
Permite comparar y optimizar que puede mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Load testing

**Cómo funciona:**
Load testing prueba sistemas bajo carga esperada, permitiendo identificar problemas de rendimiento y escalabilidad.

**Ventajas:**
- Identifica problemas bajo carga
- Identifica problemas de escalabilidad
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tomar tiempo
- Requiere infraestructura

**Cuándo usar:**
- Antes de producción
- Sistemas de alto rendimiento
- Cuando se necesita validar escalabilidad
- Sistemas críticos

**Impacto en performance:**
Permite identificar problemas que pueden prevenir degradación del rendimiento del 50-100%. El impacto es crítico.

---

### Stress testing

**Cómo funciona:**
Stress testing prueba sistemas más allá de carga esperada, permitiendo identificar límites y problemas de estabilidad.

**Ventajas:**
- Identifica límites
- Identifica problemas de estabilidad
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tomar tiempo
- Requiere infraestructura

**Cuándo usar:**
- Antes de producción
- Sistemas críticos
- Cuando se necesita identificar límites
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificar límites que pueden prevenir degradación del rendimiento. El impacto es en estabilidad.

---

### Flame graphs

**Cómo funciona:**
Flame graphs visualizan perfiles de CPU, permitiendo identificar hot paths visualmente.

**Ventajas:**
- Visualización clara
- Identifica hot paths fácilmente
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede requerir interpretación

**Cuándo usar:**
- Cuando se necesita visualizar perfiles
- Sistemas de alto rendimiento
- Después de profiling
- Cuando se necesita identificar hot paths

**Impacto en performance:**
Permite identificar hot paths que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Percentile metrics (P95, P99)

**Cómo funciona:**
Métricas de percentiles (P95, P99) miden latencia de la mayoría de requests, proporcionando mejor visibilidad que promedios.

**Ventajas:**
- Mejor visibilidad que promedios
- Identifica problemas de tail latency
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede requerir configuración

**Cuándo usar:**
- Siempre en sistemas de producción
- Sistemas críticos
- Cuando se necesita visibilidad de latencia
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificar problemas de tail latency que pueden mejorar la experiencia de usuario significativamente.

---

### Real traffic replay

**Cómo funciona:**
Real traffic replay reproduce tráfico real en sistemas de prueba, permitiendo pruebas más realistas.

**Ventajas:**
- Pruebas más realistas
- Identifica problemas reales
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tomar tiempo
- Requiere datos de tráfico

**Cuándo usar:**
- Antes de producción
- Sistemas críticos
- Cuando se necesita pruebas realistas
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificar problemas reales que pueden prevenir degradación del rendimiento. El impacto es crítico.

---

### Performance regression tests

**Cómo funciona:**
Performance regression tests detectan degradación de rendimiento automáticamente, permitiendo acción rápida.

**Ventajas:**
- Detecta degradación automáticamente
- Permite acción rápida
- Mejor para sistemas críticos
- Previene problemas mayores

**Desventajas:**
- Requiere implementación
- Puede requerir mantenimiento

**Cuándo usar:**
- Siempre en CI/CD
- Sistemas críticos
- Cuando se necesita detectar degradación
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite detectar degradación temprano que puede prevenir problemas mayores. El impacto es crítico.

---

### Performance budgets

**Cómo funciona:**
Performance budgets definen límites de rendimiento (ej: tamaño de bundle, tiempo de carga), previniendo degradación.

**Ventajas:**
- Previene degradación
- Mejor para sistemas críticos
- Permite acción proactiva
- Mejor gestión

**Desventajas:**
- Requiere definición
- Requiere monitoreo

**Cuándo usar:**
- Siempre en CI/CD
- Sistemas críticos
- Cuando se necesita prevenir degradación
- Sistemas de alto rendimiento

**Impacto en performance:**
Previene degradación que puede afectar el rendimiento significativamente. El impacto es en estabilidad.

---

### Continuous performance monitoring

**Cómo funciona:**
Monitoreo continuo de rendimiento monitorea métricas de rendimiento continuamente, permitiendo identificación temprana de problemas.

**Ventajas:**
- Identificación temprana de problemas
- Mejor para sistemas críticos
- Permite acción proactiva
- Mejor visibilidad

**Desventajas:**
- Requiere infraestructura
- Requiere configuración

**Cuándo usar:**
- Siempre en sistemas de producción
- Sistemas críticos
- Cuando se necesita visibilidad continua
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificación temprana que puede prevenir degradación del rendimiento. El impacto es crítico.

---

### Load testing tools (JMeter, Gatling, k6)

**Cómo funciona:**
Herramientas de load testing (JMeter, Gatling, k6) facilitan pruebas de carga, permitiendo identificación de problemas.

**Ventajas:**
- Facilita pruebas de carga
- Identifica problemas
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tomar tiempo
- Requiere configuración

**Cuándo usar:**
- Antes de producción
- Sistemas de alto rendimiento
- Cuando se necesita validar carga
- Sistemas críticos

**Impacto en performance:**
Permite identificar problemas que pueden prevenir degradación del rendimiento. El impacto es crítico.

---

### Profiling tools (perf, dotTrace, Visual Studio Profiler)

**Cómo funciona:**
Herramientas de profiling facilitan análisis de rendimiento, permitiendo identificación de cuellos de botella.

**Ventajas:**
- Facilita análisis
- Identifica cuellos de botella
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tener overhead
- Requiere conocimiento

**Cuándo usar:**
- Cuando se necesita identificar problemas
- Sistemas de alto rendimiento
- Después de medir
- Cuando se necesita análisis profundo

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Real User Monitoring (RUM)

**Cómo funciona:**
RUM monitorea rendimiento desde la perspectiva de usuarios reales, proporcionando métricas reales de experiencia.

**Ventajas:**
- Métricas reales de usuarios
- Mejor visibilidad
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere infraestructura
- Puede tener overhead
- Requiere configuración

**Cuándo usar:**
- Siempre en sistemas de producción
- Sistemas críticos
- Cuando se necesita métricas reales
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificar problemas reales que pueden mejorar la experiencia de usuario significativamente.

---

### Synthetic monitoring

**Cómo funciona:**
Synthetic monitoring simula usuarios, proporcionando métricas consistentes y alertas proactivas.

**Ventajas:**
- Métricas consistentes
- Alertas proactivas
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere infraestructura
- Puede no reflejar usuarios reales completamente

**Cuándo usar:**
- Sistemas críticos
- Cuando se necesita alertas proactivas
- Sistemas de alto rendimiento
- Cuando se necesita métricas consistentes

**Impacto en performance:**
Permite identificación temprana que puede prevenir degradación del rendimiento. El impacto es crítico.

---

### Performance testing in CI/CD

**Cómo funciona:**
Performance testing en CI/CD ejecuta pruebas de rendimiento automáticamente, previniendo degradación.

**Ventajas:**
- Previene degradación automáticamente
- Mejor para sistemas críticos
- Permite acción rápida
- Mejor gestión

**Desventajas:**
- Requiere implementación
- Puede aumentar tiempo de CI/CD

**Cuándo usar:**
- Siempre en CI/CD
- Sistemas críticos
- Cuando se necesita prevenir degradación
- Sistemas de alto rendimiento

**Impacto en performance:**
Previene degradación que puede afectar el rendimiento significativamente. El impacto es crítico.

---

### CPU flame graphs

**Cómo funciona:**
CPU flame graphs visualizan perfiles de CPU, permitiendo identificar hot paths visualmente.

**Ventajas:**
- Visualización clara
- Identifica hot paths fácilmente
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede requerir interpretación

**Cuándo usar:**
- Cuando se necesita visualizar perfiles de CPU
- Sistemas de alto rendimiento
- Después de profiling
- Cuando se necesita identificar hot paths

**Impacto en performance:**
Permite identificar hot paths que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Memory allocation profiling

**Cómo funciona:**
Memory allocation profiling identifica allocations de memoria, permitiendo identificar memory leaks y optimizaciones.

**Ventajas:**
- Identifica memory leaks
- Identifica allocations excesivas
- Permite optimización dirigida
- Mejor efectividad

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Cuando se necesita identificar problemas de memoria
- Sistemas de alto rendimiento
- Después de medir
- Cuando la memoria es un problema

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Lock contention profiling

**Cómo funciona:**
Lock contention profiling identifica contención de locks, permitiendo identificar problemas de concurrencia.

**Ventajas:**
- Identifica contención de locks
- Permite optimización dirigida
- Mejor efectividad
- Mejor para sistemas concurrentes

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Sistemas concurrentes
- Cuando hay problemas de rendimiento
- Sistemas de alto rendimiento
- Cuando se sospecha contención de locks

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### I/O wait profiling

**Cómo funciona:**
I/O wait profiling identifica tiempo esperando I/O, permitiendo identificar cuellos de botella de I/O.

**Ventajas:**
- Identifica cuellos de botella de I/O
- Permite optimización dirigida
- Mejor efectividad
- Mejor para sistemas I/O intensivos

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Sistemas I/O intensivos
- Cuando hay problemas de rendimiento
- Sistemas de alto rendimiento
- Cuando se sospecha I/O como cuello de botella

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Network latency profiling

**Cómo funciona:**
Network latency profiling identifica latencia de red, permitiendo identificar problemas de red.

**Ventajas:**
- Identifica problemas de red
- Permite optimización dirigida
- Mejor efectividad
- Mejor para sistemas distribuidos

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Sistemas distribuidos
- Cuando hay problemas de rendimiento
- Sistemas de alto rendimiento
- Cuando se sospecha red como problema

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Database query profiling

**Cómo funciona:**
Database query profiling identifica queries lentas, permitiendo optimización de queries.

**Ventajas:**
- Identifica queries lentas
- Permite optimización dirigida
- Mejor efectividad
- Mejor para sistemas con bases de datos

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Sistemas con bases de datos
- Cuando hay problemas de rendimiento
- Sistemas de alto rendimiento
- Cuando se sospecha base de datos como cuello de botella

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-1000x. El impacto es dramático.

---

### End-to-end latency tracing

**Cómo funciona:**
End-to-end latency tracing rastrea latencia a través de todo el sistema, permitiendo identificar cuellos de botella.

**Ventajas:**
- Identifica cuellos de botella end-to-end
- Permite optimización dirigida
- Mejor efectividad
- Mejor para sistemas complejos

**Desventajas:**
- Requiere herramientas
- Puede tener overhead

**Cuándo usar:**
- Sistemas complejos
- Cuando hay problemas de rendimiento
- Sistemas de alto rendimiento
- Cuando se necesita visibilidad end-to-end

**Impacto en performance:**
Permite identificar problemas que pueden mejorar el rendimiento en un 10-100x. El impacto es crítico.

---

### Performance regression detection

**Cómo funciona:**
Detección de regresión de rendimiento identifica degradación automáticamente, permitiendo acción rápida.

**Ventajas:**
- Detecta degradación automáticamente
- Permite acción rápida
- Mejor para sistemas críticos
- Previene problemas mayores

**Desventajas:**
- Requiere implementación
- Puede requerir mantenimiento

**Cuándo usar:**
- Siempre en CI/CD
- Sistemas críticos
- Cuando se necesita detectar degradación
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite detectar degradación temprano que puede prevenir problemas mayores. El impacto es crítico.

---

### Automated performance testing

**Cómo funciona:**
Pruebas de rendimiento automatizadas ejecutan pruebas automáticamente, previniendo degradación.

**Ventajas:**
- Previene degradación automáticamente
- Mejor para sistemas críticos
- Permite acción rápida
- Mejor gestión

**Desventajas:**
- Requiere implementación
- Puede requerir mantenimiento

**Cuándo usar:**
- Siempre en CI/CD
- Sistemas críticos
- Cuando se necesita prevenir degradación
- Sistemas de alto rendimiento

**Impacto en performance:**
Previene degradación que puede afectar el rendimiento significativamente. El impacto es crítico.

---

### Performance baseline establishment

**Cómo funciona:**
Establecer baseline de rendimiento proporciona punto de referencia, permitiendo comparación y detección de degradación.

**Ventajas:**
- Punto de referencia
- Permite comparación
- Permite detección de degradación
- Mejor gestión

**Desventajas:**
- Requiere establecimiento
- Puede requerir actualización

**Cuándo usar:**
- Siempre al inicio
- Sistemas críticos
- Cuando se necesita punto de referencia
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite comparación y detección que puede prevenir degradación. El impacto es crítico.

---

### Performance comparison across versions

**Cómo funciona:**
Comparar rendimiento entre versiones identifica mejoras o degradación, permitiendo optimización dirigida.

**Ventajas:**
- Identifica mejoras o degradación
- Permite optimización dirigida
- Mejor efectividad
- Mejor gestión

**Desventajas:**
- Requiere implementación
- Puede tomar tiempo

**Cuándo usar:**
- Después de cambios
- Sistemas críticos
- Cuando se necesita comparar versiones
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite identificar mejoras o degradación que puede guiar optimizaciones. El impacto es crítico.

---

### Performance improvement measurement

**Cómo funciona:**
Medir mejoras de rendimiento cuantifica el impacto de optimizaciones, permitiendo validación y ROI.

**Ventajas:**
- Cuantifica impacto
- Permite validación
- Permite ROI
- Mejor gestión

**Desventajas:**
- Requiere medición
- Puede tomar tiempo

**Cuándo usar:**
- Después de optimizar
- Sistemas críticos
- Cuando se necesita validar mejoras
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite validar mejoras que pueden guiar futuras optimizaciones. El impacto es en gestión.

---

### Performance ROI calculation

**Cómo funciona:**
Calcular ROI de optimizaciones de rendimiento cuantifica el valor de optimizaciones, permitiendo priorización.

**Ventajas:**
- Cuantifica valor
- Permite priorización
- Mejor gestión
- Mejor para decisiones

**Desventajas:**
- Requiere cálculo
- Puede ser subjetivo

**Cuándo usar:**
- Al priorizar optimizaciones
- Sistemas críticos
- Cuando se necesita justificar optimizaciones
- Sistemas de alto rendimiento

**Impacto en performance:**
Permite priorización que puede mejorar la efectividad de optimizaciones. El impacto es en gestión.

---

## Performance Anti Patterns

Esta sección cubre patrones y prácticas que deben evitarse porque degradan el rendimiento.

### Premature optimization

**Cómo funciona:**
Optimizar código antes de medir y entender dónde están los cuellos de botella reales puede llevar a código más complejo sin beneficios reales.

**Ventajas de evitarlo:**
- Código más simple y mantenible
- Enfoque en optimizaciones reales
- Mejor uso del tiempo

**Desventajas:**
- Puede requerir refactorización posterior

**Cuándo evitar:**
- Siempre medir primero
- Optimizar solo hot paths identificados
- Después de profiling

**Impacto en performance:**
Evitar premature optimization permite enfocarse en optimizaciones reales que pueden mejorar el rendimiento en un 10-100x, mientras que optimizaciones prematuras pueden no tener impacto o incluso empeorar el rendimiento.

---

### Blocking async code

**Cómo funciona:**
Bloquear código async (usando .Result, .Wait(), o GetAwaiter().GetResult()) puede causar deadlocks y degradar el rendimiento.

**Ventajas de evitarlo:**
- Evita deadlocks
- Mejor escalabilidad
- Mejor rendimiento

**Desventajas:**
- Requiere async en todo el stack

**Cuándo evitar:**
- Siempre evitar bloquear async
- Usar async/await correctamente
- En aplicaciones de servidor especialmente

**Impacto en performance:**
Evitar blocking puede prevenir deadlocks y mejorar el throughput en un 2-10x al permitir mejor utilización de threads.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Bloquear async
public string GetData(string url)
{
    return GetDataAsync(url).Result; // Puede causar deadlock
}

// ✅ Bueno: Usar async/await
public async Task<string> GetDataAsync(string url)
{
    return await GetDataAsync(url);
}
```

---

### Database queries in loops

**Cómo funciona:**
Hacer queries a la base de datos dentro de loops causa N+1 queries, degradando dramáticamente el rendimiento.

**Ventajas de evitarlo:**
- Mucho menos queries
- Mejor rendimiento
- Menor carga en la base de datos

**Desventajas:**
- Requiere cambios en el código
- Puede requerir eager loading o joins

**Cuándo evitar:**
- Siempre evitar queries en loops
- Usar batch queries o eager loading
- Cualquier ORM

**Impacto en performance:**
Evitar queries en loops puede mejorar el rendimiento en un 10-1000x dependiendo de cuántas queries se eliminen.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Query en loop
foreach (var order in orders)
{
    order.Items = await _context.OrderItems
        .Where(i => i.OrderId == order.Id)
        .ToListAsync(); // N+1 queries
}

// ✅ Bueno: Eager loading
var orders = await _context.Orders
    .Include(o => o.Items) // Una query
    .ToListAsync();
```

---

### Ignoring metrics

**Cómo funciona:**
No monitorear métricas de rendimiento hace imposible identificar problemas y optimizar efectivamente.

**Ventajas de evitarlo:**
- Identificación temprana de problemas
- Optimización basada en datos
- Mejor experiencia de usuario

**Desventajas:**
- Requiere infraestructura de monitoreo
- Requiere tiempo para configurar

**Cuándo evitar:**
- Siempre monitorear métricas clave
- Aplicaciones de producción
- Sistemas críticos

**Impacto en performance:**
Monitorear métricas permite identificar y resolver problemas que pueden mejorar el rendimiento en un 10-100x. Sin métricas, los problemas pueden pasar desapercibidos.

---

### Over-fetching data

**Cómo funciona:**
Obtener más datos de los necesarios (SELECT * o campos innecesarios) aumenta el uso de ancho de banda, memoria y tiempo de procesamiento.

**Ventajas de evitarlo:**
- Menor uso de ancho de banda
- Menor uso de memoria
- Mejor rendimiento
- Menos I/O

**Cuándo evitar:**
- Siempre seleccionar solo campos necesarios
- Evitar SELECT * en queries
- Proyecciones específicas en Entity Framework

**Impacto en performance:**
Puede mejorar el rendimiento en un 20-80% al reducir la cantidad de datos transferidos y procesados.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Over-fetching
var users = context.Users.ToList(); // Trae todos los campos

// ✅ Bueno: Solo campos necesarios
var userNames = context.Users.Select(u => u.Name).ToList();
```

---

### Under-fetching data (N+1 queries)

**Cómo funciona:**
Hacer múltiples queries cuando se podría hacer una (N+1 problem) causa muchos round-trips a la base de datos, degradando el rendimiento.

**Ventajas de evitarlo:**
- Menos round-trips
- Mejor rendimiento
- Menor latencia total

**Cuándo evitar:**
- Siempre usar Include() o proyecciones
- Batch queries cuando sea posible
- Usar Dapper con múltiples queries

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-1000x al reducir el número de queries. El impacto es dramático con muchos registros.

**Ejemplo en C#:**
```csharp
// ❌ Malo: N+1 queries
var orders = context.Orders.ToList();
foreach (var order in orders)
{
    order.Customer = context.Customers.Find(order.CustomerId); // Query por cada order
}

// ✅ Bueno: Una query con Include
var orders = context.Orders.Include(o => o.Customer).ToList();
```

---

### Ignoring connection pool limits

**Cómo funciona:**
No configurar apropiadamente los límites del connection pool puede causar agotamiento de conexiones o subutilización de recursos.

**Ventajas de evitarlo:**
- Mejor utilización de recursos
- Previene agotamiento de conexiones
- Mejor rendimiento
- Mejor escalabilidad

**Cuándo evitar:**
- Siempre configurar límites apropiados
- Monitorear uso del pool
- Ajustar según carga

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% cuando se agotan las conexiones. El impacto es crítico para estabilidad.

---

### Not monitoring production performance

**Cómo funciona:**
No monitorear el rendimiento en producción impide identificar problemas y optimizaciones necesarias.

**Ventajas de evitarlo:**
- Identificación temprana de problemas
- Optimización basada en datos
- Mejor experiencia de usuario

**Cuándo evitar:**
- Siempre monitorear métricas clave
- Aplicaciones de producción
- Sistemas críticos

**Impacto en performance:**
Monitorear métricas permite identificar y resolver problemas que pueden mejorar el rendimiento en un 10-100x. Sin métricas, los problemas pueden pasar desapercibidos.

---

### Optimizing without measuring

**Cómo funciona:**
Optimizar sin medir primero puede llevar a optimizar código que no es un cuello de botella, desperdiciando esfuerzo.

**Ventajas de evitarlo:**
- Enfoque en optimizaciones reales
- Mejor uso del tiempo
- Optimizaciones con mayor impacto

**Cuándo evitar:**
- Siempre medir primero
- Usar profiling tools
- Identificar hot paths

**Impacto en performance:**
Medir primero permite identificar optimizaciones que pueden mejorar el rendimiento en un 10-100x, mientras que optimizar sin medir puede no tener impacto.

---

### Ignoring database query plans

**Cómo funciona:**
No revisar query plans impide identificar problemas como table scans, índices faltantes, o estimaciones incorrectas.

**Ventajas de evitarlo:**
- Identifica problemas de rendimiento
- Detecta índices faltantes
- Optimiza queries

**Cuándo evitar:**
- Siempre revisar query plans para queries lentas
- Después de cambios en esquema
- Optimización de rendimiento

**Impacto en performance:**
Puede identificar optimizaciones que mejoran el rendimiento en un 10-1000x. El impacto es dramático cuando hay problemas en queries.

---

### Not using prepared statements

**Cómo funciona:**
No usar prepared statements causa re-parsing de queries en cada ejecución, degradando el rendimiento.

**Ventajas de evitarlo:**
- Mejor rendimiento
- Protección contra SQL injection
- Reutilización eficiente

**Cuándo evitar:**
- Siempre para queries repetidas
- Queries con parámetros
- Aplicaciones de alto rendimiento

**Impacto en performance:**
Puede mejorar el rendimiento en un 10-30% para queries repetidas. El impacto es mayor con muchas ejecuciones.

---

### Ignoring cache hit rates

**Cómo funciona:**
No monitorear cache hit rates impide identificar si el cache es efectivo o necesita ajustes.

**Ventajas de evitarlo:**
- Identifica efectividad del cache
- Permite optimizar TTL
- Mejora hit ratio

**Cuándo evitar:**
- Siempre monitorear hit rates
- Ajustar estrategia de cache según métricas
- Optimizar TTL basado en datos

**Impacto en performance:**
Optimizar cache basado en hit rates puede mejorar el rendimiento en un 20-50% al mejorar la efectividad del cache.

---

### Not setting appropriate timeouts

**Cómo funciona:**
No configurar timeouts apropiados puede causar que requests esperen indefinidamente, degradando el rendimiento y causando problemas de recursos.

**Ventajas de evitarlo:**
- Previene esperas indefinidas
- Mejor utilización de recursos
- Mejor experiencia de usuario
- Previene acumulación de requests

**Cuándo evitar:**
- Siempre configurar timeouts
- Timeouts apropiados para cada operación
- Timeouts escalonados (conexión, lectura, total)

**Impacto en performance:**
Puede prevenir degradación del rendimiento del 50-100% cuando hay problemas de red o servicios lentos. El impacto es crítico para estabilidad.

**Ejemplo en C#:**
```csharp
// ✅ Configurar timeouts apropiados
services.AddHttpClient("MyApi", client =>
{
    client.Timeout = TimeSpan.FromSeconds(30); // Timeout total
});

// ✅ Timeouts en base de datos
var connectionString = "Server=...;Connection Timeout=30;Command Timeout=60;";
```

---

### Ignoring memory leaks

**Cómo funciona:**
Memory leaks causan que la memoria crezca continuamente, eventualmente causando OutOfMemoryException y degradando el rendimiento.

**Ventajas de evitarlo:**
- Previene OutOfMemoryException
- Mejor rendimiento estable
- Mejor utilización de recursos

**Cuándo evitar:**
- Siempre identificar y corregir leaks
- Usar memory profilers
- Revisar event handlers y subscriptions

**Impacto en performance:**
Corregir memory leaks previene crashes y degradación del rendimiento. El impacto es crítico para estabilidad.

**Ejemplo en C#:**
```csharp
// ❌ Malo: Memory leak (event handler no removido)
public class BadMemoryLeak
{
    public void Subscribe()
    {
        SomeEvent += Handler; // Nunca se remueve
    }
}

// ✅ Bueno: Remover event handlers
public class GoodMemoryLeak
{
    public void Subscribe()
    {
        SomeEvent += Handler;
    }
    
    public void Unsubscribe()
    {
        SomeEvent -= Handler; // Remover cuando no se necesita
    }
}
```

---

### Not profiling production workloads

**Cómo funciona:**
No hacer profiling en producción impide identificar problemas reales que solo ocurren bajo carga real.

**Ventajas de evitarlo:**
- Identifica problemas reales
- Datos de producción reales
- Optimizaciones basadas en datos reales

**Cuándo evitar:**
- Profiling periódico en producción
- Continuous profiling
- Análisis de hot paths en producción

**Impacto en performance:**
Profiling en producción puede identificar problemas que mejoran el rendimiento en un 10-100x. El impacto es mayor con datos reales.

---

### Over-engineering solutions

**Cómo funciona:**
Sobrediseñar soluciones con complejidad innecesaria puede degradar el rendimiento y hacer el código más difícil de optimizar.

**Ventajas de evitarlo:**
- Código más simple
- Más fácil de optimizar
- Mejor rendimiento
- Más mantenible

**Cuándo evitar:**
- Mantener soluciones simples
- Agregar complejidad solo cuando sea necesario
- YAGNI (You Aren't Gonna Need It)

**Impacto en performance:**
Soluciones simples pueden ser más rápidas y fáciles de optimizar. La complejidad innecesaria puede degradar el rendimiento.

---

### Not considering network latency

**Cómo funciona:**
No considerar la latencia de red en el diseño puede llevar a arquitecturas con muchos round-trips, degradando el rendimiento.

**Ventajas de evitarlo:**
- Menos round-trips
- Mejor rendimiento
- Mejor experiencia de usuario
- Arquitecturas más eficientes

**Cuándo evitar:**
- Siempre considerar latencia de red
- Minimizar round-trips
- Batch requests cuando sea posible
- Usar CDN para contenido estático

**Impacto en performance:**
Considerar latencia de red puede mejorar el rendimiento en un 10-100x al reducir round-trips. El impacto es dramático con alta latencia.

---

### Ignoring serialization costs

**Cómo funciona:**
No considerar el costo de serialización puede degradar el rendimiento significativamente, especialmente con JSON y objetos grandes.

**Ventajas de evitarlo:**
- Mejor rendimiento
- Menor uso de CPU
- Menor uso de memoria

**Cuándo evitar:**
- Usar serialización eficiente (protobuf, MessagePack)
- Evitar serialización innecesaria
- Cachear resultados serializados

**Impacto en performance:**
Usar serialización eficiente puede mejorar el rendimiento en un 3-10x. El impacto es mayor con objetos grandes.

**Ejemplo en C#:**
```csharp
// ❌ Malo: JSON lento para objetos grandes
var json = JsonSerializer.Serialize(largeObject); // Lento

// ✅ Bueno: Protobuf más rápido
var bytes = largeObject.ToByteArray(); // Más rápido
```

---

### Not optimizing hot paths

**Cómo funciona:**
No optimizar código ejecutado frecuentemente (hot paths) desperdicia oportunidades de mejorar el rendimiento significativamente.

**Ventajas de evitarlo:**
- Mejor rendimiento general
- Mayor impacto de optimizaciones
- Mejor uso del tiempo

**Cuándo evitar:**
- Identificar hot paths con profiling
- Optimizar código ejecutado frecuentemente
- Enfocar esfuerzo en código crítico

**Impacto en performance:**
Optimizar hot paths puede mejorar el rendimiento general en un 10-100x. El impacto es dramático cuando se optimiza código ejecutado millones de veces.

---

## Conclusión

Este documento proporciona una guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#. Las técnicas están organizadas por categorías y cada una incluye:

- **Cómo funciona**: Explicación técnica
- **Ventajas y Desventajas**: Trade-offs
- **Cuándo usar**: Casos de uso apropiados
- **Impacto en performance**: Beneficios esperados
- **Ejemplos en C#**: Código práctico cuando es aplicable

### Priorización de Optimizaciones

1. **Medir primero**: Siempre usar profiling para identificar cuellos de botella reales
2. **Hot paths**: Enfocar optimizaciones en código ejecutado frecuentemente
3. **Allocations**: Reducir allocations en hot paths tiene alto impacto
4. **I/O**: Optimizar operaciones de I/O (base de datos, red, archivos)
5. **Caching**: Implementar caching estratégico
6. **Concurrencia**: Usar async/await y patrones concurrentes apropiados

### Recursos Adicionales

- [.NET Performance Best Practices](https://docs.microsoft.com/dotnet/fundamentals/performance/)
- [BenchmarkDotNet](https://benchmarkdotnet.org/) - Herramienta de benchmarking
- [PerfView](https://github.com/Microsoft/perfview) - Profiler de .NET
- [dotTrace](https://www.jetbrains.com/profiler/) - Profiler comercial

---

*Documento generado a partir de performance-checklist.txt. Última actualización: 2024*

---

