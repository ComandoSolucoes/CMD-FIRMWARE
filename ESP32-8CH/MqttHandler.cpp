#include "MqttHandler.h"
#include <mqtt/CMDMqtt.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>

MqttHandler::MqttHandler(IOManager* ioMgr) 
    : ioManager(ioMgr),
      mqtt(nullptr),
      mqttConfigured(false),
      lastReconnectAttempt(0),
      reconnectInterval(30000),
      reconnectFailCount(0),
      telemetryInterval(DEFAULT_TELEMETRY_INTERVAL),
      lastTelemetryTime(0) {
}

void MqttHandler::begin(CMDMqtt* mqttClient) {
    mqtt = mqttClient;
 
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    char macStr[13];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    macAddress = String(macStr);
    
    prefs.begin(PREFS_NAMESPACE, false);
    loadTelemetryInterval();
    prefs.end();
    
    LOG_INFOF("MQTT Handler inicializado - MAC: %s", macAddress.c_str());
}

bool MqttHandler::isMqttConfigured() {
    if (!mqtt) return false;
    String broker = mqtt->getBroker();
    return (broker.length() > 0 && broker != "0.0.0.0");
}

void MqttHandler::handle() {
    if (!mqtt) return;
    
    if (!mqttConfigured) {
        mqttConfigured = isMqttConfigured();
        if (!mqttConfigured) return;
    }
    
    if (!mqtt->isConnected()) { 
        unsigned long now = millis();
        if (now - lastReconnectAttempt >= reconnectInterval) {
            lastReconnectAttempt = now;
            
            if (mqtt->connect()) {
                LOG_INFO("✓ Conectado ao broker MQTT");
                reconnectFailCount = 0;
                reconnectInterval = 30000;
                setupSubscriptions();
                publishAllStates();
                publishTelemetry();
            } else {
                reconnectFailCount++;
                if (reconnectFailCount >= 10) {
                    reconnectInterval = 120000;
                }
            }
        }
        return;
    }

    unsigned long now = millis();
    if (now - lastTelemetryTime >= telemetryInterval) {
        lastTelemetryTime = now;
        publishTelemetry();
    }
}

void MqttHandler::setupSubscriptions() {
    if (!mqtt || !mqtt->isConnected()) return; 
    
    String topic = String(MQTT_CMD_PREFIX) + macAddress + "/POWER";
    mqtt->subscribe(topic.c_str());
    LOG_INFOF("✓ Subscrito: %s", topic.c_str());
   
    for (int i = 2; i <= NUM_OUTPUTS; i++) {
        topic = String(MQTT_CMD_PREFIX) + macAddress + "/POWER" + String(i);
        mqtt->subscribe(topic.c_str());
        LOG_INFOF("✓ Subscrito: %s", topic.c_str());
    }
}

void MqttHandler::publishRelayState(uint8_t relay) {
    if (!mqtt || !mqtt->isConnected() || relay >= NUM_OUTPUTS) return; 
    
    const char* state = ioManager->getRelayState(relay) ? "ON" : "OFF";

    String topic = String(MQTT_STAT_PREFIX) + macAddress + "/POWER";
    if (relay > 0) {
        topic += String(relay + 1);
    } else {
        topic += "1";
    }
    
    mqtt->publish(topic.c_str(), state, true);
    LOG_INFOF("Publicado: %s = %s", topic.c_str(), state);
}

void MqttHandler::publishInputState(uint8_t input) {
    if (!mqtt || !mqtt->isConnected() || input >= NUM_INPUTS) return;
    
    const char* state = ioManager->getInputState(input) ? "HIGH" : "LOW";
    String topic = String(MQTT_STAT_PREFIX) + macAddress + "/INPUT" + String(input + 1);
    
    mqtt->publish(topic.c_str(), state, true);
    LOG_INFOF("Publicado: %s = %s", topic.c_str(), state);
}

void MqttHandler::publishAllStates() {
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        publishRelayState(i);
    }
    for (int i = 0; i < NUM_INPUTS; i++) {
        publishInputState(i);
    }
}

void MqttHandler::publishTelemetry() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    StaticJsonDocument<512> doc;
    for (int i = 0; i < NUM_OUTPUTS; i++) {
        String key = "POWER" + String(i + 1);
        doc[key] = ioManager->getRelayState(i) ? "ON" : "OFF";
    }
    
    char jsonString[512];
    serializeJson(doc, jsonString);
    
    String topic = String(MQTT_TELE_PREFIX) + macAddress + "/STATE";
    mqtt->publish(topic.c_str(), jsonString, false);
    
    LOG_INFOF("Telemetria: %s", jsonString);
}

void MqttHandler::handleMqttMessage(String topic, String payload) {
    if (!ioManager) return;
    
    LOG_INFOF("MQTT ← %s: %s", topic.c_str(), payload.c_str());
    
    String prefix = String(MQTT_CMD_PREFIX) + macAddress + "/";
    if (topic.startsWith(prefix)) {
        topic = topic.substring(prefix.length());
    }
    
    payload.toUpperCase();
    if (payload != "ON" && payload != "OFF") {
        LOG_ERROR("Comando inválido (aceito: ON, OFF)");
        return;
    }
    
    bool newState = (payload == "ON");
    
    if (topic == "POWER") {
        ioManager->setRelay(0, newState);
        LOG_INFOF("Relé 1: %s", payload.c_str());
        return;
    }
    
    if (topic.startsWith("POWER")) {
        int relayNum = topic.substring(5).toInt();
        if (relayNum >= 2 && relayNum <= NUM_OUTPUTS) {
            ioManager->setRelay(relayNum - 1, newState);
            LOG_INFOF("Relé %d: %s", relayNum, payload.c_str());
        } else {
            LOG_ERRORF("Relé %d inválido (válido: 1-%d)", relayNum, NUM_OUTPUTS);
        }
        return;
    }
    
    LOG_ERRORF("Tópico desconhecido: %s", topic.c_str());
}

void MqttHandler::onRelayChanged(uint8_t relay, bool state) {
    publishRelayState(relay);
}

void MqttHandler::onInputChanged(uint8_t input, bool state) {
    publishInputState(input);
}

uint32_t MqttHandler::getPublishInterval() {
    return getTelemetryInterval();
}

void MqttHandler::setPublishInterval(uint32_t ms) {
    setTelemetryInterval(ms);
}

void MqttHandler::savePublishInterval() {
    saveTelemetryInterval();
}

void MqttHandler::setTelemetryInterval(uint32_t ms) {
    telemetryInterval = constrain(ms, MIN_TELEMETRY_INTERVAL, MAX_TELEMETRY_INTERVAL);
    saveTelemetryInterval();
    LOG_INFOF("Intervalo de telemetria: %d ms", telemetryInterval);
}

uint32_t MqttHandler::getTelemetryInterval() {
    return telemetryInterval;
}

void MqttHandler::saveTelemetryInterval() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUInt(PREFS_TELEMETRY_INTERVAL, telemetryInterval);
    prefs.end();
}

void MqttHandler::loadTelemetryInterval() {
    telemetryInterval = prefs.getUInt(PREFS_TELEMETRY_INTERVAL, DEFAULT_TELEMETRY_INTERVAL);
}