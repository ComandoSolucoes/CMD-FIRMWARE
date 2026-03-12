#ifndef WEB_INTERFACE_ETH18_H
#define WEB_INTERFACE_ETH18_H

#include <Arduino.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "IOManager.h"
#include "MqttHandler.h"
#include "I2CSlaveManager.h"

class WebInterface {
public:
    WebInterface(IOManager* ioMgr, MqttHandler* mqttHdl, I2CSlaveManager* slaveMgr);

    void begin();
    void handle();

private:
    WebServer server;
    IOManager*       io;
    MqttHandler*     mqtt;
    I2CSlaveManager* slaves;

    // Credenciais da interface web
    const char* wwwUser = "admin";
    const char* wwwPass = "admin";

    // Rotas
    void setupRoutes();

    // Páginas
    void handleRoot();
    void handleApi();
    void handleApiOutputs();
    void handleApiInputs();
    void handleApiStatus();
    void handleApiConfig();
    void handleNotFound();

    // Helpers
    bool authenticate();
    String buildPage(const String& title, const String& body);
    String buildDashboard();
};

#endif // WEB_INTERFACE_ETH18_H