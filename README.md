# 🏭 Taller IoT Industrial — IITA / UCASAL

> Sistema completo de telemetría IoT: desde sensores en campo hasta dashboard web y app móvil en tiempo real.

<p align="center">
  <img src="https://img.shields.io/badge/ESP32-LoRa_32_V2-blue?style=flat-square" alt="Heltec">
  <img src="https://img.shields.io/badge/LoRa-915_MHz-orange?style=flat-square" alt="LoRa">
  <img src="https://img.shields.io/badge/Firebase-RTDB-yellow?style=flat-square" alt="Firebase">
  <img src="https://img.shields.io/badge/Expo-SDK_54-purple?style=flat-square" alt="Expo">
</p>

---

## 📋 Descripción

Proyecto de taller desarrollado para el **Instituto de Investigaciones en Tecnologías y Aplicaciones (IITA)** de la **Universidad Católica de Salta (UCASAL)**, Argentina.

El sistema recoge datos de sensores ambientales en campo (temperatura, humedad, presión atmosférica, altitud), los transmite por **radio LoRa** a una estación base, que los sube a la nube vía **Firebase**. Los datos se visualizan en tiempo real en un **dashboard web** y una **app móvil**, presentados en 3 contextos industriales diferentes:

| Vista | Contexto | Visualización |
|-------|----------|---------------|
| 🌍 **Ambiental** | Estación meteorológica | Condición climática, confort térmico, punto de rocío |
| 🏭 **Caldera** | Control industrial | Termómetro, gauge de presión, zona operativa |
| ⚗️ **Litio** | Planta de extracción | Pileta de salmuera, tasa de evaporación, concentración Li |

---

## 🏗️ Arquitectura

```
┌──────────────────────┐         LoRa 915 MHz        ┌──────────────────────┐
│   PLACA TX (campo)   │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ► │   PLACA RX (base)    │
│                      │    TelemetriaPacket           │                      │
│  BMP180 → presión    │    27 bytes + anti-replay    │  OLED: dashboard     │
│  BMP180 → altitud    │                              │  Serial: telemetría  │
│  DHT22  → temp       │                              │  WiFi → NTP (hora)   │
│  DHT22  → humedad    │                              │  WiFi → Firebase     │
│  OLED: status TX     │                              │                      │
└──────────────────────┘                              └──────────┬───────────┘
                                                                 │ HTTPS
                                                                 ▼
                                                      ┌──────────────────────┐
                                                      │   Firebase RTDB      │
                                                      │   (Google Cloud)     │
                                                      └──────────┬───────────┘
                                                                 │
                                                      ┌──────────┴───────────┐
                                                      │                      │
                                                 ┌────▼─────┐        ┌──────▼──────┐
                                                 │ Web App  │        │  App Móvil  │
                                                 │ Dashboard│        │ Expo + RN   │
                                                 │ 3 vistas │        │ 3 vistas    │
                                                 └──────────┘        └─────────────┘
```

---

## 🔧 Hardware

| Componente | Especificación |
|-----------|---------------|
| **Placas** | 2× Heltec WiFi LoRa 32 V2 |
| **MCU** | ESP32-D0WDQ6 (dual-core, 240 MHz) |
| **Radio** | SX1276 @ 915 MHz (banda ISM Argentina) |
| **Display** | OLED SSD1306 128×64 I2C |
| **Sensor temp/hum** | DHT22 (GPIO 13) |
| **Sensor presión** | BMP180 (I2C) |
| **USB** | CP2102 |

---

## 📂 Estructura del Proyecto

```
UCASAL_DEMO/
│
├── 📁 Ejemplos progresivos (aprendizaje)
│   ├── 01_blink/              LED onboard (GPIO 25)
│   ├── 02_oled_demo/          Pantalla OLED: texto, gráficos, barras
│   ├── 03_logo_oled/          Logo IITA animado en OLED
│   ├── 04_sensor_bmp180/      Presión atmosférica + altitud + gráfico
│   ├── 05_sensor_dht22/       Temperatura + humedad + gráfico dual
│   └── 06_estacion_clima/     Estación climática completa (BMP180 + DHT22)
│
├── 📡 Fase 1 — Telemetría LoRa
│   ├── 07_lora_tx/            Transmisor: sensores → paquete LoRa
│   └── 08_lora_rx/            Receptor: LoRa → OLED + Serial
│
├── 📶 Fase 2 — WiFi + NTP
│   └── 09_rx_wifi_ntp/        Receptor con timestamp real (NTP)
│
├── ☁️ Fase 3 — Firebase
│   └── 10_firebase_rtdb/      Receptor → Firebase Realtime Database
│
├── 🖥️ Fase 4 — Dashboard Web
│   └── web/                   Dashboard multi-vista industrial
│                              HTML + CSS + JS + Chart.js + Firebase SDK
│
├── 📱 Fase 5 — App Móvil
│   └── app/                   React Native + Expo SDK 54
│                              3 vistas industriales, Firebase real-time
│
├── .claude/skills/            Skills de referencia para Claude Code
├── CLAUDE.md                  Instrucciones del proyecto
└── README.md                  ← Estás acá
```

---

## 🚀 Guía Rápida

### Requisitos previos

- [PlatformIO](https://platformio.org/) (para firmware ESP32)
- [Node.js](https://nodejs.org/) ≥ 18 (para web y app)
- [Expo Go](https://expo.dev/go) en el celular (para la app)
- Cuenta en [Firebase](https://firebase.google.com/) (para la base de datos en la nube)

### 1. Clonar y configurar credenciales

```bash
git clone <url-del-repo>
cd UCASAL_DEMO
```

Copiar el archivo de ejemplo de credenciales y completar con tus datos:

```bash
cp secrets.h.example include/secrets.h
```

Editar `include/secrets.h` con tu WiFi y Firebase:

```cpp
#define SECRET_WIFI_SSID     "tu-red-wifi"
#define SECRET_WIFI_PASS     "tu-password"
#define SECRET_FIREBASE_HOST "tu-proyecto.firebaseio.com"
#define SECRET_FIREBASE_KEY  "tu-api-key"
```

### 2. Subir firmware a las placas

```bash
# Transmisor (placa de campo)
cd 07_lora_tx && pio run --target upload

# Receptor con Firebase (placa base)
cd 10_firebase_rtdb && pio run --target upload
```

### 3. Ver el dashboard web

```bash
python -m http.server 8080 --directory web
```

Abrir `http://localhost:8080` en el navegador.

### 4. Correr la app móvil

```bash
cd app
npm install
npx expo start --clear
```

Escanear el QR con Expo Go (Android) o la cámara (iOS).
El celular y la PC deben estar en la **misma red WiFi**.

### 5. Build APK (opcional)

```bash
cd app
npx eas-cli login
npx eas-cli build --platform android --profile preview
```

---

## 📡 Protocolo de Telemetría

El paquete LoRa es un struct de **27 bytes** compartido entre transmisor y receptor:

```cpp
struct TelemetriaPacket {
    uint8_t  nodeId;        // ID del nodo
    uint32_t secuencia;     // Contador anti-replay
    float    temperatura;   // °C (DHT22)
    float    humedad;       // % (DHT22)
    float    presion;       // hPa (BMP180)
    float    altitud;       // metros (BMP180)
    float    sensTermica;   // °C (calculada)
    uint16_t bateria_mV;    // Voltaje batería
    uint8_t  errores;       // Errores acumulados
} __attribute__((packed));
```

---

## 🏭 Vistas Industriales

Las 3 vistas usan los **mismos datos** de telemetría pero los presentan en contextos industriales diferentes, con cálculos derivados específicos:

### 🌍 Monitoreo Ambiental
- Condición climática (emoji + texto)
- Índice de confort (0-100)
- Punto de rocío (fórmula de Magnus)
- Altitud barométrica

### 🏭 Caldera Industrial
- Termómetro vertical animado
- Gauge de presión con zonas
- Zona operativa: `NORMAL` / `PRECAUCIÓN` / `PELIGRO`
- Eficiencia de vapor simulada

### ⚗️ Planta de Litio
- Pileta de salmuera con olas animadas
- Tasa de evaporación (Penman simplificado)
- Concentración de Li simulada
- Índice solar

---

## 🛡️ Seguridad

- Las credenciales (WiFi, Firebase) van en `include/secrets.h`, **nunca** en el código fuente
- `secrets.h` está en `.gitignore` para evitar commits accidentales
- La API key de Firebase es pública por diseño (Google), las reglas de la RTDB controlan el acceso
- Para producción: usar MQTT con TLS mutual, tokens JWT, y cifrado AES en LoRa

---

## 📊 Stack Tecnológico

| Capa | Tecnología | Versión |
|------|-----------|---------|
| Firmware | PlatformIO + Arduino | ESP32 |
| Radio | LoRa (RadioLib) | SX1276 |
| Display | U8g2 | SSD1306 |
| Sensores | DHT + BMP180 | I2C/GPIO |
| Cloud | Firebase RTDB | Web SDK 10.7 |
| Web | HTML + CSS + Chart.js | 4.4.1 |
| Móvil | React Native + Expo | SDK 54 |
| Build | EAS Build | APK/AAB |

---

## 🗺️ Roadmap

- [x] Fase 1 — Telemetría LoRa TX/RX
- [x] Fase 2 — WiFi + NTP en receptor
- [x] Fase 3 — Firebase Realtime Database
- [x] Fase 4 — Dashboard web multi-vista industrial
- [x] Fase 5 — App móvil React Native + Expo
- [ ] Histórico de datos (time-series)
- [ ] Push notifications (alertas de umbral)
- [ ] Multi-nodo (N transmisores)
- [ ] Migración a MQTT + TimescaleDB (producción)

---

## 👥 Créditos

Desarrollado para el **IITA** — Instituto de Investigaciones en Tecnologías y Aplicaciones  
**UCASAL** — Universidad Católica de Salta, Argentina

---

## 📄 Licencia

Proyecto educativo. Uso libre para fines académicos y de investigación.
