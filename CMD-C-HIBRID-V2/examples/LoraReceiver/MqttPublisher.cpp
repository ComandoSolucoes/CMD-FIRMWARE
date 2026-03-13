#include "MqttPublisher.h"
#include <mqtt/CMDMqtt.h>

MqttPublisher* MqttPublisher::instance = nullptr;

MqttPublisher::MqttPublisher(LoRaReceiver* receiver)
    : loraReceiver(receiver),
      mqtt(nullptr),
      autoPublish(true) {
    instance = this;
}

void MqttPublisher::begin(CMDMqtt* mqttClient) {
    mqtt = mqttClient;
    
    LOG_INFO("Inicializando MqttPublisher...");
    
    // Abre Preferences
    prefs.begin(PREFS_NAMESPACE, false);
    
    // Carrega configuração
    loadConfig();
    
    // Registra callback do LoRa
    if (loraReceiver) {
        loraReceiver->setMessageCallback(staticMessageCallback);
    }
    
    LOG_INFOF("Auto Publish: %s", autoPublish ? "HABILITADO" : "DESABILITADO");
    LOG_INFO("MqttPublisher inicializado!");
}

void MqttPublisher::handle() {
    // Por enquanto não precisa fazer nada no loop
    // A publicação é feita via callback
}

// ==================== PUBLICAÇÃO ====================

void MqttPublisher::publishReceivedMessage(const ReceivedMessage& message) {
    if (!mqtt || !mqtt->isConnected()) {
        LOG_ERROR("MQTT não conectado, não foi possível publicar");
        return;
    }
    
    // Cria JSON
    JsonDocument doc;
    
    // Parse do payload (assume formato: key=value,key=value)
    // Exemplo: dist_cm=123.45,temp_c=25.5,timestamp=12345
    String payload = message.payload;
    
    // Tenta parsear campos conhecidos
    int distIdx = payload.indexOf("dist_cm=");
    int tempIdx = payload.indexOf("temp_c=");
    int tsIdx = payload.indexOf("timestamp=");
    
    if (distIdx >= 0) {
        String distStr = payload.substring(distIdx + 8);
        int commaIdx = distStr.indexOf(',');
        if (commaIdx > 0) distStr = distStr.substring(0, commaIdx);
        
        float dist = distStr.toFloat();
        if (distStr == "NaN") {
            doc["distance_cm"] = nullptr;
        } else {
            doc["distance_cm"] = dist;
        }
    }
    
    if (tempIdx >= 0) {
        String tempStr = payload.substring(tempIdx + 7);
        int commaIdx = tempStr.indexOf(',');
        if (commaIdx > 0) tempStr = tempStr.substring(0, commaIdx);
        doc["temperature"] = tempStr.toFloat();
    }
    
    if (tsIdx >= 0) {
        String tsStr = payload.substring(tsIdx + 10);
        int commaIdx = tsStr.indexOf(',');
        if (commaIdx > 0) tsStr = tsStr.substring(0, commaIdx);
        doc["sensor_timestamp"] = tsStr.toInt();
    }
    
    // Adiciona informações do receptor
    doc["rssi"] = message.rssi;
    doc["snr"] = message.snr;
    doc["received_at"] = message.timestamp;
    doc["receiver"] = DEVICE_MODEL;
    
    // Serializa
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publica
    bool success = mqtt->publish(MQTT_TOPIC_RECEIVED, jsonString, false);
    
    if (success) {
        LOG_INFOF("📡 MQTT: Publicado em %s", MQTT_TOPIC_RECEIVED);
    } else {
        LOG_ERROR("Falha ao publicar no MQTT");
    }
}

// ==================== CALLBACK ====================

void MqttPublisher::staticMessageCallback(const ReceivedMessage& message) {
    if (instance) {
        instance->handleReceivedMessage(message);
    }
}

void MqttPublisher::handleReceivedMessage(const ReceivedMessage& message) {
    if (autoPublish) {
        publishReceivedMessage(message);
    }
}

// ==================== CONFIGURAÇÃO ====================

void MqttPublisher::setAutoPublish(bool enabled) {
    autoPublish = enabled;
    LOG_INFOF("Auto Publish: %s", autoPublish ? "HABILITADO" : "DESABILITADO");
}

bool MqttPublisher::isAutoPublishEnabled() {
    return autoPublish;
}

// ==================== PERSISTÊNCIA ====================

void MqttPublisher::saveConfig() {
    prefs.putBool(PREFS_AUTO_PUBLISH, autoPublish);
    LOG_INFO("Configuração MqttPublisher salva");
}

void MqttPublisher::loadConfig() {
    autoPublish = prefs.getBool(PREFS_AUTO_PUBLISH, true);
    LOG_INFOF("Auto Publish carregado: %s", autoPublish ? "HABILITADO" : "DESABILITADO");
}
