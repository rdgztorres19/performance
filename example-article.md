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