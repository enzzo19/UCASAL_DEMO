# Dashboard UI — Diseño Industrial IoT

Skill de referencia para diseño de dashboards industriales IoT.  
Basada en mejores prácticas de UXPin, IoT For All, Order Group, Sigma Computing.

---

## Principios de Diseño

### Regla de los 5 Segundos
El usuario debe entender el dato más crítico en **menos de 5 segundos**.  
Colocar KPIs primarios arriba-izquierda (patrón F de lectura).

### Jerarquía Visual
```
┌─────────────────────────────────────────┐
│  HEADER: Logo + Status + Navegación     │  ← Identidad + estado
├─────────────────────────────────────────┤
│  SELECTOR DE VISTAS (cards clickeables) │  ← Contexto industrial
├─────────────────────────────────────────┤
│  CONTEXT BOX (descripción de la vista)  │  ← Qué estoy mirando
├─────────────────────────────────────────┤
│  KPI CARDS (6 métricas principales)     │  ← Datos inmediatos
├─────────────────────────────────────────┤
│  CHARTS (tendencias temporales)         │  ← Análisis de tendencia
├─────────────────────────────────────────┤
│  BOTTOM: Signal + Alerts                │  ← Estado sistema + alertas
└─────────────────────────────────────────┘
```

### Proximidad y Agrupación
- Agrupar métricas relacionadas visualmente (temperatura + sensación térmica)
- Usar whitespace generoso entre secciones (1-1.5rem gap)
- Cards con bordes sutiles (`1px solid`) para delimitar sin agobiar

---

## Sistema de Colores — Dark Theme Industrial

### Paleta Base
```css
--bg-primary:    #0a0e17;   /* Fondo principal — casi negro azulado */
--bg-card:       #141b2d;   /* Fondo tarjetas */
--bg-card-hover: #1a2340;   /* Hover tarjetas */
--border:        #1e2940;   /* Bordes sutiles */
--text-primary:  #e2e8f0;   /* Texto principal — blanco cálido */
--text-secondary:#94a3b8;   /* Texto secundario */
--text-muted:    #64748b;   /* Texto desactivado */
```

### Paleta de Acentos (semánticos)
```css
--accent-red:    #ef4444;   /* Crítico, alarma, caldera */
--accent-orange: #f97316;   /* Temperatura, advertencia */
--accent-amber:  #f59e0b;   /* General, batería, industrial */
--accent-green:  #10b981;   /* OK, online, altitud */
--accent-blue:   #3b82f6;   /* Humedad, agua, datos */
--accent-purple: #8b5cf6;   /* Presión, análisis */
--accent-cyan:   #06b6d4;   /* Litio, agua, químico */
```

### Acentos por Vista Industrial
| Vista | Color Primario | RGB | Uso |
|-------|---------------|-----|-----|
| Ambiental | `#f59e0b` | `245,158,11` | Campo, clima |
| Caldera | `#ef4444` | `239,68,68` | Fuego, vapor, peligro |
| Litio | `#06b6d4` | `6,182,212` | Agua, químico, frío |

---

## Cards de Sensores

### Estructura de una Card
```html
<div class="card [tipo]">
  <div class="card-header">
    <span class="card-label">NOMBRE SENSOR</span>
    <span class="card-icon">🌡️</span>
  </div>
  <div class="card-value">
    <span id="valXxx">--</span>
    <span class="card-unit">°C</span>
  </div>
</div>
```

### Reglas de Diseño
- **Barra superior de color** (`::before`, 3px height) identifica categoría
- **Gradientes** en barra: dual-color para cada sensor
- **Font monospace** (JetBrains Mono) para valores numéricos
- **Hover**: elevar card (`translateY(-2px)`) + fondo más claro
- Labels en **UPPERCASE**, `letter-spacing: 1px`, `font-size: 0.75rem`
- Valores en **2rem**, bold, color del acento correspondiente

### Grid Responsivo
```css
.cards-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
    gap: 1rem;
}
/* Mobile: 2 columnas */
@media (max-width: 600px) {
    grid-template-columns: repeat(2, 1fr);
}
```

---

## Vista Multi-Aplicación (View Switching)

### Patrón de Navegación por Cards
3 cards clickeables arriba, cada una representa una aplicación industrial.  
Al clickear, se actualiza todo el dashboard sin recargar la página.

### Configuración por Vista
Cada vista define:
```javascript
{
    name: 'Nombre Aplicación',
    label: '🏭 ETIQUETA',
    accent: '#color',
    accentRGB: 'r, g, b',
    contextIcon: '🏭',
    contextText: '<strong>Título</strong> — Descripción del escenario',
    cards: {
        temp: { label: 'Nombre Sensor', icon: '♨️', unit: '°C' },
        // ... 6 cards
    },
    alerts: {
        tempCrit: 55, tempWarn: 42, tempLow: 5,
        // ... umbrales específicos
    },
    chartTitle1: '📈 Título Gráfico 1',
    chartTitle2: '📈 Título Gráfico 2'
}
```

### Transición entre Vistas
- Fade out (0.2s opacity → 0.3)
- Actualizar labels, iconos, umbrales, contexto
- Fade in (restaurar opacity)
- Cambiar CSS custom properties para acentos dinámicos

---

## Alertas Industriales

### Niveles de Alerta
| Nivel | Icono | Color | Uso |
|-------|-------|-------|-----|
| Crítico | 🔴 | `--accent-red` | Valor fuera de rango peligroso |
| Advertencia | 🟡 | `--accent-amber` | Valor cercano a límite |
| Info | 🔵 / ℹ️ | `--accent-blue` | Información contextual |
| OK | ✅ | `--accent-green` | Todo normal |

### Umbrales por Industria
Los umbrales cambian según la vista activa:
- **Ambiental**: temp >40°C crítico, >35°C advertencia
- **Caldera**: temp >55°C crítico, >42°C advertencia
- **Litio**: temp >50°C crítico, >40°C advertencia

### Alertas Contextuales
Cada vista puede añadir alertas específicas:
- Caldera: "Presión vapor ALTA" cuando presión > 920 hPa
- Litio: "Evaporación óptima" cuando humedad < 30%

---

## Status Badge de Conexión

```css
.status-badge.online {
    background: rgba(16, 185, 129, 0.15);  /* glow verde */
    color: var(--accent-green);
    border: 1px solid rgba(16, 185, 129, 0.3);
}
```
- Punto pulsante con `animation: pulse 2s ease-in-out infinite`
- Texto: "En Vivo" / "Desconectado"
- Controlado por Firebase `.info/connected`

---

## Responsive Breakpoints

| Breakpoint | Cambios |
|------------|---------|
| `>900px` | Layout completo, charts 2 columnas |
| `≤900px` | Charts y bottom en 1 columna |
| `≤700px` | Selector de vistas en 1 columna |
| `≤600px` | Cards de sensores en 2 columnas, fonts reducidos |

---

## Fuentes

- **Inter** (400-800): UI general, labels, descripciones
- **JetBrains Mono** (400-500): valores numéricos, timestamps, datos técnicos
- Cargar desde Google Fonts con `display=swap`

---

## Fuentes de Referencia
- UXPin Dashboard Design Principles
- IoT For All — Ultimate IoT Dashboard
- Order Group — Best Design Practices for Industrial IoT
- Sigma Computing — Best Practices Dashboard Design
- DataCamp — Effective Dashboard Design
