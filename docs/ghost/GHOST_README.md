# Ghost CMS - Setup para Performance Content

Este directorio contiene todo lo necesario para publicar el contenido de performance optimization en un sitio Ghost CMS.

## 📁 Archivos Incluidos

### Scripts

- **`convert-single-article.js`** - Script para extraer una técnica del README.md y formatearla para Ghost
  - Uso: `node convert-single-article.js "nombre-de-tecnica"`
  - Listar todas: `node convert-single-article.js --list`

### Guías

- **`GHOST_SETUP.md`** - Guía completa de instalación de Ghost en servidor propio
- **`QUICK_START.md`** - Guía rápida para agregar nuevos artículos
- **`ghost-ads-config.md`** - Configuración de anuncios en Ghost
- **`example-article.md`** - Artículo de ejemplo ya formateado

### Contenido

- **`README.md`** - Contenido completo con 599 técnicas de performance
- **`performance-checklist.txt`** - Checklist original

## 🚀 Inicio Rápido

### Opción 1: Instalación Local (Recomendado para empezar)

Si ya tienes Node.js y MySQL instalados localmente:

```bash
# Ver guía rápida
cat SETUP_RAPIDO.md

# O guía detallada
cat GHOST_LOCAL_SETUP.md
```

**Setup rápido en 5 minutos**: Ver `SETUP_RAPIDO.md`

### Opción 2: Instalación en Servidor

Sigue la guía completa en `GHOST_SETUP.md` para instalar Ghost en tu servidor.

### 2. Agregar Primer Artículo

```bash
# Extraer una técnica
node convert-single-article.js "nombre-de-tecnica"

# Copiar el contenido mostrado
# Pegar en Ghost Admin → New Post
```

Ver `QUICK_START.md` para instrucciones detalladas.

### 3. Configurar Anuncios

Sigue la guía en `ghost-ads-config.md` para agregar Google AdSense u otros proveedores.

## 📝 Flujo de Trabajo

```
1. node convert-single-article.js "nombre-tecnica"
   ↓
2. Copiar contenido generado
   ↓
3. Ghost Admin → New Post → Pegar
   ↓
4. Agregar tags y publicar
```

## 🏷️ Categorías/Tags Sugeridos

- Hardware & Operating System
- Memory Management
- Disk & Storage
- File IO
- Networking & IO
- Databases
- Caching
- Message Queues
- Concurrency
- Data Structures
- Algorithms
- System Design
- .NET & C# Performance
- Logging & Observability
- Media & Content Optimization
- Compilation & Code Generation
- Measurement & Optimization
- Performance Anti Patterns

## 📚 Documentación

- **Setup Local Rápido**: Ver `SETUP_RAPIDO.md` (5 minutos)
- **Setup Local Detallado**: Ver `GHOST_LOCAL_SETUP.md` (con SEO y AdSense)
- **Setup en Servidor**: Ver `GHOST_SETUP.md`
- **Agregar artículos**: Ver `QUICK_START.md`
- **Configurar ads**: Ver `ghost-ads-config.md`

## 🔧 Requisitos

- Node.js 18.x o superior
- MySQL 8.0+ o PostgreSQL 12+
- Servidor con IP público (o dominio)
- Nginx (para reverse proxy)
- Ghost CLI instalado

## 💡 Tips

- Usa `--list` para ver todas las técnicas disponibles
- Los tags se sugieren automáticamente según la categoría
- El contenido está en markdown, listo para Ghost
- Puedes editar el contenido en Ghost antes de publicar

## 🆘 Ayuda

Si tienes problemas:

1. **Script no funciona**: Verifica que README.md esté en el mismo directorio
2. **Ghost no inicia**: Ver `GHOST_SETUP.md` sección "Solución de Problemas"
3. **Anuncios no aparecen**: Ver `ghost-ads-config.md` sección "Solución de Problemas"

## 📊 Estadísticas

- **Total de técnicas**: 599
- **Categorías principales**: 19
- **Formato**: Markdown compatible con Ghost
- **Ejemplos de código**: Incluidos en C#

## 🎯 Próximos Pasos

1. ✅ Instalar Ghost (ver `GHOST_SETUP.md`)
2. ✅ Agregar primer artículo (ver `QUICK_START.md`)
3. ✅ Configurar anuncios (ver `ghost-ads-config.md`)
4. 📝 Agregar más artículos gradualmente
5. 🎨 Personalizar tema si lo deseas
6. 📱 Configurar redes sociales para compartir

---

**¡Listo para empezar!** 🚀

