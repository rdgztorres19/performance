# Pasos para Setup en Producción

## 🎯 Resumen

En producción necesitas:
1. **Instalar Ghost separadamente** (NO del Git)
2. **Clonar tu repo** (solo scripts)
3. **Configurar Ghost con MySQL**
4. **Configurar scripts**

## 📋 Paso a Paso

### 1. Conectarse al Servidor

```bash
ssh usuario@tu-servidor-ip
```

### 2. Crear Usuario Ghost (si no existe)

```bash
sudo adduser --disabled-password --gecos "" ghost
sudo usermod -aG sudo ghost
```

### 3. Instalar Dependencias del Sistema

```bash
# Actualizar sistema
sudo apt update && sudo apt upgrade -y

# Instalar Node.js 22.x (requerido por Ghost 6.10.3)
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
sudo apt-get install -y nodejs

# Instalar MySQL
sudo apt install mysql-server -y
sudo mysql_secure_installation

# Instalar Nginx (para reverse proxy)
sudo apt install nginx -y

# Instalar Git
sudo apt install git -y
```

### 4. Configurar MySQL

```bash
sudo mysql -u root -p
```

En MySQL:

```sql
CREATE DATABASE ghost_production;
CREATE USER 'ghost'@'localhost' IDENTIFIED BY 'TU_PASSWORD_SEGURO';
GRANT ALL PRIVILEGES ON ghost_production.* TO 'ghost'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

### 5. Instalar Ghost CLI

```bash
sudo npm install -g ghost-cli@latest
```

### 6. Instalar Ghost (como usuario ghost)

```bash
# Cambiar al usuario ghost
sudo su - ghost

# Crear directorio para Ghost
sudo mkdir -p /var/www/ghost-blog
sudo chown ghost:ghost /var/www/ghost-blog

# Instalar Ghost
cd /var/www/ghost-blog
ghost install production
```

Durante `ghost install`, responder:
- **Blog URL**: `https://tudominio.com` o `http://TU_IP_PUBLICA`
- **MySQL hostname**: `localhost`
- **MySQL username**: `ghost`
- **MySQL password**: La que creaste en el paso 4
- **Database name**: `ghost_production`
- **Set up Nginx?**: `Yes`
- **Set up SSL?**: `Yes` (si tienes dominio)
- **Set up systemd?**: `Yes`
- **Start Ghost?**: `Yes`

### 7. Clonar tu Repo (Scripts)

```bash
# Cambiar a tu usuario (no root, no ghost)
exit  # Salir de usuario ghost
cd /var/www

# Clonar repo
git clone https://github.com/tu-usuario/performance.git
cd performance

# Instalar dependencias de los scripts
npm install
```

### 8. Configurar Scripts

#### Opción A: Usar el script config-mysql.js (si necesitas cambiar algo)

```bash
cd /var/www/performance
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js
```

#### Opción B: Configurar manualmente (ya está configurado en ghost install)

Si `ghost install` ya configuró todo, puedes saltar este paso.

### 9. Obtener API Key de Ghost

1. Accede a Ghost Admin:
   ```
   https://tudominio.com/ghost
   # o
   http://TU_IP_PUBLICA/ghost
   ```

2. Ve a **Settings → Integrations → Add custom integration**

3. Crea una nueva integración

4. Copia el **Admin API Key**

### 10. Usar Scripts en Producción

Para insertar artículos:

```bash
cd /var/www/performance

GHOST_URL=https://tudominio.com \
GHOST_ADMIN_API_KEY=tu-api-key-aqui \
node ghost-blog/insert-article.js articles/mi-articulo.md
```

### 11. (Opcional) Crear archivo .env para facilitar

```bash
cd /var/www/performance
cat > .env << EOF
GHOST_URL=https://tudominio.com
GHOST_ADMIN_API_KEY=tu-api-key-aqui
GHOST_PATH=/var/www/ghost-blog
EOF

chmod 600 .env  # Solo lectura para el propietario
```

Luego puedes usar un script wrapper o cargar las variables:

```bash
source .env
node ghost-blog/insert-article.js articles/mi-articulo.md
```

## ✅ Verificación

### Verificar Ghost está corriendo

```bash
cd /var/www/ghost-blog
ghost status
```

Debe mostrar: `running (production)`

### Verificar estructura de directorios

```bash
ls -la /var/www/
```

Debe mostrar:
- `ghost-blog/` - Instalación de Ghost (NO del Git)
- `performance/` - Tu repo clonado (scripts)

### Verificar que scripts funcionan

```bash
cd /var/www/performance
GHOST_PATH=/var/www/ghost-blog node ghost-blog/config-mysql.js --help
```

## 🔑 Puntos Clave

1. ✅ **Ghost está en `/var/www/ghost-blog/`** - Instalado separadamente, NO del Git
2. ✅ **Scripts están en `/var/www/performance/`** - Clonado del Git
3. ✅ **`current/`, `versions/`, `content/`** - NO están en Git, están en `/var/www/ghost-blog/`
4. ✅ **Scripts usan `GHOST_PATH`** - Para encontrar Ghost en cualquier ubicación
5. ✅ **Configuraciones con contraseñas** - NO están en Git (están en `.gitignore`)

## 🚨 Troubleshooting

### Error: "Ghost was installed using the root user"

```bash
# Eliminar instalación incorrecta
sudo rm -rf /root/performance/ghost-blog

# Instalar como usuario ghost (ver paso 6)
```

### Error: "Cannot find module 'current/index.js'"

Esto significa que `current` está roto. Ghost debe instalarse correctamente:

```bash
cd /var/www/ghost-blog
rm -rf current versions
ghost install production
```

### Error: Node version incompatible

Ghost 6.10.3 requiere Node 22:

```bash
# Verificar versión
node --version

# Si no es 22.x, actualizar
curl -fsSL https://deb.nodesource.com/setup_22.x | sudo -E bash -
sudo apt-get install -y nodejs
```

## 📚 Resumen de Comandos Útiles

```bash
# Ghost
cd /var/www/ghost-blog
ghost start          # Iniciar
ghost stop           # Detener
ghost restart        # Reiniciar
ghost status         # Estado
ghost log            # Ver logs
ghost update         # Actualizar Ghost

# Scripts
cd /var/www/performance
GHOST_URL=https://tudominio.com \
GHOST_ADMIN_API_KEY=tu-api-key \
node ghost-blog/insert-article.js articles/articulo.md
```

