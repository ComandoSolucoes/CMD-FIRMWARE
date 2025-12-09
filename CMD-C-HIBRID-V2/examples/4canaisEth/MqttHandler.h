#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "IOManager.h"

// Forward declaration
class CMDMqtt;

class MqttHandler {
public:
    MqttHandler(IOManager* ioManager);
    
    // Inicialização
    void begin(CMDMqtt* mqttClient);
    void handle();
    
    // Publicação
    void publishRelayState(uint8_t relay);
    void publishInputState(uint8_t input);
    void publishAllStates();
    void publishStatusJson();
    
    // Configuração de intervalo de publicação
    void setPublishInterval(uint32_t ms);
    uint32_t getPublishInterval();
    void savePublishInterval();
    void loadPublishInterval();
    
private:
    IOManager* io;
    CMDMqtt* mqtt;
    Preferences prefs;
    
    uint32_t publishInterval;
    unsigned long lastPublishTime;
    
    // Métodos privados
    void setupSubscriptions();
    void handleMqttMessage(String topic, String payload);
    
    // Callbacks estáticos (ponte para instância)
    static MqttHandler* instance;
    static void staticMqttCallback(String topic, String payload);
};

#endif // MQTT_HANDLER_H