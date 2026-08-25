#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include "telemetria_packet.h"
#include "logo_bitmap.h"

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25

// LoRa SX1276
#define PIN_LORA_NSS  18
#define PIN_LORA_DIO0 26
#define PIN_LORA_RST  14
#define PIN_LORA_DIO1 35

// ========== OBJETOS ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

SX1276 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);

// ========== ESTADO ==========
bool loraOK = false;

TelemetriaPacket pkt;
bool paqueteRecibido = false;
uint32_t paquetesRX = 0;
uint32_t paquetesPerdidos = 0;
uint32_t ultimaSecuencia = 0;
bool primerPaquete = true;

float rssi = 0;
float snr = 0;

unsigned long ultimoRX = 0;

// ========== RECIBIR PAQUETE ==========
void intentarRecibir() {
    uint8_t buf[sizeof(TelemetriaPacket)];
    int state = radio.receive(buf, sizeof(TelemetriaPacket));

    if (state == RADIOLIB_ERR_NONE) {
        memcpy(&pkt, buf, sizeof(TelemetriaPacket));

        // Validar versión
        if (pkt.version != TELEMETRIA_VERSION) {
            Serial.printf("⚠ Version incorrecta: %d (esperado %d)\n",
                pkt.version, TELEMETRIA_VERSION);
            return;
        }

        // Contar paquetes perdidos
        if (!primerPaquete) {
            uint32_t esperada = ultimaSecuencia + 1;
            if (pkt.secuencia > esperada) {
                paquetesPerdidos += (pkt.secuencia - esperada);
            }
        }
        primerPaquete = false;
        ultimaSecuencia = pkt.secuencia;

        // Métricas de señal
        rssi = radio.getRSSI();
        snr = radio.getSNR();

        paquetesRX++;
        paqueteRecibido = true;
        ultimoRX = millis();

        // LED blink
        digitalWrite(PIN_LED, HIGH);

    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        Serial.printf("LoRa RX error: %d\n", state);
    }
}

// ========== SERIAL ==========
void imprimirSerial() {
    Serial.printf("[RX #%lu | seq:%lu] ", paquetesRX, pkt.secuencia);
    Serial.printf("Node:%d T:%.1fC H:%.1f%% P:%.1fhPa A:%.0fm ST:%.1fC ",
        pkt.nodeId, pkt.temperatura, pkt.humedad,
        pkt.presion, pkt.altitud, pkt.sensTermica);
    Serial.printf("Bat:%dmV | RSSI:%.0f SNR:%.1f | Lost:%lu\n",
        pkt.bateria_mV, rssi, snr, paquetesPerdidos);
}

// ========== PANTALLA: DASHBOARD RX ==========
void pantallaDashboard() {
    display.clearBuffer();
    char buf[32];

    // Header
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "RX LoRa 915MHz");

    // Indicador de señal (barras según RSSI)
    int barras = 0;
    if (rssi > -70) barras = 4;
    else if (rssi > -85) barras = 3;
    else if (rssi > -100) barras = 2;
    else if (rssi > -115) barras = 1;

    for (int i = 0; i < 4; i++) {
        int h = 2 + i * 2;
        int x = 110 + i * 5;
        if (i < barras) {
            display.drawBox(x, 10 - h, 3, h);
        } else {
            display.drawFrame(x, 10 - h, 3, h);
        }
    }
    display.drawHLine(0, 13, 128);

    // Datos de telemetría
    display.setFont(u8g2_font_6x10_tr);

    snprintf(buf, sizeof(buf), "T: %.1f C", pkt.temperatura);
    display.drawStr(0, 25, buf);
    snprintf(buf, sizeof(buf), "H: %.1f %%", pkt.humedad);
    display.drawStr(66, 25, buf);

    snprintf(buf, sizeof(buf), "P: %.1f hPa", pkt.presion);
    display.drawStr(0, 37, buf);
    snprintf(buf, sizeof(buf), "A: %.0f m", pkt.altitud);
    display.drawStr(80, 37, buf);

    display.drawHLine(0, 40, 128);

    // Info de señal + paquetes
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "RSSI:%.0f SNR:%.1f", rssi, snr);
    display.drawStr(0, 50, buf);

    snprintf(buf, sizeof(buf), "RX:%lu Lost:%lu", paquetesRX, paquetesPerdidos);
    display.drawStr(0, 59, buf);

    // Tiempo desde último paquete
    unsigned long secsAgo = (millis() - ultimoRX) / 1000;
    if (secsAgo < 5) {
        snprintf(buf, sizeof(buf), "LIVE");
    } else {
        snprintf(buf, sizeof(buf), "%lus ago", secsAgo);
    }
    display.drawStr(96, 59, buf);

    display.sendBuffer();
}

// ========== PANTALLA: ESPERANDO ==========
void pantallaEsperando() {
    display.clearBuffer();

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "RX LoRa 915MHz");
    display.drawHLine(0, 13, 128);

    display.setFont(u8g2_font_ncenB10_tr);
    const char* t = "Esperando...";
    display.drawStr((128 - display.getStrWidth(t)) / 2, 35, t);

    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(20, 50, "SF9 | 125kHz | 0x12");

    // Animación: punto que rota
    unsigned long tick = (millis() / 300) % 4;
    const char* dots[] = {".", "..", "...", "...."};
    display.drawStr(100, 50, dots[tick]);

    // Uptime
    unsigned long seg = millis() / 1000;
    char buf[16];
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", seg / 3600, (seg % 3600) / 60, seg % 60);
    display.drawStr(40, 62, buf);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n╔══════════════════════════════════════╗");
    Serial.println("║  RECEPTOR LoRa — Heltec V2           ║");
    Serial.println("║  Recibe telemetria 915 MHz            ║");
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

    // ========== SPLASH: LOGO IITA IoT ==========
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

    // ========== INIT LORA ==========
    Serial.print("[..] LoRa inicializando... ");
    int state = radio.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR,
                            LORA_SYNC, LORA_POWER, LORA_PREAMBLE, 0);
    if (state == RADIOLIB_ERR_NONE) {
        loraOK = true;
        Serial.println("OK");
        Serial.printf("     Freq: %.1f MHz | SF%d | BW: %.0f kHz | CR 4/%d\n",
            LORA_FREQ, LORA_SF, LORA_BW, LORA_CR);
    } else {
        Serial.printf("ERROR %d\n", state);
    }

    // Pantalla info receptor
    display.clearBuffer();
    display.drawRFrame(0, 0, 128, 64, 4);
    display.setFont(u8g2_font_ncenB08_tr);
    const char* st1 = "RECEPTOR LoRa";
    display.drawStr((128 - display.getStrWidth(st1)) / 2, 16, st1);
    display.drawHLine(10, 20, 108);
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(10, 33, "Escuchando...");
    display.drawStr(10, 45, "915MHz SF9 125kHz");
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(10, 57, loraOK ? "LoRa: OK" : "LoRa: ERROR!!");
    display.sendBuffer();
    delay(2000);

    Serial.println("\n--- Escuchando paquetes LoRa ---\n");
}

// ========== LOOP ==========
void loop() {
    if (!loraOK) return;

    // Intentar recibir (con timeout corto de ~1s)
    intentarRecibir();

    if (paqueteRecibido) {
        paqueteRecibido = false;
        imprimirSerial();
        pantallaDashboard();

        // LED off después de mostrar
        delay(50);
        digitalWrite(PIN_LED, LOW);
    } else {
        // Si no hay paquete, refrescar pantalla
        static unsigned long ultimoRefresh = 0;
        if (millis() - ultimoRefresh > 500) {
            ultimoRefresh = millis();
            if (paquetesRX > 0) {
                pantallaDashboard();
            } else {
                pantallaEsperando();
            }
        }
    }
}
