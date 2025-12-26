# Código SEO para Ghost - Listo para Copiar

## Paso 1: Acceder a Ghost Admin

1. Abre tu navegador
2. Ve a: **http://localhost:2368/ghost**
3. Crea tu cuenta de administrador

## Paso 2: Configurar SEO en Site Header

Ve a: **Settings → Code Injection → Site Header**

Copia y pega este código:

```html
<!-- SEO Meta Tags -->
<meta name="description" content="Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#">
<meta name="keywords" content="performance, optimization, .NET, C#, programming, software development, best practices">
<meta name="author" content="Tu Nombre">

<!-- Open Graph / Facebook -->
<meta property="og:type" content="website">
<meta property="og:url" content="https://tudominio.com/">
<meta property="og:title" content="Performance Optimization Guide">
<meta property="og:description" content="Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#">
<meta property="og:image" content="https://tudominio.com/content/images/tu-imagen.jpg">

<!-- Twitter -->
<meta property="twitter:card" content="summary_large_image">
<meta property="twitter:url" content="https://tudominio.com/">
<meta property="twitter:title" content="Performance Optimization Guide">
<meta property="twitter:description" content="Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#">
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

<!-- Preconnect para recursos externos -->
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="dns-prefetch" href="https://pagead2.googlesyndication.com">
<link rel="dns-prefetch" href="https://googleads.g.doubleclick.net">
```

## Paso 3: Configurar AdSense en Site Header

**IMPORTANTE**: Reemplaza `ca-pub-TU_PUBLISHER_ID` con tu ID real de AdSense.

Agrega esto DESPUÉS del código SEO anterior:

```html
<!-- Google AdSense -->
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>
```

## Paso 4: Activar AdSense en Site Footer

Ve a: **Settings → Code Injection → Site Footer**

Copia y pega:

```html
<!-- Activar Auto Ads de Google AdSense -->
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

## Paso 5: Configurar Settings Generales

Ve a: **Settings → General**

Configura:
- **Site Title**: "Performance Optimization Guide"
- **Site Description**: "Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#"
- **Site URL**: `http://localhost:2368` (por ahora, cambiarás a tu dominio después)

## Listo!

Ahora tu blog tiene SEO y AdSense configurados.

