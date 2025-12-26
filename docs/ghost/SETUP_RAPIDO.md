# Setup Rápido - Ghost Local con SEO y AdSense

Guía rápida paso a paso para tener Ghost funcionando localmente con SEO y AdSense.

## ⚡ Setup en 5 Minutos

### 1. Instalar Ghost CLI

```bash
npm install -g ghost-cli@latest
```

### 2. Crear e Instalar Ghost

```bash
# Crear directorio
mkdir ~/ghost-local
cd ~/ghost-local

# Instalar Ghost
ghost install local
```

**Cuando pregunte por MySQL:**
- Host: `localhost`
- Usuario: `root` (o tu usuario)
- Password: (tu password MySQL)
- Database: `ghost_local`

### 3. Iniciar Ghost

```bash
ghost start
```

Abre: **http://localhost:2368/ghost**

Crea tu cuenta de admin.

## 🔍 Configurar SEO

### En Ghost Admin:

1. **Settings → General**:
   - Site Title: "Performance Optimization Guide"
   - Site Description: "Guía completa de técnicas de optimización de rendimiento"
   - Site URL: `http://localhost:2368`

2. **Settings → Code Injection → Site Header**:

```html
<!-- SEO Meta Tags -->
<meta name="description" content="Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#">
<meta name="keywords" content="performance, optimization, .NET, C#, programming">

<!-- Open Graph -->
<meta property="og:type" content="website">
<meta property="og:title" content="Performance Optimization Guide">
<meta property="og:description" content="Guía completa de técnicas de optimización de rendimiento">
<meta property="og:image" content="URL_DE_TU_IMAGEN">

<!-- Twitter -->
<meta property="twitter:card" content="summary_large_image">
<meta property="twitter:title" content="Performance Optimization Guide">
<meta property="twitter:description" content="Guía completa de técnicas de optimización de rendimiento">
```

## 💰 Configurar AdSense

### 1. Obtener Publisher ID

- Ve a [AdSense](https://www.google.com/adsense/)
- Crea cuenta y obtén tu ID: `ca-pub-XXXXXXXXXX`

### 2. Agregar en Site Header

**Settings → Code Injection → Site Header**:

```html
<!-- Google AdSense -->
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>
```

**Reemplaza `TU_PUBLISHER_ID`** con tu ID real.

### 3. Activar en Site Footer

**Settings → Code Injection → Site Footer**:

```html
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

## ✅ Verificar

1. Abre **http://localhost:2368**
2. Click derecho → "Ver código fuente"
3. Busca tus meta tags (SEO)
4. Busca `googlesyndication.com` (AdSense)

## 📝 Agregar Primer Artículo

```bash
# Desde el directorio del proyecto
cd /Users/rdgztorres19/Documents/Projects/performance

# Extraer técnica
node convert-single-article.js "nombre-tecnica"

# Copiar contenido y pegar en Ghost Admin → New Post
```

## 🚀 Listo!

- ✅ Ghost corriendo en localhost
- ✅ SEO configurado
- ✅ AdSense configurado
- ✅ Listo para publicar

**Próximo paso**: Agregar artículos usando `QUICK_START.md`

