#ifndef CMD_MQTT_H
#define CMD_MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>

// ✅ ESP8266: WiFi e Preferences diferentes
#ifdef ESP32
  #include <WiFi.h>
  #include <Preferences.h>
#else
  #include <ESP8266WiFi.h>
  #include "utils/CMDPreferences.h"
  typedef CMDPreferences Preferences;
#endif

#include "config/CMDConfig.h"

// Callback para mensagens recebidas (tópico completo, payload)
typedef std::function<void(String topic, String payload)> MqttMessageCallback;

class CMDMqtt {
public:
    CMDMqtt();
    
    // Inicialização
    void begin(const char* deviceMac);
    void handle();
    
    // Configuração
    bool saveConfig(const String& broker, uint16_t port, 
                   const String& user, const String& pass,
                   const String& clientId, uint8_t qos);
    bool loadConfig();
    bool clearConfig();
    bool hasConfig();
    
    // Conexão
    bool connect();
    void disconnect();
    bool isConnected();
    String getStatusText();
    
    // Tópico raiz (cmd-c/{MAC})
    String getBaseTopic() { return baseTopic; }
    
    // Subscribe (relativo ao base topic)
    bool subscribe(const String& relativeTopic);
    bool subscribeAbsolute(const String& absoluteTopic);
    bool unsubscribe(const String& relativeTopic);
    bool unsubscribeAbsolute(const String& absoluteTopic);
    
    // Publish (relativo ao base topic)
    bool publish(const String& relativeTopic, const String& payload, bool retain = false);
    bool publishAbsolute(const String& absoluteTopic, const String& payload, bool retain = false);
    
    // Publish de status (tópicos de sistema)
    bool publishStatus(const String& status);
    bool publishOnline();
    
    // Callback
    void setCallback(MqttMessageCallback callback);
    
    // Informações de configuração
    String getBroker() { return broker; }
    uint16_t getPort() { return port; }
    String getUser() { return user; }
    String getClientId() { return clientId; }
    uint8_t getQoS() { return qos; }
    
    // Histórico de mensagens recebidas (últimas 10)
    struct Message {
        String topic;
        String payload;
        unsigned long timestamp;
    };
    const Message* getLastMessages(int& count);
    int getMessageCount() { return messageCount; }
    
    // Estatísticas
    unsigned long getLastReconnectAttempt() { return lastReconnectAttempt; }
    unsigned long getReconnectInterval() { return reconnectInterval; }
    
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    Preferences prefs;
    
    // Configurações
    String broker;
    uint16_t port;
    String user;
    String pass;
    String clientId;
    uint8_t qos;
    bool hasCredentials;
    
    // Tópicos de sistema
    String baseTopic;        // cmd-c/{MAC}
    String statusTopic;      // cmd-c/{MAC}/status
    String onlineTopic;      // cmd-c/{MAC}/online
    
    // Reconexão automática
    unsigned long lastReconnectAttempt;
    const unsigned long reconnectInterval = 5000; // 5 segundos
    bool firstConnection;
    
    // Callback do usuário
    MqttMessageCallback userCallback;
    
    // Histórico de mensagens
    static const int MAX_MESSAGES = 10;
    Message messageHistory[MAX_MESSAGES];
    int messageCount;
    int messageIndex;
    
    // Métodos privados
    void setupSystemTopics();
    void mqttCallback(char* topic, byte* payload, unsigned int length);
    static void staticMqttCallback(char* topic, byte* payload, unsigned int length);
    void addMessageToHistory(const String& topic, const String& payload);
    
    // Instância estática para callback
    static CMDMqtt* instance;
};

#endif // CMD_MQTT_H
