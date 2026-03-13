#ifndef WEB_INTERFACE_ETH18_H
#define WEB_INTERFACE_ETH18_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "IOManager.h"
#include "MqttHandler.h"
#include "I2CSlaveManager.h"
#include "EthConfig.h"

class CMDWebServer;

class WebInterface {
public:
    WebInterface(IOManager* ioMgr, MqttHandler* mqttHdl,
                 I2CSlaveManager* slaveMgr, EthConfig* ethCfg);

    void registerRoutes(CMDWebServer* webServer);

private:
    IOManager*       io;
    MqttHandler*     mqtt;
    I2CSlaveManager* slaves;
    EthConfig*       eth;
    WebServer*       server;

    // Handlers existentes
    void handleApiOutputs();
    void handleApiOutputControl();
    void handleApiInputs();
    void handleApiStatus();
    void handleApiGetConfig();
    void handleApiSaveConfig();

    // Handlers novos — IP fixo ETH
    void handleApiGetEthConfig();
    void handleApiSaveEthConfig();
    void handleApiClearEthConfig();

    String buildDashboard();
};

#endif // WEB_INTERFACE_ETH18_H