#ifndef CMD_WIFI_H
#define CMD_WIFI_H

#include <Arduino.h>

// ✅ ESP8266: Biblioteca WiFi diferente
#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif

// ✅ ESP8266: Usa CMDPreferences em vez de Preferences nativo
#ifdef ESP32
  #include <Preferences.h>
#else
  #include "utils/CMDPreferences.h"
  typedef CMDPreferences Preferences;  // Alias para compatibilidade
#endif

#include "config/CMDConfig.h"

/**
 * CMDWiFi - Gerenciador WiFi com suporte a AP e STA
 * 
 * Funcionalidades:
 * - Modo AP automático (sem credenciais)
 * - Modo STA (com credenciais salvas)
 * - IP Fixo
 * - Factory Reset (6 boots rápidos)
 * - Persistência de credenciais
 * 
 * Compatível com ESP32 e ESP8266
 */
class CMDWiFi {
public:
    CMDWiFi();
    
    // ==================== INICIALIZAÇÃO ====================
    
    /**
     * Inicializa o WiFi Manager
     * @param deviceName Nome base do dispositivo (será: deviceName-MAC)
     */
    void begin(const char* deviceName);
    
    // ==================== CREDENCIAIS ====================
    
    bool loadCredentials();
    bool saveCredentials(const String& ssid, const String& password);
    bool forgetCredentials();
    bool hasCredentials();
    
    // ==================== CONEXÃO ====================
    
    void startAP(const char* deviceName);
    void connectSTA();
    
    // ==================== IP FIXO ====================
    
    bool saveStaticIP(const String& ip, const String& gateway, const String& subnet);
    bool loadStaticIP();
    bool clearStaticIP();
    bool hasStaticIP();
    String getStaticIP();
    String getStaticGateway();
    String getStaticSubnet();
    
    // ==================== INFORMAÇÕES ====================
    
    String getSSID();
    String getPassword();
    String getAPName();
    bool isAPActive();
    
    // ==================== FACTORY RESET ====================
    
    /**
     * Detecta boot rápido (6 boots em 10s = factory reset)
     */
    void handleBootCount();
    
    /**
     * Zera contador após sistema estável (10s)
     */
    void clearBootCountAfterStableRun();
    
    // ==================== UTILITÁRIOS ====================
    
    /**
     * Retorna últimos 6 dígitos do MAC (ex: A1B2C3)
     */
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