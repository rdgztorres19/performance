# Cómo Obtener y Configurar tu Admin API Key de Ghost

## Pasos Rápidos

### 1. Acceder a Ghost Admin

1. Abre: **http://localhost:2368/ghost**
2. Inicia sesión

### 2. Ir a Integrations

1. Click en **Settings** (en el menú lateral)
2. Click en **Integrations** (en el submenú)

### 3. Crear Custom Integration

1. Scroll hacia abajo hasta **"Custom integrations"**
2. Click en **"Add custom integration"**
3. Dale un nombre: `Script Integration` (o el que prefieras)
4. Click en **"Create"**

### 4. Copiar Admin API Key

1. Verás la página de la integration creada
2. Busca la sección **"Admin API"**
3. Copia el **"Admin API Key"** completo

**⚠️ IMPORTANTE:**
- El formato debe ser: `id:secret` (dos partes separadas por `:`)
- NO debe tener espacios al inicio o final
- Debe ser el "Admin API Key" (no el "Content API Key")
- Copia TODO el texto, desde el inicio hasta el final

Ejemplo de formato correcto:
```
694e153e2374c1121cd015e4:a92f5dd286ab73a128e2aa85049b08cdf98706a59ea539c245bd8dd812a06524
```

### 5. Configurar en el Script

Edita el archivo `insert-article.js` y actualiza la línea:
```javascript
const GHOST_ADMIN_API_KEY = 'tu-api-key-completo-aqui';
```

### 6. Probar el API Key

```bash
cd /Users/rdgztorres19/Documents/Projects/performance/ghost-blog
node test-api-key.js
```

Si ves `✅ API Key válido!`, entonces está correcto.

---

## Si el API Key no funciona

1. **Crea una nueva Integration:**
   - Settings → Integrations
   - "Add custom integration"
   - Dale un nombre
   - Copia el nuevo API Key

2. **Verifica que Ghost esté corriendo:**
   ```bash
   curl http://localhost:2368/ghost/api/admin/site/
   ```

3. **Verifica que no haya espacios:**
   - El API Key no debe tener espacios al inicio o final
   - No debe tener saltos de línea

---

**¡Listo!** Con el API key configurado, el script insertará artículos automáticamente.

