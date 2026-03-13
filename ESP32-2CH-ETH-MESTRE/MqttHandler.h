#ifndef MQTT_HANDLER_ETH18_H
#define MQTT_HANDLER_ETH18_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <ETH.h>
#include <ArduinoJson.h>
#include <CMDCore.h>
#include "Config.h"
#include "IOManager.h"

class MqttHandler {
public:
    MqttHandler(IOManager* ioMgr);

    // Inicializa usando a configuração MQTT salva pelo CMD-C (/mqtt page)
    // Se não houver config no CMD-C, usa broker/port/user/pass hardcoded em fallback
    void begin(CMDCore* corePtr,
               const char* fallbackBroker = "",
               uint16_t    fallbackPort   = 1883,
               const char* fallbackUser   = "",
               const char* fallbackPass   = "");

    void handle();

    // Publicações (chamadas pelos callbacks do IOManager)
    void publishOutputState(uint8_t index, bool state);
    void publishInputState(uint8_t index, bool state);
    void publishTelemetry();

    bool     isConnected();
    String   getDeviceName()             { return deviceName; }
    void     setDeviceName(const String& name) { deviceName = name; }
    void     setTelemetryInterval(uint32_t ms);
    uint32_t getTelemetryInterval()      { return telemetryInterval; }

private:
    IOManager*   io;
    CMDCore*     core;
    WiFiClient   ethClient;       // WiFiClient funciona sobre ETH no ESP32
    PubSubClient mqttClient;

    // Configuração MQTT (carregada do CMDCore ou fallback)
    String   broker;
    uint16_t port;
    String   mqttUser;
    String   mqttPass;
    String   deviceName;          // nome do dispositivo = base do tópico Tasmota

    uint32_t      telemetryInterval;
    unsigned long lastTelemetryTime;
    unsigned long lastReconnectAttempt;

    // Carrega config do CMDMqtt (se disponível) ou usa fallback
    void loadConfig(const char* fallbackBroker, uint16_t fallbackPort,
                    const char* fallbackUser,   const char* fallbackPass);

    void reconnect();
    void subscribe();

    // Helpers de tópico (formato Tasmota)
    String cmdTopic (const String& cmd);   // cmnd/<device>/<cmd>
    String statTopic(const String& key);   // stat/<device>/<key>
    String teleTopic(const String& key);   // tele/<device>/<key>

    // Callback MQTT (padrão PubSubClient — ponteiro estático)
    static void messageCallback(char* topic, byte* payload, unsigned int length);
    void handleMessage(const String& topic, const String& payload);
    void handlePowerCommand(uint8_t outputIndex, const String& payload);
    void handleAllPowerCommand(const String& payload);

    static MqttHandler* instance;
};

#endif // MQTT_HANDLER_ETH18_H