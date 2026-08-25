#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <RadioLib.h>
#include <WiFi.h>
#include <time.h>
#include <FirebaseESP32.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include "telemetria_packet.h"
#include "logo_bitmap.h"
#include "secrets.h"

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

// ========== NTP CONFIG ==========
#define NTP_SERVER_1       "pool.ntp.org"
#define NTP_SERVER_2       "time.google.com"
#define NTP_GMT_OFFSET     -10800
#define NTP_DST_OFFSET     0
#define NTP_SYNC_INTERVAL  3600000UL

// ========== WiFi CONFIG ==========
#define WIFI_TIMEOUT_MS    15000
#define WIFI_RETRY_MS      10000

// ========== FIREBASE CONFIG ==========
#define FB_ENVIO_ACTUAL_MS    3000UL    // Actualizar /actual cada 3s

// ========== OBJETOS ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

SX1276 radio = new Module(PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1);

// Firebase
FirebaseData fbdo;
FirebaseAuth fbAuth;
FirebaseConfig fbConfig;

// ========== ESTADO ==========
bool loraOK = false;
bool wifiConectado = false;
bool ntpSincronizado = false;
bool firebaseOK = false;
uint32_t fbEnviosOK = 0;
uint32_t fbErrores = 0;

TelemetriaPacket pkt;
bool paqueteRecibido = false;
uint32_t paquetesRX = 0;
uint32_t paquetesPerdidos = 0;
uint32_t ultimaSecuencia = 0;
bool primerPaquete = true;

float rssi = 0;
float snr  = 0;

unsigned long ultimoRX           = 0;
unsigned long ultimoNTPSync      = 0;
unsigned long ultimoWiFiRetry    = 0;
unsigned long ultimoFBActual     = 0;

// ========== TIEMPO ==========
String obtenerTimestamp() {
    struct tm ti;
    if (!getLocalTime(&ti, 100)) return "----/--/-- --:--:--";
    char buf[24];
    strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &ti);
    return String(buf);
}

String obtenerHora() {
    struct tm ti;
    if (!getLocalTime(&ti, 100)) return "--:--:--";
    char buf[12];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
    return String(buf);
}

unsigned long obtenerEpoch() {
    time_t now;
    time(&now);
    return (unsigned long)now;
}

// ========== NTP ==========
void sincronizarNTP() {
    if (!wifiConectado) return;

    Serial.print("[..] Sincronizando NTP... ");
    configTime(NTP_GMT_OFFSET, NTP_DST_OFFSET, NTP_SERVER_1, NTP_SERVER_2);

    struct tm ti;
    if (getLocalTime(&ti, 5000)) {
        ntpSincronizado = true;
        ultimoNTPSync = millis();
        char buf[24];
        strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &ti);
        Serial.printf("OK -> %s (UTC-3)\n", buf);
    } else {
        Serial.println("FALLO");
    }
}

// ========== FIREBASE INIT ==========
void initFirebase() {
    Serial.print("[..] Firebase inicializando... ");

    fbConfig.api_key = SECRET_FIREBASE_API_KEY;
    fbConfig.database_url = SECRET_FIREBASE_DB_URL;

    fbAuth.user.email = SECRET_FIREBASE_USER_EMAIL;
    fbAuth.user.password = SECRET_FIREBASE_USER_PASS;

    fbConfig.token_status_callback = tokenStatusCallback;

    Firebase.begin(&fbConfig, &fbAuth);
    Firebase.reconnectNetwork(true);

    fbdo.setBSSLBufferSize(4096, 1024);
    fbdo.setResponseSize(2048);

    // Esperar token (máx 10s)
    unsigned long inicio = millis();
    while (!Firebase.ready() && millis() - inicio < 10000) {
        delay(100);
    }

    firebaseOK = Firebase.ready();
    Serial.println(firebaseOK ? "OK" : "FALLO (reintentará)");
}

// ========== FIREBASE: ENVIAR DATOS ACTUALES ==========
void fbEnviarActual() {
    if (!Firebase.ready() || !ntpSincronizado) return;

    unsigned long epoch = obtenerEpoch();
    const char* nodePath = "/actual";

    FirebaseJson json;
    json.set("temperatura", pkt.temperatura);
    json.set("humedad", pkt.humedad);
    json.set("presion", pkt.presion);
    json.set("altitud", pkt.altitud);
    json.set("sens_termica", pkt.sensTermica);
    json.set("bateria_mV", pkt.bateria_mV);
    json.set("rssi_lora", (int)rssi);
    json.set("snr_lora", snr);
    json.set("paquetes_rx", (int)paquetesRX);
    json.set("paquetes_perdidos", (int)paquetesPerdidos);
    json.set("timestamp", (int)epoch);

    // Fecha legible
    struct tm ti;
    time_t t = epoch;
    localtime_r(&t, &ti);
    char fechaHora[20];
    strftime(fechaHora, sizeof(fechaHora), "%Y-%m-%d %H:%M:%S", &ti);
    json.set("fecha_hora", fechaHora);

    if (Firebase.set(fbdo, nodePath, json)) {
        fbEnviosOK++;
        Serial.printf("[FB] Actual OK (%lu envios)\n", fbEnviosOK);
    } else {
        fbErrores++;
        Serial.printf("[FB] Actual ERROR: %s\n", fbdo.errorReason().c_str());
    }
}

// ========== RECIBIR PAQUETE LoRa ==========
void intentarRecibir() {
    uint8_t buf[sizeof(TelemetriaPacket)];
    int state = radio.receive(buf, sizeof(TelemetriaPacket));

    if (state == RADIOLIB_ERR_NONE) {
        memcpy(&pkt, buf, sizeof(TelemetriaPacket));

        if (pkt.version != TELEMETRIA_VERSION) {
            Serial.printf("⚠ Version incorrecta: %d (esperado %d)\n",
                pkt.version, TELEMETRIA_VERSION);
            return;
        }

        if (!primerPaquete) {
            uint32_t esperada = ultimaSecuencia + 1;
            if (pkt.secuencia > esperada) {
                paquetesPerdidos += (pkt.secuencia - esperada);
            }
        }
        primerPaquete = false;
        ultimaSecuencia = pkt.secuencia;

        rssi = radio.getRSSI();
        snr  = radio.getSNR();

        paquetesRX++;
        paqueteRecibido = true;
        ultimoRX = millis();

        digitalWrite(PIN_LED, HIGH);

    } else if (state != RADIOLIB_ERR_RX_TIMEOUT) {
        Serial.printf("LoRa RX error: %d\n", state);
    }
}

// ========== SERIAL CON TIMESTAMP ==========
void imprimirSerial() {
    String ts = obtenerTimestamp();
    Serial.printf("[%s] RX #%lu seq:%lu Node:%d | ",
        ts.c_str(), paquetesRX, pkt.secuencia, pkt.nodeId);
    Serial.printf("T:%.1fC H:%.1f%% P:%.1fhPa A:%.0fm ST:%.1fC ",
        pkt.temperatura, pkt.humedad, pkt.presion,
        pkt.altitud, pkt.sensTermica);
    Serial.printf("Bat:%dmV | RSSI:%.0f SNR:%.1f | Lost:%lu\n",
        pkt.bateria_mV, rssi, snr, paquetesPerdidos);
}

// ========== PANTALLA: DASHBOARD ==========
void pantallaDashboard() {
    display.clearBuffer();
    char buf[32];

    // ── Header: hora + iconos estado ──
    display.setFont(u8g2_font_6x10_tr);
    String hora = obtenerHora();
    display.drawStr(0, 9, hora.c_str());

    display.setFont(u8g2_font_5x7_tr);

    // WiFi indicator
    if (wifiConectado) {
        display.drawDisc(62, 5, 2);
    } else {
        display.drawCircle(62, 5, 2);
    }

    // Firebase indicator
    if (Firebase.ready()) {
        display.drawStr(70, 9, "FB");
    }

    // NTP indicator
    if (ntpSincronizado) {
        display.drawStr(85, 9, "NTP");
    }

    // Barras señal LoRa
    int barras = 0;
    if      (rssi > -70)  barras = 4;
    else if (rssi > -85)  barras = 3;
    else if (rssi > -100) barras = 2;
    else if (rssi > -115) barras = 1;

    for (int i = 0; i < 4; i++) {
        int h = 2 + i * 2;
        int x = 112 + i * 4;
        if (i < barras) {
            display.drawBox(x, 10 - h, 3, h);
        } else {
            display.drawFrame(x, 10 - h, 3, h);
        }
    }
    display.drawHLine(0, 12, 128);

    // ── Datos telemetría ──
    display.setFont(u8g2_font_6x10_tr);

    snprintf(buf, sizeof(buf), "T:%.1f C", pkt.temperatura);
    display.drawStr(0, 24, buf);
    snprintf(buf, sizeof(buf), "H:%.1f %%", pkt.humedad);
    display.drawStr(68, 24, buf);

    snprintf(buf, sizeof(buf), "P:%.1f hPa", pkt.presion);
    display.drawStr(0, 36, buf);
    snprintf(buf, sizeof(buf), "A:%.0fm", pkt.altitud);
    display.drawStr(80, 36, buf);

    display.drawHLine(0, 39, 128);

    // ── Info señal + Firebase ──
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "RSSI:%.0f SNR:%.1f", rssi, snr);
    display.drawStr(0, 49, buf);

    // LIVE / tiempo / LOST
    unsigned long segsAgo = (millis() - ultimoRX) / 1000;
    if (segsAgo < 3) {
        if ((millis() / 500) % 2 == 0) {
            display.drawStr(100, 49, "LIVE");
        }
    } else if (segsAgo < 60) {
        snprintf(buf, sizeof(buf), "%lus", segsAgo);
        display.drawStr(108, 49, buf);
    } else {
        display.drawStr(104, 49, "LOST");
    }

    // Paquetes + Firebase envíos
    snprintf(buf, sizeof(buf), "RX:%lu L:%lu", paquetesRX, paquetesPerdidos);
    display.drawStr(0, 58, buf);
    snprintf(buf, sizeof(buf), "FB:%lu", fbEnviosOK);
    display.drawStr(90, 58, buf);

    display.sendBuffer();
}

// ========== PANTALLA: ESPERANDO ==========
void pantallaEsperando() {
    display.clearBuffer();

    display.setFont(u8g2_font_6x10_tr);
    String hora = obtenerHora();
    display.drawStr(0, 9, hora.c_str());

    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(55, 9, wifiConectado ? "WiFi" : "----");
    display.drawStr(80, 9, Firebase.ready() ? "FB OK" : "FB --");
    display.drawHLine(0, 12, 128);

    display.setFont(u8g2_font_ncenB10_tr);
    const char* t = "Esperando...";
    display.drawStr((128 - display.getStrWidth(t)) / 2, 33, t);

    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(15, 46, "SF9 | 125kHz | 915MHz");

    unsigned long tick = (millis() / 300) % 4;
    const char* dots[] = {".", "..", "...", "...."};
    display.drawStr(108, 46, dots[tick]);

    if (ntpSincronizado) {
        display.drawStr(15, 58, "NTP OK | Firebase listo");
    } else if (wifiConectado) {
        display.drawStr(20, 58, "Sincronizando NTP...");
    } else {
        display.drawStr(20, 58, "Sin WiFi — sin hora");
    }

    display.sendBuffer();
}

// ========== PANTALLA: CONECTANDO WiFi ==========
void pantallaWiFiConectando(int progreso) {
    display.clearBuffer();
    display.drawRFrame(0, 0, 128, 64, 4);

    display.setFont(u8g2_font_ncenB08_tr);
    const char* titulo = "Conectando WiFi";
    display.drawStr((128 - display.getStrWidth(titulo)) / 2, 16, titulo);
    display.drawHLine(10, 20, 108);

    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(10, 35, SECRET_WIFI_SSID);

    display.drawFrame(10, 42, 108, 10);
    int w = (progreso * 104) / 100;
    if (w > 104) w = 104;
    if (w > 0) display.drawBox(12, 44, w, 6);

    display.setFont(u8g2_font_5x7_tr);
    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", progreso > 100 ? 100 : progreso);
    display.drawStr(56, 62, buf);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n╔══════════════════════════════════════════════╗");
    Serial.println("║  RECEPTOR LoRa + WiFi/NTP + Firebase RTDB   ║");
    Serial.println("║  Telemetria 915 MHz → Nube en tiempo real    ║");
    Serial.println("╚══════════════════════════════════════════════╝\n");

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
                int bitIdx  = col % 8;
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

    // ========== INIT LoRa ==========
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

    // ========== WiFi ==========
    pantallaWiFiConectando(0);

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
    Serial.printf("[..] Conectando WiFi: %s ", SECRET_WIFI_SSID);

    unsigned long wifiInicio = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - wifiInicio < WIFI_TIMEOUT_MS) {
        int progreso = ((millis() - wifiInicio) * 100) / WIFI_TIMEOUT_MS;
        pantallaWiFiConectando(progreso);
        delay(250);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        wifiConectado = true;
        pantallaWiFiConectando(100);
        Serial.printf("\n[OK] WiFi conectado — IP: %s  RSSI: %d dBm\n",
            WiFi.localIP().toString().c_str(), WiFi.RSSI());
        delay(500);
    } else {
        wifiConectado = false;
        Serial.println("\n[!!] WiFi NO conectado — reintentará cada 10s");
    }

    // ========== NTP ==========
    sincronizarNTP();

    // ========== FIREBASE ==========
    if (wifiConectado) {
        initFirebase();
    }

    // ========== PANTALLA RESUMEN ==========
    display.clearBuffer();
    display.drawRFrame(0, 0, 128, 64, 4);

    display.setFont(u8g2_font_ncenB08_tr);
    const char* st1 = "RX LoRa+Firebase";
    display.drawStr((128 - display.getStrWidth(st1)) / 2, 14, st1);
    display.drawHLine(10, 18, 108);

    display.setFont(u8g2_font_5x7_tr);
    char infoBuf[32];
    snprintf(infoBuf, sizeof(infoBuf), "LoRa: %s  | 915MHz SF9",
        loraOK ? "OK" : "!!");
    display.drawStr(10, 30, infoBuf);
    snprintf(infoBuf, sizeof(infoBuf), "WiFi: %s  | %s",
        wifiConectado ? "OK" : "NO", SECRET_WIFI_SSID);
    display.drawStr(10, 40, infoBuf);
    snprintf(infoBuf, sizeof(infoBuf), "NTP:  %s",
        ntpSincronizado ? "OK (UTC-3)" : "Pendiente");
    display.drawStr(10, 50, infoBuf);
    snprintf(infoBuf, sizeof(infoBuf), "Firebase: %s",
        firebaseOK ? "OK" : "Pendiente");
    display.drawStr(10, 60, infoBuf);

    display.sendBuffer();
    delay(2500);

    Serial.println("\n--- Escuchando LoRa + subiendo a Firebase ---\n");
}

// ========== LOOP ==========
void loop() {
    if (!loraOK) return;

    // ── Recibir paquete LoRa ──
    intentarRecibir();

    if (paqueteRecibido) {
        paqueteRecibido = false;
        imprimirSerial();
        pantallaDashboard();

        delay(50);
        digitalWrite(PIN_LED, LOW);
    } else {
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

    // ── Firebase: enviar datos actuales cada 15s ──
    if (paquetesRX > 0 && millis() - ultimoFBActual > FB_ENVIO_ACTUAL_MS) {
        ultimoFBActual = millis();
        fbEnviarActual();
    }

    // ── WiFi: verificar y reconectar ──
    static unsigned long ultimoWiFiCheck = 0;
    if (millis() - ultimoWiFiCheck > 10000) {
        ultimoWiFiCheck = millis();

        if (WiFi.status() == WL_CONNECTED) {
            if (!wifiConectado) {
                wifiConectado = true;
                Serial.printf("[OK] WiFi reconectado — IP: %s\n",
                    WiFi.localIP().toString().c_str());
                if (!ntpSincronizado) sincronizarNTP();
                if (!firebaseOK) initFirebase();
            }
        } else {
            if (wifiConectado) {
                Serial.println("[!!] WiFi desconectado");
                wifiConectado = false;
            }
            if (millis() - ultimoWiFiRetry > WIFI_RETRY_MS) {
                ultimoWiFiRetry = millis();
                Serial.println("[..] Reintentando WiFi...");
                WiFi.disconnect();
                WiFi.begin(SECRET_WIFI_SSID, SECRET_WIFI_PASS);
            }
        }
    }

    // ── NTP: re-sync cada hora ──
    if (wifiConectado && millis() - ultimoNTPSync > NTP_SYNC_INTERVAL) {
        sincronizarNTP();
    }
}
