#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>

// ========== PINES HELTEC V2 ==========
#define PIN_OLED_SDA  4
#define PIN_OLED_SCL  15
#define PIN_OLED_RST  16
#define PIN_VEXT      21
#define PIN_LED       25
#define PIN_BATTERY   37

// ========== DISPLAY ==========
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(
    U8G2_R0, PIN_OLED_RST, PIN_OLED_SCL, PIN_OLED_SDA
);

// ========== FUNCIONES ==========
float leerBateria() {
    analogSetAttenuation(ADC_11db);
    uint32_t suma = 0;
    for (int i = 0; i < 16; i++) {
        suma += analogRead(PIN_BATTERY);
    }
    return (suma / 16.0 / 4095.0) * 3.3 * 2.0;
}

int porcentajeBateria(float voltaje) {
    if (voltaje >= 4.2) return 100;
    if (voltaje <= 3.0) return 0;
    return (int)((voltaje - 3.0) / (4.2 - 3.0) * 100);
}

// Pantalla 1: Splash de bienvenida
void pantallaInicio() {
    display.clearBuffer();

    // Marco redondeado
    display.drawRFrame(0, 0, 128, 64, 6);

    // Título grande
    display.setFont(u8g2_font_ncenB10_tr);
    const char* titulo = "UCASAL";
    int w = display.getStrWidth(titulo);
    display.drawStr((128 - w) / 2, 24, titulo);

    // Subtítulo
    display.setFont(u8g2_font_6x10_tr);
    const char* sub = "Heltec LoRa32 V2";
    w = display.getStrWidth(sub);
    display.drawStr((128 - w) / 2, 40, sub);

    // Línea decorativa
    display.drawHLine(20, 44, 88);

    const char* sub2 = "OLED Test OK!";
    w = display.getStrWidth(sub2);
    display.drawStr((128 - w) / 2, 58, sub2);

    display.sendBuffer();
}

// Pantalla 2: Info del sistema
void pantallaInfo() {
    float bat = leerBateria();
    int pct = porcentajeBateria(bat);
    unsigned long uptime = millis() / 1000;

    display.clearBuffer();

    // Encabezado
    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Sistema Info");
    display.drawHLine(0, 13, 128);

    // Datos
    display.setFont(u8g2_font_6x10_tr);

    char buf[32];
    snprintf(buf, sizeof(buf), "CPU: ESP32 240MHz");
    display.drawStr(0, 26, buf);

    snprintf(buf, sizeof(buf), "Bat: %.2fV (%d%%)", bat, pct);
    display.drawStr(0, 38, buf);

    // Barra de batería visual
    display.drawFrame(80, 42, 44, 8);
    display.drawBox(80, 42, 44 * pct / 100, 8);
    display.drawBox(124, 44, 3, 4); // pestaña de pila

    snprintf(buf, sizeof(buf), "Up: %lus", uptime);
    display.drawStr(0, 62, buf);

    snprintf(buf, sizeof(buf), "Free: %dKB", ESP.getFreeHeap() / 1024);
    display.drawStr(64, 62, buf);

    display.sendBuffer();
}

// Pantalla 3: Demo de gráficos
void pantallaGraficos() {
    display.clearBuffer();

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Graficos Demo");
    display.drawHLine(0, 13, 128);

    // Círculos concéntricos
    for (int r = 5; r <= 20; r += 5) {
        display.drawCircle(30, 40, r);
    }

    // Rectángulos anidados
    for (int i = 0; i < 4; i++) {
        display.drawFrame(65 + i * 3, 18 + i * 3, 50 - i * 6, 40 - i * 6);
    }

    // Triángulo
    display.drawTriangle(100, 20, 120, 55, 80, 55);

    display.sendBuffer();
}

// Pantalla 4: Animación de barras
void pantallaBarras(int frame) {
    display.clearBuffer();

    display.setFont(u8g2_font_ncenB08_tr);
    display.drawStr(0, 10, "Sensor Sim");
    display.drawHLine(0, 13, 128);

    display.setFont(u8g2_font_5x7_tr);

    const char* labels[] = {"T", "H", "P", "L"};
    for (int i = 0; i < 4; i++) {
        int x = 5 + i * 32;
        // Valor animado con seno
        int valor = 20 + (int)(18.0 * sin((frame + i * 15) * 0.1));

        // Label
        display.drawStr(x + 10, 26, labels[i]);

        // Barra vertical (de abajo hacia arriba)
        display.drawFrame(x + 8, 28, 12, 34);
        display.drawBox(x + 9, 28 + (33 - valor), 10, valor);

        // Valor numérico
        char buf[8];
        snprintf(buf, sizeof(buf), "%d", valor + 10);
        display.drawStr(x + 8, 26, labels[i]);
    }

    display.sendBuffer();
}

// ========== SETUP ==========
void setup() {
    Serial.begin(115200);
    Serial.println("\n=== Test OLED - Heltec LoRa32 V2 ===");

    // Encender Vext (alimenta la OLED)
    pinMode(PIN_VEXT, OUTPUT);
    digitalWrite(PIN_VEXT, LOW);

    // LED indicador
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, HIGH);

    // Inicializar display
    display.begin();
    display.setContrast(255);

    Serial.println("OLED inicializada OK");
    Serial.println("Mostrando pantalla de inicio...");

    pantallaInicio();
    delay(3000);
}

// ========== LOOP ==========
int pantalla = 0;
int frame = 0;
unsigned long ultimoCambio = 0;

void loop() {
    unsigned long ahora = millis();

    if (pantalla == 3) {
        // Pantalla de barras: actualizar rápido para animar
        pantallaBarras(frame++);
        delay(80);

        if (ahora - ultimoCambio > 5000) {
            pantalla = 0;
            ultimoCambio = ahora;
            Serial.println(">> Pantalla: Inicio");
        }
    } else {
        // Otras pantallas: cambiar cada 3 segundos
        if (ahora - ultimoCambio > 3000) {
            ultimoCambio = ahora;
            pantalla++;

            switch (pantalla) {
                case 1:
                    Serial.println(">> Pantalla: Info del Sistema");
                    pantallaInfo();
                    break;
                case 2:
                    Serial.println(">> Pantalla: Graficos");
                    pantallaGraficos();
                    break;
                case 3:
                    Serial.println(">> Pantalla: Barras Animadas");
                    frame = 0;
                    break;
            }
        }
    }
}
