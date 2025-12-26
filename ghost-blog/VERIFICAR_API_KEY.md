# ⚠️ API Key Inválido

El API Key que proporcionaste no está funcionando. Sigue estos pasos para obtenerlo correctamente:

## Pasos para Obtener el API Key Correcto

### 1. Abre Ghost Admin
```
http://localhost:2368/ghost
```

### 2. Ve a Settings → Integrations

### 3. Busca tu Integration o crea una nueva

Si ya tienes una:
- Click en el nombre de la integration
- Busca la sección **"Admin API"**
- Verás algo como:
  ```
  Admin API Key
  [Un código largo que empieza con números/letras]:[otro código largo]
  ```

### 4. Copia el API Key COMPLETO

El formato debe ser:
```
id:secret
```

Ejemplo de formato correcto:
```
694e153e2374c1121cd015e4:a92f5dd286ab73a128e2aa85049b08cdf98706a59ea539c245bd8dd812a06524
```

**⚠️ IMPORTANTE:**
- Debe tener DOS partes separadas por `:`
- NO debe tener espacios
- Debe ser el "Admin API Key" (no el "Content API Key")
- Copia TODO el texto, desde el inicio hasta el final

### 5. Actualiza el script

Edita el archivo `insert-article.js` y reemplaza la línea:
```javascript
const GHOST_ADMIN_API_KEY = 'TU_API_KEY_AQUI';
```

### 6. Prueba el API Key

```bash
cd /Users/rdgztorres19/Documents/Projects/performance/ghost-blog
node test-api-key.js
```

Si ves `✅ API Key válido!`, entonces está correcto.

---

## Si el problema persiste

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

