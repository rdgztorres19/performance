# Configuración de Contraseña MySQL Remoto

## Problema

El script está fallando con el error:
```
ERROR 1045 (28000): Access denied for user 'root'@'localhost' (using password: YES)
```

Esto significa que la **contraseña de MySQL en el servidor remoto es diferente** a la contraseña SSH.

## Soluciones

### Opción 1: Configurar contraseña con variable de entorno (Recomendado)

```bash
REMOTE_MYSQL_PASSWORD=tu_contraseña_mysql node scripts/backup-and-restore-db.js
```

### Opción 2: Editar el script directamente

Edita `scripts/backup-and-restore-db.js` y cambia:

```javascript
dbPassword: process.env.REMOTE_MYSQL_PASSWORD || '' // Cambiar '' por tu contraseña
```

A:

```javascript
dbPassword: process.env.REMOTE_MYSQL_PASSWORD || 'TU_CONTRASEÑA_MYSQL_AQUI'
```

### Opción 3: Usar MySQL sin contraseña (si está configurado así)

Si MySQL root no tiene contraseña en el servidor remoto, déjalo vacío:

```javascript
dbPassword: '' // Sin contraseña
```

## ¿Cuál es la contraseña de MySQL en el servidor remoto?

Para encontrarla, puedes:

1. **Conectarte al servidor y verificar:**
```bash
ssh root@66.179.188.92
mysql -u root -p
# Te pedirá la contraseña
```

2. **O revisar archivos de configuración:**
```bash
ssh root@66.179.188.92
cat /etc/mysql/debian.cnf  # A veces tiene credenciales
# o
grep password /var/www/ghost-blog/config.production.json  # Si Ghost está instalado
```

3. **O cambiarla si no la sabes:**
```bash
ssh root@66.179.188.92
mysql -u root
# Dentro de MySQL:
ALTER USER 'root'@'localhost' IDENTIFIED BY 'nueva_contraseña';
FLUSH PRIVILEGES;
```

## Configuración Actual

- **SSH Password**: `rY6jPCor` ✅
- **MySQL Password**: Desconocida (necesitas configurarla)

## Uso Final

Una vez que sepas la contraseña:

```bash
REMOTE_MYSQL_PASSWORD=tu_contraseña_mysql node scripts/backup-and-restore-db.js
```

