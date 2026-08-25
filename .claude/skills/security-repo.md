---
name: security-repo
description: "Seguridad del repositorio: gestión de secretos, API keys, credenciales, .env, .gitignore, pre-commit hooks. NUNCA exponer datos sensibles en código."
triggers:
  - "api key"
  - "credencial"
  - "secret"
  - "password"
  - "token"
  - ".env"
  - "gitignore"
  - "seguridad repo"
  - "cifrar"
  - "encrypt"
---

# Seguridad del Repositorio — Protección de Secretos y Credenciales

## ⛔ REGLA ABSOLUTA

**NUNCA escribir secretos directamente en archivos de código (.cpp, .h, .ino, platformio.ini).**
Esto incluye:
- Contraseñas WiFi (SSID/password)
- API keys (ThingSpeak, Blynk, Firebase, AWS, etc.)
- Tokens de autenticación (OAuth, Bearer, JWT)
- Credenciales MQTT (usuario/contraseña, certificados)
- URLs de brokers/servidores privados
- Certificados TLS/SSL
- Claves de cifrado LoRa (AppKey, NwkSKey, AppSKey)

## Estructura de Archivos de Secretos

```
proyecto/
├── src/
│   ├── main.cpp          ← Código fuente (SIN secretos)
│   └── config.h          ← #include "secrets.h" + valores por defecto
├── include/
│   └── secrets.h         ← 🔒 SECRETOS REALES (en .gitignore)
├── secrets.h.example     ← 📝 Plantilla con valores de ejemplo
├── platformio.ini
└── .gitignore            ← Incluye secrets.h, *.pem, .env, etc.
```

## Patrón: secrets.h

### secrets.h.example (SÍ va al repo — plantilla)
```cpp
// ============================================
// COPIA ESTE ARCHIVO COMO: include/secrets.h
// Llena con tus valores reales
// ⚠️ NUNCA subas secrets.h al repositorio
// ============================================

#ifndef SECRETS_H
#define SECRETS_H

// WiFi
#define SECRET_WIFI_SSID     "TU_RED_WIFI"
#define SECRET_WIFI_PASS     "TU_PASSWORD"

// MQTT
#define SECRET_MQTT_SERVER   "broker.ejemplo.com"
#define SECRET_MQTT_PORT     1883
#define SECRET_MQTT_USER     ""
#define SECRET_MQTT_PASS     ""

// API Keys
#define SECRET_API_KEY       "tu-api-key-aqui"
#define SECRET_API_URL       "https://api.ejemplo.com"

// LoRaWAN (ABP)
#define SECRET_DEV_ADDR      0x00000000
#define SECRET_NWK_SKEY      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define SECRET_APP_SKEY      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// LoRaWAN (OTAA)
#define SECRET_DEV_EUI       { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define SECRET_APP_EUI       { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define SECRET_APP_KEY       { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }

// OTA
#define SECRET_OTA_PASS      "admin"

#endif
```

### Uso en main.cpp
```cpp
#include "secrets.h"  // Archivo local, NO va al repo

const char* ssid = SECRET_WIFI_SSID;
const char* pass = SECRET_WIFI_PASS;
```

### config.h (va al repo — valores por defecto seguros)
```cpp
#ifndef CONFIG_H
#define CONFIG_H

#include "secrets.h"

// ---- Configuración pública (segura) ----
#define DEVICE_NAME     "Heltec-Estacion"
#define SEND_INTERVAL   30000   // ms entre envíos
#define SERIAL_BAUD     115200

// ---- WiFi ----
#ifndef SECRET_WIFI_SSID
  #error "Falta secrets.h — copia secrets.h.example a include/secrets.h"
#endif

const char* WIFI_SSID = SECRET_WIFI_SSID;
const char* WIFI_PASS = SECRET_WIFI_PASS;

#endif
```

## .gitignore — Patrones de Seguridad Obligatorios

```gitignore
# ===== SECRETOS Y CREDENCIALES =====
**/secrets.h
**/secrets.cpp
**/.env
**/.env.*
!**/.env.example
**/credentials.*
!**/credentials.example
**/private_key*
**/*.pem
**/*.key
**/*.p12
**/*.pfx
**/*.jks

# ===== PlatformIO =====
.pio/
.vscode/.browse.c_cpp.db*
.vscode/c_cpp_properties.json
.vscode/launch.json
.vscode/ipch

# ===== Sistema =====
.DS_Store
Thumbs.db
*.swp
*.swo
*~
```

## Protección Adicional: Validar que secrets.h No Se Suba

### Verificación en compilación (platformio.ini)
```ini
build_flags =
    -DSECRETS_REQUIRED
```

### Verificación en código
```cpp
#if defined(SECRETS_REQUIRED) && !__has_include("secrets.h")
  #error "===== ERROR: Falta include/secrets.h ====="
  #error "Copia secrets.h.example a include/secrets.h y llena tus datos"
#endif
```

## Patrón: Credenciales WiFi por Portal Cautivo (sin hardcodear)

```cpp
#include <WiFiManager.h>  // tzapu/WiFiManager

void setup() {
    WiFiManager wm;
    // Si no hay credenciales guardadas, abre AP "Heltec-Config"
    // El usuario se conecta y configura SSID/pass por web
    bool conectado = wm.autoConnect("Heltec-Config", "setup1234");
    
    if (!conectado) {
        Serial.println("Fallo WiFi, reiniciando...");
        ESP.restart();
    }
    Serial.println("WiFi OK via WiFiManager");
}
```

### platformio.ini
```ini
lib_deps =
    tzapu/WiFiManager@^0.16.0
```

## Patrón: Almacenar Secretos en NVS (Flash interna del ESP32)

```cpp
#include <Preferences.h>

Preferences prefs;

void guardarCredencial(const char* key, const char* value) {
    prefs.begin("secrets", false);  // false = lectura/escritura
    prefs.putString(key, value);
    prefs.end();
}

String leerCredencial(const char* key) {
    prefs.begin("secrets", true);  // true = solo lectura
    String val = prefs.getString(key, "");
    prefs.end();
    return val;
}

// Uso:
// guardarCredencial("wifi_ssid", "MiRed");
// String ssid = leerCredencial("wifi_ssid");
```

## Qué Hacer Si un Secreto Ya Se Subió al Repo

1. **Revocar inmediatamente** la API key / contraseña comprometida
2. Generar nuevas credenciales
3. Eliminar del historial de git:
   ```bash
   # Eliminar archivo del historial completo
   git filter-branch --force --index-filter \
     "git rm --cached --ignore-unmatch include/secrets.h" \
     --prune-empty --tag-name-filter cat -- --all
   
   # Force push (⚠️ reescribe historia)
   git push origin --force --all
   ```
4. Agregar al `.gitignore` para que no vuelva a pasar
5. Verificar que no quede en forks o clones

## Instrucciones para el Asistente

Al generar código que use credenciales:
1. **SIEMPRE** usar el patrón `secrets.h` — nunca poner valores reales en código
2. **SIEMPRE** crear/verificar que existe `secrets.h.example` con valores placeholder
3. **SIEMPRE** verificar que `.gitignore` incluya `**/secrets.h`
4. Si el usuario pega una API key en el chat, **advertir** que no debe ir en el código fuente
5. Sugerir WiFiManager para credenciales WiFi cuando sea viable
6. Para LoRaWAN, las claves van en `secrets.h`, nunca en `main.cpp`
7. Al crear un proyecto nuevo, copiar el `.gitignore` base del root
8. **NUNCA** hacer echo, print, o log de secretos por serial en producción
