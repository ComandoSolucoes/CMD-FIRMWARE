#include "MqttHandler.h"
#include <mqtt/CMDMqtt.h>
#include <ArduinoJson.h>

MqttHandler* MqttHandler::instance = nullptr;

MqttHandler::MqttHandler(RelayManager* relayManager) 
    : relay(relayManager),
      mqtt(nullptr),
      publishInterval(DEFAULT_PUBLISH_INTERVAL),
      lastPublishTime(0),
      bootTime(0) {
    instance = this;
}

void MqttHandler::begin(CMDMqtt* mqttClient) {
    LOG_INFO("Inicializando MqttHandler...");
    
    mqtt = mqttClient;
    bootTime = millis();
    
    // Carrega intervalo de publicação
    loadPublishInterval();
    
    // Configura subscrições
    setupSubscriptions();
    
    // Define callback para mensagens MQTT
    mqtt->setCallback(staticMqttCallback);
    
    LOG_SUCCESS("MqttHandler inicializado!");
}

void MqttHandler::handle() {
    if (!mqtt || !mqtt->isConnected()) {
        return;
    }
    
    // Publicação periódica
    unsigned long now = millis();
    if (now - lastPublishTime >= publishInterval) {
        publishStatusJson();
        lastPublishTime = now;
    }
}

void MqttHandler::publishRelayState() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    String topic = mqtt->getBaseTopic() + MQTT_TOPIC_RELAY_STATE;
    String payload = relay->getRelayState() ? "ON" : "OFF";
    
    mqtt->publish(topic.c_str(), payload.c_str(), true);
}

void MqttHandler::publishStatusJson() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    JsonDocument doc;
    
    // Estado do relé
    doc["relay"]["state"] = relay->getRelayState() ? "ON" : "OFF";
    doc["relay"]["pin"] = relay->getRelayPin();
    doc["relay"]["logic"] = relay->getRelayLogic() == RELAY_LOGIC_NORMAL ? "normal" : "inverted";
    
    // Modo de operação
    String mode;
    switch (relay->getOperationMode()) {
        case MODE_NORMAL: mode = "normal"; break;
        case MODE_TIMER: mode = "timer"; break;
        case MODE_PULSE: mode = "pulse"; break;
        case MODE_SCHEDULE: mode = "schedule"; break;
    }
    doc["operation_mode"] = mode;
    
    // Temporizador (se ativo)
    if (relay->isTimerActive()) {
        doc["timer"]["active"] = true;
        doc["timer"]["remaining"] = relay->getTimerRemaining();
    }
    
    // Pulso (se ativo)
    if (relay->isPulseActive()) {
        doc["pulse"]["active"] = true;
    }
    
    // Estatísticas
    #if ENABLE_STATISTICS
    doc["stats"]["total_on_time"] = relay->getTotalOnTime();
    doc["stats"]["switch_count"] = relay->getSwitchCount();
    #endif
    
    // Uptime
    doc["uptime"] = (millis() - bootTime) / 1000;
    
    // Memória livre
    doc["free_heap"] = ESP.getFreeHeap();
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = mqtt->getBaseTopic() + MQTT_TOPIC_STATUS;
    mqtt->publish(topic.c_str(), payload.c_str(), false);
}

void MqttHandler::publishUptime() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    uint32_t uptimeSeconds = (millis() - bootTime) / 1000;
    String topic = mqtt->getBaseTopic() + MQTT_TOPIC_UPTIME;
    
    mqtt->publish(topic.c_str(), String(uptimeSeconds).c_str(), false);
}

void MqttHandler::publishAllStates() {
    publishRelayState();
    publishStatusJson();
}

void MqttHandler::setPublishInterval(uint32_t ms) {
    if (ms >= MIN_PUBLISH_INTERVAL && ms <= MAX_PUBLISH_INTERVAL) {
        publishInterval = ms;
        LOG_INFOF("Intervalo de publicação: %d ms", ms);
    }
}

uint32_t MqttHandler::getPublishInterval() {
    return publishInterval;
}

void MqttHandler::savePublishInterval() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUInt(PREFS_PUBLISH_INTERVAL, publishInterval);
    prefs.end();
    LOG_SUCCESS("Intervalo de publicação salvo!");
}

void MqttHandler::loadPublishInterval() {
    prefs.begin(PREFS_NAMESPACE, true);
    publishInterval = prefs.getUInt(PREFS_PUBLISH_INTERVAL, DEFAULT_PUBLISH_INTERVAL);
    prefs.end();
}

void MqttHandler::setupSubscriptions() {
    if (!mqtt) return;
    
    String baseTopic = mqtt->getBaseTopic();
    
    // Subscreve aos tópicos de controle
    mqtt->subscribe((baseTopic + MQTT_TOPIC_RELAY_SET).c_str());
    mqtt->subscribe((baseTopic + MQTT_TOPIC_PULSE).c_str());
    
    LOG_INFO("Inscrições MQTT configuradas");
}

void MqttHandler::handleMqttMessage(String topic, String payload) {
    // Remove o base topic
    String baseTopic = mqtt->getBaseTopic();
    if (topic.startsWith(baseTopic)) {
        topic = topic.substring(baseTopic.length());
    }
    
    LOG_INFOF("MQTT RX: %s = %s", topic.c_str(), payload.c_str());
    
    // Comando do relé
    if (topic == MQTT_TOPIC_RELAY_SET) {
        payload.toUpperCase();
        
        if (payload == "ON") {
            relay->setRelay(true);
            publishRelayState();
        } 
        else if (payload == "OFF") {
            relay->setRelay(false);
            publishRelayState();
        } 
        else if (payload == "TOGGLE") {
            relay->toggleRelay();
            publishRelayState();
        }
    }
    // Comando de pulso
    else if (topic == MQTT_TOPIC_PULSE) {
        uint32_t duration = payload.toInt();
        if (duration > 0) {
            relay->activatePulse(duration);
            publishRelayState();
        }
    }
}

void MqttHandler::staticMqttCallback(String topic, String payload) {
    if (instance != nullptr) {
        instance->handleMqttMessage(topic, payload);
    }
}
