# Scripts de Base de Datos

## backup-and-restore-db.sh / backup-and-restore-db.js

Script para copiar la base de datos de MySQL local a un servidor remoto.

### Configuración

**Origen (Local):**
- Host: localhost:3306
- Base de datos: ghost_local
- Usuario: root
- Contraseña: sbrQp10

**Destino (Remoto):**
- Host: 66.179.188.92:22 (SSH)
- Usuario SSH: root
- Contraseña SSH: rY6jPCor
- Base de datos: performance_enginnering_blog

### Uso

#### Opción 1: Script Bash

```bash
cd scripts
./backup-and-restore-db.sh
```

#### Opción 2: Script Node.js

```bash
cd scripts
node backup-and-restore-db.js
```

### Requisitos

1. **MySQL client** (mysqldump) instalado
2. **sshpass** instalado:
   - macOS: `brew install hudochenkov/sshpass/sshpass`
   - Linux: `sudo apt-get install sshpass`

### Qué hace el script

1. ✅ Crea un dump de la base de datos local (`ghost_local`)
2. ✅ Transfiere el dump al servidor remoto por SSH
3. ✅ Crea la base de datos remota si no existe
4. ✅ Importa el dump en la base de datos remota
5. ✅ Limpia archivos temporales

### Notas

- El script usa `--single-transaction` para evitar bloqueos durante el dump
- Crea la base de datos con charset `utf8mb4` y collation `utf8mb4_unicode_ci`
- Los archivos temporales se guardan en `/tmp/`

### Troubleshooting

**Error: "sshpass: command not found"**
```bash
# macOS
brew install hudochenkov/sshpass/sshpass

# Linux
sudo apt-get install sshpass
```

**Error: "Access denied"**
- Verifica que las credenciales de MySQL local sean correctas
- Verifica que las credenciales SSH del servidor remoto sean correctas

**Error: "Host key verification failed"**
- El script usa `-o StrictHostKeyChecking=no` para evitar esto automáticamente

