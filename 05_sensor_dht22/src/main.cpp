#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <DHT.h>

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25

// ========== DHT22 ==========
#define PIN_DHT       13
#define DHT_TYPE      DHT22

// ========== OBJETOS ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

DHT dht(PIN_DHT, DHT_TYPE);

// ========== DATOS SENSOR ==========
float temperatura = 0;
float humedad = 0;
float sensTermica = 0;   // Heat index
bool lecturaOK = false;

float tempMin = 999, tempMax = -999;
float humMin = 999, humMax = 0;

// Historial para gráficos (últimos 64 valores)
#define HIST_SIZE 64
float histTemp[HIST_SIZE];
float histHum[HIST_SIZE];
int histIndex = 0;
bool histLleno = false;

// ========== LEER SENSOR ==========
void leerDHT22() {
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
        lecturaOK = false;
        return;
    }

    lecturaOK = true;
    temperatura = t;
    humedad = h;
    sensTermica = dht.computeHeatIndex(t, h, false);  // false = Celsius

    // Min/Max
    if (temperatura < tempMin) tempMin = temperatura;
    if (temperatura > tempMax) tempMax = temperatura;
    if (humedad < humMin) humMin = humedad;
    if (humedad > humMax) humMax = humedad;

    // Historial
    histTemp[histIndex] = temperatura;
    histHum[histIndex] = humedad;
    histIndex = (histIndex + 1) % HIST_SIZE;
    if (histIndex == 0) histLleno = true;
}

// ========== IMPRIMIR POR SERIAL ==========
void imprimirSerial() {
    if (!lecturaOK) {
        Serial.println("DHT22: Error de lectura");
        return;
    }
    Serial.println("─────────────────────────────");
    Serial.printf("  Temperatura:    %.1f °C\n", temperatura);
    Serial.printf("  Humedad:        %.1f %%\n", humedad);
    Serial.printf("  Sens. Termica:  %.1f °C\n", sensTermica);
    Serial.printf("  Temp Min/Max:   %.1f / %.1f °C\n", tempMin, tempMax);
    Serial.printf("  Hum  Min/Max:   %.1f / %.1f %%\n", humMin, humMax);
}

// ========== PANTALLA: DATOS PRINCIPALES ==========
void pantallaDatos() {
    display.clearBuffer();

    // Encabezado
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "DHT22 Sensor");
    display.drawHLine(0, 13, 128);

    char buf[32];
    int ancho;

    // Temperatura grande
    display.setFont(u8g2_font_ncenB14_tr);
    snprintf(buf, sizeof(buf), "%.1f", temperatura);
    display.drawStr(0, 34, buf);
    ancho = display.getStrWidth(buf);
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(ancho + 2, 24, "o");
    display.drawStr(ancho + 8, 34, "C");

    // Humedad grande
    display.setFont(u8g2_font_ncenB14_tr);
    snprintf(buf, sizeof(buf), "%.1f", humedad);
    display.drawStr(0, 56, buf);
    ancho = display.getStrWidth(buf);
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(ancho + 4, 56, "%");

    // Min/Max a la derecha
    display.drawVLine(84, 14, 50);
    display.setFont(u8g2_font_5x7_tr);

    display.drawStr(87, 24, "T");
    snprintf(buf, sizeof(buf), "%.1f/%.1f", tempMin, tempMax);
    display.drawStr(87, 34, buf);

    display.drawStr(87, 46, "H");
    snprintf(buf, sizeof(buf), "%.0f/%.0f", humMin, humMax);
    display.drawStr(87, 56, buf);

    // Sensación térmica
    display.setFont(u8g2_font_5x7_tr);
    snprintf(buf, sizeof(buf), "ST:%.1fC", sensTermica);
    display.drawStr(87, 64, buf);

    display.sendBuffer();
}

// ========== PANTALLA: GRÁFICO DUAL ==========
void pantallaGrafico() {
    display.clearBuffer();

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Historial");
    display.setFont(u8g2_font_5x7_tr);
    char buf[24];
    snprintf(buf, sizeof(buf), "%.1fC  %.0f%%", temperatura, humedad);
    display.drawStr(60, 10, buf);
    display.drawHLine(0, 13, 128);

    int count = histLleno ? HIST_SIZE : histIndex;
    if (count < 2) {
        display.drawStr(20, 40, "Recopilando datos...");
        display.sendBuffer();
        return;
    }

    // Gráfico de temperatura (mitad superior: y=16..38)
    int gY1 = 16, gH1 = 22;
    // Gráfico de humedad (mitad inferior: y=42..63)
    int gY2 = 42, gH2 = 21;
    int gX = 16, gW = 110;

    // --- Temperatura ---
    float tMin = 999, tMax = -999;
    for (int i = 0; i < count; i++) {
        if (histTemp[i] < tMin) tMin = histTemp[i];
        if (histTemp[i] > tMax) tMax = histTemp[i];
    }
    if (tMax - tMin < 1.0) { tMin -= 0.5; tMax += 0.5; }

    display.setFont(u8g2_font_4x6_tr);
    snprintf(buf, sizeof(buf), "%.0f", tMax);
    display.drawStr(0, gY1 + 5, buf);
    snprintf(buf, sizeof(buf), "%.0f", tMin);
    display.drawStr(0, gY1 + gH1, buf);

    for (int i = 1; i < count; i++) {
        int i0 = histLleno ? (histIndex + i - 1) % HIST_SIZE : i - 1;
        int i1 = histLleno ? (histIndex + i) % HIST_SIZE : i;
        int x0 = gX + (i - 1) * gW / (count - 1);
        int x1 = gX + i * gW / (count - 1);
        int y0 = gY1 + gH1 - (int)((histTemp[i0] - tMin) / (tMax - tMin) * gH1);
        int y1 = gY1 + gH1 - (int)((histTemp[i1] - tMin) / (tMax - tMin) * gH1);
        display.drawLine(x0, y0, x1, y1);
    }
    display.drawFrame(gX - 1, gY1, gW + 2, gH1 + 1);

    // Etiqueta
    display.setFont(u8g2_font_4x6_tr);
    display.drawStr(gX, gY2 - 2, "Temp");

    // --- Humedad ---
    float hMin = 999, hMax = -999;
    for (int i = 0; i < count; i++) {
        if (histHum[i] < hMin) hMin = histHum[i];
        if (histHum[i] > hMax) hMax = histHum[i];
    }
    if (hMax - hMin < 1.0) { hMin -= 0.5; hMax += 0.5; }

    snprintf(buf, sizeof(buf), "%.0f", hMax);
    display.drawStr(0, gY2 + 5, buf);
    snprintf(buf, sizeof(buf), "%.0f", hMin);
    display.drawStr(0, gY2 + gH2, buf);

    for (int i = 1; i < count; i++) {
        int i0 = histLleno ? (histIndex + i - 1) % HIST_SIZE : i - 1;
        int i1 = histLleno ? (histIndex + i) % HIST_SIZE : i;
        int x0 = gX + (i - 1) * gW / (count - 1);
        int x1 = gX + i * gW / (count - 1);
        int y0 = gY2 + gH2 - (int)((histHum[i0] - hMin) / (hMax - hMin) * gH2);
        int y1 = gY2 + gH2 - (int)((histHum[i1] - hMin) / (hMax - hMin) * gH2);
        display.drawLine(x0, y0, x1, y1);
    }
    display.drawFrame(gX - 1, gY2, gW + 2, gH2 + 1);

    display.setFont(u8g2_font_4x6_tr);
    display.drawStr(gX + gW - 12, gY2 - 2, "Hum");

    display.sendBuffer();
}

// ========== PANTALLA: ERROR ==========
void pantallaError() {
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB10_tr);
    const char* t = "ERROR";
    display.drawStr((128 - display.getStrWidth(t)) / 2, 24, t);

    display.setFont(u8g2_font_6x10_tr);
    const char* m = "DHT22 sin respuesta";
    display.drawStr((128 - display.getStrWidth(m)) / 2, 42, m);

    display.setFont(u8g2_font_5x7_tr);
    char buf[32];
    snprintf(buf, sizeof(buf), "Verificar GPIO %d", PIN_DHT);
    display.drawStr((128 - display.getStrWidth(buf)) / 2, 56, buf);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== DHT22 + OLED — Heltec LoRa32 V2 ===");
    Serial.printf("Pin DHT22: GPIO %d\n\n", PIN_DHT);

    // Vext ON
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);

    // LED
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    // Display
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    display.begin();
    display.setContrast(255);

    // DHT22
    dht.begin();
    Serial.println("DHT22 inicializado, esperando primera lectura...");

    // Splash
    display.clearBuffer();
    display.setFont(u8g2_font_ncenB10_tr);
    const char* titulo = "DHT22";
    display.drawStr((128 - display.getStrWidth(titulo)) / 2, 28, titulo);
    display.setFont(u8g2_font_6x10_tr);
    const char* sub = "Temp + Humedad";
    display.drawStr((128 - display.getStrWidth(sub)) / 2, 46, sub);
    display.sendBuffer();
    delay(2000);
}

// ========== LOOP ==========
int pantalla = 0;
unsigned long ultimaLectura = 0;
unsigned long ultimoCambioPantalla = 0;
int erroresConsecutivos = 0;

void loop() {
    // Leer sensor cada 2.5 segundos (DHT22 necesita mínimo 2s entre lecturas)
    if (millis() - ultimaLectura > 2500) {
        ultimaLectura = millis();
        leerDHT22();

        if (lecturaOK) {
            erroresConsecutivos = 0;
            imprimirSerial();
        } else {
            erroresConsecutivos++;
            Serial.printf("DHT22: Error lectura (%d consecutivos)\n", erroresConsecutivos);
            if (erroresConsecutivos > 5) {
                pantallaError();
            }
        }
    }

    // Alternar pantallas cada 6 segundos
    if (millis() - ultimoCambioPantalla > 6000) {
        ultimoCambioPantalla = millis();
        pantalla = (pantalla + 1) % 2;
    }

    // Refrescar pantalla cada 500ms
    static unsigned long ultimoRefresh = 0;
    if (millis() - ultimoRefresh > 500 && lecturaOK) {
        ultimoRefresh = millis();
        switch (pantalla) {
            case 0: pantallaDatos(); break;
            case 1: pantallaGrafico(); break;
        }
    }
}
