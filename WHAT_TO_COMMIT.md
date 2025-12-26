# ¿Qué Subir a GitHub?

## ✅ SÍ debe estar en Git (Subir)

### 1. Scripts (Lo más importante)
- ✅ `ghost-blog/insert-article.js` - Script para insertar artículos
- ✅ `ghost-blog/config-mysql.js` - Script para configurar MySQL
- ✅ `convert-single-article.js` - Script de conversión

### 2. Documentación
- ✅ `docs/` - Toda la documentación
- ✅ `ghost-blog/docs/` - Documentación del blog (templates, guías, etc.)
- ✅ `README.md` - Documentación principal
- ✅ `ARCHITECTURE.md` - Arquitectura del proyecto
- ✅ `docs/MIGRATION_GUIDE.md` - Guía de migración

### 3. Contenido
- ✅ `articles/` - Artículos markdown
- ✅ `example-article.md` - Artículo de ejemplo

### 4. Configuración del Proyecto
- ✅ `package.json` - Dependencias de los scripts (no de Ghost)
- ✅ `.gitignore` - Archivos a ignorar

## ❌ NO debe estar en Git (Eliminar)

### 1. Instalación de Ghost
- ❌ `ghost-blog/current/` - Symlink (lo crea Ghost automáticamente)
- ❌ `ghost-blog/versions/` - Core de Ghost (muy grande, 579MB)
- ❌ `ghost-blog/content/` - Contenido de Ghost (imágenes, media, themes)
- ❌ `ghost-blog/.ghost-cli` - Configuración de Ghost CLI
- ❌ `ghost-blog/.ghostpid` - PID file de Ghost

### 2. Configuraciones Sensibles
- ❌ `ghost-blog/config.development.json` - Contiene contraseñas
- ❌ `ghost-blog/config.production.json` - Contiene contraseñas
- ❌ `.env` - Variables de entorno con secretos

### 3. Dependencias
- ❌ `node_modules/` - Se instalan con `npm install`
- ❌ `package-lock.json` - Opcional (ya está en .gitignore)

### 4. Logs y Temporales
- ❌ `ghost-blog/content/logs/` - Logs de Ghost
- ❌ `.DS_Store` - Archivos del sistema

## 📋 Resumen: Qué Subir

```
✅ Subir a GitHub:
├── ghost-blog/
│   ├── insert-article.js          ✅ Script
│   ├── config-mysql.js            ✅ Script
│   └── docs/                      ✅ Documentación
├── articles/                      ✅ Artículos
├── docs/                          ✅ Documentación
├── package.json                   ✅ Config del proyecto
├── .gitignore                     ✅ Config de Git
├── README.md                      ✅ Documentación
└── ARCHITECTURE.md                ✅ Documentación

❌ NO Subir:
├── ghost-blog/
│   ├── current/                   ❌ Symlink
│   ├── versions/                  ❌ Ghost core
│   ├── content/                   ❌ Contenido Ghost
│   ├── config.*.json              ❌ Configs con passwords
│   ├── .ghost-cli                 ❌ Config Ghost
│   └── .ghostpid                  ❌ PID file
└── node_modules/                  ❌ Dependencias
```

## 🔧 Cómo Limpiar (Eliminar de Git lo que no debe estar)

Ejecuta estos comandos para eliminar de Git los archivos que no deberían estar:

```bash
# Eliminar Ghost core del repositorio
git rm -r --cached ghost-blog/current ghost-blog/versions ghost-blog/content ghost-blog/.ghost-cli ghost-blog/.ghostpid 2>/dev/null || true

# Eliminar configuraciones sensibles
git rm --cached ghost-blog/config.development.json ghost-blog/config.production.json 2>/dev/null || true

# Verificar qué quedó
git status
```

## ✅ Verificación Final

Después de limpiar, ejecuta:

```bash
# Verificar que Ghost NO está en Git
git ls-files | grep -E "(current|versions|content)" && echo "❌ Aún hay archivos de Ghost!" || echo "✅ Ghost NO está en Git"

# Verificar que scripts SÍ están
git ls-files | grep -E "(insert-article|config-mysql)" && echo "✅ Scripts están en Git"
```

