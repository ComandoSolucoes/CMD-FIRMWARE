#ifndef CMD_CORE_H
#define CMD_CORE_H

#include <Arduino.h>
#include "config/CMDConfig.h"
#include "wifi/CMDWiFi.h"
#include "webserver/CMDWebServer.h"
#include "mqtt/CMDMqtt.h"
#include "auth/CMDAuth.h"  // ✅ NOVO

class CMDCore {
public:
    CMDCore(const char* deviceName = "CMD-Device");
    
    // Inicialização
    void begin();
    void handle();
    
    // Web Server
    void addRoute(const char* uri, WebServer::THandlerFunction handler);
    
    // WiFi Info
    bool isConnected();
    String getSSID();
    String getIP();
    int getRSSI();
    bool isAPMode();
    
    // Sistema
    String getMacAddress();
    String getVersion();
    uint32_t getUptime();
    
    // Factory Reset
    void clearBootCountAfterStableRun();
    
    // MQTT - Acesso público
    CMDMqtt mqtt;
    
    // Auth - Acesso público  // ✅ NOVO
    CMDAuth auth;              // ✅ NOVO

    CMDWebServer webServer;
    
private:
    String deviceName;
    CMDWiFi wifi;

    void printSystemInfo();
};

#endif // CMD_CORE_H