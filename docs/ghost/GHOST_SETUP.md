# Guía de Instalación de Ghost CMS en Servidor Propio

Esta guía te ayudará a instalar Ghost CMS en tu servidor con IP público, configurando Nginx como reverse proxy y SSL con Let's Encrypt.

## Requisitos del Sistema

- **Sistema Operativo**: Ubuntu 20.04 LTS o superior (recomendado)
- **RAM**: Mínimo 1GB, recomendado 2GB+
- **CPU**: 1 core mínimo, 2+ cores recomendado
- **Disco**: 10GB+ de espacio libre
- **Node.js**: 18.x o superior
- **Base de datos**: MySQL 8.0+ o PostgreSQL 12+
- **Nginx**: Para reverse proxy
- **Dominio**: Opcional pero recomendado (puedes usar IP directamente)

## Paso 1: Preparar el Servidor

### Actualizar el sistema

```bash
sudo apt update
sudo apt upgrade -y
```

### Crear usuario para Ghost (recomendado)

```bash
# Crear usuario ghost
sudo adduser --disabled-password --gecos "" ghost

# Agregar a grupo sudo si es necesario
sudo usermod -aG sudo ghost

# Cambiar al usuario ghost
su - ghost
```

## Paso 2: Instalar Node.js

```bash
# Instalar Node.js 18.x usando NodeSource
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# Verificar instalación
node --version  # Debe mostrar v18.x o superior
npm --version
```

## Paso 3: Instalar MySQL

```bash
# Instalar MySQL
sudo apt install mysql-server -y

# Configurar MySQL (ejecutar el script de seguridad)
sudo mysql_secure_installation

# Crear base de datos y usuario para Ghost
sudo mysql -u root -p
```

En el prompt de MySQL:

```sql
CREATE DATABASE ghost_production;
CREATE USER 'ghost'@'localhost' IDENTIFIED BY 'TU_PASSWORD_SEGURO_AQUI';
GRANT ALL PRIVILEGES ON ghost_production.* TO 'ghost'@'localhost';
FLUSH PRIVILEGES;
EXIT;
```

**Nota**: Reemplaza `TU_PASSWORD_SEGURO_AQUI` con una contraseña fuerte.

## Paso 4: Instalar Ghost CLI

```bash
# Instalar Ghost CLI globalmente
sudo npm install -g ghost-cli@latest

# Verificar instalación
ghost --version
```

## Paso 5: Instalar Ghost

```bash
# Crear directorio para Ghost
sudo mkdir -p /var/www/ghost
sudo chown ghost:ghost /var/www/ghost

# Cambiar al directorio
cd /var/www/ghost

# Instalar Ghost (como usuario ghost)
ghost install
```

Durante la instalación, Ghost CLI te preguntará:

1. **Blog URL**: 
   - Si tienes dominio: `https://tudominio.com`
   - Si solo tienes IP: `http://TU_IP_PUBLICA` (puedes agregar SSL después)

2. **MySQL hostname**: `localhost`

3. **MySQL username**: `ghost`

4. **MySQL password**: La que creaste anteriormente

5. **Database name**: `ghost_production`

6. **Ghost database setup**: Dejar en blanco (Ghost lo creará)

7. **Set up Nginx?**: `Yes`

8. **Set up SSL?**: 
   - Si tienes dominio: `Yes` (usará Let's Encrypt)
   - Si solo tienes IP: `No` (puedes configurarlo después)

9. **Set up systemd?**: `Yes`

10. **Start Ghost?**: `Yes`

## Paso 6: Configuración Manual de Nginx (si es necesario)

Si Ghost CLI no configuró Nginx automáticamente, puedes hacerlo manualmente:

```bash
sudo nano /etc/nginx/sites-available/ghost
```

Agregar la siguiente configuración:

```nginx
server {
    listen 80;
    server_name TU_DOMINIO_O_IP;

    location / {
        proxy_pass http://127.0.0.1:2368;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }
}
```

Habilitar el sitio:

```bash
sudo ln -s /etc/nginx/sites-available/ghost /etc/nginx/sites-enabled/
sudo nginx -t  # Verificar configuración
sudo systemctl reload nginx
```

## Paso 7: Configurar SSL con Let's Encrypt (si tienes dominio)

```bash
# Instalar Certbot
sudo apt install certbot python3-certbot-nginx -y

# Obtener certificado SSL
sudo certbot --nginx -d tudominio.com

# Renovar automáticamente (certbot configura esto automáticamente)
sudo certbot renew --dry-run
```

## Paso 8: Configurar Firewall

```bash
# Permitir HTTP y HTTPS
sudo ufw allow 'Nginx Full'

# O si solo usas IP sin SSL
sudo ufw allow 'Nginx HTTP'

# Habilitar firewall
sudo ufw enable
```

## Paso 9: Acceder a Ghost Admin

1. Abre tu navegador y ve a:
   - Con dominio: `https://tudominio.com/ghost`
   - Con IP: `http://TU_IP_PUBLICA/ghost`

2. Crea tu cuenta de administrador

3. ¡Listo! Ya puedes empezar a publicar contenido.

## Comandos Útiles de Ghost

```bash
# Ver estado de Ghost
ghost status

# Iniciar Ghost
ghost start

# Detener Ghost
ghost stop

# Reiniciar Ghost
ghost restart

# Ver logs
ghost log

# Actualizar Ghost
ghost update
```

## Configuración de Producción

### Variables de entorno importantes

Edita el archivo de configuración:

```bash
sudo nano /var/www/ghost/config.production.json
```

Configuraciones recomendadas:

```json
{
  "url": "https://tudominio.com",
  "server": {
    "port": 2368,
    "host": "127.0.0.1"
  },
  "database": {
    "client": "mysql",
    "connection": {
      "host": "localhost",
      "user": "ghost",
      "password": "TU_PASSWORD",
      "database": "ghost_production"
    }
  },
  "mail": {
    "transport": "SMTP",
    "options": {
      "host": "smtp.gmail.com",
      "port": 587,
      "secure": false,
      "auth": {
        "user": "tu-email@gmail.com",
        "pass": "tu-app-password"
      }
    }
  },
  "logging": {
    "transports": ["file", "stdout"]
  },
  "process": "systemd",
  "paths": {
    "contentPath": "/var/www/ghost/content/"
  }
}
```

Después de editar, reinicia Ghost:

```bash
ghost restart
```

## Seguridad Adicional

### 1. Cambiar puerto de Ghost (opcional)

Si quieres cambiar el puerto interno de Ghost:

```bash
sudo nano /var/www/ghost/config.production.json
```

Cambiar `"port": 2368` a otro puerto (ej: 3000).

### 2. Configurar backup automático

```bash
# Crear script de backup
sudo nano /usr/local/bin/ghost-backup.sh
```

Contenido del script:

```bash
#!/bin/bash
BACKUP_DIR="/backup/ghost"
DATE=$(date +%Y%m%d_%H%M%S)
mkdir -p $BACKUP_DIR

# Backup de base de datos
mysqldump -u ghost -pTU_PASSWORD ghost_production > $BACKUP_DIR/db_$DATE.sql

# Backup de contenido
tar -czf $BACKUP_DIR/content_$DATE.tar.gz /var/www/ghost/content

# Eliminar backups antiguos (mantener últimos 7 días)
find $BACKUP_DIR -type f -mtime +7 -delete
```

Hacer ejecutable:

```bash
sudo chmod +x /usr/local/bin/ghost-backup.sh
```

Agregar a crontab (backup diario a las 2 AM):

```bash
sudo crontab -e
```

Agregar línea:

```
0 2 * * * /usr/local/bin/ghost-backup.sh
```

## Solución de Problemas

### Ghost no inicia

```bash
# Ver logs
ghost log

# Verificar permisos
sudo chown -R ghost:ghost /var/www/ghost

# Verificar configuración
ghost doctor
```

### Error de conexión a base de datos

```bash
# Verificar que MySQL esté corriendo
sudo systemctl status mysql

# Verificar credenciales en config.production.json
# Probar conexión manual
mysql -u ghost -p ghost_production
```

### Nginx no funciona

```bash
# Verificar configuración
sudo nginx -t

# Ver logs
sudo tail -f /var/log/nginx/error.log

# Reiniciar Nginx
sudo systemctl restart nginx
```

## Recursos Adicionales

- [Documentación oficial de Ghost](https://ghost.org/docs/)
- [Ghost CLI Documentation](https://ghost.org/docs/ghost-cli/)
- [Ghost Forum](https://forum.ghost.org/)

## Próximos Pasos

1. ✅ Ghost está instalado y funcionando
2. 📝 Configurar anuncios (ver `ghost-ads-config.md`)
3. 📄 Agregar tu primer artículo (ver `QUICK_START.md`)
4. 🎨 Personalizar el tema si lo deseas

