#!/usr/bin/env node

/**
 * Script para configurar Ghost con MySQL
 * 
 * Uso: node config-mysql.js
 * 
 * Te pedirá las credenciales de MySQL y actualizará la configuración
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
  console.log('Necesito las credenciales de tu MySQL en Docker:\n');
  
  const host = await question('Host MySQL (default: localhost): ') || 'localhost';
  const port = await question('Puerto MySQL (default: 3306): ') || '3306';
  const user = await question('Usuario MySQL (default: root): ') || 'root';
  const password = await question('Contraseña MySQL: ');
  const database = await question('Nombre de base de datos (default: ghost_local): ') || 'ghost_local';
  
  console.log('\n📝 Creando configuración...\n');
  
  const config = {
    "url": "http://localhost:2368/",
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
    "process": "local",
    "security": {
      "staffDeviceVerification": false
    },
    "paths": {
      "contentPath": path.join(__dirname, "content")
    }
  };
  
  const configPath = path.join(__dirname, 'config.development.json');
  fs.writeFileSync(configPath, JSON.stringify(config, null, 2));
  
  console.log('✅ Configuración guardada en config.development.json\n');
  console.log('📋 Próximos pasos:');
  console.log(`   1. Crear base de datos en MySQL: CREATE DATABASE ${database};`);
  console.log('   2. Instalar dependencia MySQL: npm install mysql2 --save');
  console.log('   3. Reiniciar Ghost: ghost restart\n');
  
  rl.close();
}

configureMySQL().catch(err => {
  console.error('Error:', err);
  rl.close();
  process.exit(1);
});

