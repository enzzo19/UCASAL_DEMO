---
name: heltec-lora
description: "Comunicación LoRa con el módulo SX1276 de la Heltec LoRa32 V2: configuración, envío/recepción, LoRaWAN y mejores prácticas."
triggers:
  - "lora"
  - "lorawan"
  - "sx1276"
  - "radio"
  - "transmitir"
  - "recibir"
  - "frecuencia"
  - "spreading factor"
  - "ttn"
---

# LoRa — Módulo SX1276 en Heltec LoRa32 V2

## Conexión Hardware (ya cableado en la placa)

| Señal | GPIO | Notas |
|---|---|---|
| SCK | 5 | SPI Clock |
| MISO | 19 | SPI Data In |
| MOSI | 27 | SPI Data Out |
| NSS (CS) | 18 | Chip Select |
| RST | 14 | Reset del SX1276 |
| DIO0 | 26 | Interrupción principal (TX/RX done) |
| DIO1 | 35 | Timeout / FHSS |
| DIO2 | 34 | FHSS |

## Librería Recomendada: RadioLib

RadioLib es la librería más moderna, versátil y mantenida para el SX1276.

### platformio.ini
```ini
[env:heltec_wifi_lora_32_V2]
platform = espressif32
board = heltec_wifi_lora_32_V2
framework = arduino
monitor_speed = 115200
lib_deps =
    jgromes/RadioLib@^6.6.0
```

### Inicialización
```cpp
#include <RadioLib.h>

// Pines Heltec LoRa32 V2
#define LORA_NSS   18
#define LORA_DIO0  26
#define LORA_RST   14
#define LORA_DIO1  35

SX1276 radio = new Module(LORA_NSS, LORA_DIO0, LORA_RST, LORA_DIO1);

void setup() {
    Serial.begin(115200);

    // Inicializar LoRa a 915 MHz (cambiar según región)
    // Parámetros: freq, BW, SF, CR, syncWord, power, preamble, gain
    int state = radio.begin(
        915.0,    // Frecuencia en MHz (433.0, 868.0, 915.0)
        125.0,    // Bandwidth en kHz (7.8, 10.4, 15.6, 20.8, 31.25, 41.7, 62.5, 125, 250, 500)
        7,        // Spreading Factor (6-12)
        5,        // Coding Rate (5=4/5, 6=4/6, 7=4/7, 8=4/8)
        0x12,     // Sync Word (0x12 = privado, 0x34 = LoRaWAN público)
        17,       // Potencia en dBm (2-20)
        8,        // Longitud de preámbulo
        0         // Ganancia LNA (0=auto AGC)
    );

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("LoRa inicializado correctamente");
    } else {
        Serial.printf("Error LoRa: %d\n", state);
    }
}
```

### Transmitir (bloqueante)
```cpp
void enviarMensaje(const char* mensaje) {
    Serial.printf("Enviando: %s\n", mensaje);
    int state = radio.transmit(mensaje);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.println("Enviado OK");
        Serial.printf("  Datarate: %.2f bps\n", radio.getDataRate());
    } else {
        Serial.printf("Error TX: %d\n", state);
    }
}
```

### Recibir (bloqueante)
```cpp
void recibirMensaje() {
    String msg;
    int state = radio.receive(msg);

    if (state == RADIOLIB_ERR_NONE) {
        Serial.printf("Recibido: %s\n", msg.c_str());
        Serial.printf("  RSSI: %.1f dBm\n", radio.getRSSI());
        Serial.printf("  SNR: %.1f dB\n", radio.getSNR());
        Serial.printf("  Freq error: %.1f Hz\n", radio.getFrequencyError());
    } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
        // Timeout — normal, reintentar
    } else {
        Serial.printf("Error RX: %d\n", state);
    }
}
```

### Transmitir/Recibir con Interrupciones (no bloqueante)
```cpp
volatile bool operacionCompleta = false;

// ISR — debe ser IRAM_ATTR en ESP32
ICACHE_RAM_ATTR void onDio0Rise() {
    operacionCompleta = true;
}

void setup() {
    // ... inicialización radio ...
    radio.setDio0Action(onDio0Rise);
    radio.startReceive();  // Comenzar a escuchar
}

void loop() {
    if (operacionCompleta) {
        operacionCompleta = false;

        String msg;
        int state = radio.readData(msg);
        if (state == RADIOLIB_ERR_NONE) {
            Serial.printf("Recibido: %s (RSSI: %.1f)\n", msg.c_str(), radio.getRSSI());
        }

        radio.startReceive();  // Volver a escuchar
    }

    // Aquí se puede hacer otras cosas mientras espera
}
```

### Enviar Datos Binarios (structs)
```cpp
struct SensorData {
    uint8_t  nodeId;
    float    temperatura;
    float    humedad;
    uint16_t bateria_mV;
} __attribute__((packed));

// Enviar
SensorData datos = {1, 25.3, 60.1, 3750};
radio.transmit((uint8_t*)&datos, sizeof(datos));

// Recibir
SensorData recibido;
int state = radio.receive((uint8_t*)&recibido, sizeof(recibido));
```

## Alternativa: Librería Heltec (LoRa simple)

```cpp
#include <LoRa.h>

#define LORA_BAND 915E6  // 433E6, 868E6, 915E6

void setup() {
    SPI.begin(5, 19, 27, 18);
    LoRa.setPins(18, 14, 26);  // NSS, RST, DIO0

    if (!LoRa.begin(LORA_BAND)) {
        Serial.println("Error iniciando LoRa");
        while(1);
    }

    LoRa.setSpreadingFactor(7);     // 6-12
    LoRa.setSignalBandwidth(125E3); // 7.8E3 a 500E3
    LoRa.setCodingRate4(5);         // 5-8
    LoRa.setTxPower(17);            // 2-20 dBm
    LoRa.setSyncWord(0x12);
}
```

## LoRaWAN con OTAA (The Things Network)

### platformio.ini adicional
```ini
lib_deps =
    mcci-catena/MCCI LoRaWAN LMIC library@^4.1.1
build_flags =
    -D ARDUINO_LMIC_PROJECT_CONFIG_H_SUPPRESS
    -D CFG_us915=1          ; Cambiar según región (CFG_eu868, CFG_au915)
    -D CFG_sx1276_radio=1
    -D LMIC_LORAWAN_SPEC_VERSION=LMIC_LORAWAN_SPEC_VERSION_1_0_3
```

### Pin mapping para LMIC
```cpp
#include <lmic.h>
#include <hal/hal.h>

const lmic_pinmap lmic_pins = {
    .nss  = 18,
    .rxtx = LMIC_UNUSED_PIN,
    .rst  = 14,
    .dio  = {26, 35, 34},
};
```

## Parámetros LoRa y su Efecto

| Parámetro | Rango | ↑ Aumentar = | ↓ Disminuir = |
|---|---|---|---|
| **Spreading Factor** | 6–12 | Más alcance, más lento | Menos alcance, más rápido |
| **Bandwidth** | 7.8–500 kHz | Más rápido, menos sensibilidad | Más sensibilidad, más lento |
| **Coding Rate** | 4/5–4/8 | Más resistente a errores, más lento | Más rápido, menos protección |
| **TX Power** | 2–20 dBm | Más alcance, más consumo | Menos consumo |

### Combinaciones Típicas
| Uso | SF | BW | CR | Alcance aprox. |
|---|---|---|---|---|
| Urbano, datos frecuentes | 7 | 125 kHz | 4/5 | 2-5 km |
| Suburbano | 9 | 125 kHz | 4/5 | 5-10 km |
| Rural, máximo alcance | 12 | 125 kHz | 4/8 | 10-15+ km |
| Alta velocidad, corto alcance | 7 | 250 kHz | 4/5 | 1-3 km |

## Instrucciones para el Asistente

Al generar código LoRa para la Heltec LoRa32 V2:
- Siempre definir los pines correctos (NSS=18, DIO0=26, RST=14, DIO1=35).
- Preguntar la frecuencia regional si no se especifica (915 MHz para América, 868 MHz para Europa, 433 MHz para Asia).
- Preferir RadioLib sobre la librería LoRa clásica para proyectos nuevos.
- Para comunicación punto-a-punto, usar el mismo Sync Word, SF y BW en ambos extremos.
- Recordar que LoRa es half-duplex: no puede transmitir y recibir simultáneamente.
- Advertir sobre el duty cycle: en la banda 868 MHz (EU) hay límite legal de 1% de uso del canal.
- Para paquetes grandes, advertir que el payload máximo LoRa es 255 bytes.
- En LoRaWAN, el payload máximo varía según DR (Data Rate), típicamente 51-222 bytes.
