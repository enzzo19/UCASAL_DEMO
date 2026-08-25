---
name: heltec-power
description: "Gestión de energía de la Heltec LoRa32 V2: deep sleep, light sleep, lectura de batería, Vext, y optimización de consumo."
triggers:
  - "bateria"
  - "battery"
  - "sleep"
  - "deep sleep"
  - "consumo"
  - "energia"
  - "power"
  - "despertar"
  - "wake"
  - "vext"
  - "bajo consumo"
---

# Gestión de Energía — Heltec LoRa32 V2

## Modos de Energía del ESP32

| Modo | Consumo aprox. | CPU | WiFi/BT | RAM | RTC |
|---|---|---|---|---|---|
| Activo (WiFi TX) | ~160-260 mA | ✅ | ✅ | ✅ | ✅ |
| Activo (sin radio) | ~30-70 mA | ✅ | ❌ | ✅ | ✅ |
| Light Sleep | ~0.8 mA | ⏸ | ❌ | ✅ | ✅ |
| Deep Sleep | ~10-20 µA | ❌ | ❌ | ❌ | ✅ |
| Hibernation | ~5 µA | ❌ | ❌ | ❌ | ⏸ |

> **Nota**: La Heltec en Deep Sleep consume ~800 µA debido al regulador y CP2102 USB. 
> Para lograr ~20 µA, desconectar USB y apagar Vext.

## Lectura de Voltaje de Batería

```cpp
#define BATTERY_PIN 37
#define VBAT_FACTOR 2.0  // Divisor resistivo interno

float leerBateria() {
    // Configurar ADC
    analogSetAttenuation(ADC_11db);  // Rango 0-3.3V

    // Promediar lecturas para estabilidad
    uint32_t suma = 0;
    for (int i = 0; i < 16; i++) {
        suma += analogRead(BATTERY_PIN);
    }
    float lectura = suma / 16.0;

    // Convertir a voltaje real
    float voltaje = (lectura / 4095.0) * 3.3 * VBAT_FACTOR;

    return voltaje;  // Típico: 3.0V (vacía) a 4.2V (llena)
}

int porcentajeBateria(float voltaje) {
    // Aproximación lineal para LiPo
    if (voltaje >= 4.2) return 100;
    if (voltaje <= 3.0) return 0;
    return (int)((voltaje - 3.0) / (4.2 - 3.0) * 100);
}
```

## Control de Vext (Alimentación Externa)

```cpp
// Vext controla la alimentación de la OLED y periféricos externos
// GPIO 21: LOW = encendido, HIGH = apagado (lógica invertida!)

void vextON() {
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);   // Encender
}

void vextOFF() {
    pinMode(21, OUTPUT);
    digitalWrite(21, HIGH);  // Apagar — ahorra ~10mA
}
```

## Deep Sleep

### Despertar por Timer
```cpp
#define uS_TO_S_FACTOR 1000000ULL  // Microsegundos a segundos

// En memoria RTC (sobrevive al Deep Sleep)
RTC_DATA_ATTR int contadorBoot = 0;

void setup() {
    contadorBoot++;
    Serial.begin(115200);
    Serial.printf("Boot #%d\n", contadorBoot);

    // Hacer trabajo: leer sensores, enviar datos...
    // ...

    // Configurar despertar en 60 segundos
    esp_sleep_enable_timer_wakeup(60 * uS_TO_S_FACTOR);

    // Apagar todo antes de dormir
    vextOFF();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    // btStop();  // Si se usó Bluetooth

    Serial.println("Entrando en Deep Sleep...");
    Serial.flush();
    esp_deep_sleep_start();
    // El código después de aquí NO se ejecuta
}

void loop() {
    // Nunca llega aquí en modo Deep Sleep
}
```

### Despertar por Pin Externo (botón, sensor)
```cpp
// Solo GPIOs RTC pueden despertar del Deep Sleep:
// 0, 2, 4, 12, 13, 14, 15, 25, 26, 27, 32, 33, 34, 35, 36, 39

void configurarDespertarPorPin() {
    // Despertar cuando GPIO 0 (botón PRG) se ponga en LOW
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_0, 0);  // 0=LOW, 1=HIGH

    // O despertar por múltiples pines (bitmask)
    // Despertar si GPIO 32 O GPIO 33 se ponen en HIGH
    uint64_t bitmask = (1ULL << 32) | (1ULL << 33);
    esp_sleep_enable_ext1_wakeup(bitmask, ESP_EXT1_WAKEUP_ANY_HIGH);
}

// Verificar causa de despertar
void imprimirCausaDespertar() {
    esp_sleep_wakeup_cause_t causa = esp_sleep_get_wakeup_cause();
    switch (causa) {
        case ESP_SLEEP_WAKEUP_TIMER:    Serial.println("Despertar: Timer"); break;
        case ESP_SLEEP_WAKEUP_EXT0:     Serial.println("Despertar: Pin EXT0"); break;
        case ESP_SLEEP_WAKEUP_EXT1:     Serial.println("Despertar: Pin EXT1"); break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD: Serial.println("Despertar: Touch"); break;
        default:                         Serial.println("Despertar: Reset/PowerOn"); break;
    }
}
```

### Despertar por Touch
```cpp
void configurarDespertarPorTouch() {
    // Touch threshold — ajustar según la sensibilidad deseada
    touchAttachInterrupt(T4, [](){}, 40);  // GPIO 13 = T4
    esp_sleep_enable_touchpad_wakeup();
}
```

## Light Sleep (mantiene RAM, despertar más rápido)

```cpp
void lightSleep(int segundos) {
    esp_sleep_enable_timer_wakeup(segundos * uS_TO_S_FACTOR);

    // WiFi se puede mantener con wake-on-LAN
    // esp_wifi_set_ps(WIFI_PS_MAX_MODEM);

    Serial.println("Entrando en Light Sleep...");
    Serial.flush();

    esp_light_sleep_start();  // Bloquea aquí

    // Continúa desde aquí al despertar
    Serial.println("Despertó de Light Sleep!");
}
```

## Patrón Completo: Nodo Sensor de Bajo Consumo

```cpp
#include <WiFi.h>
#include <RadioLib.h>

#define uS_TO_S_FACTOR 1000000ULL
#define SLEEP_SECONDS 300  // 5 minutos

RTC_DATA_ATTR int bootCount = 0;
RTC_DATA_ATTR int txCount = 0;

SX1276 radio = new Module(18, 26, 14, 35);

void setup() {
    bootCount++;
    Serial.begin(115200);

    imprimirCausaDespertar();

    // 1. Leer batería ANTES de encender periféricos
    float bateria = leerBateria();
    Serial.printf("Batería: %.2fV (%d%%)\n", bateria, porcentajeBateria(bateria));

    // 2. Protección de batería baja
    if (bateria < 3.2) {
        Serial.println("Batería muy baja! Sleep largo...");
        esp_sleep_enable_timer_wakeup(3600 * uS_TO_S_FACTOR);  // 1 hora
        esp_deep_sleep_start();
    }

    // 3. Leer sensores
    float temp = leerTemperatura();  // Tu función de sensor

    // 4. Enviar por LoRa
    int state = radio.begin(915.0, 125.0, 7, 5, 0x12, 17, 8, 0);
    if (state == RADIOLIB_ERR_NONE) {
        struct {
            uint16_t id;
            float temp;
            uint16_t bat_mV;
            uint16_t txNum;
        } __attribute__((packed)) paquete;

        paquete.id = 0x0001;
        paquete.temp = temp;
        paquete.bat_mV = (uint16_t)(bateria * 1000);
        paquete.txNum = txCount++;

        radio.transmit((uint8_t*)&paquete, sizeof(paquete));
        radio.sleep();  // Poner SX1276 en sleep
    }

    // 5. Apagar todo
    vextOFF();

    // 6. Dormir
    esp_sleep_enable_timer_wakeup(SLEEP_SECONDS * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
}

void loop() {} // No se usa
```

## Consejos de Optimización

| Acción | Ahorro |
|---|---|
| Apagar Vext (GPIO 21 HIGH) | ~10 mA |
| `WiFi.mode(WIFI_OFF)` | ~80-150 mA |
| `btStop()` | ~30 mA |
| `radio.sleep()` (SX1276) | ~1 mA → ~1 µA |
| Reducir frecuencia CPU: `setCpuFrequencyMhz(80)` | ~20 mA |
| Apagar LEDs (GPIO 25 LOW) | ~5 mA |
| Desactivar brownout: `WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0)` | Evita resets con batería baja |

## Instrucciones para el Asistente

Al generar código de gestión de energía:
- Siempre usar `RTC_DATA_ATTR` para variables que deban sobrevivir al Deep Sleep.
- Antes del Deep Sleep, apagar WiFi, BT, Vext, y poner LoRa en sleep.
- Recordar que tras Deep Sleep, el ESP32 **reinicia desde setup()** — no continúa desde donde estaba.
- Advertir que GPIO 37 (batería) comparte ADC1 con otros pines — evitar conflictos.
- Para nodos remotos, siempre incluir protección de batería baja.
- La calibración del ADC varía entre placas — sugerir ajuste manual si la lectura no es precisa.
- Sugerir `setCpuFrequencyMhz(80)` o `(160)` si no se necesita velocidad máxima.
