#include "lora_functions.h"
#include "led_functions.h"
#include "config.h"

void initLoRa() {
  // SPI: SCK=18, MISO=19, MOSI=23, SS=LORA_SS_PIN
  SPI.begin(18, 19, 23, LORA_SS_PIN);
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);

  Serial.printf("Inicializando LoRa (%.0f MHz)...\n", LORA_FREQUENCY/1e6);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("Falha ao configurar LoRa!");
    while (true) { delay(1000); }
  }
  Serial.println("Aguardando pacotes LoRa…");
}

void handleLoRaReceive() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;

  blinkBlue(2, 90, 70);
  while (LoRa.available()) {
    Serial.print((char)LoRa.read());
  }
  Serial.print("' com RSSI ");
  Serial.println(LoRa.packetRssi());
  showBaseByWifi();  // volta ao estado base
}
