### Memory prefetching

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