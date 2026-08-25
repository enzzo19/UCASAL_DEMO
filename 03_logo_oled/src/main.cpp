#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include "logo_bitmap.h"

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25

// ========== DISPLAY ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

// ========== ANIMACIÓN: Logo completo con efecto cortina ==========
void animarLogoCompleto() {
    // Efecto: revelar línea por línea de arriba a abajo
    for (int linea = 0; linea <= LOGO_FULL_HEIGHT; linea += 2) {
        display.clearBuffer();
        // Dibujar solo las primeras 'linea' filas del bitmap
        // Usamos un clip para simular el efecto
        display.drawXBMP(0, 0, LOGO_FULL_WIDTH, LOGO_FULL_HEIGHT, logo_full);
        // Tapar lo que aún no se revela
        display.setDrawColor(0);
        display.drawBox(0, linea, 128, 64 - linea);
        display.setDrawColor(1);
        display.sendBuffer();
        delay(30);
    }
}

// ========== PANTALLA 1: Splash con logo completo ==========
void pantallaSplash() {
    display.clearBuffer();
    display.drawXBMP(0, 0, LOGO_FULL_WIDTH, LOGO_FULL_HEIGHT, logo_full);
    display.sendBuffer();
}

// ========== PANTALLA 2: Icono + info taller ==========
void pantallaTaller() {
    display.clearBuffer();

    // Icono del IITA a la izquierda (escalado a 48x48 para dejar espacio)
    // Dibujamos el icono 64x64 desplazado para que se vea la parte principal
    display.drawXBMP(-4, 0, LOGO_ICON_WIDTH, LOGO_ICON_HEIGHT, logo_icon);

    // Línea separadora vertical
    display.drawVLine(62, 0, 64);

    // Texto a la derecha
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(67, 12, "Taller");
    display.drawStr(75, 26, "IoT");

    display.setFont(u8g2_font_5x7_tr);
    display.drawStr(67, 40, "UCASAL");
    display.drawStr(67, 50, "2026");

    // Icono WiFi simulado (3 arcos)
    display.drawCircle(110, 60, 3, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    display.drawCircle(110, 60, 7, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    display.drawCircle(110, 60, 11, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
    display.drawDisc(110, 60, 1);

    display.sendBuffer();
}

// ========== PANTALLA 3: Texto IITA grande + subtítulo ==========
void pantallaIITA() {
    display.clearBuffer();

    // Texto IITA como bitmap
    display.drawXBMP(0, 2, LOGO_IITA_TEXT_WIDTH, LOGO_IITA_TEXT_HEIGHT, logo_iita_text);

    // Línea separadora
    display.drawHLine(0, 45, 128);

    // Subtítulo
    display.setFont(u8g2_font_5x7_tr);
    const char* l1 = "Instituto de Innovacion";
    int w1 = display.getStrWidth(l1);
    display.drawStr((128 - w1) / 2, 56, l1);

    const char* l2 = "y Tecnologia Aplicada";
    int w2 = display.getStrWidth(l2);
    display.drawStr((128 - w2) / 2, 64, l2);

    display.sendBuffer();
}

// ========== PANTALLA 4: Presentación del taller ==========
void pantallaPresentation() {
    display.clearBuffer();

    // Marco exterior
    display.drawRFrame(0, 0, 128, 64, 4);

    // Título
    display.setFont(u8g2_font_ncenB08_tr);
    const char* t = "TALLER IoT";
    int tw = display.getStrWidth(t);
    display.drawStr((128 - tw) / 2, 14, t);

    // Línea doble
    display.drawHLine(10, 18, 108);
    display.drawHLine(10, 20, 108);

    // Contenido
    display.setFont(u8g2_font_6x10_tr);
    display.drawStr(10, 34, "ESP32 + LoRa");
    display.drawStr(10, 46, "OLED + WiFi + BLE");

    // Footer
    display.setFont(u8g2_font_5x7_tr);
    display.drawHLine(10, 52, 108);
    const char* footer = "Heltec WiFi LoRa 32 V2";
    int fw = display.getStrWidth(footer);
    display.drawStr((128 - fw) / 2, 62, footer);

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== IITA Logo OLED - Heltec LoRa32 V2 ===");

    // Encender Vext
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);

    // LED
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    // Inicializar display
    display.begin();
    display.setContrast(255);

    Serial.println("OLED OK - Animando logo...");

    // Animación inicial
    animarLogoCompleto();
    delay(3000);
}

// ========== LOOP ==========
int pantalla = 0;
unsigned long ultimoCambio = 0;

void loop() {
    unsigned long ahora = millis();

    if (ahora - ultimoCambio > 4000) {
        ultimoCambio = ahora;
        pantalla = (pantalla + 1) % 4;

        switch (pantalla) {
            case 0:
                Serial.println(">> Logo completo");
                pantallaSplash();
                break;
            case 1:
                Serial.println(">> Icono + Taller");
                pantallaTaller();
                break;
            case 2:
                Serial.println(">> IITA texto");
                pantallaIITA();
                break;
            case 3:
                Serial.println(">> Presentacion taller");
                pantallaPresentation();
                break;
        }
    }
}
