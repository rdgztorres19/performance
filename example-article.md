### Avoid frequent fsync calls

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