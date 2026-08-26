# Contrato de Datos — Telemetría IoT

## Paquete LoRa (struct C++)

Definido en `telemetria_packet.h`, compartido entre TX y RX:

```cpp
struct TelemetriaPacket {
    uint8_t  nodeId;        // ID del nodo transmisor
    uint32_t secuencia;     // Contador anti-replay
    float    temperatura;   // °C (DHT22)
    float    humedad;       // % (DHT22)
    float    presion;       // hPa (BMP180)
    float    altitud;       // metros (BMP180)
    float    sensTermica;   // °C (calculada)
    uint16_t bateria_mV;    // Voltaje batería
    uint8_t  errores;       // Contador de errores
} __attribute__((packed));  // 27 bytes
```

## Estructura Firebase RTDB

Path: `/actual` — se sobreescribe con cada lectura del RX:

```json
{
  "temperatura": 25.3,
  "humedad": 45.2,
  "presion": 1013.25,
  "altitud": 1187.0,
  "sensacion_termica": 24.8,
  "bateria_mV": 4120,
  "node_id": 1,
  "paquetes_rx": 1542,
  "paquetes_perdidos": 3,
  "rssi_lora": -67,
  "snr_lora": 9.5,
  "fecha_hora": "2026-08-25 14:30:15",
  "timestamp": 1756191015
}
```

## Mapeo de campos: LoRa → Firebase → Apps

| LoRa struct | Firebase key | Web/App prop | Unidad |
|-------------|-------------|-------------|--------|
| `temperatura` | `temperatura` | `data.temperatura` | °C |
| `humedad` | `humedad` | `data.humedad` | % |
| `presion` | `presion` | `data.presion` | hPa |
| `altitud` | `altitud` | `data.altitud` | m |
| `sensTermica` | `sensacion_termica` | `data.sensacion_termica` | °C |
| `bateria_mV` | `bateria_mV` | `data.bateria_mV` | mV |
| `nodeId` | `node_id` | `data.node_id` | — |
| — (RX calcula) | `rssi_lora` | `data.rssi_lora` | dBm |
| — (RX calcula) | `snr_lora` | `data.snr_lora` | dB |
| — (RX calcula) | `paquetes_rx` | `data.paquetes_rx` | count |
| — (RX calcula) | `paquetes_perdidos` | `data.paquetes_perdidos` | count |
| — (RX, NTP) | `fecha_hora` | `data.fecha_hora` | string |
| — (RX, epoch) | `timestamp` | `data.timestamp` | unix s |

## TypeScript Interface (App)

```typescript
export interface SensorData {
  temperatura: number;
  humedad: number;
  presion: number;
  altitud: number;
  sensacion_termica: number;
  bateria_mV: number;
  node_id: number;
  paquetes_rx: number;
  paquetes_perdidos: number;
  rssi_lora: number;
  snr_lora: number;
  fecha_hora: string;
  timestamp: number;
}
```

## Reglas de consistencia

> **Si se agrega/quita/renombra un campo, hay que actualizar TODOS estos archivos:**
>
> 1. `07_lora_tx/include/telemetria_packet.h` — struct TX
> 2. `08_lora_rx/include/telemetria_packet.h` — struct RX
> 3. `09_rx_wifi_ntp/include/telemetria_packet.h` — struct RX+WiFi
> 4. `10_firebase_rtdb/src/main.cpp` — JSON que se sube a Firebase
> 5. `app/src/types.ts` — TypeScript interface
> 6. `web/index.html` — JavaScript que lee de Firebase
> 7. Este skill (`telemetria-packet.md`) — documentación

## Rangos esperados de sensores

| Sensor | Campo | Mín | Máx | Nota |
|--------|-------|-----|-----|------|
| DHT22 | temperatura | -40 | 80 | ±0.5°C precisión |
| DHT22 | humedad | 0 | 100 | ±2-5% precisión |
| BMP180 | presion | 300 | 1100 | hPa, ±0.12 hPa |
| BMP180 | altitud | -500 | 9000 | metros, calculado |
| LoRa | rssi_lora | -120 | -20 | dBm, más alto = mejor |
| LoRa | snr_lora | -20 | 15 | dB, >0 es bueno |
| Batería | bateria_mV | 3000 | 4200 | Li-ion típica |
