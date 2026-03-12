#ifndef MQTT_HANDLER_ETH18_H
#define MQTT_HANDLER_ETH18_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ETH.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "IOManager.h"

class MqttHandler {
public:
    MqttHandler(IOManager* ioMgr);

    void begin(const char* broker, uint16_t port,
               const char* user, const char* pass,
               const char* deviceName);

    void handle();

    // Publica estado de uma saída (chamado pelo IOManager via callback)
    void publishOutputState(uint8_t index, bool state);

    // Publica estado de uma entrada
    void publishInputState(uint8_t index, bool state);

    // Publica telemetria completa
    void publishTelemetry();

    bool isConnected();

    void setTelemetryInterval(uint32_t ms);
    uint32_t getTelemetryInterval();

    // Nome do dispositivo (topic base)
    String getDeviceName();
    void setDeviceName(const String& name);

private:
    IOManager* io;
    WiFiClient ethClient;
    PubSubClient mqttClient;

    String deviceName;
    String broker;
    uint16_t port;
    String mqttUser;
    String mqttPass;

    uint32_t telemetryInterval;
    unsigned long lastTelemetryTime;
    unsigned long lastReconnectAttempt;

    // Tópicos Tasmota
    // cmnd/ETH-18CH-MASTER/POWER1  → controla saída 1
    // stat/ETH-18CH-MASTER/POWER1  → estado saída 1
    // tele/ETH-18CH-MASTER/SENSOR  → telemetria

    String cmdTopic(const String& cmd);   // cmnd/<device>/<cmd>
    String statTopic(const String& key);  // stat/<device>/<key>
    String teleTopic(const String& key);  // tele/<device>/<key>

    void reconnect();
    void subscribe();

    static void messageCallback(char* topic, byte* payload, unsigned int length);
    void handleMessage(const String& topic, const String& payload);

    // Parse de comandos Tasmota
    void handlePowerCommand(uint8_t outputIndex, const String& payload);
    void handleAllPowerCommand(const String& payload);

    static MqttHandler* instance; // para callback estático
};

#endif // MQTT_HANDLER_ETH18_H