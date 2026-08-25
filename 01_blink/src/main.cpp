#include <Arduino.h>

// LED integrado de la Heltec WiFi LoRa 32 V2
#define LED_ONBOARD 25

void setup() {
    Serial.begin(115200);
    pinMode(LED_ONBOARD, OUTPUT);
    Serial.println("Heltec WiFi LoRa 32 V2 — Blink Test");
}

void loop() {
    digitalWrite(LED_ONBOARD, HIGH);
    Serial.println("LED ON");
    delay(1000);

    digitalWrite(LED_ONBOARD, LOW);
    Serial.println("LED OFF");
    delay(1000);
}
