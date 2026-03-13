#include <CMDCore.h>
#include "LoRaReceiver.h"
#include "MqttPublisher.h"
#include "WebInterface.h"

// Instâncias
CMDCore core("LORA-RECEIVER");
LoRaReceiver receiver;
MqttPublisher mqttPublisher(&receiver);
WebInterface webInterface(&receiver, &mqttPublisher);

void setup() {
    Serial.begin(115200);
    Serial.println("\n\n=================================");
    Serial.println("  LoRa Receiver - CMD-C v1.0");
    Serial.println("=================================\n");
    
    // Inicializa CMD-C (WiFi, WebServer, MQTT, Auth)
    core.begin();
    
    // Inicializa componentes do receptor
    receiver.begin();
    mqttPublisher.begin(&core.mqtt);
    
    // Registra interface web customizada
    webInterface.registerRoutes(&core.webServer);
    
    Serial.println("\n✅ Sistema inicializado!");
    Serial.println("📡 Aguardando mensagens LoRa...");
    Serial.println("🌐 Acesse: http://" + core.getIP());
}

void loop() {
    core.handle();
    receiver.handle();
    mqttPublisher.handle();
}
