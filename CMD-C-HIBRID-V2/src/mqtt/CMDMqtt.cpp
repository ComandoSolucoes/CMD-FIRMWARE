#include "CMDMqtt.h"

// Instância estática para callback
CMDMqtt* CMDMqtt::instance = nullptr;

CMDMqtt::CMDMqtt() 
    : mqttClient(wifiClient), hasCredentials(false), qos(0),
      lastReconnectAttempt(0), firstConnection(true),
      messageCount(0), messageIndex(0), userCallback(nullptr) {
    instance = this;
}

void CMDMqtt::begin(const char* deviceMac) {
    // Define tópico base: cmd-c/{MAC}
    baseTopic = "cmd-c/" + String(deviceMac);
    statusTopic = baseTopic + "/status";
    onlineTopic = baseTopic + "/online";
    
    Serial.print(CMD_DEBUG_PREFIX "MQTT Base Topic: ");
    Serial.println(baseTopic);
    
    // Abre namespace de preferências MQTT
    prefs.begin("cmd-mqtt", false);
    
    // Carrega configurações salvas
    loadConfig();
    
    // Configura callback estático
    mqttClient.setCallback(staticMqttCallback);
    
    // Se tem configuração, tenta conectar
    if (hasCredentials && WiFi.status() == WL_CONNECTED) {
        connect();
    }
}

void CMDMqtt::handle() {
    // Processa mensagens MQTT se conectado
    if (mqttClient.connected()) {
        mqttClient.loop();
        return;
    }
    
    // Se não está conectado e tem credenciais, tenta reconectar
    if (hasCredentials && WiFi.status() == WL_CONNECTED) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > reconnectInterval) {
            lastReconnectAttempt = now;
            Serial.println(CMD_DEBUG_PREFIX "Tentando reconectar ao MQTT...");
            connect();
        }
    }
}

// ==================== CONFIGURAÇÃO ====================

bool CMDMqtt::saveConfig(const String& broker, uint16_t port, 
                        const String& user, const String& pass,
                        const String& clientId, uint8_t qos) {
    Serial.println(CMD_DEBUG_PREFIX "Salvando configuração MQTT...");
    
    // Se Client ID vazio, gera automaticamente
    String finalClientId = clientId;
    if (finalClientId.length() == 0) {
        // Gera baseado no base topic: cmd-c-{MAC}
        String mac = baseTopic.substring(6); // Remove "cmd-c/"
        finalClientId = "cmd-c-" + mac;
        Serial.print(CMD_DEBUG_PREFIX "Client ID gerado automaticamente: ");
        Serial.println(finalClientId);
    }
    
    prefs.putString("broker", broker);
    prefs.putUShort("port", port);
    prefs.putString("user", user);
    prefs.putString("pass", pass);
    prefs.putString("client", finalClientId);
    prefs.putUChar("qos", qos);
    prefs.putBool("configured", true);
    
    this->broker = broker;
    this->port = port;
    this->user = user;
    this->pass = pass;
    this->clientId = finalClientId;
    this->qos = qos;
    this->hasCredentials = true;
    
    Serial.print(CMD_DEBUG_PREFIX "MQTT configurado: ");
    Serial.print(broker);
    Serial.print(":");
    Serial.println(port);
    
    return true;
}

bool CMDMqtt::loadConfig() {
    hasCredentials = prefs.getBool("configured", false);
    
    if (!hasCredentials) {
        Serial.println(CMD_DEBUG_PREFIX "MQTT não configurado");
        return false;
    }
    
    broker = prefs.getString("broker", "");
    port = prefs.getUShort("port", 1883);
    user = prefs.getString("user", "");
    pass = prefs.getString("pass", "");
    clientId = prefs.getString("client", "");
    qos = prefs.getUChar("qos", 0);
    
    // Se Client ID vazio, gera automaticamente
    if (clientId.length() == 0) {
        String mac = baseTopic.substring(6); // Remove "cmd-c/"
        clientId = "cmd-c-" + mac;
        prefs.putString("client", clientId); // Salva para próxima vez
        Serial.print(CMD_DEBUG_PREFIX "Client ID gerado: ");
        Serial.println(clientId);
    }
    
    Serial.print(CMD_DEBUG_PREFIX "MQTT carregado: ");
    Serial.print(broker);
    Serial.print(":");
    Serial.println(port);
    
    return true;
}

bool CMDMqtt::clearConfig() {
    Serial.println(CMD_DEBUG_PREFIX "Limpando configuração MQTT...");
    
    disconnect();
    
    prefs.clear();
    
    broker = "";
    port = 1883;
    user = "";
    pass = "";
    clientId = "";
    qos = 0;
    hasCredentials = false;
    
    Serial.println(CMD_DEBUG_PREFIX "Configuração MQTT removida");
    
    return true;
}

bool CMDMqtt::hasConfig() {
    return hasCredentials;
}

// ==================== CONEXÃO ====================

bool CMDMqtt::connect() {
    if (!hasCredentials) {
        Serial.println(CMD_DEBUG_PREFIX "MQTT não configurado");
        return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println(CMD_DEBUG_PREFIX "WiFi não conectado");
        return false;
    }
    
    if (mqttClient.connected()) {
        return true;
    }
    
    Serial.print(CMD_DEBUG_PREFIX "Conectando ao broker MQTT ");
    Serial.print(broker);
    Serial.print(":");
    Serial.print(port);
    Serial.println("...");
    
    // Configura servidor
    mqttClient.setServer(broker.c_str(), port);
    
    // Configura LWT (Last Will Testament) - Publica "0" em online quando desconectar
    bool connected = false;
    if (user.length() > 0) {
        connected = mqttClient.connect(clientId.c_str(), 
                                      user.c_str(), 
                                      pass.c_str(),
                                      onlineTopic.c_str(),
                                      qos,
                                      true,  // retain
                                      "0");  // will message
    } else {
        connected = mqttClient.connect(clientId.c_str(),
                                      onlineTopic.c_str(),
                                      qos,
                                      true,  // retain
                                      "0");  // will message
    }
    
    if (connected) {
        Serial.println(CMD_DEBUG_PREFIX "✅ Conectado ao MQTT!");
        
        // Publica que está online
        publishOnline();
        publishStatus("connected");
        
        firstConnection = false;
        
        return true;
    } else {
        Serial.print(CMD_DEBUG_PREFIX "❌ Falha na conexão MQTT. Estado: ");
        Serial.println(mqttClient.state());
        return false;
    }
}

void CMDMqtt::disconnect() {
    if (mqttClient.connected()) {
        publishStatus("disconnected");
        mqttClient.disconnect();
        Serial.println(CMD_DEBUG_PREFIX "MQTT desconectado");
    }
}

bool CMDMqtt::isConnected() {
    return mqttClient.connected();
}

String CMDMqtt::getStatusText() {
    if (!hasCredentials) {
        return "não configurado";
    }
    
    if (!mqttClient.connected()) {
        int state = mqttClient.state();
        switch (state) {
            case -4: return "timeout na conexão";
            case -3: return "conexão perdida";
            case -2: return "falha ao conectar";
            case -1: return "desconectado";
            case  1: return "protocolo incorreto";
            case  2: return "client ID rejeitado";
            case  3: return "servidor indisponível";
            case  4: return "credenciais inválidas";
            case  5: return "não autorizado";
            default: return "reconectando...";
        }
    }
    
    return "conectado";
}

// ==================== SUBSCRIBE ====================

bool CMDMqtt::subscribe(const String& relativeTopic) {
    String fullTopic = baseTopic + "/" + relativeTopic;
    return subscribeAbsolute(fullTopic);
}

bool CMDMqtt::subscribeAbsolute(const String& absoluteTopic) {
    if (!mqttClient.connected()) {
        Serial.println(CMD_DEBUG_PREFIX "MQTT não conectado");
        return false;
    }
    
    bool success = mqttClient.subscribe(absoluteTopic.c_str(), qos);
    
    if (success) {
        Serial.print(CMD_DEBUG_PREFIX "✅ Subscribe: ");
        Serial.println(absoluteTopic);
    } else {
        Serial.print(CMD_DEBUG_PREFIX "❌ Falha no subscribe: ");
        Serial.println(absoluteTopic);
    }
    
    return success;
}

bool CMDMqtt::unsubscribe(const String& relativeTopic) {
    String fullTopic = baseTopic + "/" + relativeTopic;
    return unsubscribeAbsolute(fullTopic);
}

bool CMDMqtt::unsubscribeAbsolute(const String& absoluteTopic) {
    if (!mqttClient.connected()) {
        return false;
    }
    
    bool success = mqttClient.unsubscribe(absoluteTopic.c_str());
    
    if (success) {
        Serial.print(CMD_DEBUG_PREFIX "Unsubscribe: ");
        Serial.println(absoluteTopic);
    }
    
    return success;
}

// ==================== PUBLISH ====================

bool CMDMqtt::publish(const String& relativeTopic, const String& payload, bool retain) {
    String fullTopic = baseTopic + "/" + relativeTopic;
    return publishAbsolute(fullTopic, payload, retain);
}

bool CMDMqtt::publishAbsolute(const String& absoluteTopic, const String& payload, bool retain) {
    if (!mqttClient.connected()) {
        Serial.println(CMD_DEBUG_PREFIX "MQTT não conectado");
        return false;
    }
    
    bool success = mqttClient.publish(absoluteTopic.c_str(), payload.c_str(), retain);
    
    if (success) {
        Serial.print(CMD_DEBUG_PREFIX "📤 Publish: ");
        Serial.print(absoluteTopic);
        Serial.print(" = ");
        Serial.println(payload);
    } else {
        Serial.print(CMD_DEBUG_PREFIX "❌ Falha no publish: ");
        Serial.println(absoluteTopic);
    }
    
    return success;
}

bool CMDMqtt::publishStatus(const String& status) {
    return publishAbsolute(statusTopic, status, true);
}

bool CMDMqtt::publishOnline() {
    return publishAbsolute(onlineTopic, "1", true);
}

// ==================== CALLBACK ====================

void CMDMqtt::setCallback(MqttMessageCallback callback) {
    userCallback = callback;
}

void CMDMqtt::staticMqttCallback(char* topic, byte* payload, unsigned int length) {
    if (instance != nullptr) {
        instance->mqttCallback(topic, payload, length);
    }
}

void CMDMqtt::mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Converte payload para String
    String payloadStr = "";
    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }
    
    String topicStr = String(topic);
    
    Serial.print(CMD_DEBUG_PREFIX "📥 Recebido: ");
    Serial.print(topicStr);
    Serial.print(" = ");
    Serial.println(payloadStr);
    
    // Adiciona ao histórico
    addMessageToHistory(topicStr, payloadStr);
    
    // Chama callback do usuário se existir
    if (userCallback) {
        userCallback(topicStr, payloadStr);
    }
}

// ==================== HISTÓRICO ====================

void CMDMqtt::addMessageToHistory(const String& topic, const String& payload) {
    messageHistory[messageIndex].topic = topic;
    messageHistory[messageIndex].payload = payload;
    messageHistory[messageIndex].timestamp = millis();
    
    messageIndex = (messageIndex + 1) % MAX_MESSAGES;
    
    if (messageCount < MAX_MESSAGES) {
        messageCount++;
    }
}

const CMDMqtt::Message* CMDMqtt::getLastMessages(int& count) {
    count = messageCount;
    return messageHistory;
}