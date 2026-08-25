---
name: heltec-setup
description: "Configuración del entorno de desarrollo para Heltec LoRa32 V2: PlatformIO, Arduino IDE, compilación, flash y troubleshooting."
triggers:
  - "platformio"
  - "arduino"
  - "compilar"
  - "flashear"
  - "upload"
  - "proyecto"
  - "setup"
  - "configurar"
  - "instalar"
  - "driver"
  - "monitor serial"
  - "particiones"
---

# Setup del Entorno de Desarrollo — Heltec LoRa32 V2

## PlatformIO (Recomendado)

### Crear Proyecto Nuevo
```bash
# Desde la terminal / CLI de PlatformIO
pio init --board heltec_wifi_lora_32_V2 --project-dir mi_proyecto
```

### platformio.ini — Configuración Base
```ini
[env:heltec_wifi_lora_32_V2]
platform = espressif32
board = heltec_wifi_lora_32_V2
framework = arduino
monitor_speed = 115200
upload_speed = 921600

; Puerto serial (detectar con: pio device list)
; upload_port = COM3
; monitor_port = COM3

; Librerías comunes para Heltec
lib_deps =
    olikraus/U8g2@^2.35.0         ; OLED display
    jgromes/RadioLib@^6.6.0        ; LoRa SX1276

; Build flags útiles
build_flags =
    -D ARDUINO_HELTEC_WIFI_LORA_32_V2
    -D CORE_DEBUG_LEVEL=1          ; 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose
```

### platformio.ini — Con WiFi y MQTT
```ini
[env:heltec_wifi_lora_32_V2]
platform = espressif32
board = heltec_wifi_lora_32_V2
framework = arduino
monitor_speed = 115200

lib_deps =
    olikraus/U8g2@^2.35.0
    jgromes/RadioLib@^6.6.0
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^7.0.0

build_flags =
    -D CORE_DEBUG_LEVEL=1
```

### platformio.ini — Con LoRaWAN (LMIC)
```ini
[env:heltec_lorawan]
platform = espressif32
board = heltec_wifi_lora_32_V2
framework = arduino
monitor_speed = 115200

lib_deps =
    mcci-catena/MCCI LoRaWAN LMIC library@^4.1.1

build_flags =
    -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS
    -D CFG_us915=1               ; Cambiar: CFG_eu868, CFG_au915, CFG_as923
    -D CFG_sx1276_radio=1
    -D LMIC_LORAWAN_SPEC_VERSION=LMIC_LORAWAN_SPEC_VERSION_1_0_3
    -D hal_init=LMICHAL_init     ; Evitar conflicto con ESP32

; Partición con más espacio para LMIC
board_build.partitions = huge_app.csv
```

### Estructura de Proyecto PlatformIO
```
mi_proyecto/
├── platformio.ini
├── src/
│   └── main.cpp          ← Código principal
├── include/
│   └── config.h           ← Configuración (WiFi, MQTT, etc.)
├── lib/
│   └── README             ← Librerías locales
├── data/
│   └── index.html         ← Archivos SPIFFS/LittleFS
└── test/
    └── README
```

### Comandos PlatformIO Esenciales
```bash
# Compilar
pio run

# Compilar y subir
pio run --target upload

# Monitor serial
pio device monitor

# Compilar, subir y abrir monitor
pio run --target upload && pio device monitor

# Listar dispositivos conectados
pio device list

# Limpiar build
pio run --target clean

# Subir filesystem (SPIFFS/LittleFS)
pio run --target uploadfs

# Actualizar librerías
pio pkg update
```

## Arduino IDE (Alternativa)

### 1. Instalar soporte ESP32
- Ir a **Archivo → Preferencias → URLs Adicionales de Gestor de Tarjetas**
- Agregar: `https://resource.heltec.cn/download/package_heltec_esp32_index.json`
- Ir a **Herramientas → Gestor de Tarjetas → Buscar "Heltec"**
- Instalar **"Heltec ESP32 Series Dev-boards"**

### 2. Seleccionar Placa
- **Herramientas → Placa**: "WiFi LoRa 32(V2)"
- **Upload Speed**: 921600
- **CPU Frequency**: 240MHz
- **Flash Frequency**: 80MHz
- **Flash Size**: 8MB
- **Partition Scheme**: Default 8MB (o ajustar según necesidad)
- **PSRAM**: Disabled
- **Port**: COM correspondiente

### 3. Instalar Librerías
- **Herramientas → Gestor de Librerías**:
  - U8g2
  - RadioLib
  - PubSubClient
  - ArduinoJson
  - ESPAsyncWebServer (instalar manualmente desde GitHub)

## Driver USB CP2102

La Heltec usa el chip CP2102 para USB-Serial. Si no se detecta:

- **Windows**: Descargar driver de [Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- **macOS**: Normalmente auto-detectado en macOS 11+
- **Linux**: Incluido en el kernel (`cp210x`), verificar con `dmesg | grep cp210x`

### Verificar conexión
```bash
# Windows (PowerShell)
Get-WMIObject Win32_SerialPort | Select-Object DeviceID, Description

# PlatformIO
pio device list
```

## Particiones de Flash (8MB)

### Esquemas Comunes
| Esquema | App | SPIFFS/LittleFS | OTA | Uso |
|---|---|---|---|---|
| default_8MB | 1.3MB | 1.5MB | 1.3MB | General con OTA |
| huge_app | 3MB | 1MB | — | Apps grandes sin OTA |
| min_spiffs | 1.9MB | 128KB | 1.9MB | OTA con app grande |

### Partición Personalizada (`partitions.csv`)
```csv
# Name,   Type, SubType, Offset,   Size
nvs,      data, nvs,     0x9000,   0x5000
otadata,  data, ota,     0xe000,   0x2000
app0,     app,  ota_0,   0x10000,  0x200000
app1,     app,  ota_1,   0x210000, 0x200000
spiffs,   data, spiffs,  0x410000, 0x3F0000
```

En platformio.ini:
```ini
board_build.partitions = partitions.csv
```

## Template main.cpp Completo (Punto de Partida)

```cpp
// main.cpp — Template Heltec WiFi LoRa 32 V2
#include <Arduino.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <Wire.h>

// ========== CONFIGURACIÓN ==========
#define LORA_FREQ     915.0    // MHz: 433.0, 868.0, 915.0
#define LORA_BW       125.0    // kHz
#define LORA_SF       7        // Spreading Factor 6-12
#define LORA_CR       5        // Coding Rate 5-8
#define LORA_POWER    17       // dBm 2-20

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25
#define PIN_LORA_NSS  18
#define PIN_LORA_DIO0 26
#define PIN_LORA_RST  14
#define PIN_LORA_DIO1 35
#define PIN_BATTERY   37

// ========== OBJETOS GLOBALES ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

SX1276 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);

// ========== FUNCIONES ==========
void initDisplay() {
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);  // Encender OLED
    display.begin();
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 12, "Iniciando...");
    display.sendBuffer();
}

bool initLoRa() {
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, 0x12, LORA_POWER, 8, 0);
    return (state == RADIOLIB_ERR_NONE);
}

float readBattery() {
    analogSetAttenuation(ADC_11db);
    uint32_t sum = 0;
    for (int i = 0; i < 16; i++) sum += analogRead(PIN_BATTERY);
    return (sum / 16.0 / 4095.0) * 3.3 * 2.0;
}

void updateDisplay(const char* line1, const char* line2, const char* line3) {
    display.clearBuffer();
    display.setFont(u8g2_font_6x10_tr);
    if (line1) display.drawStr(0, 12, line1);
    if (line2) display.drawStr(0, 28, line2);
    if (line3) display.drawStr(0, 44, line3);

    // Batería en esquina
    char batBuf[16];
    snprintf(batBuf, sizeof(batBuf), "%.1fV", readBattery());
    display.drawStr(92, 62, batBuf);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== Heltec WiFi LoRa 32 V2 ===");

    pinMode(PIN_LED, OUTPUT);

    initDisplay();

    if (initLoRa()) {
        Serial.println("LoRa: OK");
        updateDisplay("LoRa: OK", "Listo!", "");
    } else {
        Serial.println("LoRa: ERROR");
        updateDisplay("LoRa: ERROR", "Verificar modulo", "");
    }
}

// ========== LOOP ==========
void loop() {
    // Tu código aquí
    delay(1000);
}
```

## Troubleshooting

| Problema | Solución |
|---|---|
| No detecta puerto COM | Instalar driver CP2102. Probar otro cable USB (debe ser de datos, no solo carga) |
| Error "Failed to connect" al flashear | Mantener presionado el botón **PRG** mientras se inicia el upload. Soltar al ver "Connecting..." |
| Pantalla OLED no enciende | Verificar `digitalWrite(21, LOW)` y reset del pin 16 |
| LoRa no inicializa | Verificar que los pines SPI no estén ocupados por otro periférico |
| Brownout reset loop | Batería baja o fuente USB débil. Desactivar brownout detector: `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0)` |
| WiFi desconecta random | Asegurar antena no obstruida. Usar `WiFi.setAutoReconnect(true)` |
| Crash / guru meditation | Habilitar debug: `CORE_DEBUG_LEVEL=4`. Decodificar backtrace con `pio run -t monitor` |
| Flash lleno | Cambiar esquema de particiones a `huge_app` o reducir librerías |

## Instrucciones para el Asistente

Al crear o configurar proyectos:
- Siempre generar el `platformio.ini` completo con las librerías necesarias.
- Usar `board = heltec_wifi_lora_32_V2` (con V2 en mayúsculas, guiones bajos).
- Incluir el template con los #define de pines para evitar números mágicos.
- Preguntar la región LoRa si no se especificó (afecta frecuencia y configuración LMIC).
- Para proyectos nuevos, sugerir la estructura PlatformIO sobre Arduino IDE.
- Si el usuario reporta problemas de conexión, verificar driver CP2102 primero.
- Siempre incluir `monitor_speed = 115200` en platformio.ini.
