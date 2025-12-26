# Instalación Local de Ghost CMS

Guía para instalar Ghost localmente en tu máquina con Node.js y MySQL ya instalados.

## Requisitos Verificados

- ✅ Node.js instalado
- ✅ MySQL instalado en localhost

## Paso 1: Instalar Ghost CLI

```bash
# Instalar Ghost CLI globalmente
npm install -g ghost-cli@latest

# Verificar instalación
ghost --version
```

## Paso 2: Crear Directorio para Ghost

```bash
# Crear directorio (puedes elegir cualquier ubicación)
mkdir ~/ghost-site
cd ~/ghost-site

# O si prefieres en el proyecto actual
cd /Users/rdgztorres19/Documents/Projects/performance
mkdir ghost-local
cd ghost-local
```

## Paso 3: Instalar Ghost

```bash
# Instalar Ghost en modo desarrollo
ghost install local
```

Durante la instalación, Ghost te preguntará:

1. **¿Continuar?** → `Y`
2. **¿Configurar base de datos?** → `Y`
3. **Base de datos**: 
   - Host: `localhost`
   - Usuario: `root` (o tu usuario MySQL)
   - Contraseña: (tu contraseña MySQL)
   - Nombre de base de datos: `ghost_local` (o el que prefieras)

Ghost creará la base de datos automáticamente.

## Paso 4: Iniciar Ghost

```bash
# Iniciar Ghost
ghost start

# O si ya está corriendo
ghost restart
```

Ghost estará disponible en: **http://localhost:2368**

## Paso 5: Acceder a Ghost Admin

1. Abre tu navegador
2. Ve a: **http://localhost:2368/ghost**
3. Crea tu cuenta de administrador
4. ¡Listo! Ya puedes empezar a usar Ghost

## Comandos Útiles

```bash
# Ver estado
ghost status

# Iniciar
ghost start

# Detener
ghost stop

# Reiniciar
ghost restart

# Ver logs
ghost log

# Actualizar Ghost
ghost update
```

## Configuración de SEO

### 1. Configuración Básica de SEO

En **Ghost Admin → Settings → General**:

- **Site Title**: Tu título del sitio
- **Site Description**: Descripción para SEO (150-160 caracteres)
- **Site URL**: `http://localhost:2368` (local) o tu dominio (producción)
- **Timezone**: Tu zona horaria

### 2. Meta Tags Personalizados

En **Settings → Code Injection → Site Header**, agrega:

```html
<!-- SEO Meta Tags -->
<meta name="description" content="Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#">
<meta name="keywords" content="performance, optimization, .NET, C#, programming, software development">
<meta name="author" content="Tu Nombre">

<!-- Open Graph / Facebook -->
<meta property="og:type" content="website">
<meta property="og:url" content="https://tudominio.com/">
<meta property="og:title" content="Performance Optimization Guide">
<meta property="og:description" content="Guía completa de técnicas de optimización de rendimiento">
<meta property="og:image" content="https://tudominio.com/content/images/tu-imagen.jpg">

<!-- Twitter -->
<meta property="twitter:card" content="summary_large_image">
<meta property="twitter:url" content="https://tudominio.com/">
<meta property="twitter:title" content="Performance Optimization Guide">
<meta property="twitter:description" content="Guía completa de técnicas de optimización de rendimiento">
<meta property="twitter:image" content="https://tudominio.com/content/images/tu-imagen.jpg">

<!-- Schema.org para SEO -->
<script type="application/ld+json">
{
  "@context": "https://schema.org",
  "@type": "WebSite",
  "name": "Performance Optimization Guide",
  "url": "https://tudominio.com/",
  "description": "Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#",
  "publisher": {
    "@type": "Organization",
    "name": "Tu Nombre"
  }
}
</script>
```

### 3. Sitemap.xml (Automático)

Ghost genera automáticamente el sitemap en: `http://localhost:2368/sitemap.xml`

### 4. robots.txt

Ghost genera automáticamente robots.txt en: `http://localhost:2368/robots.txt`

Puedes personalizarlo editando el tema o agregando en **Code Injection**.

### 5. URLs Amigables (Slugs)

Ghost genera slugs automáticamente desde los títulos de posts. Asegúrate de:
- Usar títulos descriptivos
- Incluir palabras clave relevantes
- Mantener URLs cortas y claras

### 6. Canonical URLs

Ghost incluye automáticamente canonical tags. En producción, asegúrate de:
- Configurar la URL correcta en Settings
- Usar HTTPS
- Tener dominio único (sin www y con www)

### 7. Structured Data para Artículos

En **Settings → Code Injection → Site Header**, agrega:

```html
<script type="application/ld+json">
{
  "@context": "https://schema.org",
  "@type": "BlogPosting",
  "headline": "{{title}}",
  "description": "{{excerpt}}",
  "image": "{{feature_image}}",
  "datePublished": "{{published_at}}",
  "dateModified": "{{updated_at}}",
  "author": {
    "@type": "Person",
    "name": "{{author.name}}"
  },
  "publisher": {
    "@type": "Organization",
    "name": "Tu Sitio",
    "logo": {
      "@type": "ImageObject",
      "url": "https://tudominio.com/content/images/logo.png"
    }
  }
}
</script>
```

**Nota**: Ghost tiene variables Handlebars como `{{title}}`, `{{excerpt}}`, etc. que funcionan en los temas, pero en Code Injection necesitas usar JavaScript o configurarlo en el tema.

### 8. Optimización de Imágenes

- Usa imágenes optimizadas (WebP, comprimidas)
- Agrega alt text a todas las imágenes
- Usa featured images en cada post
- Configura lazy loading si es posible

### 9. Velocidad de Carga

En **Settings → Code Injection → Site Header**, agrega:

```html
<!-- Preconnect para recursos externos -->
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>

<!-- DNS Prefetch para AdSense -->
<link rel="dns-prefetch" href="https://pagead2.googlesyndication.com">
<link rel="dns-prefetch" href="https://googleads.g.doubleclick.net">
```

## Configuración de Google AdSense

### Paso 1: Obtener Código de AdSense

1. Ve a [Google AdSense](https://www.google.com/adsense/)
2. Crea cuenta o inicia sesión
3. Agrega tu sitio (puedes usar `localhost` para desarrollo, pero necesitarás el dominio real para aprobación)
4. Obtén tu Publisher ID (formato: `ca-pub-XXXXXXXXXX`)

### Paso 2: Agregar Auto Ads (Más Fácil)

En **Settings → Code Injection → Site Header**, agrega:

```html
<!-- Google AdSense Auto Ads -->
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>
```

**Reemplaza `TU_PUBLISHER_ID`** con tu ID real (ej: `ca-pub-1234567890123456`)

En **Settings → Code Injection → Site Footer**, agrega:

```html
<!-- Activar Auto Ads -->
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

### Paso 3: Agregar Anuncios Específicos (Opcional)

#### Anuncio en Header

En **Site Header**, después del script de Auto Ads:

```html
<!-- Anuncio en Top -->
<div style="text-align: center; margin: 10px 0;">
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-TU_PUBLISHER_ID"
         data-ad-slot="TU_AD_SLOT_ID"
         data-ad-format="horizontal"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
</div>
```

#### Anuncio en Footer

En **Site Footer**, antes del script de activación:

```html
<!-- Anuncio en Footer -->
<div style="text-align: center; margin: 20px 0;">
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-TU_PUBLISHER_ID"
         data-ad-slot="TU_AD_SLOT_ID"
         data-ad-format="horizontal"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
</div>
```

#### Anuncios Dentro de Posts

En **Settings → Code Injection → Post Content**:

```html
<!-- Anuncio inline en posts -->
<div style="text-align: center; margin: 30px 0;">
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-TU_PUBLISHER_ID"
         data-ad-slot="TU_AD_SLOT_ID"
         data-ad-format="rectangle"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
</div>
```

**Nota**: Para obtener `TU_AD_SLOT_ID`, crea unidades de anuncio en AdSense.

### Configuración Completa de Ejemplo

**Site Header completo** (SEO + AdSense):

```html
<!-- SEO Meta Tags -->
<meta name="description" content="Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#">
<meta name="keywords" content="performance, optimization, .NET, C#, programming">

<!-- Preconnect -->
<link rel="preconnect" href="https://pagead2.googlesyndication.com">
<link rel="dns-prefetch" href="https://googleads.g.doubleclick.net">

<!-- Google AdSense -->
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>
```

**Site Footer completo**:

```html
<!-- Activar Auto Ads -->
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

## Verificar Configuración

### Verificar SEO

1. Abre **http://localhost:2368** en el navegador
2. Click derecho → "Ver código fuente"
3. Busca las meta tags que agregaste
4. Verifica que estén presentes

### Verificar AdSense

1. Abre la consola del navegador (F12)
2. Ve a la pestaña "Network"
3. Recarga la página
4. Busca requests a `googlesyndication.com`
5. Si aparecen, AdSense está cargando correctamente

**Nota**: Los anuncios pueden no aparecer inmediatamente:
- AdSense necesita aprobar tu sitio (puede tardar días)
- En localhost, AdSense puede no funcionar (necesitas dominio real)
- Usa el modo de prueba de AdSense para verificar

## Sincronizar a Servidor

Cuando estés listo para mover a producción:

1. **Exportar contenido**:
   - Ghost Admin → Labs → Export
   - Descarga el archivo JSON

2. **En el servidor**:
   - Instala Ghost (ver `GHOST_SETUP.md`)
   - Importa el contenido: Ghost Admin → Labs → Import

3. **O usar Ghost CLI**:
   ```bash
   # En local
   ghost backup
   
   # En servidor
   ghost restore backup-file.zip
   ```

## Solución de Problemas

### Ghost no inicia

```bash
# Ver logs
ghost log

# Verificar MySQL
mysql -u root -p
# Verificar que la base de datos existe

# Reiniciar
ghost restart
```

### Error de conexión a MySQL

```bash
# Verificar que MySQL esté corriendo
# macOS
brew services list

# Linux
sudo systemctl status mysql

# Verificar credenciales en config.development.json
cat config.development.json
```

### AdSense no aparece

- Espera 24-48 horas para aprobación
- En localhost, AdSense puede no funcionar (necesitas dominio)
- Verifica que el Publisher ID sea correcto
- Revisa la consola del navegador por errores

## Próximos Pasos

1. ✅ Ghost instalado localmente
2. ✅ SEO configurado
3. ✅ AdSense configurado
4. 📝 Agregar tu primer artículo (ver `QUICK_START.md`)
5. 🚀 Sincronizar a servidor cuando estés listo

---

**¡Listo para empezar a publicar!** 🎉

