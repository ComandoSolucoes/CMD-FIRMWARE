#ifndef LORA_RECEIVER_H
#define LORA_RECEIVER_H

#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

// Callback quando mensagem é recebida
typedef std::function<void(const ReceivedMessage& message)> MessageReceivedCallback;

class LoRaReceiver {
public:
    LoRaReceiver();
    
    // Inicialização
    void begin();
    void handle();
    
    // ==================== CONFIGURAÇÃO LORA ====================
    
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
    
    // ==================== LOG DE MENSAGENS ====================
    
    // Retorna últimas N mensagens recebidas
    const ReceivedMessage* getLastMessages(int& count);
    
    // Limpa log de mensagens
    void clearLog();
    
    // ==================== STATUS ====================
    
    bool isInitialized();
    
    // ==================== ESTATÍSTICAS ====================
    
    uint32_t getTotalReceived();
    uint32_t getErrorCount();
    void resetStatistics();
    
    // ==================== CALLBACK ====================
    
    void setMessageCallback(MessageReceivedCallback callback);
    
    // ==================== LED ====================
    
    void ledSetColor(uint8_t r, uint8_t g, uint8_t b);
    void ledOff();
    void ledShowIdle();
    void ledShowReceiving();
    void ledShowError();
    void ledBlink(uint8_t r, uint8_t g, uint8_t b, uint8_t times = 2);
    
    // ==================== PERSISTÊNCIA ====================
    
    void saveConfig();
    void loadConfig();
    
private:
    Preferences prefs;
    Adafruit_NeoPixel pixels;
    
    // Configuração LoRa
    LoRaConfig config;
    
    // Estado
    bool initialized;
    
    // Log de mensagens (buffer circular)
    ReceivedMessage messageLog[MAX_LOG_MESSAGES];
    int logIndex;
    int logCount;
    
    // Estatísticas
    uint32_t totalReceived;
    uint32_t errorCount;
    
    // Callback
    MessageReceivedCallback onMessageReceived;
    
    // Métodos privados
    bool initLoRa();
    void checkForPackets();
    void addToLog(const ReceivedMessage& message);
};

#endif // LORA_RECEIVER_H
