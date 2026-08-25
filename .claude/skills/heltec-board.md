---
name: heltec-board
description: "Referencia completa de la placa Heltec WiFi LoRa 32 V2: especificaciones, pinout, arquitectura y limitaciones."
triggers:
  - "heltec"
  - "pinout"
  - "placa"
  - "board"
  - "gpio"
  - "pines"
  - "esp32"
  - "hardware"
---

# Heltec WiFi LoRa 32 V2 — Referencia de Hardware

## Especificaciones Generales

| Parámetro | Valor |
|---|---|
| **MCU** | ESP32-D0WDQ6 (dual-core Xtensa LX6, 240 MHz) |
| **Flash** | 8 MB |
| **SRAM** | 520 KB |
| **LoRa Chip** | Semtech SX1276 |
| **Frecuencias LoRa** | 868 MHz (EU) / 915 MHz (US/AU) / 433 MHz (Asia) — depende de la variante |
| **Pantalla OLED** | 0.96" SSD1306, 128×64 píxeles, I2C |
| **WiFi** | 802.11 b/g/n, 2.4 GHz |
| **Bluetooth** | v4.2 BR/EDR + BLE |
| **USB-UART** | CP2102 |
| **Conector batería** | JST PH 2.0mm (Li-Po 3.7V) |
| **Carga de batería** | TP4054, ~500 mA |
| **Tensión operación** | 3.3V (regulador interno desde USB 5V o batería) |
| **Corriente Deep Sleep** | ~800 µA (con periféricos apagados puede bajar a ~20 µA) |
| **Dimensiones** | 50.2 × 25.5 × 9.74 mm |

## Pinout Completo

### LoRa (SPI — SX1276)
| Función | GPIO |
|---|---|
| SCK | 5 |
| MISO | 19 |
| MOSI | 27 |
| NSS (CS) | 18 |
| RST | 14 |
| DIO0 (IRQ) | 26 |
| DIO1 | 35 |
| DIO2 | 34 |

### Pantalla OLED (I2C — SSD1306)
| Función | GPIO |
|---|---|
| SDA | 4 |
| SCL | 15 |
| RST | 16 |
| Dirección I2C | 0x3C |

### Control de Energía
| Función | GPIO |
|---|---|
| Vext (alimentación externa) | 21 |
| LED integrado | 25 |
| Lectura batería (ADC) | 37 |
| Divisor batería factor | ×2 (leer ADC y multiplicar por 2) |

### GPIOs de Uso General Disponibles
| GPIO | ADC | Touch | Notas |
|---|---|---|---|
| 0 | ADC2_CH1 | T1 | BOOT (pull-up, evitar en arranque) |
| 2 | ADC2_CH2 | T2 | LED del módulo ESP32 |
| 12 | ADC2_CH5 | T5 | MTDI (cuidado con strapping pin) |
| 13 | ADC2_CH4 | T4 | Libre |
| 17 | — | — | TX2, libre |
| 22 | — | — | Libre |
| 23 | — | — | Libre |
| 32 | ADC1_CH4 | T9 | Libre |
| 33 | ADC1_CH5 | T8 | Libre |
| 36 (SVP) | ADC1_CH0 | — | Solo entrada |
| 37 | ADC1_CH1 | — | Lectura batería (ocupado) |
| 38 | ADC1_CH2 | — | Solo entrada |
| 39 (SVN) | ADC1_CH3 | — | Solo entrada |

### GPIOs OCUPADOS (NO usar para propósito general)
| GPIO | Ocupado por |
|---|---|
| 4 | OLED SDA |
| 5 | LoRa SCK |
| 14 | LoRa RST |
| 15 | OLED SCL |
| 16 | OLED RST |
| 18 | LoRa NSS |
| 19 | LoRa MISO |
| 21 | Vext control |
| 25 | LED integrado |
| 26 | LoRa DIO0 |
| 27 | LoRa MOSI |
| 34 | LoRa DIO2 (solo entrada) |
| 35 | LoRa DIO1 (solo entrada) |

## Reglas Importantes

1. **GPIO 34-39 son SOLO entrada** — no pueden configurarse como salida.
2. **ADC2 no funciona cuando WiFi está activo** — usar solo ADC1 (GPIOs 32, 33, 36, 37, 38, 39) si se usa WiFi.
3. **GPIO 12 es un strapping pin** — si se pone en HIGH al boot, el flash falla. Evitar pull-up externo.
4. **Vext (GPIO 21)**: LOW = enciende alimentación externa (OLED y sensores), HIGH = apaga. Siempre inicializar en LOW para usar la OLED.
5. **La OLED necesita reset al inicio**: pulsar GPIO 16 LOW por 50ms, luego HIGH.
6. **El LED integrado (GPIO 25)** se enciende con HIGH.
7. **Lectura de batería (GPIO 37)**: el ADC lee la mitad del voltaje real (divisor resistivo). Multiplicar por 2 y aplicar calibración: `V_bat = analogRead(37) / 4095.0 * 3.3 * 2`.

## Instrucciones para el Asistente

Cuando el usuario pregunte sobre pinout, GPIOs, o hardware de la Heltec LoRa32 V2:
- Siempre consultar las tablas de arriba antes de asignar pines.
- Advertir si un GPIO solicitado está ocupado por LoRa, OLED u otra función.
- Recordar las limitaciones de ADC2 con WiFi.
- Sugerir GPIOs libres cuando el usuario necesite conectar sensores/actuadores.
- Preferir GPIOs del grupo ADC1 si se necesita lectura analógica con WiFi activo.
