### Use memory pooling

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