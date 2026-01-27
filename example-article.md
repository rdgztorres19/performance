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