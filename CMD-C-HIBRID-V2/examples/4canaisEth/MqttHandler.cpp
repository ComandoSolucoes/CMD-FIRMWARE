#include "mqtthandler.h"
#include <mqtt/CMDMqtt.h>
#include <ArduinoJson.h>

MqttHandler* MqttHandler::instance = nullptr;

MqttHandler::MqttHandler(IOManager* ioManager) 
    : io(ioManager), 
      mqtt(nullptr),
      publishInterval(DEFAULT_PUBLISH_INTERVAL),
      lastPublishTime(0) {
    instance = this;
}

void MqttHandler::begin(CMDMqtt* mqttClient) {
    mqtt = mqttClient;
    
    LOG_INFO("Inicializando MqttHandler...");
    
    // Abre Preferences
    prefs.begin(PREFS_NAMESPACE, false);
    
    // Carrega intervalo de publicação
    loadPublishInterval();
    
    // Configura callback do MQTT
    if (mqtt) {
        mqtt->setCallback(staticMqttCallback);
        
        // Aguarda conexão MQTT antes de fazer subscribe
        if (mqtt->isConnected()) {
            setupSubscriptions();
        }
    }
    
    LOG_INFO("MqttHandler inicializado!");
}

void MqttHandler::setupSubscriptions() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    // Subscribe nos comandos de cada relé
    for (uint8_t i = 1; i <= NUM_OUTPUTS; i++) {
        char topic[50];
        snprintf(topic, sizeof(topic), MQTT_TOPIC_RELAY_SET, i);
        mqtt->subscribe(topic);
    }
    
    // Subscribe no comando para todos os relés
    mqtt->subscribe(MQTT_TOPIC_RELAY_ALL_SET);
    
    LOG_INFO("Subscriptions MQTT configuradas");
}

void MqttHandler::handle() {
    if (!mqtt) return;
    
    // Se MQTT acabou de conectar, faz subscribe
    static bool wasConnected = false;
    bool isConnected = mqtt->isConnected();
    
    if (isConnected && !wasConnected) {
        setupSubscriptions();
        // Publica estados iniciais
        publishAllStates();
        publishStatusJson();
    }
    wasConnected = isConnected;
    
    // Publicação periódica
    if (isConnected && publishInterval > 0) {
        unsigned long now = millis();
        if (now - lastPublishTime >= publishInterval) {
            lastPublishTime = now;
            publishStatusJson();
        }
    }
}

// ==================== PUBLICAÇÃO ====================

void MqttHandler::publishRelayState(uint8_t relay) {
    if (!mqtt || !mqtt->isConnected() || relay >= NUM_OUTPUTS) return;
    
    char topic[50];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_RELAY_STATE, relay + 1);
    
    String state = io->getRelayState(relay) ? "ON" : "OFF";
    mqtt->publish(topic, state, true);  // retain = true
}

void MqttHandler::publishInputState(uint8_t input) {
    if (!mqtt || !mqtt->isConnected() || input >= NUM_INPUTS) return;
    
    char topic[50];
    snprintf(topic, sizeof(topic), MQTT_TOPIC_INPUT_STATE, input + 1);
    
    String state = io->getInputState(input) ? "HIGH" : "LOW";
    mqtt->publish(topic, state, true);  // retain = true
}

void MqttHandler::publishAllStates() {
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        publishRelayState(i);
    }
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        publishInputState(i);
    }
}

void MqttHandler::publishStatusJson() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    // Cria JSON com todos os estados
    JsonDocument doc;
    
    // Array de relés
    JsonArray relays = doc["relays"].to<JsonArray>();
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        relays.add(io->getRelayState(i) ? 1 : 0);
    }
    
    // Array de entradas
    JsonArray inputs = doc["inputs"].to<JsonArray>();
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        inputs.add(io->getInputState(i) ? 1 : 0);
    }
    
    // Uptime
    doc["uptime"] = millis() / 1000;
    
    // Serializa JSON
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publica
    mqtt->publish(MQTT_TOPIC_STATUS, jsonString, true);
}

// ==================== RECEPÇÃO DE MENSAGENS ====================

void MqttHandler::staticMqttCallback(String topic, String payload) {
    if (instance) {
        instance->handleMqttMessage(topic, payload);
    }
}

void MqttHandler::handleMqttMessage(String topic, String payload) {
    payload.toUpperCase();
    
    // Extrai o número do relé do tópico
    // Formato: cmd-c/{MAC}/relay/X/set
    int relayIndex = -1;
    
    // Verifica se é comando para todos os relés
    if (topic.endsWith(MQTT_TOPIC_RELAY_ALL_SET)) {
        if (payload == "ON") {
            io->setAllRelays(true);
            publishAllStates();
            LOG_INFO("MQTT: Todos os relés ligados");
        } else if (payload == "OFF") {
            io->setAllRelays(false);
            publishAllStates();
            LOG_INFO("MQTT: Todos os relés desligados");
        }
        return;
    }
    
    // Extrai número do relé do tópico
    int slashCount = 0;
    int lastSlash = -1;
    for (size_t i = 0; i < topic.length(); i++) {
        if (topic.charAt(i) == '/') {
            slashCount++;
            if (slashCount == 3) {  // Terceira barra antes do número
                lastSlash = i;
                break;
            }
        }
    }
    
    if (lastSlash != -1 && lastSlash + 1 < (int)topic.length()) {
        String numStr = topic.substring(lastSlash + 1);
        int nextSlash = numStr.indexOf('/');
        if (nextSlash != -1) {
            numStr = numStr.substring(0, nextSlash);
        }
        relayIndex = numStr.toInt() - 1;  // Converte para índice 0-based
    }
    
    // Valida índice do relé
    if (relayIndex < 0 || relayIndex >= NUM_OUTPUTS) {
        return;
    }
    
    // Processa comando
    if (payload == "ON") {
        io->setRelay(relayIndex, true);
        publishRelayState(relayIndex);
        LOG_INFOF("MQTT: Relé %d ligado", relayIndex + 1);
        
    } else if (payload == "OFF") {
        io->setRelay(relayIndex, false);
        publishRelayState(relayIndex);
        LOG_INFOF("MQTT: Relé %d desligado", relayIndex + 1);
        
    } else if (payload == "TOGGLE") {
        io->toggleRelay(relayIndex);
        publishRelayState(relayIndex);
        LOG_INFOF("MQTT: Relé %d alternado", relayIndex + 1);
    }
}

// ==================== CONFIGURAÇÃO DE INTERVALO ====================

void MqttHandler::setPublishInterval(uint32_t ms) {
    publishInterval = constrain(ms, MIN_PUBLISH_INTERVAL, MAX_PUBLISH_INTERVAL);
    LOG_INFOF("Intervalo de publicação: %d ms", publishInterval);
}

uint32_t MqttHandler::getPublishInterval() {
    return publishInterval;
}

void MqttHandler::savePublishInterval() {
    prefs.putUInt(PREFS_PUBLISH_INTERVAL, publishInterval);
    LOG_INFO("Intervalo de publicação salvo");
}

void MqttHandler::loadPublishInterval() {
    publishInterval = prefs.getUInt(PREFS_PUBLISH_INTERVAL, DEFAULT_PUBLISH_INTERVAL);
    LOG_INFOF("Intervalo de publicação carregado: %d ms", publishInterval);
}