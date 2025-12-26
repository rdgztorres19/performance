# ✅ Ghost Instalado - Próximos Pasos

## 🎉 Estado Actual

- ✅ Ghost instalado y corriendo
- ✅ Disponible en: **http://localhost:2368**
- ✅ Admin en: **http://localhost:2368/ghost**

## 📝 Pasos Inmediatos

### 1. Crear Cuenta de Admin (2 minutos)

1. Abre: **http://localhost:2368/ghost**
2. Completa el formulario:
   - Nombre
   - Email
   - Contraseña
3. Click en "Create account & start publishing"

### 2. Configurar SEO (5 minutos)

1. En Ghost Admin, ve a: **Settings → Code Injection**
2. Click en **Site Header**
3. Abre el archivo `SEO_CODE.md` en este directorio
4. Copia TODO el código HTML
5. Pégalo en el campo "Site Header"
6. Click en "Save"

### 3. Configurar AdSense (3 minutos)

**Primero obtén tu Publisher ID:**
1. Ve a [Google AdSense](https://www.google.com/adsense/)
2. Crea cuenta o inicia sesión
3. Agrega tu sitio (puedes usar localhost para desarrollo)
4. Copia tu Publisher ID (formato: `ca-pub-XXXXXXXXXX`)

**Luego en Ghost:**

1. **Site Header** (Settings → Code Injection → Site Header):
   - Abre `ADSENSE_CODE.md`
   - Copia el código de "Site Header"
   - **Reemplaza** `ca-pub-TU_PUBLISHER_ID` con tu ID real
   - Pégalo DESPUÉS del código SEO
   - Save

2. **Site Footer** (Settings → Code Injection → Site Footer):
   - Copia el código de "Site Footer" de `ADSENSE_CODE.md`
   - Pégalo
   - Save

### 4. Configurar Settings Generales (2 minutos)

Ve a: **Settings → General**

- **Site Title**: "Performance Optimization Guide"
- **Site Description**: "Guía completa de técnicas de optimización de rendimiento para aplicaciones .NET y C#"
- **Site URL**: `http://localhost:2368` (por ahora)

Click en "Save"

### 5. Agregar Primer Artículo (5 minutos)

```bash
# Desde el directorio del proyecto
cd /Users/rdgztorres19/Documents/Projects/performance

# Extraer una técnica
node convert-single-article.js "nombre-tecnica"

# Copiar el contenido mostrado
# Ir a Ghost Admin → New Post
# Pegar contenido
# Agregar tags sugeridos
# Publicar
```

## 🎯 Resumen

1. ✅ Crear cuenta admin → http://localhost:2368/ghost
2. ✅ Configurar SEO → Settings → Code Injection → Site Header
3. ✅ Configurar AdSense → Settings → Code Injection (Header + Footer)
4. ✅ Settings generales → Settings → General
5. ✅ Agregar primer artículo usando el script

## 📁 Archivos de Ayuda

- `SEO_CODE.md` - Código SEO listo para copiar
- `ADSENSE_CODE.md` - Código AdSense listo para copiar
- `PASOS_AHORA.md` - Este archivo

## 🚀 Comandos Útiles

```bash
# Ver estado
ghost status

# Iniciar Ghost
ghost start

# Detener Ghost
ghost stop

# Reiniciar Ghost
ghost restart

# Ver logs
ghost log
```

## 💡 Tips

- Ghost usa SQLite por defecto en modo local (perfecto para desarrollo)
- Cuando estés listo para producción, puedes exportar e importar en servidor
- Los anuncios pueden tardar en aparecer (24-48 horas después de aprobación)
- En localhost, AdSense puede no funcionar hasta que tengas dominio real

---

**¡Todo listo para empezar a publicar!** 🎉
