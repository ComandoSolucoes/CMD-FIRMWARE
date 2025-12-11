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
    
    // Inicializa CMD-C (WiFi, WebServer, MQTT, Auth)
    core.begin();
    
    // Inicializa componentes
    sensor.begin();
    lora.begin();
    mqttHandler.begin(&core.mqtt);
    
    // ✅ REGISTRA CALLBACK: Sensor → LoRa
    sensor.setReadingCallback([](const SensorReading& reading) {
        Serial.println("\n📡 Callback acionado! Enviando via LoRa...");
        
        if (reading.valid) {
            lora.sendReading(reading);
        } else {
            Serial.println("⚠️ Leitura inválida, não enviando");
        }
    });
    
    Serial.println("✅ Callback do sensor registrado!");
    
    // Registra interface web
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