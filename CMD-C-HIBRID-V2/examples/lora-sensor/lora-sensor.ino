/*
 * EXEMPLO: lora-sensor.ino
 * 
 * Este exemplo mostra como usar o CMD-C modificado para registrar
 * uma página customizada em /device
 */

#include <CMDCore.h>
#include "SensorManager.h"
#include "LoRaHandler.h"
#include "MqttHandler.h"
#include "WebInterface.h"

// Instâncias
CMDCore core("LORA-SENSOR");
SensorManager sensor;
LoRaHandler lora(&sensor);
MqttHandler mqttHandler(&sensor, &lora);
WebInterface webInterface(&sensor, &lora, &mqttHandler);

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=================================");
    Serial.println("  LoRa Sensor - CMD-C v1.0");
    Serial.println("=================================\n");
    core.begin();
    sensor.begin();
    lora.begin();
    mqttHandler.begin(&core.mqtt);
    
    webInterface.registerRoutes(&core.webServer);
    
    Serial.println("\n✅ Sistema inicializado!");
    Serial.println("📡 Acesse: http://" + core.getIP());
}

void loop() {
    core.handle();
    sensor.handle();
    lora.handle();
    mqttHandler.handle();
}
