# Artículos Generados

Esta carpeta contiene los artículos markdown generados que están listos para ser insertados en Ghost.

## Uso

Para insertar un artículo en Ghost:

```bash
cd /Users/rdgztorres19/Documents/Projects/performance/ghost-blog
npm run insert-article -- ../articles/nombre-del-articulo.md
```

O usando el alias:

```bash
npm run insert -- ../articles/nombre-del-articulo.md
```

## Formato

Los artículos deben seguir este formato:

```markdown
# Título del Artículo

Contenido del artículo en markdown...

<!-- Tags sugeridos: Tag1, Tag2, Tag3 -->
```

