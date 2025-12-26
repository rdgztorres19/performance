# Arquitectura Correcta del Proyecto

## ✅ Estructura Correcta

```
/var/www/                    (En producción)
├── ghost-blog/              ← GHOST REAL (NO en Git, instalado con ghost install)
│   ├── current/             ← symlink (lo crea Ghost automáticamente)
│   ├── versions/            ← Ghost core
│   ├── content/             ← images, media, themes
│   ├── config.production.json
│   └── .ghost-cli
│
└── performance/             ← TU PROYECTO (en Git)
    ├── ghost-blog/          ← Solo para desarrollo local (no se sube)
    │   └── (ghost instalado aquí en dev)
    ├── scripts/
    │   ├── insert-article.js
    │   ├── config-mysql.js
    │   └── convert-single-article.js
    ├── articles/
    ├── docs/
    ├── package.json
    ├── node_modules/
    └── .gitignore
```

**En tu Mac (desarrollo):**
```
~/Documents/Projects/performance/    ← Tu repo Git
├── ghost-blog/                     ← Ghost local (no en Git)
│   └── (instalación local de Ghost)
├── scripts/ o directamente:
├── insert-article.js
├── config-mysql.js
├── articles/
└── docs/
```

## 🔑 Regla de Oro

**Si una carpeta tiene `current/`, NO puede tener `package.json` en el mismo nivel.**

Esto significa:
- ✅ `/var/www/ghost-blog/` → Tiene `current/`, NO tiene `package.json`
- ✅ `/var/www/performance/` → Tiene `package.json`, NO tiene `current/`

## 📁 Qué va en Git y qué NO

### ✅ SÍ va en Git:
- Scripts (`insert-article.js`, `config-mysql.js`)
- Artículos markdown (`articles/`)
- Documentación (`docs/`)
- `package.json` (de los scripts, no de Ghost)
- `.gitignore`

### ❌ NO va en Git:
- `current/` (symlink, lo crea Ghost)
- `versions/` (Ghost core)
- `content/` (images, media - se hace backup pero no en Git)
- `config.*.json` (configuraciones con contraseñas)
- `.ghost-cli`
- `node_modules/` (de Ghost o de scripts)

## 🔧 Configuración de Scripts

### `config-mysql.js`
Usa variable de entorno `GHOST_PATH`:

```bash
# En desarrollo (Mac)
GHOST_PATH=../ghost-blog node config-mysql.js

# En producción
GHOST_PATH=/var/www/ghost-blog node config-mysql.js
```

### `insert-article.js`
Ya está configurado para usar variables de entorno o valores por defecto.

## 🚀 Setup en Producción

### 1. Instalar Ghost (limpio)

```bash
cd /var/www
sudo mkdir -p ghost-blog
sudo chown ghost:ghost ghost-blog
sudo su - ghost
cd /var/www/ghost-blog
ghost install production
```

### 2. Crear base de datos

```sql
CREATE DATABASE ghost_production;
CREATE USER 'ghost'@'localhost' IDENTIFIED BY 'password';
GRANT ALL PRIVILEGES ON ghost_production.* TO 'ghost'@'localhost';
FLUSH PRIVILEGES;
```

### 3. Clonar tu repo (scripts)

```bash
cd /var/www
git clone https://github.com/tu-usuario/performance.git
cd performance
npm install
```

### 4. Configurar MySQL

```bash
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js
# O si moviste los scripts:
GHOST_PATH=/var/www/ghost-blog node scripts/config-mysql.js
```

### 5. Reiniciar Ghost

```bash
cd /var/www/ghost-blog
ghost restart
```

## 💻 Setup en Desarrollo (Mac)

### 1. Instalar Ghost localmente

```bash
cd ~/Documents/Projects/performance
ghost install local
# Ghost se instalará en ghost-blog/
```

### 2. Configurar MySQL local

```bash
# Desde la raíz del proyecto
GHOST_PATH=./ghost-blog node ghost-blog/config-mysql.js
```

### 3. Usar scripts

```bash
# Insertar artículo
node ghost-blog/insert-article.js articles/mi-articulo.md
```

## 📦 Migración Actual

Para migrar tu setup actual:

1. **El repo ya tiene la estructura correcta** - `ghost-blog/` está siendo ignorado por `.gitignore`

2. **En producción, NO clones `ghost-blog/`** - Instala Ghost separadamente:

```bash
# En producción
cd /var/www
ghost install production  # Esto crea /var/www/ghost

# O si quieres en ghost-blog:
cd /var/www
mkdir ghost-blog
cd ghost-blog
ghost install production
```

3. **Clona solo el repo para los scripts**:

```bash
cd /var/www
git clone https://github.com/tu-usuario/performance.git
cd performance
npm install
```

4. **Configura los scripts con GHOST_PATH**:

```bash
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js
```

## 🎯 Resumen

- **Ghost** = Instalación separada, NO en Git
- **Scripts** = En Git, usan `GHOST_PATH` para encontrar Ghost
- **`current/`** = Nunca en Git, lo crea Ghost automáticamente
- **`versions/`** = Nunca en Git, parte de Ghost
- **`content/`** = Backup manual, no en Git (excepto themes si quieres)

