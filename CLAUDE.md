# UCASAL DEMO — Heltec WiFi LoRa 32 V2

## Placa de Desarrollo
- **Board**: Heltec WiFi LoRa 32 **V2** (×2 — una TX, una RX)
- **MCU**: ESP32-D0WDQ6 (dual-core, 240 MHz)
- **LoRa**: SX1276 @ 915 MHz (Argentina)
- **OLED**: SSD1306 128×64 I2C
- **USB**: CP2102 → **COM3** (placa actual)

## Roadmap del Proyecto

```
FASE 1 — Telemetría LoRa               ◄── PRÓXIMO
├── 07_lora_tx/    Transmisor: sensores → paquete LoRa
└── 08_lora_rx/    Receptor: recibe paquete → muestra en OLED + Serial

FASE 2 — WiFi + NTP en Receptor
└── 09_rx_wifi_ntp/   Receptor con WiFi, timestamp NTP, telemetría mejorada

FASE 3 — Firebase RTDB
└── 10_firebase_rtdb/  Receptor sube datos en tiempo real a Firebase

FASE 4 — Dashboard Web
└── web/               App web interactiva (sector minero/industrial)
                       Lee Firebase RTDB, gráficos, alertas, mapas

FASE 5 — App Móvil
└── app/               App móvil (React Native o Flutter)
                       Datos en tiempo real desde Firebase
```

## Estructura del Proyecto

```
UCASAL_DEMO/
├── 01_blink/           ← Test básico LED onboard (GPIO 25)
├── 02_oled_demo/       ← Demo pantalla OLED SSD1306 (texto, gráficos, barras)
├── 03_logo_oled/       ← Logo IITA en OLED (conversión imagen→bitmap, animación)
├── 04_sensor_bmp180/   ← BMP180 I2C: presión atmosférica, altitud + gráfico OLED
├── 05_sensor_dht22/    ← DHT22 GPIO13: temperatura, humedad + gráfico dual OLED
├── 06_estacion_clima/  ← BMP180 + DHT22 integrados: estación climática completa
├── 07_lora_tx/         ← [FASE 1] Transmisor LoRa (sensores → radio)
├── 08_lora_rx/         ← [FASE 1] Receptor LoRa (radio → OLED + serial)
├── .claude/skills/     ← Skills de referencia para la placa
├── .gitignore          ← Protección de secretos y archivos temporales
├── secrets.h.example   ← Plantilla para credenciales (copiar a include/secrets.h)
└── CLAUDE.md
```

Para compilar y subir cualquier ejemplo:
```bash
cd <carpeta_del_ejemplo>
pio run --target upload
```

## Arquitectura del Sistema

```
┌──────────────────────┐         LoRa 915 MHz        ┌──────────────────────┐
│   PLACA TX (campo)   │ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ─ ► │   PLACA RX (base)    │
│                      │    struct SensorData          │                      │
│  BMP180 → presión    │    + secuencia anti-replay   │  OLED: dashboard     │
│  BMP180 → altitud    │                              │  Serial: telemetría  │
│  DHT22  → temp       │                              │  WiFi → NTP (hora)   │
│  DHT22  → humedad    │                              │  WiFi → Firebase     │
│  OLED: status TX     │                              │                      │
└──────────────────────┘                              └──────────┬───────────┘
                                                                 │ HTTPS
                                                                 ▼
                                                      ┌──────────────────────┐
                                                      │  Firebase RTDB       │
                                                      │  (Google Cloud)      │
                                                      └──────────┬───────────┘
                                                                 │
                                                      ┌──────────┴───────────┐
                                                      │                      │
                                                 ┌────▼─────┐        ┌──────▼──────┐
                                                 │ Web App  │        │  App Móvil  │
                                                 │ Dashboard│        │  Real-time  │
                                                 │ Minero/  │        │             │
                                                 │ Industrial│       │             │
                                                 └──────────┘        └─────────────┘
```

## Framework
- **PlatformIO** con Arduino framework
- Board ID: `heltec_wifi_lora_32_V2`
- Puerto: COM3

## Protocolo de Telemetría LoRa

Estructura del paquete (ambas placas deben compartir):
```cpp
struct TelemetriaPacket {
    uint8_t  nodeId;        // ID del nodo transmisor
    uint32_t secuencia;     // Contador anti-replay
    float    temperatura;   // °C (DHT22)
    float    humedad;       // % (DHT22)
    float    presion;       // hPa (BMP180)
    float    altitud;       // metros (BMP180)
    float    sensTermica;   // °C (calculada)
    uint16_t bateria_mV;    // Voltaje batería
    uint8_t  errores;       // Contador de errores
} __attribute__((packed));  // 27 bytes
```

## Skills Disponibles
Las siguientes skills están en `.claude/skills/`:
- `heltec-board` — Pinout completo, GPIOs, limitaciones de hardware
- `heltec-lora` — Comunicación LoRa (RadioLib, telemetría, parámetros)
- `heltec-display` — Pantalla OLED SSD1306 (U8g2, gráficos, UI)
- `heltec-wifi` — WiFi, HTTP, MQTT, WebSocket, BLE, OTA
- `heltec-power` — Deep sleep, batería, optimización de consumo
- `heltec-setup` — PlatformIO config, compilación, troubleshooting
- `security-repo` — Gestión de secretos, API keys, .gitignore, pre-commit
- `security-iot` — Seguridad IoT: TLS, cifrado LoRa, hardening, Secure Boot
- `firebase-rtdb` — Firebase Realtime Database desde ESP32 + estructura de datos

## Reglas Generales
- Código en **C++ / Arduino framework**
- Cada nuevo ejemplo va en su propia carpeta numerada (`03_xxx/`, `04_xxx/`, etc.)
- Siempre usar los `#define` de pines correctos para Heltec V2 (ver skill `heltec-board`)
- Inicializar Vext (GPIO 21 → LOW) antes de usar la OLED
- ADC2 no funciona con WiFi activo — usar solo ADC1
- Frecuencia LoRa: 915 MHz (Argentina)
- **Dos placas**: TX (campo, con sensores) y RX (base, con WiFi/Firebase)
- Paquete de telemetría compartido en `telemetria_packet.h`

## Reglas de Seguridad (OBLIGATORIAS)
- **NUNCA** escribir API keys, contraseñas, tokens o credenciales en archivos de código
- Credenciales van en `include/secrets.h` (archivo local, en `.gitignore`)
- Copiar `secrets.h.example` → `include/secrets.h` y llenar valores reales
- Usar `#include "secrets.h"` con defines `SECRET_*` en el código
- Firebase API key y URL → `secrets.h`
- WiFi SSID/password → `secrets.h`
- Ver skills `security-repo` y `security-iot` para patrones completos
