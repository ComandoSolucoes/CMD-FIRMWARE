#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "LoRaReceiver.h"
#include "MqttPublisher.h"

// Forward declaration
class CMDWebServer;

class WebInterface {
public:
    WebInterface(LoRaReceiver* receiver, MqttPublisher* publisher);
    
    // Registra rotas no servidor web do CMD-C
    void registerRoutes(CMDWebServer* webServer);
    
private:
    LoRaReceiver* lora;
    MqttPublisher* mqtt;
    WebServer* server;
    
    // Páginas HTML
    void handleDevicePage();
    
    // API REST
    void handleApiStatus();
    void handleApiMessages();
    void handleApiClearLog();
    void handleApiConfig();
    void handleApiSaveConfig();
    
    // Geração de HTML
    String generateDevicePage();
};

#endif // WEB_INTERFACE_H
