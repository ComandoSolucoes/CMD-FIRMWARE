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
    
    // Gera página HTML
    String generateDevicePage();
    
private:
    IOManager* io;
    MqttHandler* mqtt;
    WebServer* server;
    
    // Utilitários
    String getInputModeName(InputMode mode);
    String getRelayLogicName(RelayLogic logic);
    String getInitialStateName(InitialState state);
};

#endif // WEB_INTERFACE_H