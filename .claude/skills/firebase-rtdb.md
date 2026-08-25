---
name: firebase-rtdb
description: "Firebase Realtime Database desde ESP32: autenticación, escritura/lectura RTDB, estructura de datos para telemetría IoT, reglas de seguridad."
triggers:
  - "firebase"
  - "rtdb"
  - "realtime database"
  - "base de datos"
  - "nube"
  - "cloud"
  - "subir datos"
---

# Firebase Realtime Database — ESP32 / Heltec LoRa32 V2

## Librería Recomendada: Firebase ESP Client

### platformio.ini
```ini
lib_deps =
    mobizt/Firebase Arduino Client Library for ESP32 and ESP8266@^4.4.14
```

## Configuración en secrets.h

```cpp
// ---- Firebase ----
#define SECRET_FIREBASE_API_KEY    "AIzaSy...tu-api-key"
#define SECRET_FIREBASE_DB_URL     "https://tu-proyecto.firebaseio.com"
#define SECRET_FIREBASE_USER_EMAIL "dispositivo@tuproyecto.iam.gserviceaccount.com"
#define SECRET_FIREBASE_USER_PASS  "password-del-usuario"
```

## Inicialización

```cpp
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "secrets.h"

// Objetos Firebase
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool firebaseOK = false;
unsigned long ultimoEnvio = 0;

void initFirebase() {
    config.api_key = SECRET_FIREBASE_API_KEY;
    config.database_url = SECRET_FIREBASE_DB_URL;

    // Autenticación por email/password
    auth.user.email = SECRET_FIREBASE_USER_EMAIL;
    auth.user.password = SECRET_FIREBASE_USER_PASS;

    // Token callback
    config.token_status_callback = tokenStatusCallback;

    Firebase.begin(&config, &auth);
    Firebase.reconnectNetwork(true);

    // Timeout
    fbdo.setBSSLBufferSize(4096, 1024);
    fbdo.setResponseSize(2048);
}

// En loop: verificar que Firebase está listo
bool firebaseListo() {
    return Firebase.ready();
}
```

## Estructura de Datos en RTDB

```
proyecto-iot/
├── nodos/
│   └── nodo_01/
│       ├── info/
│       │   ├── nombre: "Estacion Campo 1"
│       │   ├── ubicacion: "Salta, Argentina"
│       │   ├── altitud_ref: 1193
│       │   └── ultimo_contacto: 1724538000
│       ├── actual/                          ← Última lectura
│       │   ├── temperatura: 19.8
│       │   ├── humedad: 52.2
│       │   ├── presion: 877.86
│       │   ├── altitud: 1193.5
│       │   ├── sens_termica: 19.2
│       │   ├── bateria_mV: 3750
│       │   ├── rssi_lora: -45
│       │   ├── snr_lora: 9.5
│       │   ├── timestamp: 1724538000
│       │   └── fecha_hora: "2026-08-25 10:00:00"
│       └── historial/                       ← Lecturas históricas
│           └── 2026-08-25/
│               ├── -NxAbCd123/
│               │   ├── t: 19.8
│               │   ├── h: 52.2
│               │   ├── p: 877.86
│               │   ├── ts: 1724538000
│               │   └── rssi: -45
│               └── -NxAbCd456/
│                   └── ...
├── alertas/
│   └── nodo_01/
│       └── -NxAlerta1/
│           ├── tipo: "temp_alta"
│           ├── valor: 45.2
│           ├── umbral: 40.0
│           ├── timestamp: 1724538000
│           └── resuelta: false
└── config/
    └── nodo_01/
        ├── intervalo_lectura: 30
        ├── intervalo_envio: 60
        ├── umbral_temp_alta: 40.0
        ├── umbral_temp_baja: -5.0
        └── umbral_humedad_baja: 20.0
```

## Escribir Datos

### Última lectura (sobrescribir)
```cpp
void enviarDatosActuales(float temp, float hum, float pres, float alt,
                         float st, int rssi, float snr, unsigned long epoch) {
    if (!firebaseListo()) return;

    String path = "/nodos/nodo_01/actual";
    FirebaseJson json;

    json.set("temperatura", temp);
    json.set("humedad", hum);
    json.set("presion", pres);
    json.set("altitud", alt);
    json.set("sens_termica", st);
    json.set("rssi_lora", rssi);
    json.set("snr_lora", snr);
    json.set("timestamp", (int)epoch);

    // Formato fecha legible
    char fechaHora[20];
    struct tm timeinfo;
    time_t t = epoch;
    localtime_r(&t, &timeinfo);
    strftime(fechaHora, sizeof(fechaHora), "%Y-%m-%d %H:%M:%S", &timeinfo);
    json.set("fecha_hora", fechaHora);

    if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
        Serial.println("Firebase: datos actuales OK");
    } else {
        Serial.printf("Firebase error: %s\n", fbdo.errorReason().c_str());
    }
}
```

### Historial (push — agrega entrada nueva)
```cpp
void guardarHistorial(float temp, float hum, float pres, unsigned long epoch,
                      int rssi) {
    if (!firebaseListo()) return;

    // Organizar por fecha
    struct tm timeinfo;
    time_t t = epoch;
    localtime_r(&t, &timeinfo);
    char fecha[11];
    strftime(fecha, sizeof(fecha), "%Y-%m-%d", &timeinfo);

    String path = "/nodos/nodo_01/historial/" + String(fecha);
    FirebaseJson json;

    // Nombres cortos para ahorrar espacio en RTDB
    json.set("t", temp);
    json.set("h", hum);
    json.set("p", pres);
    json.set("ts", (int)epoch);
    json.set("rssi", rssi);

    if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
        Serial.printf("Historial guardado: %s\n", fbdo.pushName().c_str());
    } else {
        Serial.printf("Historial error: %s\n", fbdo.errorReason().c_str());
    }
}
```

## Leer Configuración Remota

```cpp
void leerConfigRemota() {
    if (!firebaseListo()) return;

    String path = "/config/nodo_01";
    if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
        FirebaseJson &json = fbdo.jsonData();
        FirebaseJsonData result;

        if (json.get(result, "intervalo_envio")) {
            int intervalo = result.intValue;
            Serial.printf("Intervalo de envío: %d s\n", intervalo);
        }
        if (json.get(result, "umbral_temp_alta")) {
            float umbral = result.floatValue;
            Serial.printf("Umbral temp alta: %.1f °C\n", umbral);
        }
    }
}
```

## Reglas de Seguridad Firebase

```json
{
  "rules": {
    "nodos": {
      "$nodo_id": {
        "actual": {
          ".read": "auth != null",
          ".write": "auth != null && auth.uid === $nodo_id"
        },
        "historial": {
          ".read": "auth != null",
          ".write": "auth != null && auth.uid === $nodo_id",
          "$fecha": {
            ".indexOn": ["ts"]
          }
        }
      }
    },
    "config": {
      "$nodo_id": {
        ".read": "auth != null",
        ".write": "auth != null && auth.token.admin === true"
      }
    },
    "alertas": {
      ".read": "auth != null",
      ".write": "auth != null"
    }
  }
}
```

## NTP — Obtener Fecha/Hora Real

```cpp
#include <time.h>

void initNTP() {
    // Zona horaria Argentina: UTC-3, sin horario de verano
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");

    Serial.print("Sincronizando NTP");
    struct tm timeinfo;
    int intentos = 0;
    while (!getLocalTime(&timeinfo) && intentos < 10) {
        Serial.print(".");
        delay(1000);
        intentos++;
    }

    if (intentos < 10) {
        char buf[30];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("\nHora sincronizada: %s\n", buf);
    } else {
        Serial.println("\nError: NTP no disponible");
    }
}

unsigned long getEpoch() {
    time_t now;
    time(&now);
    return now;
}

String getFechaHora() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return "Sin hora";
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buf);
}
```

## Instrucciones para el Asistente

Al generar código con Firebase:
1. **SIEMPRE** poner API key, URL y credenciales en `secrets.h`
2. Usar nombres cortos en historial (t, h, p, ts) para minimizar almacenamiento
3. Separar datos "actuales" (sobrescribir) de "historial" (push)
4. Organizar historial por fecha para facilitar queries
5. Verificar `Firebase.ready()` antes de cada operación
6. No enviar datos con más frecuencia de lo necesario (mínimo 10-30s)
7. Implementar retry con backoff si falla el envío
8. NTP: zona horaria Argentina es UTC-3 (sin DST)
9. Usar `configTime()` estándar de ESP32, no librerías externas
10. Las reglas de seguridad en Firebase deben restringir escritura por nodo
