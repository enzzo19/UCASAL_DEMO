---
name: security-iot
description: "Seguridad IoT para ESP32/Heltec: cifrado TLS, firmware seguro, comunicación protegida, hardening del dispositivo, LoRa seguro."
triggers:
  - "seguridad iot"
  - "iot security"
  - "cifrado"
  - "tls"
  - "ssl"
  - "https"
  - "firmware seguro"
  - "secure boot"
  - "flash encrypt"
  - "hardening"
  - "ataque"
  - "vulnerabilidad"
---

# Seguridad IoT — ESP32 / Heltec LoRa32 V2

## Superficie de Ataque de un Dispositivo IoT

```
┌─────────────────────────────────────────────────────┐
│                 VECTORES DE ATAQUE                  │
├──────────────┬──────────────┬───────────────────────┤
│   FÍSICO     │    RED       │    FIRMWARE           │
│              │              │                       │
│ • Puerto USB │ • WiFi sniff │ • Flash sin cifrar    │
│ • JTAG/SWD   │ • MQTT sin   │ • OTA sin firma       │
│ • Leer flash │   auth       │ • Buffer overflow     │
│ • Dump SPI   │ • HTTP plano │ • Claves hardcoded    │
│ • Tamper HW  │ • LoRa replay│ • Serial debug activo │
│              │ • BLE sin    │ • Partición insegura  │
│              │   pairing    │                       │
└──────────────┴──────────────┴───────────────────────┘
```

## 1. Comunicación Segura — TLS/HTTPS

### Cliente HTTPS (conexión cifrada a APIs)
```cpp
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

// Certificado raíz del servidor (obtener de la CA)
const char* rootCA = R"(
-----BEGIN CERTIFICATE-----
MIIDdzCCAl+gAwIBAgIEAgAA...
-----END CERTIFICATE-----
)";  // ← Este string va en secrets.h si es privado

WiFiClientSecure secureClient;

void enviarDatosSeguro(float temp, float hum) {
    secureClient.setCACert(rootCA);
    // O para desarrollo (⚠️ NO en producción):
    // secureClient.setInsecure();

    HTTPClient https;
    https.begin(secureClient, "https://api.ejemplo.com/datos");
    https.addHeader("Content-Type", "application/json");
    https.addHeader("Authorization", "Bearer " + String(SECRET_API_KEY));

    String payload = "{\"temp\":" + String(temp) + ",\"hum\":" + String(hum) + "}";
    int httpCode = https.POST(payload);

    if (httpCode == 200) {
        Serial.println("Datos enviados (HTTPS)");
    } else {
        Serial.printf("Error HTTPS: %d\n", httpCode);
    }
    https.end();
}
```

### MQTT Seguro (TLS + autenticación)
```cpp
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

void conectarMQTTSeguro() {
    secureClient.setCACert(rootCA);          // CA del broker
    // Si el broker requiere certificado cliente:
    // secureClient.setCertificate(clientCert);
    // secureClient.setPrivateKey(clientKey);

    mqtt.setServer(SECRET_MQTT_SERVER, 8883);  // Puerto TLS
    mqtt.setCallback(callbackMQTT);

    if (mqtt.connect("heltec-01", SECRET_MQTT_USER, SECRET_MQTT_PASS)) {
        Serial.println("MQTT TLS conectado");
    }
}
```

## 2. Cifrado de Datos en Tránsito — LoRa

LoRa raw NO tiene cifrado nativo. Opciones:

### AES-128 para payloads LoRa
```cpp
#include <mbedtls/aes.h>

// Clave AES-128 (16 bytes) — va en secrets.h
// #define SECRET_AES_KEY { 0x2B, 0x7E, 0x15, 0x16, ... }
uint8_t aesKey[16] = SECRET_AES_KEY;

// Cifrar (bloques de 16 bytes)
void cifrarAES(uint8_t* input, uint8_t* output, size_t len) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, aesKey, 128);

    // Pad a múltiplo de 16
    size_t padded = ((len + 15) / 16) * 16;
    uint8_t padded_input[padded];
    memset(padded_input, 0, padded);
    memcpy(padded_input, input, len);

    for (size_t i = 0; i < padded; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_ENCRYPT,
                              padded_input + i, output + i);
    }
    mbedtls_aes_free(&aes);
}

// Descifrar
void descifrarAES(uint8_t* input, uint8_t* output, size_t len) {
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, aesKey, 128);

    for (size_t i = 0; i < len; i += 16) {
        mbedtls_aes_crypt_ecb(&aes, MBEDTLS_AES_DECRYPT,
                              input + i, output + i);
    }
    mbedtls_aes_free(&aes);
}
```

### Protección contra replay en LoRa
```cpp
// Añadir contador de secuencia a cada paquete
struct PaqueteSeguro {
    uint32_t secuencia;    // Contador incremental
    uint32_t timestamp;    // millis() o epoch
    uint8_t  datos[16];    // Payload cifrado
    uint8_t  hmac[4];      // Primeros 4 bytes del HMAC
} __attribute__((packed));

uint32_t contadorTX = 0;
uint32_t ultimoRXseq = 0;

bool validarPaquete(PaqueteSeguro* pkt) {
    // Rechazar si secuencia es menor o igual a la última recibida
    if (pkt->secuencia <= ultimoRXseq) {
        Serial.println("⚠️ Replay detectado! Descartando.");
        return false;
    }
    // Rechazar si timestamp es muy viejo (>60s)
    if (abs((long)(millis() - pkt->timestamp)) > 60000) {
        Serial.println("⚠️ Paquete expirado! Descartando.");
        return false;
    }
    ultimoRXseq = pkt->secuencia;
    return true;
}
```

## 3. Hardening del Dispositivo

### Deshabilitar debug serial en producción
```cpp
// En platformio.ini para build de producción:
// build_flags = -DPRODUCCION

#ifdef PRODUCCION
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTF(...)
#else
  #define DEBUG_PRINT(x)    Serial.println(x)
  #define DEBUG_PRINTF(...)  Serial.printf(__VA_ARGS__)
#endif

void setup() {
    #ifndef PRODUCCION
    Serial.begin(115200);
    DEBUG_PRINT("Modo desarrollo — serial activo");
    #endif
}
```

### Deshabilitar JTAG en producción
```cpp
void setup() {
    // Deshabilitar pines JTAG para que no puedan debuggear
    #ifdef PRODUCCION
    gpio_pad_select_gpio(GPIO_NUM_12);  // MTDI
    gpio_pad_select_gpio(GPIO_NUM_13);  // MTCK
    gpio_pad_select_gpio(GPIO_NUM_14);  // MTMS
    gpio_pad_select_gpio(GPIO_NUM_15);  // MTDO
    #endif
}
```

### Secure Boot (ESP32)
```bash
# Habilitar en menuconfig (PlatformIO)
# Security features → Enable hardware Secure Boot in bootloader
# Esto firma el bootloader y verifica el firmware al arrancar

# Generar clave de firma:
espsecure.py generate_signing_key secure_boot_signing_key.pem

# ⚠️ GUARDAR ESTA CLAVE EN LUGAR SEGURO
# Si se pierde, no se puede actualizar el dispositivo
```

### Flash Encryption (ESP32)
```bash
# Cifra el contenido de la flash para que no se pueda leer con herramientas externas
# Habilitar en menuconfig:
# Security features → Enable flash encryption on boot

# ⚠️ IRREVERSIBLE en modo Release
# Usar modo Development para pruebas
```

## 4. Seguridad WiFi

### Proteger el Access Point
```cpp
void crearAPSeguro() {
    WiFi.mode(WIFI_AP);
    // Usar WPA2 con contraseña fuerte (mínimo 8 chars)
    WiFi.softAP("Heltec-Config", SECRET_AP_PASS, 6, false, 2);
    //                                          canal  oculto  max_clients

    // Limitar a 2 clientes máximo
    // Canal fijo para reducir superficie de escaneo
}
```

### Timeout de inactividad WiFi
```cpp
unsigned long ultimaActividad = 0;
#define WIFI_TIMEOUT 300000  // 5 minutos

void loop() {
    if (WiFi.getMode() == WIFI_AP) {
        if (WiFi.softAPgetStationNum() == 0) {
            if (millis() - ultimaActividad > WIFI_TIMEOUT) {
                Serial.println("AP sin clientes, apagando WiFi");
                WiFi.mode(WIFI_OFF);
            }
        } else {
            ultimaActividad = millis();
        }
    }
}
```

### Validar input del servidor web
```cpp
server.on("/api/config", HTTP_POST, [](AsyncWebServerRequest *req) {
    // Validar Content-Type
    if (!req->hasHeader("Content-Type") ||
        req->header("Content-Type") != "application/json") {
        req->send(415, "text/plain", "Unsupported Media Type");
        return;
    }

    // Limitar tamaño del body
    if (req->contentLength() > 512) {
        req->send(413, "text/plain", "Payload Too Large");
        return;
    }

    // Validar token de autenticación
    if (!req->hasHeader("X-Auth-Token") ||
        req->header("X-Auth-Token") != String(SECRET_API_KEY)) {
        req->send(401, "text/plain", "Unauthorized");
        return;
    }

    // Procesar request validado...
    req->send(200, "application/json", "{\"status\":\"ok\"}");
});
```

## 5. OTA Seguro

```cpp
#include <ArduinoOTA.h>
#include <Update.h>

void setupOTASeguro() {
    ArduinoOTA.setHostname("heltec-estacion");
    ArduinoOTA.setPassword(SECRET_OTA_PASS);  // Desde secrets.h
    ArduinoOTA.setPort(3232);                  // Puerto no estándar

    // Validar que el firmware viene de fuente confiable
    ArduinoOTA.onStart([]() {
        Serial.println("OTA: Iniciando actualización...");
        // Podrías verificar un hash/firma aquí
    });

    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]: ", error);
        switch (error) {
            case OTA_AUTH_ERROR:    Serial.println("Auth Failed"); break;
            case OTA_BEGIN_ERROR:   Serial.println("Begin Failed"); break;
            case OTA_CONNECT_ERROR: Serial.println("Connect Failed"); break;
            case OTA_RECEIVE_ERROR: Serial.println("Receive Failed"); break;
            case OTA_END_ERROR:     Serial.println("End Failed"); break;
        }
    });

    ArduinoOTA.begin();
}
```

## 6. Watchdog y Recuperación

```cpp
#include <esp_task_wdt.h>

#define WDT_TIMEOUT 30  // segundos

void setup() {
    // Si el dispositivo se cuelga >30s, reinicia automáticamente
    esp_task_wdt_init(WDT_TIMEOUT, true);
    esp_task_wdt_add(NULL);
}

void loop() {
    esp_task_wdt_reset();  // "Patear" el watchdog

    // Si un sensor falla repetidamente, reiniciar
    static int fallosConsecutivos = 0;
    if (!leerSensor()) {
        fallosConsecutivos++;
        if (fallosConsecutivos > 10) {
            Serial.println("Demasiados fallos, reiniciando...");
            ESP.restart();
        }
    } else {
        fallosConsecutivos = 0;
    }
}
```

## 7. Checklist de Seguridad IoT

### Antes de Desplegar
- [ ] Credenciales en `secrets.h`, NO en código fuente
- [ ] `.gitignore` incluye `secrets.h`, `*.pem`, `.env`
- [ ] Serial debug deshabilitado (`-DPRODUCCION`)
- [ ] Conexiones usan TLS/HTTPS (no HTTP plano)
- [ ] MQTT con autenticación usuario/contraseña + TLS
- [ ] OTA protegido con contraseña fuerte
- [ ] WiFi AP con WPA2 y contraseña fuerte
- [ ] Watchdog timer habilitado
- [ ] Inputs del servidor web validados y sanitizados
- [ ] Payloads LoRa cifrados con AES si contienen datos sensibles
- [ ] Contador anti-replay en comunicación LoRa
- [ ] Sin claves por defecto (cambiar "admin", "password123", etc.)

### Nivel Avanzado (producción real)
- [ ] Secure Boot habilitado
- [ ] Flash Encryption habilitada
- [ ] JTAG deshabilitado
- [ ] Certificados TLS con rotación planificada
- [ ] Logs de seguridad (intentos fallidos de auth)
- [ ] Rate limiting en endpoints HTTP
- [ ] Firmware firmado digitalmente para OTA

## Instrucciones para el Asistente

Al generar código IoT con conectividad:
1. **SIEMPRE** usar HTTPS en lugar de HTTP cuando sea posible
2. **SIEMPRE** autenticar conexiones MQTT (usuario/pass mínimo, TLS ideal)
3. **NUNCA** usar `setInsecure()` excepto en desarrollo con advertencia clara
4. **NUNCA** dejar el puerto serial activo en builds de producción
5. Agregar watchdog a cualquier proyecto que se despliegue sin supervisión
6. Cifrar payloads LoRa si contienen datos personales o sensibles
7. Validar todo input que llegue por HTTP/MQTT/LoRa/BLE
8. Sugerir WiFiManager en lugar de credenciales hardcoded
9. Usar `Preferences.h` (NVS) para almacenar secretos en el dispositivo
10. Al crear un AP, limitar conexiones y usar timeout de inactividad
