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