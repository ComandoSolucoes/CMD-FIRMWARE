#include "LoRaReceiver.h"

LoRaReceiver::LoRaReceiver()
    : pixels(NUMPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800),
      initialized(false),
      logIndex(0),
      logCount(0),
      totalReceived(0),
      errorCount(0),
      onMessageReceived(nullptr) {
    
    // Configuração padrão
    config.spreadingFactor = DEFAULT_LORA_SPREADING;
    config.codingRate = DEFAULT_LORA_CODING_RATE;
    config.bandwidth = DEFAULT_LORA_BANDWIDTH;
    config.syncWord = DEFAULT_LORA_SYNC_WORD;
}

void LoRaReceiver::begin() {
    LOG_INFO("Inicializando LoRa Receiver...");
    
    // Abre Preferences
    prefs.begin(PREFS_NAMESPACE, false);
    
    // Carrega configurações
    loadConfig();
    
    // Inicializa NeoPixel
    pixels.begin();
    pixels.clear();
    pixels.setBrightness(LED_BRIGHTNESS);
    pixels.show();
    
    // Inicializa LoRa
    if (initLoRa()) {
        LOG_INFO("✅ LoRa inicializado com sucesso!");
        initialized = true;
        ledShowIdle();
    } else {
        LOG_ERROR("Falha ao inicializar LoRa!");
        initialized = false;
        ledShowError();
    }
}

bool LoRaReceiver::initLoRa() {
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
    
    // Coloca em modo de recepção contínua
    LoRa.receive();
    
    LOG_INFOF("Frequência: %.1f MHz", LORA_FREQUENCY / 1e6);
    LOG_INFOF("Spreading Factor: SF%d", config.spreadingFactor);
    LOG_INFOF("Bandwidth: %.1f kHz", config.bandwidth / 1e3);
    LOG_INFO("Modo: Recepção contínua");
    
    return true;
}

void LoRaReceiver::handle() {
    if (!initialized) return;
    
    checkForPackets();
}

void LoRaReceiver::checkForPackets() {
    int packetSize = LoRa.parsePacket();
    
    if (packetSize > 0) {
        // Feedback visual
        ledShowReceiving();
        
        // Lê o pacote
        String payload = "";
        while (LoRa.available()) {
            payload += (char)LoRa.read();
        }
        
        // Obtém RSSI e SNR
        int rssi = LoRa.packetRssi();
        float snr = LoRa.packetSnr();
        
        // Cria registro
        ReceivedMessage message;
        message.payload = payload;
        message.rssi = rssi;
        message.snr = snr;
        message.timestamp = millis();
        
        // Adiciona ao log
        addToLog(message);
        
        // Atualiza estatísticas
        totalReceived++;
        
        LOG_INFOF("📥 LoRa RX: %s (RSSI: %d dBm, SNR: %.1f dB)", 
                  payload.c_str(), rssi, snr);
        
        // Chama callback
        if (onMessageReceived) {
            onMessageReceived(message);
        }
        
        // Volta LED para idle
        delay(100);
        ledShowIdle();
    }
}

void LoRaReceiver::addToLog(const ReceivedMessage& message) {
    // Buffer circular
    messageLog[logIndex] = message;
    logIndex = (logIndex + 1) % MAX_LOG_MESSAGES;
    
    if (logCount < MAX_LOG_MESSAGES) {
        logCount++;
    }
}

// ==================== CONFIGURAÇÃO LORA ====================

void LoRaReceiver::setSpreadingFactor(uint8_t sf) {
    config.spreadingFactor = constrain(sf, MIN_LORA_SPREADING, MAX_LORA_SPREADING);
    if (initialized) {
        LoRa.setSpreadingFactor(config.spreadingFactor);
    }
    LOG_INFOF("LoRa Spreading Factor: SF%d", config.spreadingFactor);
}

uint8_t LoRaReceiver::getSpreadingFactor() {
    return config.spreadingFactor;
}

void LoRaReceiver::setBandwidth(uint32_t bw) {
    config.bandwidth = bw;
    if (initialized) {
        LoRa.setSignalBandwidth(config.bandwidth);
    }
    LOG_INFOF("LoRa Bandwidth: %.1f kHz", config.bandwidth / 1e3);
}

uint32_t LoRaReceiver::getBandwidth() {
    return config.bandwidth;
}

void LoRaReceiver::setCodingRate(uint8_t cr) {
    config.codingRate = constrain(cr, 5, 8);
    if (initialized) {
        LoRa.setCodingRate4(config.codingRate);
    }
    LOG_INFOF("LoRa Coding Rate: 4/%d", config.codingRate);
}

uint8_t LoRaReceiver::getCodingRate() {
    return config.codingRate;
}

void LoRaReceiver::setSyncWord(uint8_t sw) {
    config.syncWord = sw;
    if (initialized) {
        LoRa.setSyncWord(config.syncWord);
    }
    LOG_INFOF("LoRa Sync Word: 0x%02X", config.syncWord);
}

uint8_t LoRaReceiver::getSyncWord() {
    return config.syncWord;
}

void LoRaReceiver::applyConfig() {
    if (!initialized) return;
    
    LoRa.setSpreadingFactor(config.spreadingFactor);
    LoRa.setSignalBandwidth(config.bandwidth);
    LoRa.setCodingRate4(config.codingRate);
    LoRa.setSyncWord(config.syncWord);
    
    LOG_INFO("Configurações LoRa aplicadas");
}

// ==================== LOG DE MENSAGENS ====================

const ReceivedMessage* LoRaReceiver::getLastMessages(int& count) {
    count = logCount;
    return messageLog;
}

void LoRaReceiver::clearLog() {
    logIndex = 0;
    logCount = 0;
    LOG_INFO("Log de mensagens limpo");
}

// ==================== STATUS ====================

bool LoRaReceiver::isInitialized() {
    return initialized;
}

// ==================== ESTATÍSTICAS ====================

uint32_t LoRaReceiver::getTotalReceived() {
    return totalReceived;
}

uint32_t LoRaReceiver::getErrorCount() {
    return errorCount;
}

void LoRaReceiver::resetStatistics() {
    totalReceived = 0;
    errorCount = 0;
    LOG_INFO("Estatísticas resetadas");
}

// ==================== CALLBACK ====================

void LoRaReceiver::setMessageCallback(MessageReceivedCallback callback) {
    onMessageReceived = callback;
}

// ==================== LED ====================

void LoRaReceiver::ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
}

void LoRaReceiver::ledOff() {
    ledSetColor(LED_COLOR_OFF);
}

void LoRaReceiver::ledShowIdle() {
    ledSetColor(LED_COLOR_IDLE);
}

void LoRaReceiver::ledShowReceiving() {
    ledSetColor(LED_COLOR_RECEIVING);
}

void LoRaReceiver::ledShowError() {
    ledSetColor(LED_COLOR_ERROR);
}

void LoRaReceiver::ledBlink(uint8_t r, uint8_t g, uint8_t b, uint8_t times) {
    for (uint8_t i = 0; i < times; i++) {
        ledSetColor(r, g, b);
        delay(LED_BLINK_ON_MS);
        ledShowIdle();
        delay(LED_BLINK_OFF_MS);
    }
}

// ==================== PERSISTÊNCIA ====================

void LoRaReceiver::saveConfig() {
    prefs.putUChar(PREFS_LORA_SPREADING, config.spreadingFactor);
    prefs.putUInt(PREFS_LORA_BANDWIDTH, config.bandwidth);
    prefs.putUChar(PREFS_LORA_CODING_RATE, config.codingRate);
    prefs.putUChar(PREFS_LORA_SYNC_WORD, config.syncWord);
    
    LOG_INFO("Configurações LoRa salvas");
}

void LoRaReceiver::loadConfig() {
    config.spreadingFactor = prefs.getUChar(PREFS_LORA_SPREADING, DEFAULT_LORA_SPREADING);
    config.bandwidth = prefs.getUInt(PREFS_LORA_BANDWIDTH, DEFAULT_LORA_BANDWIDTH);
    config.codingRate = prefs.getUChar(PREFS_LORA_CODING_RATE, DEFAULT_LORA_CODING_RATE);
    config.syncWord = prefs.getUChar(PREFS_LORA_SYNC_WORD, DEFAULT_LORA_SYNC_WORD);
    
    // Valida limites
    config.spreadingFactor = constrain(config.spreadingFactor, MIN_LORA_SPREADING, MAX_LORA_SPREADING);
    
    LOG_INFO("Configurações LoRa carregadas");
}
