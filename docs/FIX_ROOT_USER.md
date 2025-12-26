# Solución Rápida: Error "Ghost was installed using the root user"

Si recibiste este error, sigue estos pasos para corregirlo:

## Solución Paso a Paso

### 1. Crear Usuario Ghost

```bash
# Crear usuario ghost
sudo adduser --disabled-password --gecos "" ghost

# Agregar al grupo sudo (para poder usar sudo cuando sea necesario)
sudo usermod -aG sudo ghost
```

### 2. Eliminar Instalación Incorrecta

```bash
# Eliminar la instalación que hiciste como root
sudo rm -rf /root/performance/ghost-blog

# También eliminar cualquier directorio de Ghost en /root
sudo rm -rf /root/.ghost
```

### 3. Cambiar al Usuario Ghost

```bash
# Cambiar al usuario ghost
sudo su - ghost

# Verificar que estás como usuario ghost (debe mostrar: ghost@ubuntu:~$)
whoami
```

### 4. Instalar Ghost CLI

```bash
# Instalar Ghost CLI globalmente
sudo npm install -g ghost-cli@latest

# Verificar instalación
ghost --version
```

### 5. Instalar Ghost Correctamente

```bash
# Crear directorio para Ghost
sudo mkdir -p /var/www/ghost
sudo chown ghost:ghost /var/www/ghost

# Cambiar al directorio
cd /var/www/ghost

# Instalar Ghost (esto te hará preguntas interactivas)
ghost install
```

Durante `ghost install`, responderás:

- **Blog URL**: `http://TU_IP_PUBLICA` (o tu dominio si lo tienes)
- **MySQL hostname**: `localhost`
- **MySQL username**: `ghost` (o el usuario que creaste para MySQL)
- **MySQL password**: Tu contraseña de MySQL
- **Database name**: `ghost_production` (o el nombre que prefieras)
- **Set up Nginx?**: `Yes` (si quieres usar Nginx como reverse proxy)
- **Set up SSL?**: `No` (a menos que tengas un dominio configurado)
- **Set up systemd?**: `Yes` (para que Ghost inicie automáticamente)
- **Start Ghost?**: `Yes`

### 6. Verificar Instalación

```bash
# Ver estado de Ghost
ghost status

# Ver logs
ghost log

# Si está funcionando, deberías poder acceder a:
# http://TU_IP_PUBLICA/ghost
```

## Notas Importantes

1. **NUNCA ejecutes Ghost como root** - Siempre usa un usuario regular como `ghost`
2. **Todos los comandos de Ghost** deben ejecutarse como usuario `ghost`, no como root
3. **Si necesitas privilegios sudo** para crear directorios o instalar paquetes, usa `sudo`, pero NO ejecutes Ghost con sudo
4. **El directorio `/var/www/ghost`** debe pertenecer al usuario `ghost`:

```bash
sudo chown -R ghost:ghost /var/www/ghost
```

## Si Necesitas Acceder a Ghost Admin

Una vez que Ghost esté funcionando:

1. Accede a: `http://TU_IP_PUBLICA/ghost`
2. Crea tu cuenta de administrador
3. Configura tu sitio

## Comandos Útiles (Como Usuario Ghost)

```bash
# Cambiar al usuario ghost primero
sudo su - ghost

# Luego estos comandos:
cd /var/www/ghost
ghost start          # Iniciar Ghost
ghost stop           # Detener Ghost
ghost restart        # Reiniciar Ghost
ghost status         # Ver estado
ghost log            # Ver logs
ghost update         # Actualizar Ghost
```

## Si Tienes Problemas

Si aún tienes problemas después de seguir estos pasos:

1. Verifica que estás como usuario `ghost`: `whoami`
2. Verifica permisos: `ls -la /var/www/ghost`
3. Ver logs: `ghost log` o `cat /var/www/ghost/content/logs/*.log`
4. Revisa la guía completa en `docs/DEPLOYMENT.md`

