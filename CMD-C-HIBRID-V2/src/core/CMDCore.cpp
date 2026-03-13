#include "CMDCore.h"

CMDCore::CMDCore(const char* deviceName) 
    : deviceName(deviceName), webServer(&wifi, &mqtt, &auth) {}

void CMDCore::begin() {
    Serial.begin(115200);
    delay(100);
    
    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║         CMD-C CORE LIBRARY             ║");
    Serial.println("║            Version " CMD_VERSION "                ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();
    
    printSystemInfo();
    
    // Inicializa WiFi
    wifi.begin(deviceName.c_str());
    
    // Inicializa Auth
    auth.begin();
    
    // Inicializa MQTT
    String macSuffix = CMDWiFi::getMacSuffix();
    mqtt.begin(macSuffix.c_str());
    
    // Inicia servidor web
    webServer.begin();
    
    Serial.println();
    Serial.println(CMD_DEBUG_PREFIX "✅ CMD-C inicializado com sucesso!");
    Serial.println(CMD_DEBUG_PREFIX "Dispositivo: " + deviceName);
    
    if (isAPMode()) {
        Serial.println(CMD_DEBUG_PREFIX "🔧 Modo AP - Configure o WiFi em: http://" + getIP());
    } else {
        Serial.println(CMD_DEBUG_PREFIX "🌐 Modo STA - Conectado em: " + getSSID());
        Serial.println(CMD_DEBUG_PREFIX "🌐 Acesse: http://" + getIP());
    }
    
    if (mqtt.hasConfig()) {
        Serial.println(CMD_DEBUG_PREFIX "📡 MQTT configurado - Base topic: " + mqtt.getBaseTopic());
    } else {
        Serial.println(CMD_DEBUG_PREFIX "📡 MQTT não configurado");
    }
    
    if (auth.hasPassword()) {
        Serial.println(CMD_DEBUG_PREFIX "🔐 Autenticação: Habilitada (user: admin)");
    } else {
        Serial.println(CMD_DEBUG_PREFIX "🔓 Autenticação: Desabilitada (configure em /security)");
    }
    
    Serial.println();
}

void CMDCore::handle() {
    webServer.handle();
    wifi.clearBootCountAfterStableRun();
    mqtt.handle();
}

void CMDCore::addRoute(const char* uri, WebServer::THandlerFunction handler) {
    webServer.addRoute(uri, handler);
}

void CMDCore::addRoutePost(const char* uri, WebServer::THandlerFunction handler) {
    webServer.addRoutePost(uri, handler);
}

bool CMDCore::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String CMDCore::getSSID() {
    return WiFi.SSID();
}

String CMDCore::getIP() {
    if (isAPMode()) {
        return WiFi.softAPIP().toString();
    }
    return WiFi.localIP().toString();
}

int CMDCore::getRSSI() {
    return WiFi.RSSI();
}

bool CMDCore::isAPMode() {
    return wifi.isAPActive();
}

String CMDCore::getMacAddress() {
    return CMDWiFi::getMacSuffix();
}

String CMDCore::getVersion() {
    return String(CMD_VERSION);
}

uint32_t CMDCore::getUptime() {
    return millis() / 1000;
}

void CMDCore::clearBootCountAfterStableRun() {
    wifi.clearBootCountAfterStableRun();
}

void CMDCore::printSystemInfo() {
    Serial.println(CMD_DEBUG_PREFIX "===== Informações do Sistema =====");
    
    // ✅ ESP8266: Funções de chip info são diferentes
    #ifdef ESP32
        Serial.print(CMD_DEBUG_PREFIX "Chip: ");
        Serial.println(ESP.getChipModel());
        Serial.print(CMD_DEBUG_PREFIX "Cores: ");
        Serial.println(ESP.getChipCores());
    #else
        // ESP8266: getChipId() retorna o ID único do chip
        Serial.print(CMD_DEBUG_PREFIX "Chip: ESP8266 (ID: 0x");
        Serial.print(ESP.getChipId(), HEX);
        Serial.println(")");
        Serial.print(CMD_DEBUG_PREFIX "Core: ");
        Serial.println(ESP.getCoreVersion());
    #endif
    
    Serial.print(CMD_DEBUG_PREFIX "CPU: ");
    Serial.print(ESP.getCpuFreqMHz());
    Serial.println(" MHz");
    
    // ✅ ESP8266: getFlashChipSize() vs getFlashChipRealSize()
    #ifdef ESP32
        Serial.print(CMD_DEBUG_PREFIX "Flash: ");
        Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
        Serial.println(" MB");
    #else
        Serial.print(CMD_DEBUG_PREFIX "Flash: ");
        Serial.print(ESP.getFlashChipRealSize() / 1024 / 1024);
        Serial.println(" MB");
    #endif
    
    Serial.print(CMD_DEBUG_PREFIX "RAM Livre: ");
    Serial.print(ESP.getFreeHeap() / 1024);
    Serial.println(" KB");
    Serial.print(CMD_DEBUG_PREFIX "MAC: ");
    Serial.println(WiFi.macAddress());
    Serial.println(CMD_DEBUG_PREFIX "==================================");
}