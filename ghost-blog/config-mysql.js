#!/usr/bin/env node

/**
 * Script para configurar Ghost con MySQL
 * 
 * Uso: 
 *   node config-mysql.js
 *   GHOST_PATH=/var/www/ghost-blog node config-mysql.js  (producción)
 *   GHOST_PATH=./ghost-blog node config-mysql.js        (desarrollo)
 * 
 * Te pedirá las credenciales de MySQL y actualizará la configuración
 * en la ruta especificada por GHOST_PATH
 */

const fs = require('fs');
const path = require('path');
const readline = require('readline');

const rl = readline.createInterface({
  input: process.stdin,
  output: process.stdout
});

function question(query) {
  return new Promise(resolve => rl.question(query, resolve));
}

async function configureMySQL() {
  console.log('\n🔧 Configuración de Ghost con MySQL\n');
  
  // Obtener ruta de Ghost desde variable de entorno o usar default
  const GHOST_PATH = process.env.GHOST_PATH || path.join(__dirname, '..', 'ghost-blog');
  const isProduction = process.env.NODE_ENV === 'production';
  const configFileName = isProduction ? 'config.production.json' : 'config.development.json';
  const configPath = path.join(GHOST_PATH, configFileName);
  
  console.log(`📁 Ruta de Ghost: ${GHOST_PATH}`);
  console.log(`📄 Archivo de configuración: ${configPath}\n`);
  
  // Verificar que el directorio existe
  if (!fs.existsSync(GHOST_PATH)) {
    console.error(`❌ Error: El directorio ${GHOST_PATH} no existe.`);
    console.error(`💡 Crea el directorio o configura GHOST_PATH correctamente.`);
    console.error(`   Ejemplo: GHOST_PATH=/var/www/ghost-blog node config-mysql.js\n`);
    rl.close();
    process.exit(1);
  }
  
  console.log('Necesito las credenciales de tu MySQL:\n');
  
  const host = await question('Host MySQL (default: localhost): ') || 'localhost';
  const port = await question('Puerto MySQL (default: 3306): ') || '3306';
  const user = await question('Usuario MySQL (default: root): ') || 'root';
  const password = await question('Contraseña MySQL: ');
  const database = await question(`Nombre de base de datos (default: ghost_${isProduction ? 'production' : 'local'}): `) || `ghost_${isProduction ? 'production' : 'local'}`;
  const url = await question('URL del blog (default: http://localhost:2368/): ') || 'http://localhost:2368/';
  
  console.log('\n📝 Creando configuración...\n');
  
  const config = {
    "url": url,
    "server": {
      "port": 2368,
      "host": "127.0.0.1"
    },
    "database": {
      "client": "mysql2",
      "connection": {
        "host": host,
        "port": parseInt(port),
        "user": user,
        "password": password,
        "database": database
      }
    },
    "mail": {
      "transport": "Direct"
    },
    "logging": {
      "transports": [
        "file",
        "stdout"
      ]
    },
    "process": isProduction ? "systemd" : "local",
    "security": {
      "staffDeviceVerification": false
    },
    "paths": {
      "contentPath": path.join(GHOST_PATH, "content")
    }
  };
  
  fs.writeFileSync(configPath, JSON.stringify(config, null, 2));
  
  console.log(`✅ Configuración guardada en ${configPath}\n`);
  console.log('📋 Próximos pasos:');
  console.log(`   1. Crear base de datos en MySQL: CREATE DATABASE ${database};`);
  console.log(`   2. Reiniciar Ghost: cd ${GHOST_PATH} && ghost restart\n`);
  
  rl.close();
}

configureMySQL().catch(err => {
  console.error('Error:', err);
  rl.close();
  process.exit(1);
});

