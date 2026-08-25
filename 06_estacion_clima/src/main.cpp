#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25
#define PIN_DHT       13

// ========== OBJETOS ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

Adafruit_BMP085 bmp;
DHT dht(PIN_DHT, DHT22);

bool bmpOK = false;

// ========== DATOS SENSORES ==========
// DHT22
float temperatura = 0;
float humedad = 0;
float sensTermica = 0;
bool dhtOK = false;

// BMP180
float presion = 0;       // hPa
float altitud = 0;       // metros

// Min/Max
float tempMin = 999, tempMax = -999;
float humMin = 999, humMax = 0;
float presMin = 9999, presMax = 0;

// Historial (últimos 64 valores)
#define HIST_SIZE 64
float histTemp[HIST_SIZE];
float histHum[HIST_SIZE];
float histPres[HIST_SIZE];
int histIndex = 0;
bool histLleno = false;

// ========== LEER SENSORES ==========
void leerSensores() {
    // DHT22: temperatura + humedad
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (!isnan(t) && !isnan(h)) {
        dhtOK = true;
        temperatura = t;
        humedad = h;
        sensTermica = dht.computeHeatIndex(t, h, false);

        if (t < tempMin) tempMin = t;
        if (t > tempMax) tempMax = t;
        if (h < humMin) humMin = h;
        if (h > humMax) humMax = h;
    } else {
        dhtOK = false;
    }

    // BMP180: presión + altitud
    if (bmpOK) {
        presion = bmp.readPressure() / 100.0;
        altitud = bmp.readAltitude(101325);

        if (presion < presMin) presMin = presion;
        if (presion > presMax) presMax = presion;
    }

    // Historial
    histTemp[histIndex] = temperatura;
    histHum[histIndex] = humedad;
    histPres[histIndex] = presion;
    histIndex = (histIndex + 1) % HIST_SIZE;
    if (histIndex == 0) histLleno = true;
}

// ========== SERIAL ==========
void imprimirSerial() {
    Serial.println("══════════════════════════════════");
    Serial.println("  ESTACION CLIMATICA");
    Serial.println("──────────────────────────────────");
    if (dhtOK) {
        Serial.printf("  Temperatura:  %.1f °C  (%.1f/%.1f)\n", temperatura, tempMin, tempMax);
        Serial.printf("  Humedad:      %.1f %%   (%.0f/%.0f)\n", humedad, humMin, humMax);
        Serial.printf("  Sens.Termica: %.1f °C\n", sensTermica);
    } else {
        Serial.println("  DHT22: ERROR lectura");
    }
    if (bmpOK) {
        Serial.printf("  Presion:      %.2f hPa (%.1f/%.1f)\n", presion, presMin, presMax);
        Serial.printf("  Altitud:      %.1f m\n", altitud);
    } else {
        Serial.println("  BMP180: NO conectado");
    }
}

// ========== PANTALLA 1: DASHBOARD PRINCIPAL ==========
void pantallaDashboard() {
    display.clearBuffer();
    char buf[32];
    int ancho;

    // Título
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Estacion Clima");
    display.drawHLine(0, 13, 128);

    // --- Temperatura (grande, arriba izquierda) ---
    display.setFont(u8g2_font_ncenB14_tr);
    snprintf(buf, sizeof(buf), "%.1f", temperatura);
    display.drawStr(0, 32, buf);
    ancho = display.getStrWidth(buf);
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(ancho + 2, 22, "o");
    display.drawStr(ancho + 7, 32, "C");

    // --- Humedad (abajo izquierda) ---
    display.setFont(u8g2_font_ncenB10_tr);
    snprintf(buf, sizeof(buf), "%.1f", humedad);
    display.drawStr(0, 48, buf);
    ancho = display.getStrWidth(buf);
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(ancho + 3, 48, "%");

    // --- Línea vertical separadora ---
    display.drawVLine(68, 14, 50);

    // --- Presión (derecha arriba) ---
    display.setFont(u8g2_font_6x10_tr);
    snprintf(buf, sizeof(buf), "%.1f", presion);
    display.drawStr(72, 26, buf);
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(72, 34, "hPa");

    // --- Altitud (derecha medio) ---
    display.setFont(u8g2_font_6x10_tr);
    snprintf(buf, sizeof(buf), "%.0f m", altitud);
    display.drawStr(72, 48, buf);

    // --- Footer: sensación térmica ---
    display.drawHLine(0, 53, 128);
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "ST:%.1fC  %s", sensTermica,
        dhtOK ? (bmpOK ? "ALL OK" : "BMP:ERR") : "DHT:ERR");
    display.drawStr(0, 63, buf);

    // Uptime
    unsigned long seg = millis() / 1000;
    snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", seg / 3600, (seg % 3600) / 60, seg % 60);
    display.drawStr(86, 63, buf);

    display.sendBuffer();
}

// ========== PANTALLA 2: DETALLE + MIN/MAX ==========
void pantallaDetalle() {
    display.clearBuffer();
    char buf[32];

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Detalle Min/Max");
    display.drawHLine(0, 13, 128);

    display.setFont(u8g2_font_6x10_tr);

    //          Valor         Min        Max
    // Temp     XX.X °C       XX.X       XX.X
    // Hum      XX.X %        XX         XX
    // Pres     XXXX.X hPa    XXXX.X     XXXX.X

    // Encabezados
    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(38, 22, "Actual");
    display.drawStr(80, 22, "Min");
    display.drawStr(108, 22, "Max");

    // Temperatura
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 33, "T:");
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "%.1fC", temperatura);
    display.drawStr(38, 33, buf);
    snprintf(buf, sizeof(buf), "%.1f", tempMin);
    display.drawStr(78, 33, buf);
    snprintf(buf, sizeof(buf), "%.1f", tempMax);
    display.drawStr(106, 33, buf);

    // Humedad
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 44, "H:");
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "%.1f%%", humedad);
    display.drawStr(38, 44, buf);
    snprintf(buf, sizeof(buf), "%.0f", humMin);
    display.drawStr(78, 44, buf);
    snprintf(buf, sizeof(buf), "%.0f", humMax);
    display.drawStr(106, 44, buf);

    // Presión
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(0, 55, "P:");
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "%.1f", presion);
    display.drawStr(30, 55, buf);
    snprintf(buf, sizeof(buf), "%.1f", presMin);
    display.drawStr(72, 55, buf);
    snprintf(buf, sizeof(buf), "%.1f", presMax);
    display.drawStr(102, 55, buf);

    // Altitud
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "Alt: %.0f m", altitud);
    display.drawStr(0, 64, buf);

    display.sendBuffer();
}

// ========== PANTALLA 3: GRÁFICO TEMPERATURA ==========
void pantallaGrafTemp() {
    display.clearBuffer();
    char buf[24];

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Temperatura");
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "%.1f C", temperatura);
    display.drawStr(84, 10, buf);
    display.drawHLine(0, 13, 128);

    int count = histLleno ? HIST_SIZE : histIndex;
    if (count < 2) {
        display.drawStr(20, 40, "Recopilando datos...");
        display.sendBuffer();
        return;
    }

    int gY = 16, gH = 46, gX = 18, gW = 108;
    float vMin = 999, vMax = -999;
    for (int i = 0; i < count; i++) {
        if (histTemp[i] < vMin) vMin = histTemp[i];
        if (histTemp[i] > vMax) vMax = histTemp[i];
    }
    if (vMax - vMin < 1.0) { vMin -= 0.5; vMax += 0.5; }

    display.setFont(u8g2_font_4x6_tr);
    snprintf(buf, sizeof(buf), "%.0f", vMax);
    display.drawStr(0, gY + 5, buf);
    snprintf(buf, sizeof(buf), "%.0f", vMin);
    display.drawStr(0, gY + gH, buf);

    for (int i = 1; i < count; i++) {
        int i0 = histLleno ? (histIndex + i - 1) % HIST_SIZE : i - 1;
        int i1 = histLleno ? (histIndex + i) % HIST_SIZE : i;
        int x0 = gX + (i - 1) * gW / (count - 1);
        int x1 = gX + i * gW / (count - 1);
        int y0 = gY + gH - (int)((histTemp[i0] - vMin) / (vMax - vMin) * gH);
        int y1 = gY + gH - (int)((histTemp[i1] - vMin) / (vMax - vMin) * gH);
        display.drawLine(x0, y0, x1, y1);
    }
    display.drawFrame(gX - 1, gY, gW + 2, gH + 1);
    display.sendBuffer();
}

// ========== PANTALLA 4: GRÁFICO PRESIÓN ==========
void pantallaGrafPres() {
    display.clearBuffer();
    char buf[24];

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Presion");
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "%.1f hPa", presion);
    display.drawStr(60, 10, buf);
    display.drawHLine(0, 13, 128);

    int count = histLleno ? HIST_SIZE : histIndex;
    if (count < 2) {
        display.drawStr(20, 40, "Recopilando datos...");
        display.sendBuffer();
        return;
    }

    int gY = 16, gH = 46, gX = 24, gW = 102;
    float vMin = 9999, vMax = -9999;
    for (int i = 0; i < count; i++) {
        if (histPres[i] < vMin) vMin = histPres[i];
        if (histPres[i] > vMax) vMax = histPres[i];
    }
    if (vMax - vMin < 0.5) { vMin -= 0.5; vMax += 0.5; }

    display.setFont(u8g2_font_4x6_tr);
    snprintf(buf, sizeof(buf), "%.0f", vMax);
    display.drawStr(0, gY + 5, buf);
    snprintf(buf, sizeof(buf), "%.0f", vMin);
    display.drawStr(0, gY + gH, buf);

    for (int i = 1; i < count; i++) {
        int i0 = histLleno ? (histIndex + i - 1) % HIST_SIZE : i - 1;
        int i1 = histLleno ? (histIndex + i) % HIST_SIZE : i;
        int x0 = gX + (i - 1) * gW / (count - 1);
        int x1 = gX + i * gW / (count - 1);
        int y0 = gY + gH - (int)((histPres[i0] - vMin) / (vMax - vMin) * gH);
        int y1 = gY + gH - (int)((histPres[i1] - vMin) / (vMax - vMin) * gH);
        display.drawLine(x0, y0, x1, y1);
    }
    display.drawFrame(gX - 1, gY, gW + 2, gH + 1);
    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Estacion Climatica — Heltec LoRa32 V2 ===");
    Serial.println("Sensores: DHT22 (GPIO13) + BMP180 (I2C 4/15)");
    Serial.println("================================================\n");

    // Vext ON
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);
    pinMode(PIN_LED, OUTPUT);

    // I2C + Display
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    display.begin();
    display.setContrast(255);

    // DHT22
    dht.begin();

    // BMP180
    bmpOK = bmp.begin();

    // Status
    Serial.printf("BMP180: %s\n", bmpOK ? "OK" : "NO DETECTADO");
    Serial.println("DHT22: Esperando primera lectura...\n");

    digitalWrite(PIN_LED, HIGH);

    // Splash
    display.clearBuffer();
    display.drawRFrame(0, 0, 128, 64, 6);
    display.setFont(u8g2_font_ncenB10_tr);
    const char* t1 = "Estacion";
    display.drawStr((128 - display.getStrWidth(t1)) / 2, 22, t1);
    const char* t2 = "Climatica";
    display.drawStr((128 - display.getStrWidth(t2)) / 2, 38, t2);
    display.setFont(u8g2_font_5x7_tr);
    const char* t3 = "BMP180 + DHT22";
    display.drawStr((128 - display.getStrWidth(t3)) / 2, 54, t3);
    display.sendBuffer();
    delay(2500);
}

// ========== LOOP ==========
int pantalla = 0;
unsigned long ultimaLectura = 0;
unsigned long ultimoCambio = 0;

void loop() {
    // Leer cada 2.5 segundos
    if (millis() - ultimaLectura > 2500) {
        ultimaLectura = millis();
        leerSensores();
        imprimirSerial();
    }

    // Rotar pantallas cada 5 segundos
    if (millis() - ultimoCambio > 5000) {
        ultimoCambio = millis();
        pantalla = (pantalla + 1) % 4;
    }

    // Refrescar cada 500ms
    static unsigned long ultimoRefresh = 0;
    if (millis() - ultimoRefresh > 500) {
        ultimoRefresh = millis();
        switch (pantalla) {
            case 0: pantallaDashboard(); break;
            case 1: pantallaDetalle(); break;
            case 2: pantallaGrafTemp(); break;
            case 3: pantallaGrafPres(); break;
        }
    }
}
