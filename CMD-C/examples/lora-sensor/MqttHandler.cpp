#include "MqttHandler.h"
#include <mqtt/CMDMqtt.h>
#include <ArduinoJson.h>

MqttHandler* MqttHandler::instance = nullptr;

MqttHandler::MqttHandler(SensorManager* sensorManager, LoRaHandler* loraHandler)
    : sensor(sensorManager),
      lora(loraHandler),
      mqtt(nullptr),
      publishInterval(DEFAULT_MQTT_PUBLISH_INTERVAL),
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
        
        // Se já conectado, faz subscribe
        if (mqtt->isConnected()) {
            setupSubscriptions();
        }
    }
    
    LOG_INFO("MqttHandler inicializado!");
}

void MqttHandler::setupSubscriptions() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    // Subscribe no tópico de configuração
    mqtt->subscribe(MQTT_TOPIC_CONFIG);
    
    LOG_INFO("Subscriptions MQTT configuradas");
}

void MqttHandler::handle() {
    if (!mqtt) return;
    
    // Se MQTT acabou de conectar, faz subscribe
    static bool wasConnected = false;
    bool isConnected = mqtt->isConnected();
    
    if (isConnected && !wasConnected) {
        setupSubscriptions();
        // Publica status inicial
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

void MqttHandler::publishDistance(const SensorReading& reading) {
    if (!mqtt || !mqtt->isConnected()) return;
    
    String value;
    if (reading.valid) {
        value = String(reading.distance_cm, 2);
    } else {
        value = "NaN";
    }
    
    mqtt->publish(MQTT_TOPIC_DISTANCE, value, true);  // retain = true
    
    LOG_INFOF("📡 MQTT: distance = %s cm", value.c_str());
}

void MqttHandler::publishStatusJson() {
    if (!mqtt || !mqtt->isConnected()) return;
    
    // Cria JSON com status completo
    JsonDocument doc;
    
    // Info do dispositivo
    doc["device"] = DEVICE_MODEL;
    doc["version"] = FIRMWARE_VERSION;
    doc["uptime"] = millis() / 1000;
    
    // Última leitura do sensor
    SensorReading reading = sensor->getLastReading();
    if (reading.valid) {
        doc["sensor"]["distance_cm"] = reading.distance_cm;
    } else {
        doc["sensor"]["distance_cm"] = nullptr;
    }
    doc["sensor"]["temperature"] = reading.temperature;
    doc["sensor"]["timestamp"] = reading.timestamp;
    doc["sensor"]["interval_ms"] = sensor->getReadInterval();
    
    // Estatísticas do sensor
    doc["sensor"]["stats"]["total"] = sensor->getTotalReadings();
    doc["sensor"]["stats"]["valid"] = sensor->getValidReadings();
    doc["sensor"]["stats"]["success_rate"] = sensor->getSuccessRate();
    
    // LoRa
    doc["lora"]["initialized"] = lora->isInitialized();
    doc["lora"]["tx_power"] = lora->getTxPower();
    doc["lora"]["spreading"] = lora->getSpreadingFactor();
    doc["lora"]["packets_sent"] = lora->getTotalPackets();
    doc["lora"]["success_rate"] = lora->getSuccessRate();
    
    // Serializa
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Publica
    mqtt->publish(MQTT_TOPIC_STATUS, jsonString, true);
    
    LOG_INFO("📡 MQTT: status publicado");
}

// ==================== RECEPÇÃO DE MENSAGENS ====================

void MqttHandler::staticMqttCallback(String topic, String payload) {
    if (instance) {
        instance->handleMqttMessage(topic, payload);
    }
}

void MqttHandler::handleMqttMessage(String topic, String payload) {
    // Tópico de configuração: cmd-c/{MAC}/sensor/config/set
    if (topic.endsWith(MQTT_TOPIC_CONFIG)) {
        LOG_INFOF("📥 MQTT config recebida: %s", payload.c_str());
        
        // Parse JSON
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            LOG_ERROR("Erro ao parsear JSON de configuração");
            return;
        }
        
        // Atualiza configurações
        bool configChanged = false;
        
        if (doc.containsKey("temp_c")) {
            float temp = doc["temp_c"];
            sensor->setAmbientTemperature(temp);
            configChanged = true;
        }
        
        if (doc.containsKey("read_interval_ms")) {
            uint32_t interval = doc["read_interval_ms"];
            sensor->setReadInterval(interval);
            configChanged = true;
        }
        
        if (doc.containsKey("mqtt_interval_ms")) {
            uint32_t interval = doc["mqtt_interval_ms"];
            setPublishInterval(interval);
            configChanged = true;
        }
        
        if (doc.containsKey("lora_tx_power")) {
            uint8_t power = doc["lora_tx_power"];
            lora->setTxPower(power);
            configChanged = true;
        }
        
        if (doc.containsKey("lora_spreading")) {
            uint8_t sf = doc["lora_spreading"];
            lora->setSpreadingFactor(sf);
            configChanged = true;
        }
        
        if (doc.containsKey("led_enabled")) {
            bool enabled = doc["led_enabled"];
            sensor->setLedEnabled(enabled);
            configChanged = true;
        }
        
        // Salva configurações
        if (configChanged) {
            sensor->saveConfig();
            lora->saveConfig();
            savePublishInterval();
            
            // Publica status atualizado
            publishStatusJson();
            
            LOG_INFO("✅ Configurações atualizadas via MQTT");
        }
    }
}

// ==================== CONFIGURAÇÃO DE INTERVALO ====================

void MqttHandler::setPublishInterval(uint32_t ms) {
    publishInterval = constrain(ms, MIN_MQTT_PUBLISH_INTERVAL, MAX_MQTT_PUBLISH_INTERVAL);
    LOG_INFOF("Intervalo MQTT: %d ms", publishInterval);
}

uint32_t MqttHandler::getPublishInterval() {
    return publishInterval;
}

void MqttHandler::savePublishInterval() {
    prefs.putUInt(PREFS_MQTT_INTERVAL, publishInterval);
    LOG_INFO("Intervalo MQTT salvo");
}

void MqttHandler::loadPublishInterval() {
    publishInterval = prefs.getUInt(PREFS_MQTT_INTERVAL, DEFAULT_MQTT_PUBLISH_INTERVAL);
    LOG_INFOF("Intervalo MQTT carregado: %d ms", publishInterval);
}
