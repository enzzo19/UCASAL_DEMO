---
name: heltec-display
description: "Pantalla OLED SSD1306 de la Heltec LoRa32 V2: inicialización, texto, gráficos, fuentes y patrones de UI."
triggers:
  - "oled"
  - "pantalla"
  - "display"
  - "ssd1306"
  - "mostrar"
  - "dibujar"
  - "screen"
  - "fuente"
  - "font"
---

# Pantalla OLED SSD1306 — Heltec LoRa32 V2

## Hardware

| Parámetro | Valor |
|---|---|
| Controlador | SSD1306 |
| Resolución | 128 × 64 píxeles |
| Interfaz | I2C |
| Dirección I2C | 0x3C |
| SDA | GPIO 4 |
| SCL | GPIO 15 |
| RST | GPIO 16 |
| Alimentación | Controlada por Vext (GPIO 21) |

## IMPORTANTE: Secuencia de Inicialización

La OLED de la Heltec **requiere** dos pasos previos:
1. Activar Vext (GPIO 21 → LOW) para alimentar la pantalla.
2. Hacer reset del controlador (GPIO 16 → LOW, esperar, → HIGH).

## Librería Recomendada: U8g2 (o U8x8 para solo texto)

### platformio.ini
```ini
[env:heltec_wifi_lora_32_V2]
platform = espressif32
board = heltec_wifi_lora_32_V2
framework = arduino
monitor_speed = 115200
lib_deps =
    olikraus/U8g2@^2.35.0
```

### Inicialización con U8g2
```cpp
#include <U8g2lib.h>
#include <Wire.h>

// Constructor para Heltec LoRa32 V2
// SSD1306, 128x64, I2C con pines personalizados y reset
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
    U8G2_R0,       // Rotación (R0=normal, R2=180°)
    /* reset= */ 16,
    /* clock= */ 15,
    /* data= */  4
);

void setup() {
    // 1. Activar alimentación externa (Vext)
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);  // LOW = encendido

    // 2. Inicializar display
    u8g2.begin();
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 12, "Heltec LoRa32 V2");
    u8g2.drawStr(0, 28, "Listo!");
    u8g2.sendBuffer();
}
```

### Texto Básico
```cpp
void mostrarTexto() {
    u8g2.clearBuffer();

    // Fuentes comunes (número = tamaño en px)
    u8g2.setFont(u8g2_font_ncenB08_tr);  // 8px negrita
    u8g2.drawStr(0, 12, "Titulo");

    u8g2.setFont(u8g2_font_6x10_tr);     // 6x10 monoespaciada
    u8g2.drawStr(0, 28, "Linea 2");

    // Texto con formato (printf-style)
    char buf[32];
    float temp = 23.5;
    snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
    u8g2.drawStr(0, 44, buf);

    // Texto centrado
    const char* texto = "Centrado";
    int ancho = u8g2.getStrWidth(texto);
    u8g2.drawStr((128 - ancho) / 2, 60, texto);

    u8g2.sendBuffer();
}
```

### Gráficos Primitivos
```cpp
void dibujarGraficos() {
    u8g2.clearBuffer();

    // Línea
    u8g2.drawLine(0, 0, 127, 63);

    // Rectángulo (contorno)
    u8g2.drawFrame(10, 10, 50, 30);

    // Rectángulo (relleno)
    u8g2.drawBox(70, 10, 50, 30);

    // Círculo (contorno)
    u8g2.drawCircle(64, 32, 20);

    // Círculo (relleno)
    u8g2.drawDisc(64, 32, 15);

    // Píxel individual
    u8g2.drawPixel(100, 50);

    // Triángulo
    u8g2.drawTriangle(20, 60, 40, 40, 60, 60);

    // Rectángulo redondeado
    u8g2.drawRFrame(5, 5, 118, 54, 8);  // radio = 8

    u8g2.sendBuffer();
}
```

### Barra de Progreso
```cpp
void barraProgreso(int porcentaje) {
    // porcentaje: 0-100
    int ancho = 100;
    int alto = 12;
    int x = (128 - ancho) / 2;
    int y = 40;

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_ncenB08_tr);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", porcentaje);
    int textW = u8g2.getStrWidth(buf);
    u8g2.drawStr((128 - textW) / 2, y - 4, buf);

    // Marco
    u8g2.drawFrame(x, y, ancho, alto);
    // Relleno proporcional
    int relleno = (ancho - 2) * porcentaje / 100;
    if (relleno > 0) {
        u8g2.drawBox(x + 1, y + 1, relleno, alto - 2);
    }

    u8g2.sendBuffer();
}
```

### Pantalla de Dashboard (ejemplo práctico)
```cpp
void dashboard(float temp, float hum, int rssi, const char* estado) {
    u8g2.clearBuffer();

    // Título con línea separadora
    u8g2.setFont(u8g2_font_ncenB08_tr);
    u8g2.drawStr(0, 10, "LoRa Monitor");
    u8g2.drawHLine(0, 13, 128);

    // Datos en formato tabla
    u8g2.setFont(u8g2_font_6x10_tr);

    char buf[32];
    snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
    u8g2.drawStr(0, 26, buf);

    snprintf(buf, sizeof(buf), "Hum:  %.1f %%", hum);
    u8g2.drawStr(0, 38, buf);

    snprintf(buf, sizeof(buf), "RSSI: %d dBm", rssi);
    u8g2.drawStr(0, 50, buf);

    // Estado en la parte inferior
    u8g2.drawHLine(0, 53, 128);
    u8g2.drawStr(0, 63, estado);

    u8g2.sendBuffer();
}
```

### Bitmap / Icono Personalizado

```cpp
// Ejemplo: icono WiFi 16x16
static const uint8_t icon_wifi[] U8X8_PROGMEM = {
    0x00, 0x00, 0xE0, 0x07, 0xF8, 0x1F, 0x1C, 0x38,
    0xC6, 0x63, 0xF2, 0x4F, 0x38, 0x1C, 0x88, 0x11,
    0xC0, 0x03, 0xE0, 0x07, 0x00, 0x00, 0x80, 0x01,
    0xC0, 0x03, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00
};

void dibujarIcono() {
    u8g2.drawXBMP(56, 0, 16, 16, icon_wifi);
}
```

### Convertir Imagen/Logo a Bitmap para OLED

Para mostrar un logo o imagen en la OLED, hay que convertirla a un array C monocromático.
Se usa un script Python con Pillow. El proyecto `03_logo_oled/` tiene el ejemplo completo.

**Procedimiento probado:**

1. **Requisito**: `pip install Pillow`

2. **Script de conversión** (adaptar rutas y nombre):
```python
from PIL import Image, ImageEnhance, ImageFilter, ImageOps

def image_to_xbm_array(img, name):
    """Convierte imagen PIL monocromática a array C estilo XBM para U8g2."""
    w, h = img.size
    pixels = list(img.getdata())
    bytes_list = []
    for y in range(h):
        for x_byte in range(0, w, 8):
            byte_val = 0
            for bit in range(8):
                x = x_byte + bit
                if x < w:
                    if pixels[y * w + x] > 0:
                        byte_val |= (1 << bit)
            bytes_list.append(byte_val)

    lines = [f"#define {name.upper()}_WIDTH  {w}"]
    lines.append(f"#define {name.upper()}_HEIGHT {h}")
    lines.append(f"static const uint8_t {name}[] PROGMEM = {{")
    for i in range(0, len(bytes_list), 16):
        chunk = bytes_list[i:i+16]
        lines.append("    " + ", ".join(f"0x{b:02x}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)

# Cargar, escalar a 128x64 manteniendo aspect ratio, centrar
img = Image.open("logo.webp")
ratio = min(128 / img.width, 64 / img.height)
resized = img.resize((int(img.width * ratio), int(img.height * ratio)), Image.LANCZOS)
canvas = Image.new("L", (128, 64), 0)
canvas.paste(resized, ((128 - resized.width) // 2, (64 - resized.height) // 2))
canvas = ImageEnhance.Contrast(canvas).enhance(2.5)
canvas = canvas.point(lambda x: 255 if x > 100 else 0).convert("1")

with open("logo_bitmap.h", "w") as f:
    f.write("#pragma once\n" + image_to_xbm_array(canvas, "logo_full"))
```

3. **En el código Arduino**:
```cpp
#include "logo_bitmap.h"

void mostrarLogo() {
    display.clearBuffer();
    display.drawXBMP(0, 0, LOGO_FULL_WIDTH, LOGO_FULL_HEIGHT, logo_full);
    display.sendBuffer();
}
```

4. **Animación cortina (revelar de arriba a abajo)**:
```cpp
void animarLogo() {
    for (int linea = 0; linea <= 64; linea += 2) {
        display.clearBuffer();
        display.drawXBMP(0, 0, LOGO_FULL_WIDTH, LOGO_FULL_HEIGHT, logo_full);
        display.setDrawColor(0);
        display.drawBox(0, linea, 128, 64 - linea);  // tapar lo no revelado
        display.setDrawColor(1);
        display.sendBuffer();
        delay(30);
    }
}
```

**Lecciones aprendidas con logos a color → monocromático:**

- **Colores claros sobre fondo claro** (ej: blanco sobre naranja) desaparecen al convertir a B/N porque tienen luminancia similar. Solución: procesar esas zonas por separado con `ImageOps.invert()` y `ImageFilter.EDGE_ENHANCE_MORE`.
- **Texto oscuro sobre fondo blanco** necesita invertirse para OLED (que es blanco sobre negro): `point(lambda x: 0 if x > 100 else 255)`.
- **Siempre generar una preview PNG** antes de flashear para verificar que se ve bien.
- **Procesar por cuadrantes/regiones** si la imagen tiene zonas con distintos contrastes — un umbral global no sirve para todo.
- **El umbral de binarización** (el `> 100` en `point()`) varía según la imagen. Probar entre 80-130.
- **Usar `PROGMEM`** para los arrays — los bitmaps van en Flash, no en RAM.

## Alternativa Ligera: U8x8 (solo texto, sin buffer)

Para cuando la RAM es crítica y solo se necesita texto:

```cpp
#include <U8x8lib.h>

U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(/* reset= */ 16, /* clock= */ 15, /* data= */ 4);

void setup() {
    pinMode(21, OUTPUT);
    digitalWrite(21, LOW);

    u8x8.begin();
    u8x8.setFont(u8x8_font_chroma48medium8_r);

    // Coordenadas en tiles (8x8 px), no píxeles
    // Pantalla = 16 columnas × 8 filas de tiles
    u8x8.drawString(0, 0, "Linea 0");
    u8x8.drawString(0, 1, "Linea 1");
    u8x8.drawString(0, 2, "Linea 2");
}
```

## Alternativa: Librería Heltec Oficial

```cpp
#include "heltec.h"

void setup() {
    Heltec.begin(
        true,   // Display ON
        true,   // LoRa ON
        true,   // Serial ON
        true,   // PABOOST ON
        915E6   // Frecuencia LoRa
    );

    Heltec.display->clear();
    Heltec.display->setFont(ArialMT_Plain_10);
    Heltec.display->drawString(0, 0, "Hola Mundo!");
    Heltec.display->display();
}
```

## Fuentes U8g2 Recomendadas para 128x64

| Fuente | Tamaño | Uso |
|---|---|---|
| `u8g2_font_ncenB08_tr` | 8px negrita | Títulos pequeños |
| `u8g2_font_ncenB10_tr` | 10px negrita | Títulos |
| `u8g2_font_ncenB14_tr` | 14px negrita | Valores grandes |
| `u8g2_font_6x10_tr` | 6×10 mono | Datos, logs |
| `u8g2_font_5x7_tr` | 5×7 mono | Texto pequeño, muchas líneas |
| `u8g2_font_4x6_tr` | 4×6 mono | Texto muy pequeño |
| `u8g2_font_open_iconic_weather_2x_t` | 16px | Iconos clima |

> `_tr` = transparente, solo dibuja píxeles encendidos  
> `_tf` = full, dibuja fondo también  
> Las fuentes grandes consumen mucha Flash — usar solo las necesarias.

## Instrucciones para el Asistente

Al generar código para la OLED:
- SIEMPRE incluir la activación de Vext (`pinMode(21, OUTPUT); digitalWrite(21, LOW);`).
- SIEMPRE usar los pines correctos: RST=16, SCL=15, SDA=4.
- Para U8g2, siempre usar `clearBuffer()` → dibujar → `sendBuffer()`.
- Recordar que la pantalla es 128×64 px — diseñar layouts acordes.
- Ofrecer U8x8 como alternativa si el usuario tiene problemas de RAM.
- Al dibujar texto, recordar que `drawStr(x, y)` usa Y como baseline (parte inferior del texto), no la esquina superior.
- Para caracteres especiales (á, é, ñ), usar fuentes con sufijo `_te` (extended) o `_t` (unicode).
