# Guía Rápida: Agregar Artículos a Ghost

Esta guía te muestra cómo agregar nuevos artículos de performance a tu sitio Ghost de forma rápida y fácil.

## Proceso Rápido (3 pasos)

### Paso 1: Extraer la técnica del README

Usa el script `convert-single-article.js` para extraer una técnica específica:

```bash
# Desde el directorio del proyecto
node convert-single-article.js "nombre-de-la-tecnica"
```

**Ejemplos:**

```bash
# Buscar por nombre completo
node convert-single-article.js "Use async and await correctly"

# Buscar por parte del nombre
node convert-single-article.js "async"

# Listar todas las técnicas disponibles
node convert-single-article.js --list
```

El script mostrará:
- ✅ El contenido formateado listo para copiar
- 📌 Título sugerido
- 🏷️ Tag sugerido
- 📁 Categoría
- 💾 Archivo guardado automáticamente

### Paso 2: Copiar el contenido

1. El script mostrará el contenido en la terminal
2. También se guardará en un archivo: `ghost-article-[slug].md`
3. Copia todo el contenido mostrado (desde el título hasta el final)

### Paso 3: Pegar en Ghost

1. **Accede a Ghost Admin**:
   - Ve a `https://tudominio.com/ghost` (o `http://TU_IP/ghost`)
   - Inicia sesión

2. **Crear nuevo post**:
   - Click en "New post" o el botón "+" en la esquina superior derecha

3. **Pegar contenido**:
   - Pega el contenido copiado en el editor
   - Ghost reconocerá automáticamente el formato markdown

4. **Agregar metadata**:
   - **Título**: Ya está incluido en el contenido (puedes editarlo si quieres)
   - **Tags**: Agrega los tags sugeridos (aparecen en el comentario al final)
     - Ejemplo: `Hardware & Operating System`, `Performance`, `Optimization`
   - **Excerpt**: Opcional, Ghost puede generarlo automáticamente
   - **Featured image**: Opcional, agrega una imagen si lo deseas

5. **Publicar**:
   - Click en "Publish" en la esquina superior derecha
   - Elige "Publish now" o programa para más tarde

## Ejemplo Completo

### 1. Extraer técnica

```bash
$ node convert-single-article.js "Reduce context switching"

================================================================================
📄 ARTÍCULO LISTO PARA GHOST
================================================================================

📌 Título: Reduce context switching
🏷️  Tag sugerido: Hardware & Operating System
📁 Categoría: Hardware and Operating System
🔗 Slug: reduce-context-switching

--------------------------------------------------------------------------------
CONTENIDO (copia y pega en Ghost):
--------------------------------------------------------------------------------

# Reduce context switching

**Cómo funciona:**
El context switching ocurre cuando el sistema operativo cambia de un proceso/hilo a otro...

[... resto del contenido ...]

<!-- Tags sugeridos: Hardware & Operating System, Performance, Optimization -->

================================================================================
✅ Listo para copiar y pegar en Ghost Admin → New Post
================================================================================

💾 También guardado en: ghost-article-reduce-context-switching.md
```

### 2. En Ghost Admin

1. Click en "New post"
2. Selecciona todo el contenido mostrado (desde `# Reduce context switching` hasta el final)
3. Copia (Ctrl+C / Cmd+C)
4. Pega en el editor de Ghost (Ctrl+V / Cmd+V)
5. Ghost formateará automáticamente el markdown

### 3. Agregar Tags

1. En el panel derecho, busca "Tags"
2. Agrega los tags sugeridos:
   - `Hardware & Operating System`
   - `Performance`
   - `Optimization`
3. Puedes crear nuevos tags o usar los existentes

### 4. Publicar

1. Click en "Publish" (esquina superior derecha)
2. Revisa la preview si lo deseas
3. Click en "Publish now"
4. ¡Listo! El artículo está publicado

## Tips y Mejores Prácticas

### Organización por Categorías

Crea tags principales que correspondan a las secciones del README:

- `Hardware & Operating System`
- `Memory Management`
- `Disk & Storage`
- `File IO`
- `Networking & IO`
- `Databases`
- `Caching`
- `Message Queues`
- `Concurrency`
- `Data Structures`
- `Algorithms`
- `System Design`
- `.NET & C# Performance`
- `Logging & Observability`
- `Media & Content Optimization`
- `Compilation & Code Generation`
- `Measurement & Optimization`
- `Performance Anti Patterns`

### Búsqueda de Técnicas

Si no recuerdas el nombre exacto:

```bash
# Listar todas las técnicas
node convert-single-article.js --list

# Buscar por palabra clave
node convert-single-article.js "cache"
# Esto encontrará todas las técnicas que contengan "cache"
```

### Edición del Contenido

Puedes editar el contenido en Ghost antes de publicar:

- **Agregar imágenes**: Usa el botón de imagen en el editor
- **Modificar formato**: Ghost tiene un editor visual completo
- **Agregar código destacado**: El código C# ya está formateado, pero puedes mejorarlo
- **Agregar enlaces**: Puedes enlazar entre artículos relacionados

### Programar Publicaciones

En lugar de "Publish now", puedes:

1. Click en la fecha junto a "Publish"
2. Seleccionar fecha y hora futura
3. Ghost publicará automáticamente en ese momento

### Reutilizar Tags

Ghost guarda los tags que usas, así que:

- Los tags aparecerán como sugerencias al escribir
- Puedes hacer click en un tag para ver todos los artículos de esa categoría
- Esto ayuda a los lectores a navegar contenido relacionado

## Flujo de Trabajo Recomendado

1. **Planifica**: Decide qué técnicas quieres publicar
2. **Extrae**: Usa el script para obtener el contenido
3. **Revisa**: Lee el contenido antes de publicar
4. **Publica**: Agrega tags y publica
5. **Promociona**: Comparte en redes sociales si lo deseas

## Solución de Problemas

### El script no encuentra la técnica

```bash
# Ver todas las técnicas disponibles
node convert-single-article.js --list

# Buscar con parte del nombre
node convert-single-article.js "async"  # Encontrará todas con "async"
```

### El formato no se ve bien en Ghost

- Ghost soporta markdown nativo
- Si hay problemas, verifica que copiaste todo el contenido
- Puedes usar el editor visual de Ghost para ajustar

### Tags no aparecen

- Los tags están en el comentario al final del contenido
- Debes agregarlos manualmente en el panel derecho de Ghost
- Ghost no lee los comentarios HTML automáticamente

## Próximos Pasos

- ✅ Ya sabes cómo agregar artículos fácilmente
- 📊 Revisa las estadísticas en Ghost Admin
- 🎨 Personaliza el tema si lo deseas
- 📱 Configura redes sociales para compartir automáticamente
- 💰 Configura anuncios (ver `ghost-ads-config.md`)

