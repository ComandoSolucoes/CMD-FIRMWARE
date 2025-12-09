#include "CMDWiFi.h"

#ifdef ESP32
  #include <esp_system.h>
  #include <esp_timer.h>
#else
  #include <LittleFS.h>
#endif

CMDWiFi::CMDWiFi() : credentials(false), apActive(false), useStaticIP(false) {}

void CMDWiFi::begin(const char* deviceName) {
    apName = String(deviceName) + "-" + getMacSuffix();
    
    Serial.println(CMD_DEBUG_PREFIX "=================================");
    Serial.println(CMD_DEBUG_PREFIX "CMD-C WiFi Manager v" CMD_VERSION);
    Serial.println(CMD_DEBUG_PREFIX "=================================");
    
    // Abre namespace de preferências
    prefs.begin(CMD_PREFS_NAMESPACE, false);
    
    // Verifica factory reset por boot rápido
    handleBootCount();
    
    // Carrega credenciais e IP fixo
    loadCredentials();
    loadStaticIP();
    
    // Decide: AP ou STA
    if (hasCredentials()) {
        Serial.println(CMD_DEBUG_PREFIX "Credenciais encontradas. Conectando...");
        if (useStaticIP) {
            Serial.println(CMD_DEBUG_PREFIX "IP fixo configurado");
        }
        connectSTA();
    } else {
        Serial.println(CMD_DEBUG_PREFIX "Sem credenciais. Iniciando AP...");
        startAP(deviceName);
    }
}

// ==================== CREDENCIAIS ====================

bool CMDWiFi::loadCredentials() {
    savedSSID = prefs.getString("ssid", "");
    savedPassword = prefs.getString("pass", "");
    credentials = savedSSID.length() > 0;
    
    if (credentials) {
        Serial.print(CMD_DEBUG_PREFIX "SSID salvo: ");
        Serial.println(savedSSID);
    }
    
    return credentials;
}

bool CMDWiFi::saveCredentials(const String& ssid, const String& password) {
    // ✅ Marca flag ANTES de salvar para evitar contagem de boot
    prefs.putBool("ignore_next_boot", true);
    
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    
    // ✅ Commit em batch (uma vez só)
    #ifndef ESP32
        prefs.commit();
    #endif
    
    savedSSID = ssid;
    savedPassword = password;
    credentials = true;
    
    Serial.print(CMD_DEBUG_PREFIX "Credenciais salvas: ");
    Serial.println(ssid);
    
    return true;
}

bool CMDWiFi::forgetCredentials() {
    Serial.println(CMD_DEBUG_PREFIX "Apagando credenciais...");
    
    prefs.clear();
    
    #ifndef ESP32
        prefs.commit();
    #endif
    
    savedSSID = "";
    savedPassword = "";
    credentials = false;
    useStaticIP = false;
    
    #ifdef ESP32
        WiFi.disconnect(true, true);
    #else
        WiFi.disconnect(true);
    #endif
    
    delay(100);
    
    Serial.println(CMD_DEBUG_PREFIX "Credenciais apagadas!");
    
    return true;
}

bool CMDWiFi::hasCredentials() {
    return credentials;
}

// ==================== IP FIXO ====================

bool CMDWiFi::saveStaticIP(const String& ip, const String& gateway, const String& subnet) {
    Serial.println(CMD_DEBUG_PREFIX "Salvando IP fixo...");
    
    IPAddress ipAddr = parseIP(ip);
    IPAddress gwAddr = parseIP(gateway);
    IPAddress snAddr = parseIP(subnet);
    
    if (ipAddr == IPAddress(0,0,0,0) || gwAddr == IPAddress(0,0,0,0) || snAddr == IPAddress(0,0,0,0)) {
        Serial.println(CMD_DEBUG_PREFIX "IP inválido!");
        return false;
    }
    
    prefs.putString("static_ip", ip);
    prefs.putString("static_gw", gateway);
    prefs.putString("static_sn", subnet);
    prefs.putBool("use_static", true);
    
    // ✅ Commit em batch
    #ifndef ESP32
        prefs.commit();
    #endif
    
    staticIP = ipAddr;
    staticGateway = gwAddr;
    staticSubnet = snAddr;
    useStaticIP = true;
    
    Serial.print(CMD_DEBUG_PREFIX "IP fixo salvo: ");
    Serial.println(ip);
    
    return true;
}

bool CMDWiFi::loadStaticIP() {
    useStaticIP = prefs.getBool("use_static", false);
    
    if (!useStaticIP) {
        return false;
    }
    
    String ip = prefs.getString("static_ip", "");
    String gw = prefs.getString("static_gw", "");
    String sn = prefs.getString("static_sn", "");
    
    if (ip.length() == 0) {
        useStaticIP = false;
        return false;
    }
    
    staticIP = parseIP(ip);
    staticGateway = parseIP(gw);
    staticSubnet = parseIP(sn);
    
    Serial.print(CMD_DEBUG_PREFIX "IP fixo carregado: ");
    Serial.println(ip);
    
    return true;
}

bool CMDWiFi::clearStaticIP() {
    Serial.println(CMD_DEBUG_PREFIX "Removendo IP fixo...");
    
    prefs.remove("static_ip");
    prefs.remove("static_gw");
    prefs.remove("static_sn");
    prefs.putBool("use_static", false);
    
    // ✅ Commit em batch
    #ifndef ESP32
        prefs.commit();
    #endif
    
    useStaticIP = false;
    
    Serial.println(CMD_DEBUG_PREFIX "IP fixo removido. Voltando para DHCP.");
    
    return true;
}

bool CMDWiFi::hasStaticIP() {
    return useStaticIP;
}

String CMDWiFi::getStaticIP() {
    if (!useStaticIP) return "";
    return staticIP.toString();
}

String CMDWiFi::getStaticGateway() {
    if (!useStaticIP) return "";
    return staticGateway.toString();
}

String CMDWiFi::getStaticSubnet() {
    if (!useStaticIP) return "";
    return staticSubnet.toString();
}

IPAddress CMDWiFi::parseIP(const String& ipStr) {
    IPAddress ip;
    if (!ip.fromString(ipStr)) {
        return IPAddress(0, 0, 0, 0);
    }
    return ip;
}

// ==================== CONEXÃO ====================

void CMDWiFi::startAP(const char* deviceName) {
    IPAddress apIP(CMD_AP_IP_1, CMD_AP_IP_2, CMD_AP_IP_3, CMD_AP_IP_4);
    IPAddress gateway(CMD_AP_IP_1, CMD_AP_IP_2, CMD_AP_IP_3, CMD_AP_IP_4);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, gateway, subnet);
    
    bool success = WiFi.softAP(apName.c_str(), CMD_AP_PASSWORD);
    
    if (success) {
        apActive = true;
        Serial.println(CMD_DEBUG_PREFIX "AP iniciado com sucesso!");
        Serial.print(CMD_DEBUG_PREFIX "SSID: ");
        Serial.println(apName);
        Serial.print(CMD_DEBUG_PREFIX "Senha: " CMD_AP_PASSWORD);
        Serial.println();
        Serial.print(CMD_DEBUG_PREFIX "IP: ");
        Serial.println(WiFi.softAPIP());
    } else {
        Serial.println(CMD_DEBUG_PREFIX "Falha ao iniciar AP!");
    }
}

void CMDWiFi::connectSTA() {
    WiFi.mode(WIFI_STA);
    
    if (useStaticIP) {
        Serial.println(CMD_DEBUG_PREFIX "Aplicando IP fixo...");
        if (!WiFi.config(staticIP, staticGateway, staticSubnet)) {
            Serial.println(CMD_DEBUG_PREFIX "Falha ao aplicar IP fixo!");
            useStaticIP = false;
        }
    }
    
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
    
    Serial.print(CMD_DEBUG_PREFIX "Conectando");
    
    unsigned long startAttempt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
        apActive = false;
        Serial.println(CMD_DEBUG_PREFIX "WiFi conectado!");
        Serial.print(CMD_DEBUG_PREFIX "IP: ");
        Serial.println(WiFi.localIP());
        Serial.print(CMD_DEBUG_PREFIX "RSSI: ");
        Serial.print(WiFi.RSSI());
        Serial.println(" dBm");
        
        if (useStaticIP) {
            Serial.println(CMD_DEBUG_PREFIX "Modo: IP Fixo");
        } else {
            Serial.println(CMD_DEBUG_PREFIX "Modo: DHCP");
        }
    } else {
        Serial.println(CMD_DEBUG_PREFIX "Falha na conexão. Iniciando AP...");
        startAP(apName.c_str());
    }
}

String CMDWiFi::getSSID() {
    return savedSSID;
}

String CMDWiFi::getPassword() {
    return savedPassword;
}

String CMDWiFi::getAPName() {
    return apName;
}

bool CMDWiFi::isAPActive() {
    return apActive;
}

// ==================== FACTORY RESET ====================

void CMDWiFi::handleBootCount() {
    // ✅ NOVO: Verifica flag de ignorar boot (após salvar credenciais WiFi)
    bool ignoreThisBoot = prefs.getBool("ignore_next_boot", false);
    
    if (ignoreThisBoot) {
        Serial.println(CMD_DEBUG_PREFIX "Boot pós-configuração WiFi - ignorando contagem");
        prefs.putBool("ignore_next_boot", false);
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
        
        #ifndef ESP32
            prefs.commit();
        #endif
        
        return;
    }
    
    // ✅ Melhor detecção de software reset no ESP8266
    #ifdef ESP32
        auto reason = esp_reset_reason();
        if (reason == ESP_RST_SW) {
    #else
        String reasonStr = ESP.getResetReason();
        // ESP8266: Aceita múltiplos tipos de reset por software
        if (reasonStr.indexOf("Software") >= 0 || 
            reasonStr.indexOf("System") >= 0 ||
            reasonStr.indexOf("restart") >= 0) {
    #endif
        Serial.print(CMD_DEBUG_PREFIX "Reset por software: ");
        #ifndef ESP32
            Serial.println(reasonStr);
        #endif
        Serial.println(CMD_DEBUG_PREFIX "Ignorando contagem de boot");
        
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
        
        #ifndef ESP32
            prefs.commit();
        #endif
        
        return;
    }
    
    uint8_t bootCount = prefs.getUChar("boot_count", 0);
    unsigned long lastBootTime = prefs.getULong("boot_time", 0);
    
    #ifdef ESP32
        uint64_t currentTimeUs = esp_timer_get_time();
        uint64_t currentTimeSec = currentTimeUs / 1000000;
    #else
        unsigned long currentTimeSec = millis() / 1000;
    #endif
    
    bootCount++;
    
    Serial.printf(CMD_DEBUG_PREFIX "Boot %d/%d detectado (reset: ", 
                  bootCount, CMD_BOOT_COUNT_THRESHOLD);
    #ifndef ESP32
        Serial.print(ESP.getResetReason());
    #endif
    Serial.println(")");
    
    prefs.putUChar("boot_count", bootCount);
    prefs.putULong("boot_time", currentTimeSec);
    
    #ifndef ESP32
        prefs.commit();
    #endif
    
    if (bootCount >= CMD_BOOT_COUNT_THRESHOLD) {
        executeFactoryReset();
    }
}

void CMDWiFi::clearBootCountAfterStableRun() {
    static bool cleared = false;
    if (!cleared && millis() > CMD_BOOT_CLEAR_MS) {
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
        
        #ifndef ESP32
            prefs.commit();
        #endif
        
        cleared = true;
        Serial.println(CMD_DEBUG_PREFIX "Sistema estável. Contador zerado.");
    }
}

void CMDWiFi::executeFactoryReset() {
    Serial.println();
    Serial.println(CMD_DEBUG_PREFIX "============================================");
    Serial.println(CMD_DEBUG_PREFIX "  FACTORY RESET ATIVADO");
    Serial.println(CMD_DEBUG_PREFIX "  6 BOOTS RÁPIDOS DETECTADOS");
    Serial.println(CMD_DEBUG_PREFIX "============================================");
    
    prefs.putUChar("boot_count", 0);
    prefs.putULong("boot_time", 0);
    
    prefs.clear();
    
    #ifndef ESP32
        prefs.commit();
    #endif
    
    #ifdef ESP32
        WiFi.disconnect(true, true);
    #else
        WiFi.disconnect(true);
        
        Serial.println(CMD_DEBUG_PREFIX "Formatando LittleFS...");
        LittleFS.format();
    #endif
    
    savedSSID = "";
    savedPassword = "";
    credentials = false;
    useStaticIP = false;
    
    Serial.println(CMD_DEBUG_PREFIX "Todas as configurações apagadas!");
    Serial.println(CMD_DEBUG_PREFIX "  - WiFi");
    Serial.println(CMD_DEBUG_PREFIX "  - IP Fixo");
    Serial.println(CMD_DEBUG_PREFIX "  - MQTT");
    Serial.println(CMD_DEBUG_PREFIX "  - Senha Web");
    Serial.println(CMD_DEBUG_PREFIX "Reiniciando em modo AP...");
    
    delay(1000);
    ESP.restart();
}

// ==================== UTILITÁRIOS ====================

String CMDWiFi::getMacSuffix() {
    #ifdef ESP32
        uint64_t mac = ESP.getEfuseMac();
        char buf[7];
        snprintf(buf, sizeof(buf), "%02X%02X%02X",
                 (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
        return String(buf);
    #else
        String mac = WiFi.macAddress();
        mac.replace(":", "");
        return mac.substring(6);
    #endif
}