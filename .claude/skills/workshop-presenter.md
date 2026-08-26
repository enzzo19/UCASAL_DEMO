# Workshop Presenter — Guía de Demostración IITA IoT

## Checklist pre-demo

### Hardware
- [ ] Placa TX (campo): sensores BMP180 + DHT22 conectados, batería/USB
- [ ] Placa RX (base): conectada por USB a la notebook (COM3)
- [ ] Ambas placas con firmware actualizado (`pio run --target upload`)
- [ ] Verificar que TX transmite (OLED muestra datos + "TX OK")
- [ ] Verificar que RX recibe (OLED muestra datos + RSSI)

### Software
- [ ] WiFi disponible (RX necesita conectarse para NTP + Firebase)
- [ ] `include/secrets.h` configurado en la placa RX (WiFi + Firebase)
- [ ] Firebase RTDB recibiendo datos (verificar en consola Firebase)
- [ ] Web dashboard accesible (`python -m http.server 8080 --directory web`)
- [ ] App Expo compilando (`cd app && npx expo start --clear`)
- [ ] Celular con Expo Go v54+ instalado y en la misma red WiFi

### Credenciales (NO mostrar en pantalla)
- [ ] Firebase console: acceso preparado pero no mostrar API keys
- [ ] WiFi: tener SSID/password en `secrets.h`, no en código visible
- [ ] Expo: logueado con `npx eas-cli login`

## Orden de presentación sugerido

### Bloque 1 — Hardware (15 min)
1. Mostrar las placas físicas, señalar componentes (ESP32, SX1276, OLED, antena)
2. Explicar sensores: DHT22 (temp+hum), BMP180 (presión+altitud)
3. Mostrar OLED del TX con datos en vivo
4. Carpetas de referencia: `01_blink/` → `06_estacion_clima/` (progresión)

### Bloque 2 — Comunicación LoRa (10 min)
1. Explicar LoRa vs LoRaWAN (usamos LoRa crudo, por qué)
2. Mostrar `telemetria_packet.h` — el struct compartido
3. Separar las placas — mostrar que funciona a distancia
4. OLED del RX mostrando datos recibidos + RSSI + SNR
5. Carpetas: `07_lora_tx/`, `08_lora_rx/`

### Bloque 3 — Cloud (10 min)
1. Mostrar cómo RX se conecta a WiFi y obtiene hora NTP
2. Explicar Firebase RTDB — base de datos en tiempo real
3. Mostrar consola Firebase con datos actualizándose
4. Carpetas: `09_rx_wifi_ntp/`, `10_firebase_rtdb/`

### Bloque 4 — Visualización (15 min)
1. Abrir dashboard web — mostrar vista General (ambiental)
2. Cambiar a vista Caldera — señalar termómetro, gauge, zona operativa
3. Cambiar a vista Litio — pileta de salmuera, evaporación
4. Explicar cálculos derivados (punto de rocío, eficiencia, concentración)
5. Abrir app en celular — mostrar las mismas 3 vistas
6. Carpetas: `web/`, `app/`

### Bloque 5 — Q&A y próximos pasos (10 min)
1. Limitaciones del demo vs producción (Firebase vs MQTT+TimescaleDB)
2. LoRa vs LoRaWAN para escalar
3. Ideas de proyectos: agricultura, minería, industria, smart city

## Troubleshooting en vivo

| Problema | Acción rápida |
|----------|--------------|
| TX no transmite | Verificar sensores, reiniciar placa, revisar Serial Monitor |
| RX no recibe | Verificar frecuencia 915 MHz, acercar placas, reiniciar |
| WiFi no conecta | Verificar SSID/pass en secrets.h, verificar red disponible |
| Firebase no actualiza | Verificar URL y API key, revisar reglas RTDB |
| Web no muestra datos | F12 → Console, verificar errores de Firebase |
| App "Conectando..." | Verificar Firebase config, verificar conexión celular |
| Expo Go incompatible | Usar `npx expo start --clear`, verificar SDK 54 |

## Datos de prueba (si las placas no están disponibles)

Se puede escribir datos manualmente en Firebase RTDB para demo sin hardware:

```json
{
  "actual": {
    "temperatura": 28.5,
    "humedad": 62.0,
    "presion": 1013.25,
    "altitud": 1187.0,
    "sensacion_termica": 30.1,
    "bateria_mV": 4050,
    "node_id": 1,
    "paquetes_rx": 100,
    "paquetes_perdidos": 2,
    "rssi_lora": -72,
    "snr_lora": 8.5,
    "fecha_hora": "2026-08-25 15:00:00",
    "timestamp": 1756192800
  }
}
```
