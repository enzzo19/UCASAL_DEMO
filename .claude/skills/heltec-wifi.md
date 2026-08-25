---
name: heltec-wifi
description: "WiFi y Bluetooth en la Heltec LoRa32 V2: modos STA/AP, HTTP server/client, MQTT, WebSocket, BLE y OTA."
triggers:
  - "wifi"
  - "http"
  - "mqtt"
  - "websocket"
  - "bluetooth"
  - "ble"
  - "ota"
  - "access point"
  - "servidor web"
  - "web server"
  - "api"
  - "rest"
---

# WiFi, Bluetooth y Conectividad — Heltec LoRa32 V2

## WiFi — Modo Station (conectarse a una red)

```cpp
#include <WiFi.h>

// Credenciales desde secrets.h (ver skill security-repo)
#include "secrets.h"
const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASS = SECRET_WIFI_PASS;

void conectarWiFi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    Serial.print("Conectando a WiFi");
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 20) {
        delay(500);
        Serial.print(".");
        intentos++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\nConectado! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    } else {
        Serial.println("\nError: No se pudo conectar");
    }
}
```

## WiFi — Modo Access Point (crear red propia)

```cpp
#include <WiFi.h>

void crearAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Heltec-Sensor", "password123", 1, false, 4);
    // Params: SSID, password, canal, oculto, max_conexiones

    Serial.printf("AP creado. IP: %s\n", WiFi.softAPIP().toString().c_str());
    // IP por defecto: 192.168.4.1
}
```

## WiFi — Modo Dual (STA + AP simultáneo)

```cpp
void modoDual() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("Heltec-Config", "admin1234");
    WiFi.begin(WIFI_SSID, WIFI_PASS);
}
```

## Servidor Web (AsyncWebServer — recomendado)

### platformio.ini
```ini
lib_deps =
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
```

### Servidor con API REST
```cpp
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

// Variables globales de sensores
float temperatura = 0;
float humedad = 0;
int paquetesLora = 0;

void configurarServidor() {
    // Página principal
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta charset='UTF-8'>";
        html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
        html += "<title>Heltec Monitor</title>";
        html += "<style>body{font-family:sans-serif;max-width:600px;margin:auto;padding:20px;}</style>";
        html += "</head><body>";
        html += "<h1>Heltec LoRa32 Monitor</h1>";
        html += "<p>Temperatura: " + String(temperatura, 1) + " °C</p>";
        html += "<p>Humedad: " + String(humedad, 1) + " %</p>";
        html += "<p>Paquetes LoRa: " + String(paquetesLora) + "</p>";
        html += "</body></html>";
        request->send(200, "text/html", html);
    });

    // API JSON
    server.on("/api/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String json = "{";
        json += "\"temperatura\":" + String(temperatura, 1) + ",";
        json += "\"humedad\":" + String(humedad, 1) + ",";
        json += "\"paquetes\":" + String(paquetesLora) + ",";
        json += "\"rssi_wifi\":" + String(WiFi.RSSI()) + ",";
        json += "\"uptime\":" + String(millis() / 1000);
        json += "}";
        request->send(200, "application/json", json);
    });

    // Recibir configuración POST
    server.on("/api/config", HTTP_POST,
        [](AsyncWebServerRequest *request) {},
        NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
            String body = String((char*)data).substring(0, len);
            Serial.printf("Config recibida: %s\n", body.c_str());
            request->send(200, "application/json", "{\"status\":\"ok\"}");
        }
    );

    server.begin();
    Serial.println("Servidor HTTP en puerto 80");
}
```

## Cliente HTTP (enviar datos a servidor externo)

```cpp
#include <HTTPClient.h>

void enviarDatosHTTP() {
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    http.begin("http://api.ejemplo.com/datos");
    http.addHeader("Content-Type", "application/json");

    String payload = "{\"temp\":" + String(temperatura) + ",\"hum\":" + String(humedad) + "}";
    int httpCode = http.POST(payload);

    if (httpCode > 0) {
        Serial.printf("HTTP Response: %d\n", httpCode);
        String response = http.getString();
        Serial.println(response);
    } else {
        Serial.printf("HTTP Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}
```

## MQTT (PubSubClient)

### platformio.ini
```ini
lib_deps =
    knolleary/PubSubClient@^2.8
```

### Código MQTT
```cpp
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqtt(espClient);

const char* MQTT_SERVER = "broker.hivemq.com";  // o tu broker
const int   MQTT_PORT   = 1883;
const char* MQTT_USER   = "";    // si requiere auth
const char* MQTT_PASS   = "";

void callbackMQTT(char* topic, byte* payload, unsigned int length) {
    String mensaje;
    for (unsigned int i = 0; i < length; i++) {
        mensaje += (char)payload[i];
    }
    Serial.printf("MQTT [%s]: %s\n", topic, mensaje.c_str());

    // Procesar comandos
    if (String(topic) == "heltec/cmd") {
        if (mensaje == "led_on") digitalWrite(25, HIGH);
        if (mensaje == "led_off") digitalWrite(25, LOW);
    }
}

void conectarMQTT() {
    mqtt.setServer(MQTT_SERVER, MQTT_PORT);
    mqtt.setCallback(callbackMQTT);

    String clientId = "Heltec-" + String(random(0xffff), HEX);

    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
        Serial.println("MQTT conectado");
        mqtt.subscribe("heltec/cmd");
        mqtt.publish("heltec/status", "online");
    }
}

void publicarDatos() {
    if (!mqtt.connected()) conectarMQTT();

    String payload = "{\"temp\":" + String(temperatura) + "}";
    mqtt.publish("heltec/sensores", payload.c_str());
}

void loop() {
    mqtt.loop();  // SIEMPRE llamar en el loop
    // ... resto del código
}
```

## WebSocket (comunicación bidireccional en tiempo real)

### platformio.ini
```ini
lib_deps =
    me-no-dev/ESPAsyncWebServer@^1.2.3
    me-no-dev/AsyncTCP@^1.1.1
```

```cpp
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WS cliente #%u conectado\n", client->id());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WS cliente #%u desconectado\n", client->id());
            break;
        case WS_EVT_DATA: {
            String msg = String((char*)data).substring(0, len);
            Serial.printf("WS recibido: %s\n", msg.c_str());
            // Responder al cliente
            client->text("ACK: " + msg);
            break;
        }
    }
}

void setupWebSocket() {
    ws.onEvent(onWebSocketEvent);
    server.addHandler(&ws);
    server.begin();
}

// Broadcast a todos los clientes
void broadcastDatos() {
    String json = "{\"temp\":" + String(temperatura) + "}";
    ws.textAll(json);
}
```

## BLE (Bluetooth Low Energy)

```cpp
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>

BLECharacteristic *pCharacteristic;
bool clienteConectado = false;

// UUIDs personalizados (generar en https://www.uuidgenerator.net/)
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789abc"
#define CHARACTERISTIC_UUID "abcdefab-1234-1234-1234-abcdefabcdef"

class MiServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { clienteConectado = true; }
    void onDisconnect(BLEServer* pServer) { clienteConectado = false; }
};

void setupBLE() {
    BLEDevice::init("Heltec-Sensor");
    BLEServer *pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MiServerCallbacks());

    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());

    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->start();
}

void actualizarBLE() {
    if (clienteConectado) {
        String valor = String(temperatura, 1);
        pCharacteristic->setValue(valor.c_str());
        pCharacteristic->notify();
    }
}
```

## OTA (Actualización por WiFi)

```cpp
#include <ArduinoOTA.h>

void setupOTA() {
    ArduinoOTA.setHostname("heltec-lora32");
    ArduinoOTA.setPassword("admin");

    ArduinoOTA.onStart([]() { Serial.println("OTA: Iniciando..."); });
    ArduinoOTA.onEnd([]() { Serial.println("OTA: Completo!"); });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        Serial.printf("OTA: %u%%\r", (progress / (total / 100)));
    });
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.printf("OTA Error[%u]\n", error);
    });

    ArduinoOTA.begin();
    Serial.println("OTA listo");
}

void loop() {
    ArduinoOTA.handle();  // SIEMPRE llamar en el loop
    // ... resto
}
```

## Instrucciones para el Asistente

Al generar código de conectividad:
- **WiFi + ADC**: Recordar que ADC2 NO funciona con WiFi activo. Solo usar ADC1 (GPIOs 32, 33, 36, 37, 38, 39).
- **WiFi + LoRa simultáneo**: Funciona bien, pero ambos comparten CPU. Usar tareas FreeRTOS si la temporización es crítica.
- **Memoria**: WiFi + WebServer + LoRa consumen mucha RAM. Monitorear con `ESP.getFreeHeap()`.
- **Credenciales**: Nunca hardcodear — usar `secrets.h` (ver skill `security-repo`). Sugerir WiFiManager o configuración por AP.
- **MQTT**: Siempre llamar `mqtt.loop()` en el loop principal.
- **BLE + WiFi**: Se pueden usar simultáneamente pero consume mucha memoria (~60KB extra para BLE).
- **OTA**: Requiere espacio de flash suficiente (partición OTA). Incluir `ArduinoOTA.handle()` en el loop.
- Para servidores web, preferir AsyncWebServer sobre WebServer (no bloqueante).
