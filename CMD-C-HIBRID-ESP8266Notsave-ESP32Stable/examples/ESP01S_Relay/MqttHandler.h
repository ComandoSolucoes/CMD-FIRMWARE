#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "RelayManager.h"

// Forward declaration
class CMDMqtt;

class MqttHandler {
public:
    MqttHandler(RelayManager* relayManager);
    
    // Inicialização
    void begin(CMDMqtt* mqttClient);
    void handle();
    
    // Publicação
    void publishRelayState();
    void publishStatusJson();
    void publishUptime();
    void publishAllStates();
    
    // Configuração de intervalo de publicação
    void setPublishInterval(uint32_t ms);
    uint32_t getPublishInterval();
    void savePublishInterval();
    void loadPublishInterval();
    
private:
    RelayManager* relay;
    CMDMqtt* mqtt;
    Preferences prefs;
    
    uint32_t publishInterval;
    unsigned long lastPublishTime;
    unsigned long bootTime;
    
    // Métodos privados
    void setupSubscriptions();
    void handleMqttMessage(String topic, String payload);
    
    // Callbacks estáticos (ponte para instância)
    static MqttHandler* instance;
    static void staticMqttCallback(String topic, String payload);
};

#endif // MQTT_HANDLER_H
