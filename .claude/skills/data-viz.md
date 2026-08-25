# Data Viz — Visualización de Datos en Tiempo Real

Skill de referencia para gráficos, charts y visualización de datos IoT  
en dashboards industriales. Basada en Chart.js 4.x y mejores prácticas.

---

## Principios de Visualización

### Selección de Tipo de Gráfico
| Dato | Tipo | Razón |
|------|------|-------|
| Tendencia temporal | **Line chart** | Mejor para datos continuos en el tiempo |
| Comparación categórica | **Bar chart** | Humanos distinguen bien longitudes |
| Proporción | **Donut** (no pie) | Más limpio, centro para KPI |
| Distribución | **Scatter** | Correlación entre variables |
| Estado instantáneo | **Gauge / Meter** | Rango min-max con valor actual |

**Evitar**: pie charts (difícil comparar ángulos), 3D charts (distorsionan).

### Patrón F de Lectura
```
┌──────────────────────────────┐
│  KPI principal    KPI 2      │  ← Los ojos van aquí primero
│  (grande)         (medio)    │
├──────────────────────────────┤
│  Chart 1 (tendencia)  |  C2 │  ← Segundo nivel de atención
├──────────────────────────────┤
│  Tabla / Detalle             │  ← Último
└──────────────────────────────┘
```

### Regla: Datos > Decoración
- **Ratio dato/tinta alto**: maximizar la información por pixel
- **Sin gridlines pesados**: usar colores muy sutiles (`rgba(30,41,64,0.5)`)
- **Sin bordes de chart**: fondo del contenedor es suficiente
- **Point radius: 0** en líneas continuas — el trazo es la info

---

## Chart.js 4.x — Configuración Base

### Setup Común para Dark Theme
```javascript
const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    animation: { duration: 300 },      // Rápido para real-time
    plugins: {
        legend: {
            labels: {
                color: '#94a3b8',       // text-secondary
                font: { size: 11 }
            }
        }
    },
    scales: {
        x: {
            ticks: {
                color: '#64748b',       // text-muted
                maxTicksLimit: 8,       // No saturar eje X
                font: { size: 10 }
            },
            grid: { color: 'rgba(30,41,64,0.5)' }
        }
    }
};
```

### Line Chart — Temperatura + Humedad (Dual Y-axis)
```javascript
const chartTH = new Chart(ctx, {
    type: 'line',
    data: {
        labels: histLabels,     // timestamps
        datasets: [
            {
                label: 'Temperatura (°C)',
                data: histTemp,
                borderColor: '#f97316',                    // accent-orange
                backgroundColor: 'rgba(249,115,22,0.1)',   // fill suave
                borderWidth: 2,
                pointRadius: 0,         // Sin puntos — línea limpia
                tension: 0.3,           // Curva suave
                fill: true,
                yAxisID: 'y'
            },
            {
                label: 'Humedad (%)',
                data: histHum,
                borderColor: '#3b82f6',
                backgroundColor: 'rgba(59,130,246,0.1)',
                borderWidth: 2,
                pointRadius: 0,
                tension: 0.3,
                fill: true,
                yAxisID: 'y1'
            }
        ]
    },
    options: {
        ...chartOptions,
        scales: {
            ...chartOptions.scales,
            y: {
                position: 'left',
                ticks: { color: '#f97316' },
                grid: { color: 'rgba(30,41,64,0.5)' },
                title: { display: true, text: '°C', color: '#f97316' }
            },
            y1: {
                position: 'right',
                ticks: { color: '#3b82f6' },
                grid: { drawOnChartArea: false },  // Sin grid en eje secundario
                title: { display: true, text: '%', color: '#3b82f6' }
            }
        }
    }
});
```

### Colores de Dataset por Sensor
| Sensor | Line Color | Fill Color | Eje |
|--------|-----------|------------|-----|
| Temperatura | `#f97316` | `rgba(249,115,22,0.1)` | Y izquierdo |
| Humedad | `#3b82f6` | `rgba(59,130,246,0.1)` | Y derecho |
| Presión | `#8b5cf6` | `rgba(139,92,246,0.1)` | Y único |
| Altitud | `#10b981` | `rgba(16,185,129,0.1)` | Y único |

---

## Real-Time — Actualización en Vivo

### Patrón de Acumulación
```javascript
const MAX_POINTS = 100;    // Ventana de datos visible

function addDataPoint(label, temp, hum, pres) {
    histLabels.push(label);
    histTemp.push(temp);
    histHum.push(hum);
    histPres.push(pres);

    // Ventana deslizante — eliminar datos viejos
    if (histLabels.length > MAX_POINTS) {
        histLabels.shift();
        histTemp.shift();
        histHum.shift();
        histPres.shift();
    }

    // Actualizar sin animación para velocidad
    chartTH.update('none');
    chartP.update('none');
}
```

### Deduplicación por Timestamp
```javascript
let lastTimestamp = null;

function processData(data) {
    if (!data) return;
    if (data.timestamp === lastTimestamp) return;  // Sin cambio
    lastTimestamp = data.timestamp;
    // ... procesar
}
```

### Doble Fuente: Listener + Polling
```javascript
// 1. Firebase real-time listener (reacciona a cambios)
ref.on('value', (snapshot) => processData(snapshot.val()));

// 2. Polling fallback cada 3s (por si el listener falla)
setInterval(() => {
    ref.get().then((snapshot) => processData(snapshot.val()));
}, 3000);
```

**¿Por qué ambos?**  
- `on('value')` solo dispara cuando el dato **cambia** en Firebase
- Si el ESP32 envía el mismo valor 2 veces, Firebase no notifica
- El polling `get()` asegura que la UI siempre refleja el último dato

---

## Contenedor de Gráficos

### HTML
```html
<div class="chart-container">
    <div class="chart-title">📈 Título del Gráfico</div>
    <div class="chart-wrapper">
        <canvas id="chartId"></canvas>
    </div>
</div>
```

### CSS
```css
.chart-container {
    background: var(--bg-card);
    border: 1px solid var(--border);
    border-radius: 12px;
    padding: 1.25rem;
}
.chart-title {
    font-size: 0.85rem;
    font-weight: 600;
    color: var(--text-secondary);
    text-transform: uppercase;
    letter-spacing: 1px;
    margin-bottom: 1rem;
}
.chart-wrapper {
    position: relative;
    height: 220px;          /* Altura fija para consistencia */
}
```

### Grid de Charts
```css
.charts-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;   /* 2 columnas */
    gap: 1rem;
}
@media (max-width: 900px) {
    grid-template-columns: 1fr;        /* 1 columna en móvil */
}
```

---

## Labels Temporales

### Formato de Timestamp para Eje X
```javascript
const label = new Date().toLocaleTimeString('es-AR', {
    hour: '2-digit',
    minute: '2-digit',
    second: '2-digit'
});
// → "14:23:45"
```

### MaxTicksLimit
- Pantalla grande: `maxTicksLimit: 8`
- Pantalla chica: `maxTicksLimit: 5`
- Evitar labels superpuestos — Chart.js auto-omite con este límite

---

## Gauge / Medidor (para futuro)

### Implementación con SVG + CSS
```html
<svg viewBox="0 0 120 120" class="gauge">
    <circle cx="60" cy="60" r="50"
            fill="none" stroke="var(--border)" stroke-width="10"
            stroke-dasharray="235.6" stroke-dashoffset="78.5"
            transform="rotate(150 60 60)" />
    <circle cx="60" cy="60" r="50"
            fill="none" stroke="var(--accent-orange)" stroke-width="10"
            stroke-dasharray="235.6"
            stroke-dashoffset="calc(235.6 - (235.6 * var(--value) / 100))"
            transform="rotate(150 60 60)"
            stroke-linecap="round" />
    <text x="60" y="65" text-anchor="middle"
          fill="var(--text-primary)" font-size="20"
          font-family="JetBrains Mono">
        75°C
    </text>
</svg>
```

### Rangos de Color para Gauge
```css
/* 0-33%: verde, 33-66%: amber, 66-100%: rojo */
--gauge-color: var(--accent-green);
/* Calcular con JS según valor/rango */
```

---

## Sparklines Inline (para cards de sensores)

### Mini gráfico dentro de la card
```javascript
// Sparkline: últimos 20 valores en un mini canvas
const spark = new Chart(miniCtx, {
    type: 'line',
    data: {
        labels: last20Labels,
        datasets: [{
            data: last20Values,
            borderColor: '#f97316',
            borderWidth: 1.5,
            pointRadius: 0,
            tension: 0.4,
            fill: false
        }]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: { legend: { display: false } },
        scales: {
            x: { display: false },
            y: { display: false }
        }
    }
});
```

---

## Tooltips Personalizados

### Chart.js Tooltip para Dark Theme
```javascript
plugins: {
    tooltip: {
        backgroundColor: 'rgba(20, 27, 45, 0.95)',
        titleColor: '#e2e8f0',
        bodyColor: '#94a3b8',
        borderColor: 'rgba(30, 41, 64, 0.8)',
        borderWidth: 1,
        cornerRadius: 8,
        padding: 12,
        titleFont: { family: 'Inter', weight: 600 },
        bodyFont: { family: 'JetBrains Mono', size: 12 },
        displayColors: true,
        boxWidth: 8,
        boxHeight: 8,
        boxPadding: 4
    }
}
```

---

## Progressive Disclosure

### Hover para Detalle
- Cards muestran valor principal
- Hover revela mini-tendencia o rango min/max
- Click abre vista expandida del sensor

### Niveles de Información
1. **Glance** (KPI cards): valor actual, unidad, icono
2. **Scan** (charts): tendencia últimos N minutos
3. **Deep** (expandido): historial completo, estadísticas, exportar

---

## Fuentes de Referencia
- Chart.js 4.x Documentation
- DataCamp — Effective Dashboard Design
- Pencil & Paper — Dashboard UX Patterns
- 5of10 — Dashboard Design Best Practices 2025
- Sigma Computing — 8 Best Practices for Dashboard Design
