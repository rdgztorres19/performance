#!/usr/bin/env node

const fs = require('fs');
const path = require('path');

/**
 * Script para extraer una técnica específica del README.md y formatearla para Ghost CMS
 * 
 * Uso: node convert-single-article.js "nombre-de-tecnica"
 * Ejemplo: node convert-single-article.js "Use async and await correctly"
 */

function slugify(text) {
    return text
        .toString()
        .toLowerCase()
        .trim()
        .replace(/\s+/g, '-')
        .replace(/[^\w\-]+/g, '')
        .replace(/\-\-+/g, '-')
        .replace(/^-+/, '')
        .replace(/-+$/, '');
}

function extractSection(content, startIndex) {
    // Buscar el siguiente "---" o el siguiente "##" que no sea "###"
    let endIndex = content.length;
    
    // Buscar el siguiente separador "---" después del inicio
    const nextSeparator = content.indexOf('\n---\n', startIndex);
    if (nextSeparator !== -1) {
        endIndex = nextSeparator;
    }
    
    // Si no hay separador, buscar el siguiente "## " (sección principal)
    const nextMainSection = content.indexOf('\n## ', startIndex);
    if (nextMainSection !== -1 && nextMainSection < endIndex) {
        endIndex = nextMainSection;
    }
    
    return content.substring(startIndex, endIndex).trim();
}

function findTechnique(content, searchTerm) {
    const lines = content.split('\n');
    const searchLower = searchTerm.toLowerCase();
    
    // Buscar todas las técnicas (###)
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (line.startsWith('### ')) {
            const title = line.substring(4).trim();
            const titleLower = title.toLowerCase();
            
            // Buscar coincidencia exacta o parcial
            if (titleLower.includes(searchLower) || searchLower.includes(titleLower)) {
                // Encontrar el inicio del contenido (después del título)
                let startIndex = content.indexOf(line);
                
                // Buscar el inicio real del contenido (saltar líneas vacías después del título)
                let contentStart = startIndex + line.length;
                while (contentStart < content.length && 
                       (content[contentStart] === '\n' || content[contentStart] === ' ')) {
                    contentStart++;
                }
                
                // Extraer toda la sección hasta el siguiente "---" o "##"
                const section = extractSection(content, contentStart);
                
                return {
                    title: title,
                    content: section,
                    fullSection: line + '\n\n' + section
                };
            }
        }
    }
    
    return null;
}

function getCategoryFromContent(content, techniqueIndex) {
    // Buscar la sección principal (##) más cercana antes de esta técnica
    const beforeContent = content.substring(0, techniqueIndex);
    const mainSections = beforeContent.match(/\n## [^\n]+/g);
    
    if (mainSections && mainSections.length > 0) {
        const lastSection = mainSections[mainSections.length - 1];
        return lastSection.replace('\n## ', '').trim();
    }
    
    return 'Performance Optimization';
}

function formatForGhost(title, content, category) {
    // El contenido ya está en markdown, solo necesitamos agregar metadata
    const slug = slugify(title);
    const tag = category.replace(/\.NET/g, 'NET').replace(/and/g, '&');
    
    // Crear el markdown formateado para Ghost
    let ghostContent = `# ${title}\n\n`;
    ghostContent += content;
    
    // Agregar tags al final como comentario para referencia
    ghostContent += `\n\n<!-- Tags sugeridos: ${tag}, Performance, Optimization -->`;
    
    return {
        title: title,
        slug: slug,
        content: ghostContent,
        tag: tag,
        category: category
    };
}

function listAllTechniques(content) {
    const techniques = [];
    const lines = content.split('\n');
    
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (line.startsWith('### ')) {
            const title = line.substring(4).trim();
            techniques.push(title);
        }
    }
    
    return techniques;
}

// Main
const args = process.argv.slice(2);

if (args.length === 0) {
    console.log('Uso: node convert-single-article.js "nombre-de-tecnica"');
    console.log('\nEjemplos:');
    console.log('  node convert-single-article.js "Use async and await correctly"');
    console.log('  node convert-single-article.js "async"');
    console.log('\nPara listar todas las técnicas:');
    console.log('  node convert-single-article.js --list');
    process.exit(1);
}

if (args[0] === '--list') {
    const readmePath = path.join(__dirname, 'README.md');
    if (!fs.existsSync(readmePath)) {
        console.error('Error: README.md no encontrado');
        process.exit(1);
    }
    
    const content = fs.readFileSync(readmePath, 'utf-8');
    const techniques = listAllTechniques(content);
    
    console.log(`\nTotal de técnicas encontradas: ${techniques.length}\n`);
    techniques.forEach((tech, index) => {
        console.log(`${index + 1}. ${tech}`);
    });
    process.exit(0);
}

const searchTerm = args.join(' ');
const readmePath = path.join(__dirname, 'README.md');

if (!fs.existsSync(readmePath)) {
    console.error('Error: README.md no encontrado en el directorio actual');
    process.exit(1);
}

const content = fs.readFileSync(readmePath, 'utf-8');
const technique = findTechnique(content, searchTerm);

if (!technique) {
    console.error(`\n❌ No se encontró la técnica: "${searchTerm}"`);
    console.log('\n💡 Sugerencias:');
    console.log('  - Usa --list para ver todas las técnicas disponibles');
    console.log('  - Intenta con una parte del nombre de la técnica');
    console.log('  - Verifica que el nombre sea exacto (case-insensitive)');
    process.exit(1);
}

// Encontrar la categoría
const techniqueIndex = content.indexOf(technique.fullSection);
const category = getCategoryFromContent(content, techniqueIndex);

// Formatear para Ghost
const ghostFormat = formatForGhost(technique.title, technique.content, category);

// Mostrar resultado
console.log('\n' + '='.repeat(80));
console.log('📄 ARTÍCULO LISTO PARA GHOST');
console.log('='.repeat(80));
console.log(`\n📌 Título: ${ghostFormat.title}`);
console.log(`🏷️  Tag sugerido: ${ghostFormat.tag}`);
console.log(`📁 Categoría: ${ghostFormat.category}`);
console.log(`🔗 Slug: ${ghostFormat.slug}`);
console.log('\n' + '-'.repeat(80));
console.log('CONTENIDO (copia y pega en Ghost):');
console.log('-'.repeat(80) + '\n');
console.log(ghostFormat.content);
console.log('\n' + '='.repeat(80));
console.log('✅ Listo para copiar y pegar en Ghost Admin → New Post');
console.log('='.repeat(80) + '\n');

// Opcional: guardar en archivo
const outputFile = `ghost-article-${ghostFormat.slug}.md`;
fs.writeFileSync(outputFile, ghostFormat.content, 'utf-8');
console.log(`💾 También guardado en: ${outputFile}\n`);

