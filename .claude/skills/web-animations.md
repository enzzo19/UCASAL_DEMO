# Web Animations — Micro-Interacciones y Efectos Visuales

Skill de referencia para animaciones CSS, transiciones, glassmorphism y micro-interacciones  
en dashboards industriales IoT. Basada en tendencias UI 2025-2026.

---

## Principios de Animación

### Regla de Oro
Las animaciones deben **informar**, no decorar. Cada animación tiene un propósito:
- **Feedback**: confirmar que una acción ocurrió (click en card)
- **Estado**: indicar que algo está vivo (pulse en status dot)
- **Transición**: suavizar cambio de contexto (fade entre vistas)
- **Atención**: dirigir la mirada a datos críticos (glow en alerta)

### Timing Functions
```css
/* Para UI general — suave y natural */
transition: all 0.3s ease;

/* Para cambios de vista — ligeramente elástico */
transition: all 0.35s cubic-bezier(0.4, 0, 0.2, 1);

/* Para datos que se actualizan — rápido y preciso */
animation: { duration: 300 }  /* Chart.js */
```

---

## Catálogo de Micro-Interacciones

### 1. Hover en Cards de Sensores
```css
.card {
    transition: all 0.3s ease;
}
.card:hover {
    background: var(--bg-card-hover);
    transform: translateY(-2px);
}
```
**Propósito**: Feedback táctil, indicar que es interactivo.

### 2. Pulse en Status Dot
```css
@keyframes pulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.4; }
}
.status-dot {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: currentColor;
    animation: pulse 2s ease-in-out infinite;
}
```
**Propósito**: Indicar conexión activa ("está vivo").

### 3. Spinner de Carga
```css
@keyframes spin {
    to { transform: rotate(360deg); }
}
.spinner {
    width: 48px;
    height: 48px;
    border: 4px solid var(--border);
    border-top-color: var(--accent-amber);
    border-radius: 50%;
    animation: spin 0.8s linear infinite;
}
```
**Propósito**: Feedback visual durante conexión a Firebase.

### 4. Fade de Overlay
```css
.overlay {
    transition: opacity 0.5s;
}
.overlay.hidden {
    opacity: 0;
    pointer-events: none;
}
```
**Propósito**: Transición suave al conectar.

### 5. View Selector — Card Activa
```css
.view-card {
    transition: all 0.35s cubic-bezier(0.4, 0, 0.2, 1);
}
.view-card:hover {
    border-color: rgba(var(--card-accent-rgb), 0.4);
    transform: translateY(-3px);
}
.view-card.active {
    border-color: rgba(var(--card-accent-rgb), 0.7);
    box-shadow: 0 0 24px rgba(var(--card-accent-rgb), 0.15),
                inset 0 1px 0 rgba(var(--card-accent-rgb), 0.1);
}
```
**Propósito**: Identificar vista activa con glow del color de acento.

### 6. Fade entre Vistas
```css
.fade-section {
    transition: opacity 0.25s ease;
}
.fade-section.fading {
    opacity: 0.3;
}
```
```javascript
// JS: fade out → update → fade in
sections.forEach(s => s.classList.add('fading'));
setTimeout(() => {
    applyView(v, viewId);
    sections.forEach(s => s.classList.remove('fading'));
}, 200);
```
**Propósito**: Transición suave al cambiar de aplicación industrial.

### 7. Indicador de Card Activa (punto verde)
```css
.view-card-indicator {
    width: 8px;
    height: 8px;
    border-radius: 50%;
    background: var(--accent-green);
    opacity: 0;
    transition: opacity 0.3s;
}
.view-card.active .view-card-indicator {
    opacity: 1;
}
```

---

## Glassmorphism — Efecto Vidrio Esmerilado

### Cuándo Usar
- Overlays sobre contenido (loading, modals)
- Cards premium o destacadas
- Paneles flotantes de información

### Implementación CSS
```css
.glass-card {
    background: rgba(20, 27, 45, 0.7);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 16px;
    box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
}
```

### Glass sobre fondo con gradiente
```css
.glass-overlay {
    background: rgba(10, 14, 23, 0.6);
    backdrop-filter: blur(20px) saturate(1.5);
    border: 1px solid rgba(255, 255, 255, 0.05);
}
```

### Reglas de Accesibilidad
- Siempre asegurar contraste de texto (WCAG AA mínimo)
- No usar glass para texto crítico — fondo turbio dificulta lectura
- Fallback sin `backdrop-filter`: fondo sólido con opacidad alta

---

## Neumorphism — Sombras Suaves

### Cuándo Usar (con precaución)
- Botones de acción secundaria
- Toggles y switches
- NO usar para texto o datos críticos (bajo contraste)

### Implementación CSS
```css
.neu-button {
    background: var(--bg-card);
    border-radius: 12px;
    box-shadow:
        6px 6px 12px rgba(0, 0, 0, 0.4),
        -6px -6px 12px rgba(255, 255, 255, 0.03);
}
.neu-button:active {
    box-shadow:
        inset 4px 4px 8px rgba(0, 0, 0, 0.4),
        inset -4px -4px 8px rgba(255, 255, 255, 0.03);
}
```

### Advertencia
Neumorphism tiene **baja accesibilidad** en temas oscuros.  
Preferir glassmorphism para dashboards industriales.

---

## Gradientes Dinámicos

### Gradiente de Header
```css
background: linear-gradient(135deg, #141b2d 0%, #1a1f3a 100%);
```

### Gradiente en Texto (Logo)
```css
.logo-text {
    background: linear-gradient(135deg, var(--accent-amber), var(--accent-orange));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
}
```

### Gradiente de Barra de Card (por tipo de sensor)
```css
.card.temp::before {
    background: linear-gradient(90deg, var(--accent-red), var(--accent-orange));
}
```

### Glow Radial en Cards de Vista
```css
.view-card::before {
    content: '';
    position: absolute;
    inset: 0;
    background: radial-gradient(
        ellipse at 30% 50%,
        rgba(var(--card-accent-rgb), 0.06),
        transparent 70%
    );
    opacity: 0;
    transition: opacity 0.35s;
}
.view-card:hover::before,
.view-card.active::before {
    opacity: 1;
}
```

---

## Barras RSSI Animadas

```css
.rssi-bar {
    width: 6px;
    border-radius: 2px;
    background: var(--border);
    transition: background 0.3s;
}
.rssi-bar.active {
    background: var(--accent-green);
}
/* Alturas escalonadas */
.rssi-bar:nth-child(1) { height: 5px; }
.rssi-bar:nth-child(2) { height: 9px; }
.rssi-bar:nth-child(3) { height: 13px; }
.rssi-bar:nth-child(4) { height: 17px; }
.rssi-bar:nth-child(5) { height: 20px; }
```

---

## Animaciones Avanzadas (para futuro)

### Data Flash — Parpadeo al actualizar valor
```css
@keyframes data-flash {
    0% { background: rgba(var(--view-accent-rgb), 0.2); }
    100% { background: transparent; }
}
.card-value.updated {
    animation: data-flash 0.6s ease-out;
}
```

### Alert Shake — Vibración en alerta crítica
```css
@keyframes shake {
    0%, 100% { transform: translateX(0); }
    25% { transform: translateX(-4px); }
    75% { transform: translateX(4px); }
}
.alert-crit.new {
    animation: shake 0.4s ease-out;
}
```

### Counter Roll — Conteo animado
```javascript
function animateValue(element, start, end, duration) {
    const range = end - start;
    const startTime = performance.now();
    function update(now) {
        const elapsed = now - startTime;
        const progress = Math.min(elapsed / duration, 1);
        const eased = 1 - Math.pow(1 - progress, 3); // ease-out cubic
        element.textContent = (start + range * eased).toFixed(1);
        if (progress < 1) requestAnimationFrame(update);
    }
    requestAnimationFrame(update);
}
```

---

## Performance

### Reglas de Optimización
1. **Solo animar `transform` y `opacity`** — propiedades composited (GPU)
2. **Evitar animar `width`, `height`, `top`, `left`** — causan reflow
3. **`will-change`** solo en elementos que realmente se animarán
4. **`animation: none`** en `prefers-reduced-motion: reduce`

```css
@media (prefers-reduced-motion: reduce) {
    *, *::before, *::after {
        animation-duration: 0.01ms !important;
        transition-duration: 0.01ms !important;
    }
}
```

---

## Fuentes de Referencia
- Colorlib — Dark Admin Dashboard Templates 2026
- SkillValix — CSS Micro Animations Complete Guide 2026
- Zignuts — Neumorphism vs Glassmorphism 2026
- Contra — Design Trends 2025
- DesignStudioUX — Glassmorphism Best Practices
