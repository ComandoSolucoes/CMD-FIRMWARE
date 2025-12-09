#ifndef CMD_WEBSERVER_H
#define CMD_WEBSERVER_H

#include <Arduino.h>

#ifdef ESP32
  #include <WebServer.h>
#else
  #include <ESP8266WebServer.h>
  typedef ESP8266WebServer WebServer;
#endif

#include "wifi/CMDWiFi.h"
#include "mqtt/CMDMqtt.h"
#include "config/CMDConfig.h"
#include "auth/CMDAuth.h"

class CMDWebServer {
public:
    CMDWebServer(CMDWiFi* wifiManager, CMDMqtt* mqttManager, CMDAuth* authManager);
    
    // Inicialização
    void begin();
    void handle();
    
    // Adicionar rotas customizadas
    void addRoute(const char* uri, WebServer::THandlerFunction handler);
    void addRoutePost(const char* uri, WebServer::THandlerFunction handler);
    
    // ✅ NOVO: Método para módulos registrarem rota /device com autenticação
    void registerDeviceRoute(WebServer::THandlerFunction handler);
    
    // Informações
    bool isRunning();
    
    // Acesso ao servidor web (para rotas avançadas)
    WebServer& getServer() { return server; }
    
private:
    WebServer server;
    CMDWiFi* wifi;
    CMDMqtt* mqtt;
    CMDAuth* auth;
    bool running;
    bool deviceRouteRegistered;  // ✅ NOVO: Flag para detectar registro customizado
    
    // Rotas base
    void setupRoutes();
    void handleRoot();
    void handleInfoPage();
    void handleNetworkPage();
    void handleMqttPage();
    void handleSecurityPage();
    void handleScan();
    void handleConnect();
    void handleStatus();
    void handleForgetWiFi();
    void handleWiFiInfo();
    void handleNotFound();
    
    // API REST - WiFi
    void handleApiInfo();
    void handleApiWiFi();
    void handleApiWiFiScan();
    void handleApiWiFiConnect();
    void handleApiWiFiForget();
    void handleApiReboot();
    void handleApiGetStaticIP();
    void handleApiSetStaticIP();
    void handleApiClearStaticIP();
    
    // API REST - MQTT
    void handleApiMqttStatus();
    void handleApiMqttConfig();
    void handleApiMqttSaveConfig();
    void handleApiMqttClearConfig();
    void handleApiMqttConnect();
    void handleApiMqttDisconnect();
    void handleApiMqttPublish();
    void handleApiMqttMessages();

    // API REST - Auth  
    void handleApiAuthStatus();    
    void handleApiAuthSetPassword(); 
    void handleApiAuthClearPassword(); 
    
    // Páginas
    void sendDashboardPage();
    void sendConfigPage();
    void handleDeviceDefault();
    
    // Utilitários
    int scanNetworks();
};

#endif // CMD_WEBSERVER_H