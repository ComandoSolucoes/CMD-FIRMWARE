#include "LoRaHandler.h"
#include <ArduinoJson.h>

LoRaHandler::LoRaHandler(SensorManager* sensorManager)
    : sensor(sensorManager),
      initialized(false),
      totalPackets(0),
      successfulPackets(0),
      failedPackets(0) {
    
    // Configuração padrão
    config.txPower = DEFAULT_LORA_TX_POWER;
    config.spreadingFactor = DEFAULT_LORA_SPREADING;
    config.codingRate = DEFAULT_LORA_CODING_RATE;
    config.bandwidth = DEFAULT_LORA_BANDWIDTH;
    config.syncWord = DEFAULT_LORA_SYNC_WORD;
}

void LoRaHandler::begin() {
    LOG_INFO("Inicializando LoRa...");
    
    // Abre Preferences
    prefs.begin(PREFS_NAMESPACE, false);
    
    // Carrega configurações
    loadConfig();
    
    // Inicializa LoRa
    if (initLoRa()) {
        LOG_INFO("✅ LoRa inicializado com sucesso!");
        initialized = true;
    } else {
        LOG_ERROR("Falha ao inicializar LoRa!");
        initialized = false;
    }
}

bool LoRaHandler::initLoRa() {
    // Configura SPI
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
    
    // Configura pinos LoRa
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    
    // Inicializa LoRa na frequência configurada
    if (!LoRa.begin(LORA_FREQUENCY)) {
        return false;
    }
    
    // Aplica configurações
    applyConfig();
    
    LOG_INFOF("Frequência: %.1f MHz", LORA_FREQUENCY / 1e6);
    LOG_INFOF("TX Power: %d dBm", config.txPower);
    LOG_INFOF("Spreading Factor: SF%d", config.spreadingFactor);
    LOG_INFOF("Bandwidth: %.1f kHz", config.bandwidth / 1e3);
    
    return true;
}

void LoRaHandler::handle() {
    // Por enquanto, este handler não precisa fazer nada no loop
    // A transmissão é feita sob demanda ou via callback do sensor
}

// ==================== TRANSMISSÃO ====================

bool LoRaHandler::sendReading(const SensorReading& reading) {
    if (!initialized) {
        LOG_ERROR("LoRa não inicializado!");
        return false;
    }
    
    // Feedback LED
    if (sensor) sensor->ledShowTransmit();
    
    // Monta mensagem
    String message;
    if (reading.valid) {
        message = "dist_cm=" + String(reading.distance_cm, 2);
        message += ",temp_c=" + String(reading.temperature, 1);
        message += ",timestamp=" + String(reading.timestamp);
    } else {
        message = "dist_cm=NaN";
    }
    
    // Envia pacote
    LoRa.beginPacket();
    LoRa.print(message);
    bool success = LoRa.endPacket();
    
    // Atualiza estatísticas
    updateStatistics(success);
    
    if (success) {
        LOG_INFOF("📤 LoRa TX: %s", message.c_str());
    } else {
        LOG_ERROR("Falha ao enviar pacote LoRa");
    }
    
    // Volta LED para idle
    if (sensor) sensor->ledShowIdle();
    
    return success;
}

bool LoRaHandler::sendMessage(const String& message) {
    if (!initialized) return false;
    
    if (sensor) sensor->ledShowTransmit();
    
    LoRa.beginPacket();
    LoRa.print(message);
    bool success = LoRa.endPacket();
    
    updateStatistics(success);
    
    if (success) {
        LOG_INFOF("📤 LoRa TX: %s", message.c_str());
    }
    
    if (sensor) sensor->ledShowIdle();
    
    return success;
}

bool LoRaHandler::sendStatusJson() {
    if (!initialized) return false;
    
    // Cria JSON
    JsonDocument doc;
    
    SensorReading reading = sensor->getLastReading();
    
    doc["device"] = DEVICE_MODEL;
    doc["version"] = FIRMWARE_VERSION;
    
    if (reading.valid) {
        doc["distance_cm"] = reading.distance_cm;
    } else {
        doc["distance_cm"] = nullptr;
    }
    
    doc["temp_c"] = reading.temperature;
    doc["timestamp"] = reading.timestamp;
    doc["uptime"] = millis() / 1000;
    
    // Estatísticas do sensor
    doc["sensor"]["total"] = sensor->getTotalReadings();
    doc["sensor"]["valid"] = sensor->getValidReadings();
    doc["sensor"]["success_rate"] = sensor->getSuccessRate();
    
    // Estatísticas LoRa
    doc["lora"]["packets"] = totalPackets;
    doc["lora"]["success"] = successfulPackets;
    doc["lora"]["success_rate"] = getSuccessRate();
    
    // Serializa
    String jsonString;
    serializeJson(doc, jsonString);
    
    // Envia
    return sendMessage(jsonString);
}

// ==================== CONFIGURAÇÃO LORA ====================

void LoRaHandler::setTxPower(uint8_t power) {
    config.txPower = constrain(power, MIN_LORA_TX_POWER, MAX_LORA_TX_POWER);
    if (initialized) {
        LoRa.setTxPower(config.txPower);
    }
    LOG_INFOF("LoRa TX Power: %d dBm", config.txPower);
}

uint8_t LoRaHandler::getTxPower() {
    return config.txPower;
}

void LoRaHandler::setSpreadingFactor(uint8_t sf) {
    config.spreadingFactor = constrain(sf, MIN_LORA_SPREADING, MAX_LORA_SPREADING);
    if (initialized) {
        LoRa.setSpreadingFactor(config.spreadingFactor);
    }
    LOG_INFOF("LoRa Spreading Factor: SF%d", config.spreadingFactor);
}

uint8_t LoRaHandler::getSpreadingFactor() {
    return config.spreadingFactor;
}

void LoRaHandler::setBandwidth(uint32_t bw) {
    config.bandwidth = bw;
    if (initialized) {
        LoRa.setSignalBandwidth(config.bandwidth);
    }
    LOG_INFOF("LoRa Bandwidth: %.1f kHz", config.bandwidth / 1e3);
}

uint32_t LoRaHandler::getBandwidth() {
    return config.bandwidth;
}

void LoRaHandler::setCodingRate(uint8_t cr) {
    config.codingRate = constrain(cr, 5, 8);
    if (initialized) {
        LoRa.setCodingRate4(config.codingRate);
    }
    LOG_INFOF("LoRa Coding Rate: 4/%d", config.codingRate);
}

uint8_t LoRaHandler::getCodingRate() {
    return config.codingRate;
}

void LoRaHandler::setSyncWord(uint8_t sw) {
    config.syncWord = sw;
    if (initialized) {
        LoRa.setSyncWord(config.syncWord);
    }
    LOG_INFOF("LoRa Sync Word: 0x%02X", config.syncWord);
}

uint8_t LoRaHandler::getSyncWord() {
    return config.syncWord;
}

void LoRaHandler::applyConfig() {
    if (!initialized) return;
    
    LoRa.setTxPower(config.txPower);
    LoRa.setSpreadingFactor(config.spreadingFactor);
    LoRa.setSignalBandwidth(config.bandwidth);
    LoRa.setCodingRate4(config.codingRate);
    LoRa.setSyncWord(config.syncWord);
    
    LOG_INFO("Configurações LoRa aplicadas");
}

// ==================== STATUS ====================

bool LoRaHandler::isInitialized() {
    return initialized;
}

int LoRaHandler::getRSSI() {
    if (!initialized) return 0;
    return LoRa.packetRssi();
}

float LoRaHandler::getSNR() {
    if (!initialized) return 0;
    return LoRa.packetSnr();
}

// ==================== ESTATÍSTICAS ====================

void LoRaHandler::updateStatistics(bool success) {
    totalPackets++;
    if (success) {
        successfulPackets++;
    } else {
        failedPackets++;
    }
}

uint32_t LoRaHandler::getTotalPackets() {
    return totalPackets;
}

uint32_t LoRaHandler::getSuccessfulPackets() {
    return successfulPackets;
}

uint32_t LoRaHandler::getFailedPackets() {
    return failedPackets;
}

float LoRaHandler::getSuccessRate() {
    if (totalPackets == 0) return 0;
    return (100.0f * successfulPackets) / totalPackets;
}

void LoRaHandler::resetStatistics() {
    totalPackets = 0;
    successfulPackets = 0;
    failedPackets = 0;
    LOG_INFO("Estatísticas LoRa resetadas");
}

// ==================== PERSISTÊNCIA ====================

void LoRaHandler::saveConfig() {
    prefs.putUChar(PREFS_LORA_TX_POWER, config.txPower);
    prefs.putUChar(PREFS_LORA_SPREADING, config.spreadingFactor);
    
    LOG_INFO("Configurações LoRa salvas");
}

void LoRaHandler::loadConfig() {
    config.txPower = prefs.getUChar(PREFS_LORA_TX_POWER, DEFAULT_LORA_TX_POWER);
    config.spreadingFactor = prefs.getUChar(PREFS_LORA_SPREADING, DEFAULT_LORA_SPREADING);
    
    // Valida limites
    config.txPower = constrain(config.txPower, MIN_LORA_TX_POWER, MAX_LORA_TX_POWER);
    config.spreadingFactor = constrain(config.spreadingFactor, MIN_LORA_SPREADING, MAX_LORA_SPREADING);
    
    LOG_INFO("Configurações LoRa carregadas");
}
