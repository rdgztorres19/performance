#!/usr/bin/env node

/**
 * Script para insertar artículo automáticamente en Ghost usando la API
 * 
 * Uso: node insert-article.js [ruta-al-articulo.md]
 * Ejemplo: node insert-article.js ../articles/example-article.md
 */

const fs = require('fs');
const path = require('path');

// Cargar GhostAdminAPI (desde node_modules de la raíz o ghost-blog)
let GhostAdminAPI;
try {
    // Intentar desde node_modules de la raíz primero
    GhostAdminAPI = require('@tryghost/admin-api');
} catch (e) {
    // Si no está, intentar desde ghost-blog/node_modules
    try {
        const ghostBlogPath = path.join(__dirname, '..', 'ghost-blog');
        const adminApiPath = path.join(ghostBlogPath, 'node_modules', '@tryghost', 'admin-api');
        if (fs.existsSync(adminApiPath)) {
            GhostAdminAPI = require(adminApiPath);
        } else {
            throw new Error('@tryghost/admin-api no encontrado');
        }
    } catch (e2) {
        console.error('❌ Error: @tryghost/admin-api no está instalado.');
        console.error('Por favor ejecuta: npm install');
        process.exit(1);
    }
}

// Configuración - usa variables de entorno o defaults
const GHOST_URL = process.env.GHOST_URL || 'https://perfstack.dev';
const GHOST_ADMIN_API_KEY = process.env.GHOST_ADMIN_API_KEY || '694f2c677f4d7d685707e3f4:a8ecd428b0cfdb67d2f797d8980736629918825e38f56c3bd14f5be0ec3d9a92';

// Inicializar Ghost Admin API
const api = new GhostAdminAPI({
    url: GHOST_URL,
    key: GHOST_ADMIN_API_KEY,
    version: 'v5.0'
});

// Limpiar contenido (remover comentarios HTML)
function cleanContent(markdown) {
    // Remover comentarios HTML como <!-- Tags sugeridos: ... -->
    return markdown.replace(/<!--[\s\S]*?-->/g, '').trim();
}

// Extraer título del markdown
function extractTitle(markdown) {
    const match = markdown.match(/^#\s+(.+)$/m);
    return match ? match[1].trim() : 'Untitled';
}

// Extraer tags sugeridos del comentario
function extractTags(markdown) {
    // Buscar formato: <!-- Tags: tag1, tag2, tag3 -->
    // O formato: <!-- Tags sugeridos: tag1, tag2, tag3 -->
    const match = markdown.match(/<!--\s*Tags\s*(?:sugeridos)?:?\s*(.+?)\s*-->/i);
    if (match) {
        return match[1].split(',').map(tag => tag.trim()).filter(tag => tag.length > 0);
    }
    return ['Performance', 'Optimization'];
}

// Crear o obtener tag
async function getOrCreateTag(tagName) {
    try {
        // Buscar tag existente
        const tags = await api.tags.browse({ filter: `name:'${tagName}'` });
        
        if (tags && tags.length > 0) {
            return tags[0];
        }
        
        // Crear nuevo tag
        const newTag = await api.tags.add({
            name: tagName
        });
        
        return newTag;
    } catch (error) {
        // Si el tag ya existe, intentar buscarlo de nuevo
        try {
            const tags = await api.tags.browse({ filter: `name:'${tagName}'` });
            if (tags && tags.length > 0) {
                return tags[0];
            }
        } catch (e) {
            // Ignorar
        }
        console.error(`Error con tag "${tagName}":`, error.message);
        return null;
    }
}

// Generar nombre de archivo seguro desde el título
function generateSafeFileName(title) {
    return title
        .toLowerCase()
        .replace(/[^a-z0-9]+/g, '-')
        .replace(/^-+|-+$/g, '')
        .substring(0, 100) // Limitar longitud
        + '.md';
}

// Copiar archivo a la carpeta articles
function copyToArticlesFolder(sourcePath, title) {
    try {
        // Crear ruta a la carpeta articles (relativa al directorio del proyecto)
        const articlesDir = path.join(__dirname, '..', 'articles');
        
        // Asegurar que la carpeta existe
        if (!fs.existsSync(articlesDir)) {
            fs.mkdirSync(articlesDir, { recursive: true });
            console.log(`📁 Carpeta creada: ${articlesDir}`);
        }
        
        // Generar nombre de archivo destino
        const fileName = generateSafeFileName(title);
        const destPath = path.join(articlesDir, fileName);
        
        // Copiar archivo
        fs.copyFileSync(sourcePath, destPath);
        console.log(`📋 Archivo copiado a: ${destPath}`);
        
        return destPath;
    } catch (error) {
        console.error(`⚠️  Error al copiar archivo: ${error.message}`);
        // No fallar el proceso completo si falla la copia
        return null;
    }
}

// Función principal
async function insertArticle(articlePath) {
    try {
        // Leer archivo
        if (!fs.existsSync(articlePath)) {
            throw new Error(`Archivo no encontrado: ${articlePath}`);
        }
        
        const markdown = fs.readFileSync(articlePath, 'utf-8');
        const title = extractTitle(markdown);
        const content = cleanContent(markdown);
        const suggestedTags = extractTags(markdown);
        
        console.log(`\n📄 Insertando artículo: "${title}"\n`);
        
        // Obtener o crear tags
        console.log('🏷️  Procesando tags...');
        const tags = [];
        for (const tagName of suggestedTags) {
            const tag = await getOrCreateTag(tagName);
            if (tag) {
                tags.push(tag);
                console.log(`   ✓ Tag: ${tagName}`);
            }
        }
        
        // Crear post
        console.log('\n📝 Creando post en Ghost...');
        
        // Ghost acepta markdown directamente en el campo 'mobiledoc'
        // Usamos el formato mobiledoc con card de markdown
        const mobiledoc = {
            version: "0.3.1",
            atoms: [],
            cards: [
                ["markdown", { markdown: content }]
            ],
            markups: [],
            sections: [
                [10, 0] // Card section
            ]
        };
        
        const post = await api.posts.add({
            title: title,
            status: 'published',
            mobiledoc: JSON.stringify(mobiledoc),
            tags: tags.map(tag => ({ id: tag.id }))
        });
        
        console.log('\n✅ ¡Artículo publicado exitosamente!');
        const postUrl = post.url.startsWith('http') ? post.url : `${GHOST_URL}${post.url}`;
        console.log(`\n🔗 URL: ${postUrl}`);
        
        // Copiar archivo a la carpeta articles
        console.log('\n📋 Copiando archivo a carpeta articles...');
        copyToArticlesFolder(articlePath, title);
        console.log('');
        
    } catch (error) {
        console.error('\n❌ Error:', error.message);
        if (error.errors && error.errors.length > 0) {
            error.errors.forEach(err => {
                console.error(`   - ${err.message}`);
            });
        }
        if (error.message.includes('401') || error.message.includes('Unauthorized') || error.message.includes('Invalid token') || error.message.includes('Invalid API Key')) {
            console.error('\n💡 El API Key parece ser inválido. Verifica:');
            console.error('   1. Ve a Ghost Admin → Settings → Integrations');
            console.error('   2. Asegúrate de copiar el "Admin API Key" COMPLETO');
            console.error('   3. El formato debe ser: id:secret (dos partes separadas por :)');
            console.error('   4. No debe tener espacios al inicio o final');
            console.error('   5. Configura GHOST_ADMIN_API_KEY como variable de entorno\n');
        }
        process.exit(1);
    }
}

// Main
const articlePath = process.argv[2] || path.join(__dirname, '../example-article.md');

if (!articlePath || articlePath === 'undefined') {
    console.error('❌ Error: Debes proporcionar la ruta al archivo markdown');
    console.error('   Uso: node insert-article.js [ruta-al-articulo.md]');
    process.exit(1);
}

insertArticle(articlePath);

