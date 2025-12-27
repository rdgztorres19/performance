#!/bin/bash

###############################################################################
# Script para copiar base de datos de MySQL local a servidor remoto
#
# Uso: ./backup-and-restore-db.sh
#
# Parámetros:
# - Origen: localhost:3306, base de datos: ghost_local
# - Destino: 66.179.188.92, base de datos: performance_enginnering_blog
###############################################################################

set -e  # Salir si hay errores

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuración local
LOCAL_DB_HOST="localhost"
LOCAL_DB_PORT="3306"
LOCAL_DB_NAME="ghost_local"
LOCAL_DB_USER="root"
LOCAL_DB_PASSWORD="sbrQp10"

# Configuración remota
REMOTE_HOST="66.179.188.92"
REMOTE_SSH_PORT="22"
REMOTE_SSH_USER="root"
REMOTE_SSH_PASSWORD="rY6jPCor"
REMOTE_DB_NAME="blog_prod"
REMOTE_DB_USER="root"

# Archivo temporal para el dump
DUMP_FILE="/tmp/ghost_local_backup_$(date +%Y%m%d_%H%M%S).sql"

echo -e "${GREEN}🚀 Iniciando copia de base de datos...${NC}\n"

# Paso 1: Crear dump de la base de datos local
echo -e "${YELLOW}📦 Paso 1: Creando dump de la base de datos local...${NC}"
mysqldump -h "$LOCAL_DB_HOST" \
          -P "$LOCAL_DB_PORT" \
          -u "$LOCAL_DB_USER" \
          -p"$LOCAL_DB_PASSWORD" \
          --single-transaction \
          --quick \
          --lock-tables=false \
          "$LOCAL_DB_NAME" > "$DUMP_FILE"

if [ $? -eq 0 ]; then
    DUMP_SIZE=$(du -h "$DUMP_FILE" | cut -f1)
    echo -e "${GREEN}✅ Dump creado exitosamente: $DUMP_FILE (Tamaño: $DUMP_SIZE)${NC}\n"
else
    echo -e "${RED}❌ Error al crear el dump${NC}"
    exit 1
fi

# Paso 2: Transferir el dump al servidor remoto
echo -e "${YELLOW}📤 Paso 2: Transfiriendo dump al servidor remoto...${NC}"

# Instalar sshpass si no está instalado (para usar contraseña)
if ! command -v sshpass &> /dev/null; then
    echo -e "${YELLOW}⚠️  sshpass no está instalado. Instalando...${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS
        if command -v brew &> /dev/null; then
            brew install hudochenkov/sshpass/sshpass
        else
            echo -e "${RED}❌ Por favor instala sshpass: brew install hudochenkov/sshpass/sshpass${NC}"
            exit 1
        fi
    else
        # Linux
        sudo apt-get update && sudo apt-get install -y sshpass
    fi
fi

REMOTE_DUMP_FILE="/tmp/ghost_local_backup.sql"

sshpass -p "$REMOTE_SSH_PASSWORD" scp -P "$REMOTE_SSH_PORT" \
    -o StrictHostKeyChecking=no \
    -o UserKnownHostsFile=/dev/null \
    "$DUMP_FILE" \
    "$REMOTE_SSH_USER@$REMOTE_HOST:$REMOTE_DUMP_FILE"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Dump transferido exitosamente${NC}\n"
else
    echo -e "${RED}❌ Error al transferir el dump${NC}"
    rm -f "$DUMP_FILE"
    exit 1
fi

# Paso 3: Crear base de datos en el servidor remoto e importar
echo -e "${YELLOW}📥 Paso 3: Importando base de datos en el servidor remoto...${NC}"

sshpass -p "$REMOTE_SSH_PASSWORD" ssh -p "$REMOTE_SSH_PORT" \
    -o StrictHostKeyChecking=no \
    -o UserKnownHostsFile=/dev/null \
    "$REMOTE_SSH_USER@$REMOTE_HOST" << EOF
    # Crear base de datos si no existe
    mysql -u root -e "CREATE DATABASE IF NOT EXISTS $REMOTE_DB_NAME CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;" || true
    
    # Importar dump
    mysql -u root "$REMOTE_DB_NAME" < "$REMOTE_DUMP_FILE"
    
    # Limpiar archivo temporal remoto
    rm -f "$REMOTE_DUMP_FILE"
    
    echo "✅ Base de datos importada exitosamente"
EOF

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✅ Base de datos importada exitosamente en el servidor remoto${NC}\n"
else
    echo -e "${RED}❌ Error al importar la base de datos${NC}"
    rm -f "$DUMP_FILE"
    exit 1
fi

# Paso 4: Limpiar archivo temporal local
echo -e "${YELLOW}🧹 Limpiando archivos temporales...${NC}"
rm -f "$DUMP_FILE"
echo -e "${GREEN}✅ Archivos temporales eliminados${NC}\n"

echo -e "${GREEN}🎉 ¡Copia de base de datos completada exitosamente!${NC}"
echo -e "${GREEN}📊 Base de datos '$LOCAL_DB_NAME' copiada a '$REMOTE_DB_NAME' en $REMOTE_HOST${NC}\n"

