### Avoid heap fragmentation

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