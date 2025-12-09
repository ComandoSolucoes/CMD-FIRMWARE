#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "SensorManager.h"
#include "LoRaHandler.h"
#include "MqttHandler.h"

// Forward declaration
class CMDWebServer;

class WebInterface {
public:
    WebInterface(SensorManager* sensorManager, LoRaHandler* loraHandler, MqttHandler* mqttHandler);
    
    // Registra rotas no servidor web do CMD-C
    void registerRoutes(CMDWebServer* webServer);
    
private:
    SensorManager* sensor;
    LoRaHandler* lora;
    MqttHandler* mqtt;
    WebServer* server;
    
    // Páginas HTML
    void handleDevicePage();
    
    // API REST
    void handleApiStatus();
    void handleApiConfig();
    void handleApiSaveConfig();
    void handleApiTriggerReading();
    void handleApiResetStats();
    
    // Geração de HTML
    String generateDevicePage();
};

#endif // WEB_INTERFACE_H
