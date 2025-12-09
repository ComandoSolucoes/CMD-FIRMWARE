#ifndef LORA_HANDLER_H
#define LORA_HANDLER_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>
#include "Config.h"
#include "SensorManager.h"

class LoRaHandler {
public:
    LoRaHandler(SensorManager* sensorManager);
    
    // Inicialização
    void begin();
    void handle();
    
    // ==================== TRANSMISSÃO ====================
    
    // Envia leitura do sensor
    bool sendReading(const SensorReading& reading);
    
    // Envia mensagem customizada
    bool sendMessage(const String& message);
    
    // Envia JSON com status completo
    bool sendStatusJson();
    
    // ==================== CONFIGURAÇÃO LORA ====================
    
    // TX Power (2-20 dBm)
    void setTxPower(uint8_t power);
    uint8_t getTxPower();
    
    // Spreading Factor (6-12)
    void setSpreadingFactor(uint8_t sf);
    uint8_t getSpreadingFactor();
    
    // Bandwidth (Hz)
    void setBandwidth(uint32_t bw);
    uint32_t getBandwidth();
    
    // Coding Rate (5-8 = 4/5 a 4/8)
    void setCodingRate(uint8_t cr);
    uint8_t getCodingRate();
    
    // Sync Word (0x00-0xFF)
    void setSyncWord(uint8_t sw);
    uint8_t getSyncWord();
    
    // Aplica todas as configurações
    void applyConfig();
    
    // ==================== STATUS ====================
    
    bool isInitialized();
    int getRSSI();          // RSSI do último pacote recebido
    float getSNR();         // SNR do último pacote recebido
    
    // ==================== ESTATÍSTICAS ====================
    
    uint32_t getTotalPackets();
    uint32_t getSuccessfulPackets();
    uint32_t getFailedPackets();
    float getSuccessRate();
    void resetStatistics();
    
    // ==================== PERSISTÊNCIA ====================
    
    void saveConfig();
    void loadConfig();
    
private:
    SensorManager* sensor;
    Preferences prefs;
    
    // Estado
    bool initialized;
    
    // Configuração LoRa
    LoRaConfig config;
    
    // Estatísticas
    uint32_t totalPackets;
    uint32_t successfulPackets;
    uint32_t failedPackets;
    
    // Métodos privados
    bool initLoRa();
    void updateStatistics(bool success);
};

#endif // LORA_HANDLER_H
