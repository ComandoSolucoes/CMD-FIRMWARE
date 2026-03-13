#ifndef CMD_WIFI_H
#define CMD_WIFI_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include "config/CMDConfig.h"

class CMDWiFi {
public:
    CMDWiFi();
    
    // Inicialização
    void begin(const char* deviceName);
    
    // Credenciais
    bool loadCredentials();
    bool saveCredentials(const String& ssid, const String& password);
    bool forgetCredentials();
    bool hasCredentials();
    
    // Conexão
    void startAP(const char* deviceName);
    void connectSTA();
    
    // IP Fixo
    bool saveStaticIP(const String& ip, const String& gateway, const String& subnet);
    bool loadStaticIP();
    bool clearStaticIP();
    bool hasStaticIP();
    String getStaticIP();
    String getStaticGateway();
    String getStaticSubnet();
    
    // Informações
    String getSSID();
    String getPassword();
    String getAPName();
    bool isAPActive();
    
    // Factory Reset (boot count)
    void handleBootCount();
    void clearBootCountAfterStableRun();
    
    // Utilitários
    static String getMacSuffix();
    
private:
    Preferences prefs;
    String savedSSID;
    String savedPassword;
    String apName;
    bool credentials;
    bool apActive;
    
    // IP Fixo
    bool useStaticIP;
    IPAddress staticIP;
    IPAddress staticGateway;
    IPAddress staticSubnet;
    
    // Métodos privados
    void executeFactoryReset();
    IPAddress parseIP(const String& ipStr);
};

#endif // CMD_WIFI_H