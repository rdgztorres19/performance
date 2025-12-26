# 📝 Estructura y Organización de Artículos

## 🎯 Formato Estándar del Archivo Markdown

Cada artículo debe seguir esta estructura exacta:

```markdown
# Título del Artículo (Claro y Descriptivo)

**Cómo funciona:**
Descripción detallada de cómo funciona la técnica, tecnología o estrategia. 
Explica el concepto de manera clara y concisa.

**Ventajas:**
- Ventaja 1 (específica y medible si es posible)
- Ventaja 2
- Ventaja 3

**Desventajas:**
- Desventaja 1 (si aplica)
- Desventaja 2 (si aplica)

**Cuándo usar:**
- Caso de uso 1 (situación específica)
- Caso de uso 2
- Caso de uso 3

**Impacto en performance:**
Descripción cuantificable del impacto cuando sea posible. 
Ejemplos: "Reduce latencia en 30-50%", "Mejora throughput en 2x", etc.

<!-- Tags: Categoría Principal, Subcategoría, Performance, Optimization -->
```

---

## 📋 Estructura Detallada por Sección

### 1. Título (`# Título`)
- **Formato**: H1 con `#`
- **Recomendación**: Claro, descriptivo, sin emojis
- **Ejemplos buenos**:
  - ✅ "Use Connection Pooling for Database Connections"
  - ✅ "Prefer fewer fast CPU cores over many slow ones depending on workload"
  - ❌ "Connection Pooling" (muy genérico)
  - ❌ "CPU Stuff" (poco descriptivo)

### 2. Cómo funciona (`**Cómo funciona:**`)
- **Formato**: Negrita seguida de párrafo
- **Contenido**: Explicación técnica clara
- **Longitud**: 2-4 párrafos recomendado
- **Incluir**: Concepto, mecanismo, cómo se implementa

### 3. Ventajas (`**Ventajas:**`)
- **Formato**: Lista con `-`
- **Cantidad**: 3-5 ventajas
- **Estilo**: Específicas y medibles cuando sea posible
- **Ejemplo**:
  ```markdown
  - Reduce overhead de conexión en 90%
  - Mejora tiempo de respuesta de 200ms a <1ms
  - Reduce consumo de memoria en aplicaciones con alta concurrencia
  ```

### 4. Desventajas (`**Desventajas:**`)
- **Formato**: Lista con `-`
- **Opcional**: Si no hay desventajas significativas, omitir
- **Estilo**: Honesto y balanceado

### 5. Cuándo usar (`**Cuándo usar:**`)
- **Formato**: Lista con `-`
- **Contenido**: Casos de uso específicos y situaciones reales
- **Ejemplo**:
  ```markdown
  - Aplicaciones con alta concurrencia (>100 conexiones simultáneas)
  - Servicios que hacen muchas consultas a base de datos
  - APIs REST con acceso frecuente a BD
  ```

### 6. Impacto en performance (`**Impacto en performance:**`)
- **Formato**: Párrafo descriptivo
- **Contenido**: Métricas cuando sea posible
- **Ejemplos**:
  - "Reduce el tiempo de conexión de 50-200ms a <1ms"
  - "Mejora throughput en 30-50%"
  - "Puede mejorar el rendimiento single-threaded en un 20-40%"

### 7. Tags (`<!-- Tags: ... -->`)
- **Formato**: Comentario HTML al final
- **Estructura**: Ver sección de Tags abajo
- **Ubicación**: Última línea del archivo

---

## 🏷️ Sistema de Tags

### Estructura de Tags (4 tags recomendados)

```
Tag 1: Categoría Principal (obligatorio)
Tag 2: Subcategoría Específica (recomendado)
Tag 3: Performance (siempre)
Tag 4: Optimization (siempre)
```

### Categorías Principales Disponibles

| Categoría | Cuándo Usar |
|-----------|-------------|
| **Hardware & Operating System** | CPU, memoria física, SO, optimizaciones de hardware |
| **Memory Management** | Gestión de memoria, allocation, GC, pooling |
| **Disk I/O** | Lectura/escritura de archivos, async I/O |
| **Networking** | HTTP, TCP, conexiones de red, protocolos |
| **Databases** | Queries, índices, conexiones, optimización SQL |
| **Caching** | Redis, Memcached, estrategias de cache |
| **Concurrency** | Threading, async/await, paralelismo |
| **Data Structures** | Arrays, lists, dictionaries, collections |
| **Algorithms** | Búsqueda, ordenamiento, complejidad |
| **System Design** | Arquitectura, escalabilidad, microservicios |
| **.NET/C# Performance** | Específico de C# y .NET |
| **Logging** | Estrategias de logging, performance de logs |
| **Media Processing** | Imágenes, video, audio, compresión |
| **Compilation** | JIT, AOT, optimizaciones de compilación |
| **Measurement** | Profiling, benchmarking, monitoring |
| **Anti-Patterns** | Qué NO hacer, errores comunes |

### Subcategorías Comunes

- **CPU Optimization** - Optimización de CPU
- **Memory Allocation** - Asignación de memoria
- **Query Optimization** - Optimización de queries
- **Connection Pooling** - Pool de conexiones
- **Indexing** - Índices de base de datos
- **Async Programming** - Programación asíncrona
- **Threading** - Manejo de threads
- **Caching Strategies** - Estrategias de cache
- **Load Balancing** - Balanceo de carga
- **Zero Allocation** - Sin asignación de memoria
- **Garbage Collection** - Recolección de basura

### Ejemplos de Tags por Tipo de Artículo

#### Hardware
```markdown
<!-- Tags: Hardware & Operating System, CPU Optimization, Performance, Optimization -->
```

#### Base de Datos
```markdown
<!-- Tags: Databases, Query Optimization, Performance, Optimization -->
```
o
```markdown
<!-- Tags: Databases, Connection Pooling, Performance, Optimization -->
```

#### Caching
```markdown
<!-- Tags: Caching, Redis, Performance, Optimization -->
```

#### .NET/C# Específico
```markdown
<!-- Tags: .NET/C# Performance, Zero Allocation, Performance, Optimization -->
```

#### Anti-Pattern
```markdown
<!-- Tags: Anti-Patterns, Databases, Performance, Tips -->
```

---

## 📄 Ejemplo Completo

```markdown
# Use Connection Pooling for Database Connections

**Cómo funciona:**
Connection pooling es una técnica que mantiene un conjunto de conexiones de base de datos abiertas y reutilizables. En lugar de crear una nueva conexión para cada solicitud (lo cual es costoso), la aplicación toma una conexión del pool, la usa, y la devuelve al pool para que otros requests la puedan usar.

Esta técnica reduce significativamente el overhead de establecer conexiones TCP, autenticación, y negociación de protocolo que ocurre cada vez que se abre una nueva conexión.

**Ventajas:**
- Reduce overhead de conexión en 90% (de 50-200ms a <1ms)
- Mejora tiempo de respuesta general de la aplicación
- Reduce consumo de recursos del servidor de BD
- Permite mejor control de límites de conexiones
- Mejora escalabilidad de aplicaciones con alta concurrencia

**Desventajas:**
- Requiere configuración adecuada del tamaño del pool
- Puede consumir memoria si el pool es muy grande
- Conexiones inactivas pueden ocupar recursos

**Cuándo usar:**
- Aplicaciones con alta concurrencia (>100 conexiones simultáneas)
- Servicios que hacen muchas consultas a base de datos
- APIs REST con acceso frecuente a BD
- Aplicaciones web con múltiples usuarios concurrentes
- Microservicios que se conectan a bases de datos

**Impacto en performance:**
Reduce el tiempo de conexión de 50-200ms a <1ms. Mejora throughput en 30-50% en aplicaciones con alta concurrencia. Reduce carga en el servidor de base de datos significativamente.

<!-- Tags: Databases, Connection Pooling, Performance, Optimization -->
```

---

## 📁 Organización de Archivos

### Estructura de Carpetas

```
performance/
├── articles/                    # Artículos listos para publicar
│   ├── use-connection-pooling.md
│   ├── prefer-fewer-fast-cpu-cores.md
│   ├── implement-redis-caching.md
│   └── ...
├── example-article.md           # Artículo de ejemplo
└── ghost-blog/
    ├── insert-article.js        # Script de inserción
    └── docs/
        └── ESTRUCTURA_ARTICULOS.md  # Este archivo
```

### Convención de Nombres

- **Formato**: `kebab-case` (minúsculas con guiones)
- **Ejemplos**:
  - ✅ `use-connection-pooling.md`
  - ✅ `prefer-fewer-fast-cpu-cores.md`
  - ✅ `implement-redis-caching.md`
  - ❌ `Use Connection Pooling.md` (espacios)
  - ❌ `use_connection_pooling.md` (underscores)

---

## 🚀 Insertar Artículo en Ghost

### Comando Básico

```bash
cd ghost-blog
npm run insert-article -- ../articles/nombre-articulo.md
```

### Con Alias Corto

```bash
npm run insert -- ../articles/nombre-articulo.md
```

### Desde la Raíz del Proyecto

```bash
cd /Users/rdgztorres19/Documents/Projects/performance/ghost-blog
npm run insert-article -- ../example-article.md
```

### El Script Automáticamente:
1. ✅ Lee el archivo markdown
2. ✅ Extrae el título
3. ✅ Extrae los tags del comentario
4. ✅ Crea los tags si no existen
5. ✅ Publica el artículo en Ghost
6. ✅ Muestra la URL del artículo publicado

---

## ✅ Checklist Antes de Publicar

- [ ] Título claro y descriptivo
- [ ] Sección "Cómo funciona" completa
- [ ] Al menos 3 ventajas listadas
- [ ] Desventajas incluidas (si aplica)
- [ ] Casos de uso específicos en "Cuándo usar"
- [ ] Impacto en performance descrito (con métricas si es posible)
- [ ] Tags correctamente formateados (4 tags recomendados)
- [ ] Archivo guardado en `articles/` con nombre kebab-case
- [ ] Sin errores de formato markdown

---

## 📚 Referencias Adicionales

- **TAGS_GUIDE.md** - Guía completa de tags y categorización
- **API_KEY.md** - Configuración del API Key para el script
- **README.md** - Documentación general del proyecto
