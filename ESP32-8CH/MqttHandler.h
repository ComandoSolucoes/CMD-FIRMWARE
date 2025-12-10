#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "IOManager.h"

// Forward declaration (padrão CMD-C)
class CMDMqtt;

class MqttHandler {
public:
    MqttHandler(IOManager* ioMgr);
    
    void begin(CMDMqtt* mqttClient);
    void handle();
    
    // Callbacks do IOManager
    void onRelayChanged(uint8_t relay, bool state);
    void onInputChanged(uint8_t input, bool state);
    
    // Publicações manuais
    void publishAllStates();
    void publishTelemetry();
    
    // Configuração de telemetria (nomes originais)
    void setTelemetryInterval(uint32_t ms);
    uint32_t getTelemetryInterval();
    
    // ✅ ALIASES para compatibilidade com WebInterface
    uint32_t getPublishInterval();        // Chama getTelemetryInterval()
    void setPublishInterval(uint32_t ms); // Chama setTelemetryInterval()
    void savePublishInterval();           // Chama saveTelemetryInterval()
    
    // Processa mensagem MQTT (chamado pelo callback estático)
    void handleMqttMessage(String topic, String payload);
    
private:
    IOManager* ioManager;
    CMDMqtt* mqtt;
    Preferences prefs;
    
    bool mqttConfigured;
    unsigned long lastReconnectAttempt;
    unsigned long reconnectInterval;
    uint8_t reconnectFailCount;
    
    uint32_t telemetryInterval;
    unsigned long lastTelemetryTime;
    
    String macAddress;
    
    bool isMqttConfigured();
    void setupSubscriptions();
    void publishRelayState(uint8_t relay);
    void publishInputState(uint8_t input);
    
    void saveTelemetryInterval();
    void loadTelemetryInterval();
};

#endif // MQTT_HANDLER_H