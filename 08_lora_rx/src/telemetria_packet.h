#ifndef TELEMETRIA_PACKET_H
#define TELEMETRIA_PACKET_H

#include <stdint.h>

// ============================================================
//  Paquete de Telemetría — compartido entre TX y RX
//  ⚠️ Mantener idéntico en ambos proyectos
// ============================================================

#define TELEMETRIA_VERSION 1
#define NODE_ID_DEFAULT    1

struct TelemetriaPacket {
    uint8_t  version;       // Versión del protocolo
    uint8_t  nodeId;        // ID del nodo transmisor (1-254)
    uint32_t secuencia;     // Contador incremental (anti-replay)
    float    temperatura;   // °C (DHT22)
    float    humedad;       // % (DHT22)
    float    presion;       // hPa (BMP180)
    float    altitud;       // metros (BMP180)
    float    sensTermica;   // °C (calculada)
    uint16_t bateria_mV;    // Voltaje batería (ADC)
    uint8_t  errores;       // Errores acumulados de sensores
} __attribute__((packed));  // 28 bytes

// ========== Parámetros LoRa (iguales en TX y RX) ==========
#define LORA_FREQ       915.0   // MHz (Argentina)
#define LORA_BW         125.0   // kHz
#define LORA_SF         9       // Spreading Factor
#define LORA_CR         7       // Coding Rate 4/7
#define LORA_SYNC       0x12    // Sync Word privado
#define LORA_POWER      17      // dBm
#define LORA_PREAMBLE   8       // Símbolos

#endif
