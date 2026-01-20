### Memory-mapped I/O for large file access

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