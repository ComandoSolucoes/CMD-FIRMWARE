#ifndef CMD_MQTT_H
#define CMD_MQTT_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include "config/CMDConfig.h"

typedef std::function<void(String topic, String payload)> MqttMessageCallback;

class CMDMqtt {
public:
    CMDMqtt();

    void begin(const char* deviceMac);
    void handle();

    bool saveConfig(const String& broker, uint16_t port,
                   const String& user, const String& pass,
                   const String& clientId, uint8_t qos);
    bool loadConfig();
    bool clearConfig();
    bool hasConfig();

    bool connect();
    void disconnect();
    bool isConnected();
    String getStatusText();

    String getBaseTopic() { return baseTopic; }

    bool subscribe(const String& relativeTopic);
    bool subscribeAbsolute(const String& absoluteTopic);
    bool unsubscribe(const String& relativeTopic);
    bool unsubscribeAbsolute(const String& absoluteTopic);

    bool publish(const String& relativeTopic, const String& payload, bool retain = false);
    bool publishAbsolute(const String& absoluteTopic, const String& payload, bool retain = false);
    bool publishStatus(const String& status);
    bool publishOnline();

    void setCallback(MqttMessageCallback callback);

    // Getters de configuração
    String   getBroker()   { return broker; }
    uint16_t getPort()     { return port; }
    String   getUser()     { return user; }
    String   getPass()     { return pass; }   // ← getter adicionado
    String   getClientId() { return clientId; }
    uint8_t  getQoS()      { return qos; }

    struct Message {
        String topic;
        String payload;
        unsigned long timestamp;
    };
    const Message* getLastMessages(int& count);
    int getMessageCount() { return messageCount; }

    unsigned long getLastReconnectAttempt() { return lastReconnectAttempt; }
    unsigned long getReconnectInterval()    { return reconnectInterval; }

private:
    WiFiClient   wifiClient;
    PubSubClient mqttClient;
    Preferences  prefs;

    String   broker;
    uint16_t port;
    String   user;
    String   pass;
    String   clientId;
    uint8_t  qos;
    bool     hasCredentials;

    String baseTopic;
    String statusTopic;
    String onlineTopic;

    unsigned long lastReconnectAttempt;
    const unsigned long reconnectInterval = 5000;
    bool firstConnection;

    MqttMessageCallback userCallback;

    static const int MAX_MESSAGES = 10;
    Message messageHistory[MAX_MESSAGES];
    int messageCount;
    int messageIndex;

    void setupSystemTopics();
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    static void staticMqttCallback(char* topic, byte* payload, unsigned int length);
    void addMessageToHistory(const String& topic, const String& payload);

    static CMDMqtt* instance;
};

#endif // CMD_MQTT_H