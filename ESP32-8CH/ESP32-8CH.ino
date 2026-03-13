
#include <CMDCore.h>
#include "Config.h"
#include "IOManager.h"
#include "MqttHandler.h"
#include "WebInterface.h"

// Instâncias globais
CMDCore core(DEVICE_MODEL);
IOManager ioManager;
MqttHandler mqttHandler(&ioManager);
WebInterface webInterface(&ioManager, &mqttHandler);

// Callback estático para mensagens MQTT
void onMqttMessageStatic(String topic, String payload) {
    mqttHandler.handleMqttMessage(topic, payload);
}

void setup() {
    // Inicializa Serial
    Serial.begin(115200);
    delay(500);
    
    LOG_INFO("===========================================");
    LOG_INFOF("%s v%s", DEVICE_MODEL, FIRMWARE_VERSION);
    LOG_INFO("Formato MQTT: Tasmota");
    LOG_INFO("===========================================");
    
    // Inicializa CMD-C Core
    core.begin();
    
    // Inicializa Web Interface
    webInterface.registerRoutes(&core.webServer);
    
    // Configura callbacks do IOManager
    ioManager.setRelayChangeCallback([](uint8_t relay, bool state) {
        mqttHandler.onRelayChanged(relay, state);
    });
    
    ioManager.setInputChangeCallback([](uint8_t input, bool state) {
        mqttHandler.onInputChanged(input, state);
    });

    ioManager.begin();
    
    // Inicializa MQTT Handler
    mqttHandler.begin(&core.mqtt);
    
    // Configura callback MQTT
    core.mqtt.setCallback(onMqttMessageStatic);
    
    LOG_INFO("✓ Sistema inicializado com sucesso!");
    LOG_INFO("");
    LOG_INFO("Acesse a interface web para:");
    LOG_INFO("  - Configurar WiFi");
    LOG_INFO("  - Configurar MQTT");
    LOG_INFO("  - Controlar relés");
    LOG_INFO("  - Configurar modos das entradas");
    LOG_INFO("");
}

void loop() {
    // Processa CMD-C Core (WiFi, MQTT, Web)
    core.handle();
    
    // Processa IOManager (entradas, pulsos)
    ioManager.handle();
    
    // Processa MQTT Handler (reconexão, telemetria)
    mqttHandler.handle();
    
    // Pequeno delay para evitar watchdog
    delay(10);
}
