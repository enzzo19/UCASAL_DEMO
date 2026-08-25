#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <Adafruit_BMP085.h>

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25

// ========== OBJETOS ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

Adafruit_BMP085 bmp;
bool bmpOK = false;

// ========== DATOS SENSOR ==========
float presion = 0;       // hPa
float altitud = 0;       // metros
float presMin = 9999;
float presMax = 0;
float altMin = 9999;
float altMax = -9999;

// Historial de altitud para gráfico (últimos 64 valores)
#define HIST_SIZE 64
float histAlt[HIST_SIZE];
int histIndex = 0;
bool histLleno = false;

// ========== INICIALIZAR I2C Y PERIFÉRICOS ==========
void initHardware() {
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);

    pinMode(PIN_LED, OUTPUT);

    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);

    display.begin();
    display.setContrast(255);

    bmpOK = bmp.begin();
}

// ========== LEER SENSOR ==========
void leerBMP180() {
    if (!bmpOK) return;

    presion = bmp.readPressure() / 100.0;   // Pa → hPa
    altitud = bmp.readAltitude(101325);      // presión a nivel del mar estándar

    // Min/Max presión
    if (presion < presMin) presMin = presion;
    if (presion > presMax) presMax = presion;

    // Min/Max altitud
    if (altitud < altMin) altMin = altitud;
    if (altitud > altMax) altMax = altitud;

    // Historial altitud
    histAlt[histIndex] = altitud;
    histIndex = (histIndex + 1) % HIST_SIZE;
    if (histIndex == 0) histLleno = true;
}

// ========== IMPRIMIR POR SERIAL ==========
void imprimirSerial() {
    Serial.println("─────────────────────────────");
    Serial.printf("  Presion:     %.2f hPa\n", presion);
    Serial.printf("  Altitud:     %.1f m\n", altitud);
    Serial.printf("  Pres Min:    %.2f hPa\n", presMin);
    Serial.printf("  Pres Max:    %.2f hPa\n", presMax);
    Serial.printf("  Alt  Min:    %.1f m\n", altMin);
    Serial.printf("  Alt  Max:    %.1f m\n", altMax);
}

// ========== PANTALLA: DATOS PRINCIPALES ==========
void pantallaDatos() {
    display.clearBuffer();

    // Encabezado
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "BMP180 Barometro");
    display.drawHLine(0, 13, 128);

    char buf[32];

    // Presión grande
    display.setFont(u8g2_font_ncenB14_tr);
    snprintf(buf, sizeof(buf), "%.1f", presion);
    display.drawStr(0, 34, buf);
    int anchoPresion = display.getStrWidth(buf);  // medir con la fuente grande
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(anchoPresion + 4, 34, "hPa");

    // Altitud grande
    display.setFont(u8g2_font_ncenB14_tr);
    snprintf(buf, sizeof(buf), "%.0f", altitud);
    display.drawStr(0, 56, buf);
    int anchoAltitud = display.getStrWidth(buf);  // medir con la fuente grande
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(anchoAltitud + 4, 56, "m");

    // Min/Max a la derecha
    display.drawVLine(90, 14, 50);
    display.setFont(u8g2_font_5x7_tr);

    snprintf(buf, sizeof(buf), "P%.1f", presMin);
    display.drawStr(93, 26, buf);
    snprintf(buf, sizeof(buf), "P%.1f", presMax);
    display.drawStr(93, 36, buf);

    snprintf(buf, sizeof(buf), "A%.0f", altMin);
    display.drawStr(93, 50, buf);
    snprintf(buf, sizeof(buf), "A%.0f", altMax);
    display.drawStr(93, 60, buf);

    display.sendBuffer();
}

// ========== PANTALLA: GRÁFICO ALTITUD ==========
void pantallaGrafico() {
    display.clearBuffer();

    // Encabezado
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Altitud");
    display.setFont(u8g2_font_5x7_tr);
    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f m", altitud);
    display.drawStr(80, 10, buf);
    display.drawHLine(0, 13, 128);

    int gY = 16;
    int gH = 46;

    int count = histLleno ? HIST_SIZE : histIndex;
    if (count < 2) {
        display.drawStr(20, 40, "Recopilando datos...");
        display.sendBuffer();
        return;
    }

    float minVal = 9999, maxVal = -9999;
    for (int i = 0; i < count; i++) {
        if (histAlt[i] < minVal) minVal = histAlt[i];
        if (histAlt[i] > maxVal) maxVal = histAlt[i];
    }

    // Evitar rango cero
    if (maxVal - minVal < 1.0) {
        minVal -= 0.5;
        maxVal += 0.5;
    }

    // Etiquetas eje Y
    display.setFont(u8g2_font_4x6_tr);
    snprintf(buf, sizeof(buf), "%.0f", maxVal);
    display.drawStr(0, gY + 6, buf);
    snprintf(buf, sizeof(buf), "%.0f", minVal);
    display.drawStr(0, gY + gH, buf);

    // Línea del gráfico
    int gX = 20;
    int gW = 106;

    for (int i = 1; i < count; i++) {
        int idx0, idx1;
        if (histLleno) {
            idx0 = (histIndex + i - 1) % HIST_SIZE;
            idx1 = (histIndex + i) % HIST_SIZE;
        } else {
            idx0 = i - 1;
            idx1 = i;
        }

        int x0 = gX + (i - 1) * gW / (count - 1);
        int x1 = gX + i * gW / (count - 1);
        int y0 = gY + gH - (int)((histAlt[idx0] - minVal) / (maxVal - minVal) * gH);
        int y1 = gY + gH - (int)((histAlt[idx1] - minVal) / (maxVal - minVal) * gH);

        display.drawLine(x0, y0, x1, y1);
    }

    display.drawFrame(gX - 1, gY, gW + 2, gH + 1);
    display.sendBuffer();
}

// ========== PANTALLA: ERROR ==========
void pantallaError() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB10_tr);
    const char* t = "ERROR";
    display.drawStr((128 - display.getStrWidth(t)) / 2, 24, t);

    display.setFont(u8g2_font_6x10_tr);
    const char* m = "BMP180 no detectado";
    display.drawStr((128 - display.getStrWidth(m)) / 2, 42, m);

    display.setFont(u8g2_font_5x7_tr);
    const char* h = "Verificar I2C SDA=4 SCL=15";
    display.drawStr((128 - display.getStrWidth(h)) / 2, 56, h);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== BMP180 Barometro — Heltec LoRa32 V2 ===");
    Serial.println("Midiendo: Presion atmosferica + Altitud");
    Serial.printf("I2C: SDA=%d, SCL=%d\n\n", PIN_OLED_SDA, PIN_OLED_SCL);

    initHardware();

    if (bmpOK) {
        Serial.println("BMP180 detectado OK!");
        digitalWrite(PIN_LED, HIGH);
    } else {
        Serial.println("ERROR: BMP180 no encontrado!");
        Serial.println("Verificar cableado: SDA->GPIO4, SCL->GPIO15, VCC->3.3V, GND->GND");
        pantallaError();
    }
}

// ========== LOOP ==========
int pantalla = 0;
unsigned long ultimaLectura = 0;
unsigned long ultimoCambioPantalla = 0;

void loop() {
    if (!bmpOK) {
        if (millis() - ultimaLectura > 5000) {
            ultimaLectura = millis();
            bmpOK = bmp.begin();
            if (bmpOK) {
                Serial.println("BMP180 reconectado!");
                digitalWrite(PIN_LED, HIGH);
            } else {
                Serial.println("BMP180 sigue sin responder...");
                digitalWrite(PIN_LED, !digitalRead(PIN_LED));
                pantallaError();
            }
        }
        return;
    }

    // Leer sensor cada 2 segundos
    if (millis() - ultimaLectura > 2000) {
        ultimaLectura = millis();
        leerBMP180();
        imprimirSerial();
    }

    // Alternar pantallas cada 6 segundos
    if (millis() - ultimoCambioPantalla > 6000) {
        ultimoCambioPantalla = millis();
        pantalla = (pantalla + 1) % 2;
    }

    // Refrescar pantalla cada 500ms
    static unsigned long ultimoRefresh = 0;
    if (millis() - ultimoRefresh > 500) {
        ultimoRefresh = millis();
        switch (pantalla) {
            case 0: pantallaDatos(); break;
            case 1: pantallaGrafico(); break;
        }
    }
}
