# Guía de Migración: Separar Ghost de Scripts

Esta guía te ayuda a migrar tu proyecto actual para seguir la arquitectura correcta donde Ghost y los scripts están separados.

## 🎯 Objetivo

**Separar completamente Ghost (instalación) de tus scripts (proyecto Git)**

## 📋 Estado Actual vs Estado Deseado

### ❌ Estado Actual (Incorrecto)
```
performance/
├── ghost-blog/           ← Todo mezclado
│   ├── current/         ← En Git (mal)
│   ├── versions/        ← En Git (mal)
│   ├── content/         ← En Git (mal)
│   ├── insert-article.js
│   ├── config-mysql.js
│   └── package.json
```

### ✅ Estado Deseado (Correcto)

**En Desarrollo (Mac):**
```
performance/              ← Repo Git
├── ghost-blog/          ← Ghost local (ignorado por Git)
│   ├── current/         ← NO en Git
│   ├── versions/        ← NO en Git
│   └── content/         ← NO en Git
├── insert-article.js    ← En Git
├── config-mysql.js      ← En Git (o en scripts/)
├── articles/            ← En Git
├── docs/                ← En Git
└── package.json         ← En Git (de scripts)
```

**En Producción (Servidor):**
```
/var/www/
├── ghost-blog/          ← Ghost instalado (NO del Git)
│   ├── current/         ← Creado por Ghost
│   ├── versions/        ← Instalado por Ghost
│   └── content/         ← Creado por Ghost
│
└── performance/         ← Clonado de Git
    ├── insert-article.js
    ├── config-mysql.js
    ├── articles/
    └── package.json
```

## 🔧 Pasos de Migración

### Paso 1: Actualizar .gitignore (YA HECHO ✅)

El `.gitignore` ya está actualizado para excluir:
- `ghost-blog/current/`
- `ghost-blog/versions/`
- `ghost-blog/content/`
- `ghost-blog/config.*.json`

### Paso 2: Eliminar Ghost del Repositorio

```bash
# En tu Mac (desarrollo)
cd ~/Documents/Projects/performance

# Eliminar Ghost del índice de Git (pero mantenerlo localmente)
git rm -r --cached ghost-blog/current ghost-blog/versions ghost-blog/content 2>/dev/null || true

# Verificar que está siendo ignorado
git status
```

**Nota:** Estos archivos seguirán existiendo localmente, pero ya no estarán en Git.

### Paso 3: Actualizar Scripts para Usar GHOST_PATH

Los scripts ya están actualizados para usar variables de entorno:

**`config-mysql.js`:**
```bash
# Desarrollo
GHOST_PATH=./ghost-blog node ghost-blog/config-mysql.js

# Producción
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js
```

**`insert-article.js`:**
```bash
# Desarrollo (usa defaults)
node ghost-blog/insert-article.js articles/mi-articulo.md

# Producción (con variables de entorno)
GHOST_URL=https://tudominio.com \
GHOST_ADMIN_API_KEY=tu-api-key \
node ghost-blog/insert-article.js articles/mi-articulo.md
```

### Paso 4: Setup en Producción (Servidor)

#### A. Instalar Ghost Separadamente

```bash
# Conectarse al servidor
ssh usuario@tu-servidor

# Crear usuario ghost (si no existe)
sudo adduser --disabled-password --gecos "" ghost
sudo usermod -aG sudo ghost

# Cambiar al usuario ghost
sudo su - ghost

# Instalar Ghost CLI
sudo npm install -g ghost-cli@latest

# Instalar Ghost
sudo mkdir -p /var/www/ghost-blog
sudo chown ghost:ghost /var/www/ghost-blog
cd /var/www/ghost-blog
ghost install production
```

Durante `ghost install`, responder:
- **Blog URL**: `https://tudominio.com` o `http://TU_IP`
- **MySQL hostname**: `localhost`
- **MySQL username**: `ghost`
- **MySQL password**: Tu contraseña
- **Database name**: `ghost_production`
- **Set up Nginx?**: `Yes`
- **Set up SSL?**: `Yes` (si tienes dominio)
- **Set up systemd?**: `Yes`
- **Start Ghost?**: `Yes`

#### B. Clonar tu Repo (Solo Scripts)

```bash
# Como usuario ghost o tu usuario
cd /var/www
git clone https://github.com/tu-usuario/performance.git
cd performance
npm install
```

#### C. Configurar Scripts

```bash
# Configurar MySQL usando el script
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js

# Obtener API Key de Ghost
# 1. Accede a: https://tudominio.com/ghost
# 2. Settings → Integrations → Add custom integration
# 3. Copia el Admin API Key

# Crear archivo .env (opcional, recomendado)
cd /var/www/performance
cat > .env << EOF
GHOST_URL=https://tudominio.com
GHOST_ADMIN_API_KEY=tu-api-key-aqui
GHOST_PATH=/var/www/ghost-blog
EOF
```

#### D. Usar Scripts en Producción

```bash
cd /var/www/performance

# Insertar artículo
GHOST_URL=https://tudominio.com \
GHOST_ADMIN_API_KEY=tu-api-key \
node ghost-blog/insert-article.js articles/mi-articulo.md
```

### Paso 5: Commit y Push de Cambios

```bash
# En tu Mac
cd ~/Documents/Projects/performance

# Agregar cambios
git add .gitignore ghost-blog/config-mysql.js ghost-blog/insert-article.js ARCHITECTURE.md docs/MIGRATION_GUIDE.md

# Commit
git commit -m "Separate Ghost installation from scripts

- Update .gitignore to exclude Ghost core files
- Update config-mysql.js to use GHOST_PATH env variable
- Update insert-article.js to use env variables
- Add architecture documentation"

# Push
git push
```

## 🔍 Verificación

### En Desarrollo (Mac)

```bash
cd ~/Documents/Projects/performance

# Verificar que Ghost no está en Git
git ls-files | grep -E "(current|versions/)" && echo "❌ Ghost está en Git!" || echo "✅ Ghost NO está en Git"

# Verificar que scripts están en Git
git ls-files | grep -E "(insert-article|config-mysql)" && echo "✅ Scripts están en Git"
```

### En Producción (Servidor)

```bash
# Verificar estructura
ls -la /var/www/
# Debe mostrar: ghost-blog/  performance/

# Verificar Ghost está instalado
cd /var/www/ghost-blog
ghost status
# Debe mostrar Ghost corriendo

# Verificar scripts funcionan
cd /var/www/performance
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js --help
```

## 🚨 Problemas Comunes

### Error: "Cannot find module '/var/www/ghost-blog/current/index.js'"

**Causa:** El symlink `current` apunta a una ruta que no existe.

**Solución:** 
1. Eliminar `current` del repositorio
2. Instalar Ghost correctamente con `ghost install`
3. Ghost creará `current` automáticamente

### Error: "GHOST_PATH no encontrado"

**Causa:** La variable de entorno no está configurada.

**Solución:**
```bash
# Configurar siempre antes de ejecutar scripts
export GHOST_PATH=/var/www/ghost-blog
node ghost-blog/config-mysql.js

# O usar inline
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js
```

### Error: "Permission denied"

**Causa:** Permisos incorrectos en directorios.

**Solución:**
```bash
# En producción
sudo chown -R ghost:ghost /var/www/ghost-blog
sudo chown -R tu-usuario:tu-usuario /var/www/performance
```

## 📚 Resumen

1. ✅ **`.gitignore` actualizado** - Ghost excluido
2. ✅ **Scripts actualizados** - Usan `GHOST_PATH`
3. 🔄 **En desarrollo:** Ghost sigue funcionando localmente
4. 🔄 **En producción:** Instalar Ghost separadamente, clonar repo para scripts
5. ✅ **Scripts funcionan** - Con variables de entorno

## 🎉 Resultado Final

- ✅ Ghost NO está en Git
- ✅ Scripts SÍ están en Git
- ✅ Funciona en desarrollo
- ✅ Funciona en producción
- ✅ Arquitectura limpia y mantenible

