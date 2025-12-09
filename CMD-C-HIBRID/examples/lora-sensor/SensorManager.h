#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include "Config.h"

// Callback quando uma nova leitura é realizada
typedef std::function<void(const SensorReading& reading)> SensorReadingCallback;

class SensorManager {
public:
    SensorManager();
    
    // Inicialização
    void begin();
    void handle();  // Chamado no loop principal
    
    // ==================== LEITURA DO SENSOR ====================
    
    // Lê distância uma vez (retorna leitura válida ou inválida)
    SensorReading readOnce();
    
    // Lê múltiplas vezes e retorna média
    SensorReading readAverage(uint8_t numReadings = NUM_READINGS_AVG);
    
    // Última leitura válida
    SensorReading getLastReading();
    
    // Força uma leitura imediata
    void triggerReading();
    
    // ==================== CONFIGURAÇÃO ====================
    
    // Temperatura ambiente (°C) para compensação
    void setAmbientTemperature(float tempC);
    float getAmbientTemperature();
    
    // Intervalo de leitura automática (ms)
    void setReadInterval(uint32_t ms);
    uint32_t getReadInterval();
    
    // Habilita/desabilita leitura automática
    void setAutoRead(bool enabled);
    bool isAutoReadEnabled();
    
    // ==================== LED FEEDBACK ====================
    
    // Controle direto do LED
    void ledSetColor(uint8_t r, uint8_t g, uint8_t b);
    void ledOff();
    void ledShowIdle();      // Verde
    void ledShowReading();   // Azul
    void ledShowTransmit();  // Laranja
    void ledShowError();     // Vermelho
    void ledShowWiFiAP();    // Amarelo
    
    // Blink
    void ledBlink(uint8_t r, uint8_t g, uint8_t b, uint8_t times = 2);
    
    // Habilita/desabilita LED
    void setLedEnabled(bool enabled);
    bool isLedEnabled();
    
    // ==================== CALLBACK ====================
    
    void setReadingCallback(SensorReadingCallback callback);
    
    // ==================== PERSISTÊNCIA ====================
    
    void saveConfig();
    void loadConfig();
    
    // ==================== ESTATÍSTICAS ====================
    
    uint32_t getTotalReadings();
    uint32_t getValidReadings();
    uint32_t getInvalidReadings();
    float getSuccessRate();
    void resetStatistics();
    
private:
    Preferences prefs;
    Adafruit_NeoPixel pixels;
    
    // Configurações
    float ambientTemp;
    uint32_t readInterval;
    bool autoReadEnabled;
    bool ledEnabled;
    
    // Estado
    SensorReading lastReading;
    unsigned long lastReadTime;
    SensorReadingCallback onReading;
    
    // Estatísticas
    uint32_t totalReadings;
    uint32_t validReadings;
    uint32_t invalidReadings;
    
    // Métodos privados
    float readDistanceCm();
    float calculateSoundSpeed();
    void updateStatistics(bool valid);
};

#endif // SENSOR_MANAGER_H
