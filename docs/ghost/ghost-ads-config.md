# Configuración de Anuncios en Ghost CMS

Esta guía te muestra cómo agregar anuncios a tu sitio Ghost usando el sistema nativo de Code Injection, sin necesidad de modificar el código del tema.

## Método: Code Injection de Ghost

Ghost tiene un sistema integrado de "Code Injection" que permite agregar código HTML/JavaScript en ubicaciones específicas del sitio sin modificar el tema.

## Ubicaciones Disponibles

Ghost permite inyectar código en tres ubicaciones principales:

1. **Site Header**: Aparece en todas las páginas, al inicio del `<head>`
2. **Site Footer**: Aparece en todas las páginas, antes del cierre de `</body>`
3. **Post Content**: Aparece dentro del contenido de los posts (usando hooks)

## Configuración Básica

### Acceder a Code Injection

1. Ve a **Ghost Admin**: `https://tudominio.com/ghost`
2. Click en **Settings** (Configuración) en el menú lateral
3. Click en **Code Injection** en el submenú
4. Verás tres secciones:
   - **Site Header**
   - **Site Footer**
   - **Post Content**

## Configuración de Google AdSense

### Paso 1: Obtener código de AdSense

1. Ve a [Google AdSense](https://www.google.com/adsense/)
2. Crea una cuenta o inicia sesión
3. Crea un nuevo sitio
4. Obtén el código de anuncio (Auto ads o anuncios específicos)

### Paso 2: Agregar Auto Ads (Recomendado para empezar)

Auto Ads es la forma más fácil - Google coloca anuncios automáticamente.

**En Site Header**, agrega:

```html
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>
```

Reemplaza `TU_PUBLISHER_ID` con tu ID de AdSense (formato: `ca-pub-XXXXXXXXXX`).

**En Site Footer**, agrega:

```html
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

### Paso 3: Agregar Anuncios Específicos

Si prefieres controlar dónde aparecen los anuncios:

#### Anuncio en Header (arriba de todo)

**En Site Header**, después del código de Auto Ads:

```html
<!-- Anuncio en Header -->
<div style="text-align: center; margin: 20px 0;">
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-TU_PUBLISHER_ID"
         data-ad-slot="TU_AD_SLOT_ID"
         data-ad-format="auto"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
</div>
```

#### Anuncio en Footer (abajo de todo)

**En Site Footer**, antes del script de Auto Ads:

```html
<!-- Anuncio en Footer -->
<div style="text-align: center; margin: 20px 0;">
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-TU_PUBLISHER_ID"
         data-ad-slot="TU_AD_SLOT_ID"
         data-ad-format="auto"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
</div>
```

#### Anuncios Dentro del Contenido de Posts

**En Post Content**, agrega:

```html
<!-- Anuncio después del primer párrafo -->
<div class="ghost-ad-inline" style="text-align: center; margin: 30px 0;">
    <ins class="adsbygoogle"
         style="display:block"
         data-ad-client="ca-pub-TU_PUBLISHER_ID"
         data-ad-slot="TU_AD_SLOT_ID"
         data-ad-format="auto"
         data-full-width-responsive="true"></ins>
    <script>
         (adsbygoogle = window.adsbygoogle || []).push({});
    </script>
</div>
```

**Nota**: Los anuncios en Post Content aparecerán automáticamente después del primer párrafo en cada post.

## Configuración Completa de Ejemplo

### Site Header

```html
<!-- Google AdSense Auto Ads -->
<script async src="https://pagead2.googlesyndication.com/pagead/js/adsbygoogle.js?client=ca-pub-TU_PUBLISHER_ID"
     crossorigin="anonymous"></script>

<!-- Anuncio en Top -->
<div style="text-align: center; margin: 10px 0; padding: 10px; background: #f5f5f5;">
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

### Site Footer

```html
<!-- Anuncio en Footer -->
<div style="text-align: center; margin: 20px 0; padding: 20px; background: #f5f5f5;">
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

<!-- Script de Auto Ads -->
<script>
     (adsbygoogle = window.adsbygoogle || []).push({});
</script>
```

### Post Content

```html
<!-- Anuncio inline en posts -->
<div class="ghost-ad-inline" style="text-align: center; margin: 30px 0; padding: 15px;">
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

## Otros Proveedores de Anuncios

### Ezoic

```html
<!-- En Site Header -->
<script>
    (function(e,z,i,o){
        if(!e[z]){e[z]=function(){(e[z].q=e[z].q||[]).push(arguments)};
        e[z].l=1*new Date();var m=i.getElementsByTagName(o)[0],
        n=i.createElement(o);n.async=1;n.src="//"+z+".ezoic.com/"+o;
        m.parentNode.insertBefore(n,m)}
    })(window,"ezoic",document,"script");
</script>
```

### Media.net

```html
<!-- En Site Header -->
<script type="text/javascript">
    window._mNHandle = window._mNHandle || {};
    window._mNHandle.queue = window._mNHandle.queue || [];
    medianet_versionId = "YOUR_VERSION_ID";
</script>
<script src="https://contextual.media.net/dmedianet.js?cid=YOUR_CUSTOMER_ID" async="async"></script>
```

### Propeller Ads

```html
<!-- En Site Header -->
<script async="async" data-cfasync="false" src="//upgulpinon.com/1?z=YOUR_ZONE_ID"></script>
```

## Mejores Prácticas

### 1. No Sobre-saturar

- **Máximo 3-4 anuncios por página**
- Evita anuncios que bloqueen el contenido
- Considera la experiencia del usuario

### 2. Ubicaciones Efectivas

- **Header**: Bueno para visibilidad, pero no debe ser intrusivo
- **Sidebar**: Si tu tema tiene sidebar (algunos temas de Ghost no)
- **Entre párrafos**: Efectivo pero no más de 1-2 por post
- **Footer**: Menos intrusivo, buena para retención

### 3. Responsive Design

- Usa `data-full-width-responsive="true"` en AdSense
- Prueba en móvil y desktop
- Los anuncios deben adaptarse al tamaño de pantalla

### 4. Cumplimiento Legal

- **GDPR**: Si tienes visitantes de Europa, necesitas consentimiento
- **Privacy Policy**: Agrega política de privacidad
- **Cookie Notice**: Considera agregar aviso de cookies

### 5. Monitoreo

- Revisa estadísticas en AdSense regularmente
- Ajusta ubicaciones según rendimiento
- Prueba diferentes formatos

## Agregar Consentimiento GDPR (Opcional)

Si necesitas cumplir con GDPR:

### En Site Header, antes de los scripts de ads:

```html
<!-- Cookie Consent -->
<script>
    // Esperar consentimiento antes de cargar ads
    window.consentGiven = false;
    
    function loadAds() {
        if (window.consentGiven) {
            // Cargar scripts de ads aquí
        }
    }
</script>
```

Luego agrega un banner de consentimiento (puedes usar un plugin o código personalizado).

## Verificación

### Cómo verificar que los anuncios funcionan

1. **Espera 24-48 horas**: AdSense puede tardar en aprobar y mostrar anuncios
2. **Modo de prueba**: AdSense tiene un modo de prueba para verificar
3. **Inspeccionar elemento**: Verifica que los scripts se carguen correctamente
4. **Console del navegador**: Revisa que no haya errores de JavaScript

### Verificar en diferentes dispositivos

- Desktop
- Tablet
- Mobile
- Diferentes navegadores

## Solución de Problemas

### Los anuncios no aparecen

1. **Verifica el código**: Asegúrate de que el Publisher ID sea correcto
2. **Espera aprobación**: AdSense puede tardar en aprobar tu sitio
3. **Revisa políticas**: Asegúrate de cumplir con las políticas de AdSense
4. **Modo incógnito**: Prueba en modo incógnito (los bloqueadores de ads pueden interferir)

### Anuncios se ven mal

1. **Responsive**: Asegúrate de usar `data-full-width-responsive="true"`
2. **CSS**: Puedes agregar CSS personalizado en Code Injection
3. **Tamaño**: Ajusta `data-ad-format` según necesites

### Conflictos con el tema

1. **CSS del tema**: Algunos temas pueden tener estilos que interfieren
2. **Z-index**: Ajusta el z-index si los anuncios quedan detrás de otros elementos
3. **Contenedores**: Asegúrate de que los anuncios estén en contenedores apropiados

## CSS Personalizado para Anuncios

Puedes agregar CSS en **Settings → Code Injection → Site Header**:

```html
<style>
    /* Estilos para anuncios */
    .adsbygoogle {
        display: block;
        margin: 20px auto;
    }
    
    /* Anuncios inline en posts */
    .ghost-ad-inline {
        clear: both;
        margin: 30px 0;
    }
    
    /* Responsive */
    @media (max-width: 768px) {
        .adsbygoogle {
            margin: 15px auto;
        }
    }
</style>
```

## Recursos Adicionales

- [Google AdSense Help](https://support.google.com/adsense/)
- [Ghost Code Injection Documentation](https://ghost.org/docs/themes/code-injection/)
- [GDPR Compliance Guide](https://www.gdpr.eu/)

## Próximos Pasos

1. ✅ Configura los anuncios básicos
2. 📊 Monitorea el rendimiento en AdSense
3. 🎯 Optimiza ubicaciones según datos
4. 💰 Considera otros proveedores si es necesario
5. ⚖️ Asegúrate de cumplir con regulaciones locales

