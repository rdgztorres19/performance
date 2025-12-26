# Guía de Tags para Artículos

## Sistema de Categorización

### Categorías Principales

Usa estas categorías principales como primer tag:

1. **Hardware & Operating System**
   - CPU, memoria, sistema operativo
   - Ejemplo: "Prefer fewer fast CPU cores..."

2. **Memory Management**
   - Gestión de memoria, allocation, garbage collection
   - Ejemplo: "Use object pooling..."

3. **Disk I/O**
   - Lectura/escritura de disco, archivos
   - Ejemplo: "Use async file I/O..."

4. **Networking**
   - HTTP, TCP, conexiones de red
   - Ejemplo: "Implement HTTP/2..."

5. **Databases**
   - Optimización de queries, índices, conexiones
   - Ejemplo: "Use connection pooling..."

6. **Caching**
   - Redis, Memcached, cache strategies
   - Ejemplo: "Implement Redis caching..."

7. **Concurrency**
   - Threading, async/await, paralelismo
   - Ejemplo: "Use async/await instead of blocking..."

8. **Data Structures**
   - Arrays, lists, dictionaries, collections
   - Ejemplo: "Prefer Dictionary over List for lookups..."

9. **Algorithms**
   - Algoritmos de búsqueda, ordenamiento, complejidad
   - Ejemplo: "Use binary search for sorted arrays..."

10. **System Design**
    - Arquitectura, microservicios, escalabilidad
    - Ejemplo: "Implement load balancing..."

11. **.NET/C# Performance**
    - Específico de C# y .NET
    - Ejemplo: "Use Span<T> for zero-allocation..."

12. **Logging**
    - Estrategias de logging, performance de logs
    - Ejemplo: "Use structured logging..."

13. **Media Processing**
    - Procesamiento de imágenes, video, audio
    - Ejemplo: "Optimize image compression..."

14. **Compilation**
    - Compilación, JIT, AOT
    - Ejemplo: "Use AOT compilation..."

15. **Measurement**
    - Profiling, benchmarking, monitoring
    - Ejemplo: "Use Application Insights..."

16. **Anti-Patterns**
    - Qué NO hacer, errores comunes
    - Ejemplo: "Avoid N+1 queries..."

### Tags Secundarios (Subcategorías)

Tags más específicos que complementan la categoría principal:

- **CPU Optimization** - Optimización de CPU
- **Memory Allocation** - Asignación de memoria
- **Query Optimization** - Optimización de queries
- **Connection Pooling** - Pool de conexiones
- **Indexing** - Índices de base de datos
- **Async Programming** - Programación asíncrona
- **Threading** - Manejo de threads
- **Caching Strategies** - Estrategias de cache
- **Load Balancing** - Balanceo de carga
- **Microservices** - Arquitectura de microservicios
- **Zero Allocation** - Sin asignación de memoria
- **Garbage Collection** - Recolección de basura
- **JIT Optimization** - Optimización JIT
- **Profiling** - Análisis de performance

### Tags Generales (Siempre Incluir)

Estos tags deben estar en casi todos los artículos:

- **Performance** - Siempre incluir
- **Optimization** - Siempre incluir
- **Best Practices** - Si es una práctica recomendada
- **Tips** - Si es un tip rápido
- **Tutorial** - Si es un tutorial paso a paso
- **Advanced** - Si es un tema avanzado
- **Beginner** - Si es para principiantes

## Ejemplos de Estructura de Tags

### Ejemplo 1: Hardware
```markdown
<!-- Tags: Hardware & Operating System, CPU Optimization, Performance, Optimization -->
```

### Ejemplo 2: Base de Datos
```markdown
<!-- Tags: Databases, Query Optimization, SQL Server, Performance, Optimization -->
```

### Ejemplo 3: Caching
```markdown
<!-- Tags: Caching, Redis, Performance, Optimization, Best Practices -->
```

### Ejemplo 4: .NET Específico
```markdown
<!-- Tags: .NET/C# Performance, Zero Allocation, Performance, Optimization, Advanced -->
```

### Ejemplo 5: Anti-Pattern
```markdown
<!-- Tags: Anti-Patterns, Databases, Performance, Tips -->
```

## Reglas de Tags

1. **Mínimo 3 tags, máximo 5 tags** por artículo
2. **Siempre incluir "Performance" y "Optimization"**
3. **Primer tag = Categoría principal**
4. **Segundo tag = Subcategoría específica** (opcional)
5. **Últimos tags = Generales** (Performance, Optimization, etc.)

## Plantilla Rápida

```markdown
<!-- Tags: [Categoría Principal], [Subcategoría], Performance, Optimization -->
```

## Ver Tags Existentes en Ghost

Para ver qué tags ya existen en tu blog:

1. Ve a Ghost Admin: http://localhost:2368/ghost
2. Settings → Tags
3. Revisa los tags existentes
4. Usa tags existentes cuando sea posible para mantener consistencia

