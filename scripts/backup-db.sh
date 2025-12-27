#!/bin/bash

###############################################################################
# Script para hacer backup de la base de datos remota
#
# Uso: ./backup-db.sh
#
# Crea un backup de la base de datos remota y lo guarda localmente
###############################################################################

set -e  # Salir si hay errores

# Colores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuración remota
REMOTE_HOST="66.179.188.92"
REMOTE_SSH_PORT="22"
REMOTE_SSH_USER="root"
REMOTE_SSH_PASSWORD="rY6jPCor"
REMOTE_DB_NAME="blog_prod"
REMOTE_DB_USER="root"
REMOTE_DB_PASSWORD="sbrQp10"

# Directorio donde guardar backups
BACKUP_DIR="./backups"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
BACKUP_FILE="${BACKUP_DIR}/blog_prod_backup_${TIMESTAMP}.sql"

echo -e "${GREEN}🚀 Iniciando backup de base de datos...${NC}\n"

# Crear directorio de backups si no existe
mkdir -p "$BACKUP_DIR"

# Verificar si sshpass está instalado
if ! command -v sshpass &> /dev/null; then
    echo -e "${YELLOW}⚠️  sshpass no está instalado.${NC}"
    if [[ "$OSTYPE" == "darwin"* ]]; then
        echo -e "${YELLOW}Instalando con Homebrew...${NC}"
        if command -v brew &> /dev/null; then
            brew install hudochenkov/sshpass/sshpass
        else
            echo -e "${RED}❌ Por favor instala sshpass: brew install hudochenkov/sshpass/sshpass${NC}"
            exit 1
        fi
    else
        echo -e "${RED}❌ Por favor instala sshpass: sudo apt-get install sshpass${NC}"
        exit 1
    fi
fi

echo -e "${YELLOW}📦 Creando backup de la base de datos remota...${NC}"

# Crear backup en el servidor remoto y descargarlo
sshpass -p "$REMOTE_SSH_PASSWORD" ssh \
    -p "$REMOTE_SSH_PORT" \
    -o StrictHostKeyChecking=no \
    -o UserKnownHostsFile=/dev/null \
    "$REMOTE_SSH_USER@$REMOTE_HOST" \
    "mysqldump -u $REMOTE_DB_USER -p'$REMOTE_DB_PASSWORD' --single-transaction --quick --lock-tables=false $REMOTE_DB_NAME" > "$BACKUP_FILE"

if [ $? -eq 0 ]; then
    BACKUP_SIZE=$(du -h "$BACKUP_FILE" | cut -f1)
    echo -e "${GREEN}✅ Backup creado exitosamente!${NC}"
    echo -e "${GREEN}📁 Archivo: $BACKUP_FILE${NC}"
    echo -e "${GREEN}📊 Tamaño: $BACKUP_SIZE${NC}\n"
    
    # Comprimir el backup (opcional)
    echo -e "${YELLOW}📦 Comprimiendo backup...${NC}"
    gzip -f "$BACKUP_FILE"
    COMPRESSED_FILE="${BACKUP_FILE}.gz"
    COMPRESSED_SIZE=$(du -h "$COMPRESSED_FILE" | cut -f1)
    echo -e "${GREEN}✅ Backup comprimido: $COMPRESSED_FILE (Tamaño: $COMPRESSED_SIZE)${NC}\n"
    
    echo -e "${GREEN}🎉 ¡Backup completado exitosamente!${NC}"
else
    echo -e "${RED}❌ Error al crear el backup${NC}"
    rm -f "$BACKUP_FILE"
    exit 1
fi

