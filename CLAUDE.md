# UCASAL DEMO — Heltec WiFi LoRa 32 V2

## Placa de Desarrollo
- **Board**: Heltec WiFi LoRa 32 **V2**
- **MCU**: ESP32-D0WDQ6 (dual-core, 240 MHz)
- **LoRa**: SX1276 (verificar frecuencia regional antes de transmitir)
- **OLED**: SSD1306 128×64 I2C
- **USB**: CP2102 → **COM3**

## Estructura del Proyecto
Cada demo/test es un proyecto PlatformIO independiente en su propia carpeta numerada:

```
UCASAL_DEMO/
├── 01_blink/           ← Test básico LED onboard (GPIO 25)
├── 02_oled_demo/       ← Demo pantalla OLED SSD1306 (texto, gráficos, barras)
├── 03_logo_oled/       ← Logo IITA en OLED (conversión imagen→bitmap, animación)
├── 04_sensor_bmp180/   ← BMP180 I2C: presión atmosférica, altitud + gráfico OLED
├── 05_sensor_dht22/    ← DHT22 GPIO13: temperatura, humedad + gráfico dual OLED
├── 06_estacion_clima/  ← BMP180 + DHT22 integrados: estación climática completa
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

## Framework
- **PlatformIO** con Arduino framework
- Board ID: `heltec_wifi_lora_32_V2`
- Puerto: COM3

## Skills Disponibles
Las siguientes skills están en `.claude/skills/`:
- `heltec-board` — Pinout completo, GPIOs, limitaciones de hardware
- `heltec-lora` — Comunicación LoRa (RadioLib, LoRaWAN, parámetros)
- `heltec-display` — Pantalla OLED SSD1306 (U8g2, gráficos, UI)
- `heltec-wifi` — WiFi, HTTP, MQTT, WebSocket, BLE, OTA
- `heltec-power` — Deep sleep, batería, optimización de consumo
- `heltec-setup` — PlatformIO config, compilación, troubleshooting
- `security-repo` — Gestión de secretos, API keys, .gitignore, pre-commit
- `security-iot` — Seguridad IoT: TLS, cifrado LoRa, hardening, Secure Boot

## Reglas Generales
- Código en **C++ / Arduino framework**
- Cada nuevo ejemplo va en su propia carpeta numerada (`03_xxx/`, `04_xxx/`, etc.)
- Siempre usar los `#define` de pines correctos para Heltec V2 (ver skill `heltec-board`)
- Inicializar Vext (GPIO 21 → LOW) antes de usar la OLED
- ADC2 no funciona con WiFi activo — usar solo ADC1
- Frecuencia LoRa: 915 MHz (Argentina)

## Reglas de Seguridad (OBLIGATORIAS)
- **NUNCA** escribir API keys, contraseñas, tokens o credenciales en archivos de código
- Credenciales van en `include/secrets.h` (archivo local, en `.gitignore`)
- Copiar `secrets.h.example` → `include/secrets.h` y llenar valores reales
- Usar `#include "secrets.h"` con defines `SECRET_*` en el código
- Ver skills `security-repo` y `security-iot` para patrones completos
