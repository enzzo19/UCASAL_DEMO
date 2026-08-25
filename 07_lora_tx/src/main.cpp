#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include "telemetria_packet.h"
#include "logo_bitmap.h"

// ========== PINES HELTEC V2 ==========
// OLED
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21

// LoRa SX1276
#define PIN_LORA_NSS  18
#define PIN_LORA_DIO0 26
#define PIN_LORA_RST  14
#define PIN_LORA_DIO1 35

// Otros
#define PIN_LED       25
#define PIN_DHT       13
#define PIN_BATT      37   // ADC1_1 — lectura batería

// ========== INTERVALOS ==========
#define INTERVALO_TX      1000   // Enviar LoRa cada 1 segundo
#define INTERVALO_SENSOR  2500   // Leer DHT22 cada 2.5s (mínimo 2s)

// ========== OBJETOS ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

SX1276 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);

Adafruit_BMP085 bmp;
DHT dht(PIN_DHT, DHT22);

// ========== ESTADO ==========
bool bmpOK = false;
bool loraOK = false;

TelemetriaPacket pkt;
uint32_t secuencia = 0;
uint32_t paquetesEnviados = 0;
uint32_t erroresLoRa = 0;
uint8_t  erroresSensor = 0;

unsigned long ultimoTX = 0;
unsigned long ultimoSensor = 0;

// ========== LEER BATERÍA ==========
uint16_t leerBateria() {
    uint32_t raw = 0;
    for (int i = 0; i < 8; i++) {
        raw += analogRead(PIN_BATT);
    }
    raw /= 8;
    // Divisor de voltaje en placa: factor ~2, Vref 3.3V, 12-bit ADC
    return (uint16_t)((raw / 4095.0) * 3.3 * 2 * 1000);
}

// ========== LEER SENSORES ==========
void leerSensores() {
    // DHT22
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
        pkt.temperatura = t;
        pkt.humedad = h;
        pkt.sensTermica = dht.computeHeatIndex(t, h, false);
    } else {
        erroresSensor++;
    }

    // BMP180
    if (bmpOK) {
        pkt.presion = bmp.readPressure() / 100.0;
        pkt.altitud = bmp.readAltitude(101325);
    }

    // Batería
    pkt.bateria_mV = leerBateria();
    pkt.errores = erroresSensor;
}

// ========== ENVIAR PAQUETE LORA ==========
bool enviarPaquete() {
    pkt.version = TELEMETRIA_VERSION;
    pkt.nodeId = NODE_ID_DEFAULT;
    pkt.secuencia = secuencia++;

    int state = radio.transmit((uint8_t*)&pkt, sizeof(TelemetriaPacket));

    if (state == RADIOLIB_ERR_NONE) {
        paquetesEnviados++;
        return true;
    } else {
        erroresLoRa++;
        Serial.printf("LoRa TX error: %d\n", state);
        return false;
    }
}

// ========== SERIAL ==========
void imprimirSerial(bool txOK) {
    Serial.printf("[TX #%lu | seq:%lu] ", paquetesEnviados, pkt.secuencia);
    Serial.printf("T:%.1fC H:%.1f%% P:%.1fhPa A:%.0fm ST:%.1fC Bat:%dmV ",
        pkt.temperatura, pkt.humedad, pkt.presion, pkt.altitud,
        pkt.sensTermica, pkt.bateria_mV);
    Serial.printf("| %s\n", txOK ? "OK" : "FAIL");
}

// ========== PANTALLA OLED ==========
void actualizarOLED(bool txOK) {
    display.clearBuffer();
    char buf[32];

    // Header
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "TX LoRa 915MHz");

    // Indicador TX
    if (txOK) {
        display.drawDisc(120, 6, 5);  // Círculo lleno = OK
    } else {
        display.drawCircle(120, 6, 5); // Círculo vacío = error
    }
    display.drawHLine(0, 13, 128);

    // Datos sensores
    display.setFont(u8g2_font_6x10_tr);

    snprintf(buf, sizeof(buf), "T: %.1f C", pkt.temperatura);
    display.drawStr(0, 25, buf);
    snprintf(buf, sizeof(buf), "H: %.1f %%", pkt.humedad);
    display.drawStr(66, 25, buf);

    snprintf(buf, sizeof(buf), "P: %.1f hPa", pkt.presion);
    display.drawStr(0, 37, buf);
    snprintf(buf, sizeof(buf), "A: %.0f m", pkt.altitud);
    display.drawStr(80, 37, buf);

    // Línea separadora
    display.drawHLine(0, 40, 128);

    // Info LoRa
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "Seq: %lu", pkt.secuencia);
    display.drawStr(0, 50, buf);
    snprintf(buf, sizeof(buf), "OK:%lu Err:%lu", paquetesEnviados, erroresLoRa);
    display.drawStr(0, 59, buf);

    // Batería
    snprintf(buf, sizeof(buf), "Bat:%dmV", pkt.bateria_mV);
    display.drawStr(80, 50, buf);

    // Uptime
    unsigned long seg = millis() / 1000;
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", seg / 3600, (seg % 3600) / 60, seg % 60);
    display.drawStr(86, 59, buf);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║  TRANSMISOR LoRa — Heltec V2        ║");
    Serial.println("║  BMP180 + DHT22 → LoRa 915 MHz      ║");
    Serial.println("╚══════════════════════════════════════╝\n");

    // Vext ON + LED
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    // I2C + OLED
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    display.begin();
    display.setContrast(255);

    // ========== SPLASH: LOGO IITA IoT (branding) ==========

    // Paso 1: Cortina reveal del logo (izquierda a derecha)
    for (int x = 0; x <= 64; x += 2) {
        display.clearBuffer();
        for (int col = 0; col < x; col++) {
            for (int row = 0; row < 64; row++) {
                int byteIdx = row * 8 + col / 8;
                int bitIdx = col % 8;
                if (pgm_read_byte(&logo_icon[byteIdx]) & (1 << bitIdx)) {
                    display.drawPixel(col, row);
                }
            }
        }
        display.sendBuffer();
        delay(12);
    }

    // Paso 2: Logo + "IITA IoT" + ondas LoRa
    display.clearBuffer();
    display.drawXBMP(0, 0, LOGO_ICON_WIDTH, LOGO_ICON_HEIGHT, logo_icon);

    display.setFont(u8g2_font_ncenB14_tr);
    display.drawStr(70, 22, "IITA");
    display.setFont(u8g2_font_ncenB12_tr);
    display.drawStr(80, 40, "IoT");

    display.drawHLine(66, 44, 60);
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(68, 55, "Taller IoT");
    display.drawStr(68, 63, "UCASAL 2026");
    display.sendBuffer();
    delay(3000);

    // ========== INIT SENSORES + LORA ==========

    // DHT22
    dht.begin();
    Serial.println("[OK] DHT22 inicializado (GPIO 13)");

    // BMP180
    bmpOK = bmp.begin();
    Serial.printf("[%s] BMP180 I2C\n", bmpOK ? "OK" : "!!");

    // LoRa
    Serial.print("[..] LoRa inicializando... ");
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC, LORA_POWER, LORA_PREAMBLE, 0);
    if (state == RADIOLIB_ERR_NONE) {
        loraOK = true;
        Serial.println("OK");
        Serial.printf("     Freq: %.1f MHz | SF%d | BW: %.0f kHz | CR 4/%d | %d dBm\n",
            LORA_FREQ, LORA_SF, LORA_BW, LORA_CR, LORA_POWER);
    } else {
        Serial.printf("ERROR %d\n", state);
    }

    // Inicializar paquete con ceros
    memset(&pkt, 0, sizeof(TelemetriaPacket));

    // Paso 3: Pantalla info del transmisor
    display.clearBuffer();
    display.drawRFrame(0, 0, 128, 64, 4);
    display.setFont(u8g2_font_ncenB08_tr);
    const char* st1 = "TRANSMISOR LoRa";
    display.drawStr((128 - display.getStrWidth(st1)) / 2, 16, st1);
    display.drawHLine(10, 20, 108);
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(10, 33, "BMP180 + DHT22");
    display.drawStr(10, 45, "915MHz SF9 17dBm");
    display.setFont(u8g2_font_5x7_tr);
    char infoBuf[28];
    snprintf(infoBuf, sizeof(infoBuf), "Node:%d  %s  %s",
        NODE_ID_DEFAULT,
        bmpOK ? "BMP:OK" : "BMP:--",
        loraOK ? "LoRa:OK" : "LoRa:!!");
    display.drawStr(10, 57, infoBuf);
    display.sendBuffer();
    delay(2000);

    Serial.println("\n--- Transmitiendo cada 1 segundo ---\n");
}

// ========== LOOP ==========
void loop() {
    unsigned long ahora = millis();

    // Leer sensores cada 2.5 segundos
    if (ahora - ultimoSensor >= INTERVALO_SENSOR) {
        ultimoSensor = ahora;
        leerSensores();
    }

    // Transmitir cada 1 segundo
    if (ahora - ultimoTX >= INTERVALO_TX) {
        ultimoTX = ahora;

        if (loraOK) {
            // LED ON antes de TX
            digitalWrite(PIN_LED, HIGH);

            bool txOK = enviarPaquete();
            imprimirSerial(txOK);
            actualizarOLED(txOK);

            // LED OFF después de TX (blink corto)
            delay(50);
            digitalWrite(PIN_LED, LOW);
        } else {
            Serial.println("LoRa no inicializado!");
        }
    }
}
