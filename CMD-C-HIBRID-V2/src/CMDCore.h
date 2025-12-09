#ifndef CMD_CORE_H
#define CMD_CORE_H

#include <Arduino.h>
#include "config/CMDConfig.h"
#include "wifi/CMDWiFi.h"
#include "webserver/CMDWebServer.h"
#include "mqtt/CMDMqtt.h"
#include "auth/CMDAuth.h"

// ✅ Compatibilidade: WebServer tem nomes diferentes
#ifdef ESP32
  #include <WebServer.h>
#else
  #include <ESP8266WebServer.h>
  typedef ESP8266WebServer WebServer;
#endif

/**
 * CMDCore - Classe principal da biblioteca CMD-C
 * 
 * Integra todos os módulos:
 * - WiFi Manager (AP + STA)
 * - Web Server + Dashboard
 * - MQTT
 * - Autenticação
 * 
 * Compatível com ESP32 e ESP8266
 * 
 * Exemplo de uso:
 * 
 * #include <CMDCore.h>
 * 
 * CMDCore core("MeuDispositivo");
 * 
 * void setup() {
 *   core.begin();
 * }
 * 
 * void loop() {
 *   core.handle();
 * }
 */
class CMDCore {
public:
    /**
     * Construtor
     * @param deviceName Nome base do dispositivo (será: deviceName-MAC)
     */
    CMDCore(const char* deviceName = "CMD-Device");
    
    // ==================== INICIALIZAÇÃO ====================
    
    /**
     * Inicializa todos os módulos (WiFi, WebServer, MQTT, Auth)
     */
    void begin();
    
    /**
     * Deve ser chamado no loop() principal
     * Processa WiFi, WebServer e MQTT
     */
    void handle();
    
    // ==================== ROTAS CUSTOMIZADAS ====================
    
    /**
     * Adiciona rota GET customizada
     * @param uri Caminho da rota (ex: "/minha-pagina")
     * @param handler Função handler
     */
    void addRoute(const char* uri, WebServer::THandlerFunction handler);
    
    /**
     * Adiciona rota POST customizada
     */
    void addRoutePost(const char* uri, WebServer::THandlerFunction handler);
    
    // ==================== INFORMAÇÕES ====================
    
    bool isConnected();
    String getSSID();
    String getIP();
    int getRSSI();
    bool isAPMode();
    String getMacAddress();
    String getVersion();
    uint32_t getUptime();
    
    // ==================== FACTORY RESET ====================
    
    /**
     * Zera contador de boots após 10s de estabilidade
     * (chame no loop ou no handle - já é chamado automaticamente)
     */
    void clearBootCountAfterStableRun();
    
    // ==================== ACESSO PÚBLICO ====================
    
    CMDMqtt mqtt;          // Acesso direto ao MQTT
    CMDAuth auth;          // Acesso direto à autenticação
    CMDWebServer webServer;  // Acesso direto ao WebServer
    
private:
    String deviceName;
    CMDWiFi wifi;
    
    void printSystemInfo();
};

#endif // CMD_CORE_H