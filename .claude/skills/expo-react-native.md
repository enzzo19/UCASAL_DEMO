# Expo + React Native — App Móvil IITA IoT

## Stack
- **Framework**: React Native 0.81.5 + Expo SDK 54
- **Lenguaje**: TypeScript ~5.9
- **Firebase**: JS SDK v10 (modular)
- **Target**: Android (Expo Go v54 / APK standalone)

## Estructura de la App

```
app/
├── App.tsx                    ← Entry point, Firebase listener, navegación de vistas
├── index.ts                   ← Registro Expo
├── app.json                   ← Config Expo (nombre, slug, icons, package)
├── eas.json                   ← Config EAS Build (preview = APK, production = AAB)
├── package.json               ← Dependencias (expo ~54.0.0)
├── src/
│   ├── config/firebase.ts     ← Init Firebase, exports: database, ref, onValue, get
│   ├── types.ts               ← SensorData, ViewType, VIEW_CONFIGS
│   ├── theme.ts               ← Paleta de colores (dark industrial)
│   ├── utils/calculations.ts  ← Cálculos derivados industriales
│   ├── components/
│   │   ├── SensorCard.tsx     ← Card reutilizable con barra de progreso
│   │   └── DerivedMetric.tsx  ← Métrica derivada compacta
│   └── views/
│       ├── GeneralView.tsx    ← 🌍 Monitoreo Ambiental
│       ├── CalderaView.tsx    ← 🏭 Caldera Industrial
│       └── LitioView.tsx     ← ⚗️ Planta de Litio
└── assets/                    ← Icons, splash
```

## Cómo correr

```bash
# Desarrollo con Expo Go
cd app && npx expo start --clear

# Build APK (requiere cuenta Expo)
cd app && npx eas-cli build --platform android --profile preview

# Login Expo (si no está logueado)
npx eas-cli login
```

## Compatibilidad de SDK

> **IMPORTANTE**: El usuario tiene Expo Go v54 (SDK 54) en su celular.
> No usar SDK > 54 sin verificar que actualizó Expo Go.

### Versiones correctas para SDK 54
```json
{
  "expo": "~54.0.0",
  "expo-status-bar": "~3.0.9",
  "react": "19.1.0",
  "react-native": "0.81.5",
  "@types/react": "~19.1.10",
  "typescript": "~5.9.2"
}
```

## Firebase en la App

La app usa Firebase JS SDK v10 **modular** (no compat):

```typescript
import { initializeApp } from 'firebase/app';
import { getDatabase, ref, onValue, get } from 'firebase/database';
```

- Listener real-time: `onValue(ref, callback)` — se crea UNA vez en `useEffect([], [])`
- Polling fallback: `get(ref)` cada 5s con deduplicación via `useRef`
- Conexión: `onValue(ref(db, '.info/connected'))` para estado online/offline
- Path de datos: `/actual`

## Patrón de datos real-time (correcto)

```typescript
// ✅ CORRECTO — useRef para dedup, useEffect con [] vacío
const lastTimestampRef = useRef<number | null>(null);
useEffect(() => {
  const unsubscribe = onValue(dataRef, (snapshot) => {
    const val = snapshot.val();
    if (val) {
      lastTimestampRef.current = val.timestamp;
      setData(val);
    }
  });
  return () => unsubscribe();
}, []);  // ← SIN dependencias, listener estable

// ❌ INCORRECTO — useState como dep destruye el listener
// useEffect(() => { ... }, [lastTimestamp]);
```

## 3 Vistas Industriales

Misma lógica que el dashboard web. Mismos datos, diferente contexto visual:

| Vista | Accent | Visualización principal |
|-------|--------|------------------------|
| General 🌍 | Amber | Weather box + comfort index |
| Caldera 🏭 | Red | Termómetro + gauge presión + zona operativa |
| Litio ⚗️ | Cyan | Pileta salmuera + evaporación + concentración |

## Cálculos Derivados

- `dewPoint(t, rh)` — Punto de rocío (Magnus)
- `comfortIndex(t, rh)` — Índice de confort 0-100
- `weatherCondition(t, rh)` — Emoji + texto
- `evapRate(t, rh, p, alt)` — Tasa evaporación (Penman simplificado)
- `brineConcentration(t, rh)` — Concentración Li simulada
- `boilerZone(t, p)` — NORMAL/PRECAUCIÓN/PELIGRO
- `steamEfficiency(t, p, rh)` — Eficiencia vapor %

## Troubleshooting

| Problema | Solución |
|----------|----------|
| "Project incompatible with Expo Go" | Actualizar Expo Go o bajar SDK a ~54.0.0 |
| QR no funciona al reiniciar | `npx expo start --clear` |
| Datos no se actualizan | Verificar que useEffect tenga `[]` como deps |
| `eas` no se reconoce | Usar `npx eas-cli` en vez de `eas` |
| Build APK falla | Verificar versiones con `npx expo install --fix` |
