#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include <WebServer.h>
#include "Config.h"
#include "IOManager.h"
#include "MqttHandler.h"

// Forward declaration
class CMDWebServer;

class WebInterface {
public:
    WebInterface(IOManager* ioManager, MqttHandler* mqttHandler);
    
    // Registra rotas no servidor web
    void registerRoutes(CMDWebServer* webServer);
    
private:
    IOManager* io;
    MqttHandler* mqtt;
    WebServer* server;
    
    // Páginas HTML
    void handleDevicePage();
    void handleDeviceControl();
    
    // API REST
    void handleApiDeviceStatus();
    void handleApiRelayControl();
    void handleApiRelaySetAll();
    void handleApiGetConfig();
    void handleApiSaveConfig();
    
    // Geração de HTML
    String generateDevicePage();
    String generateControlSection();
    String generateConfigSection();
    
    // Utilitários
    String getInputModeName(InputMode mode);
    String getRelayLogicName(RelayLogic logic);
    String getInitialStateName(InitialState state);
};

#endif // WEB_INTERFACE_H