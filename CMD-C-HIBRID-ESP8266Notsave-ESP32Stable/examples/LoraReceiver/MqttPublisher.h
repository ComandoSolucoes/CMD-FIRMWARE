#ifndef MQTT_PUBLISHER_H
#define MQTT_PUBLISHER_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "LoRaReceiver.h"

// Forward declaration
class CMDMqtt;

class MqttPublisher {
public:
    MqttPublisher(LoRaReceiver* receiver);
    
    // Inicialização
    void begin(CMDMqtt* mqttClient);
    void handle();
    
    // Publicação
    void publishReceivedMessage(const ReceivedMessage& message);
    
    // Configuração
    void setAutoPublish(bool enabled);
    bool isAutoPublishEnabled();
    
    // Persistência
    void saveConfig();
    void loadConfig();
    
private:
    LoRaReceiver* loraReceiver;
    CMDMqtt* mqtt;
    Preferences prefs;
    
    bool autoPublish;
    
    // Métodos privados
    void handleReceivedMessage(const ReceivedMessage& message);
    
    // Callback estático
    static MqttPublisher* instance;
    static void staticMessageCallback(const ReceivedMessage& message);
};

#endif // MQTT_PUBLISHER_H
