#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "SensorManager.h"
#include "LoRaHandler.h"

// Forward declaration
class CMDMqtt;

class MqttHandler {
public:
    MqttHandler(SensorManager* sensorManager, LoRaHandler* loraHandler);
    
    // Inicialização
    void begin(CMDMqtt* mqttClient);
    void handle();
    
    // Publicação
    void publishDistance(const SensorReading& reading);
    void publishStatusJson();
    
    // Configuração de intervalo de publicação
    void setPublishInterval(uint32_t ms);
    uint32_t getPublishInterval();
    void savePublishInterval();
    void loadPublishInterval();
    
private:
    SensorManager* sensor;
    LoRaHandler* lora;
    CMDMqtt* mqtt;
    Preferences prefs;
    
    uint32_t publishInterval;
    unsigned long lastPublishTime;
    
    // Métodos privados
    void setupSubscriptions();
    void handleMqttMessage(String topic, String payload);
    
    // Callbacks estáticos
    static MqttHandler* instance;
    static void staticMqttCallback(String topic, String payload);
};

#endif // MQTT_HANDLER_H
