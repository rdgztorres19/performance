# Código AdSense Completo para Ghost

## Configuración Rápida

### 1. Site Header (Settings → Code Injection → Site Header)

```html
<!-- Google AdSense Auto Ads -->
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>

<!-- DNS Prefetch para mejor rendimiento -->
<link rel="dns-prefetch" href="https://pagead2.googlesyndication.com">
<link rel="dns-prefetch" href="https://googleads.g.doubleclick.net">
```

**Reemplaza `ca-pub-TU_PUBLISHER_ID` con tu ID real de AdSense**

### 2. Site Footer (Settings → Code Injection → Site Footer)

```html
<!-- Activar Auto Ads -->
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

## Anuncios Específicos (Opcional)

Si quieres controlar dónde aparecen los anuncios:

### Anuncio en Header (agregar en Site Header)

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

### Anuncio en Footer (agregar en Site Footer)

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

### Anuncios Dentro de Posts (Settings → Code Injection → Post Content)

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

## Cómo Obtener tu Publisher ID

1. Ve a [Google AdSense](https://www.google.com/adsense/)
2. Crea cuenta o inicia sesión
3. Agrega tu sitio
4. Obtén tu Publisher ID (formato: `ca-pub-1234567890123456`)

## Nota Importante

- En localhost, AdSense puede no funcionar (necesitas dominio real)
- Los anuncios pueden tardar 24-48 horas en aparecer después de la aprobación
- Auto Ads es más fácil - Google coloca anuncios automáticamente

