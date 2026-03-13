#include "CMDWiFi.h"

#ifdef ESP32
  #include <esp_system.h>
  #include <esp_timer.h>
#else
  #include <LittleFS.h>
  #include "utils/CMDPreferences.h"
#endif

CMDWiFi::CMDWiFi() : credentials(false), apActive(false), useStaticIP(false) {}

void CMDWiFi::begin(const char* deviceName) {
    apName = String(deviceName) + "-" + getMacSuffix();
    
    Serial.println(CMD_DEBUG_PREFIX "=================================");
    Serial.println(CMD_DEBUG_PREFIX "CMD-C WiFi Manager v" CMD_VERSION);
    Serial.println(CMD_DEBUG_PREFIX "=================================");
    
    // Abre namespace de preferências
#ifdef ESP32
    // ESP32: usa namespace global definido em config
    prefs.begin(CMD_PREFS_NAMESPACE, false);
#else
    // ESP8266: WiFi em namespace próprio
    prefs.begin("wifi", false);
#endif
    
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
    savedSSID     = prefs.getString("ssid", "");
    savedPassword = prefs.getString("pass", "");
    credentials   = savedSSID.length() > 0;
    
    if (credentials) {
        Serial.print(CMD_DEBUG_PREFIX "SSID salvo: ");
        Serial.println(savedSSID);
    }
    
    return credentials;
}

bool CMDWiFi::saveCredentials(const String& ssid, const String& password) {
    Serial.println(CMD_DEBUG_PREFIX "Salvando credenciais WiFi...");
    
    // Marca flag ANTES de salvar para evitar contagem de boot
#ifdef ESP32
    prefs.putBool("ignore_next_boot", true);
#else
    // ESP8266: ignora contagem em namespace "cmd-c"
    CMDPreferences sysPrefs;
    sysPrefs.begin("cmd-c", false);
    sysPrefs.putBool("ignore_next_boot", true);
    sysPrefs.commit();
#endif
    
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    
#ifndef ESP32
    prefs.commit();
#endif
    
    savedSSID     = ssid;
    savedPassword = password;
    credentials   = true;
    
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
    
    savedSSID   = "";
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

#if defined(ESP8266)
    // ESP8266: compacta em uma única string
    String packed = ip + "|" + gateway + "|" + subnet;
    bool ok = true;
    ok &= prefs.putString("static_cfg", packed);
    ok &= prefs.putBool("use_static", true);
    prefs.commit();

    if (!ok) {
        Serial.println(CMD_DEBUG_PREFIX "ERRO: Sem espaço para salvar IP fixo");
        return false;
    }
#else
    // ESP32: mantém chaves separadas
    prefs.putString("static_ip", ip);
    prefs.putString("static_gw", gateway);
    prefs.putString("static_sn", subnet);
    prefs.putBool("use_static", true);
#endif
    
    staticIP      = ipAddr;
    staticGateway = gwAddr;
    staticSubnet  = snAddr;
    useStaticIP   = true;
    
    Serial.print(CMD_DEBUG_PREFIX "IP fixo salvo: ");
    Serial.println(ip);
    
    return true;
}

bool CMDWiFi::loadStaticIP() {
    useStaticIP = prefs.getBool("use_static", false);
    
    if (!useStaticIP) {
        return false;
    }

    String ip, gw, sn;

#if defined(ESP8266)
    String packed = prefs.getString("static_cfg", "");
    if (packed.length() == 0) {
        useStaticIP = false;
        return false;
    }

    int p1 = packed.indexOf('|');
    int p2 = packed.indexOf('|', p1 + 1);

    if (p1 < 0 || p2 < 0) {
        useStaticIP = false;
        return false;
    }

    ip = packed.substring(0, p1);
    gw = packed.substring(p1 + 1, p2);
    sn = packed.substring(p2 + 1);
#else
    ip = prefs.getString("static_ip", "");
    gw = prefs.getString("static_gw", "");
    sn = prefs.getString("static_sn", "");
#endif
    
    if (ip.length() == 0) {
        useStaticIP = false;
        return false;
    }
    
    staticIP      = parseIP(ip);
    staticGateway = parseIP(gw);
    staticSubnet  = parseIP(sn);
    
    Serial.print(CMD_DEBUG_PREFIX "IP fixo carregado: ");
    Serial.println(ip);
    
    return true;
}

bool CMDWiFi::clearStaticIP() {
    Serial.println(CMD_DEBUG_PREFIX "Removendo IP fixo...");
    
#if defined(ESP8266)
    prefs.remove("static_cfg");
#else
    prefs.remove("static_ip");
    prefs.remove("static_gw");
    prefs.remove("static_sn");
#endif
    prefs.putBool("use_static", false);
    
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

String CMDWiFi::getSSID()         { return savedSSID;     }
String CMDWiFi::getPassword()     { return savedPassword; }
String CMDWiFi::getAPName()       { return apName;        }
bool   CMDWiFi::isAPActive()      { return apActive;      }

// ==================== FACTORY RESET ====================

void CMDWiFi::handleBootCount() {
#ifdef ESP32
    bool ignoreThisBoot = prefs.getBool("ignore_next_boot", false);
#else
    CMDPreferences sysPrefs;
    sysPrefs.begin("cmd-c", false);
    bool ignoreThisBoot = sysPrefs.getBool("ignore_next_boot", false);
#endif
    
    if (ignoreThisBoot) {
        Serial.println(CMD_DEBUG_PREFIX "Boot pós-configuração WiFi - ignorando contagem");
#ifdef ESP32
        prefs.putBool("ignore_next_boot", false);
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
#else
        sysPrefs.putBool("ignore_next_boot", false);
        sysPrefs.putUChar("boot_count", 0);
        sysPrefs.putULong("boot_time", 0);
        sysPrefs.commit();
#endif
        return;
    }
    
#ifdef ESP32
    auto reason = esp_reset_reason();
    if (reason == ESP_RST_SW) {
#else
    String reasonStr = ESP.getResetReason();
    if (reasonStr.indexOf("Software") >= 0 || 
        reasonStr.indexOf("System")   >= 0 ||
        reasonStr.indexOf("restart")  >= 0) {
#endif
        Serial.print(CMD_DEBUG_PREFIX "Reset por software: ");
#ifndef ESP32
        Serial.println(reasonStr);
#endif
        Serial.println(CMD_DEBUG_PREFIX "Ignorando contagem de boot");
        
#ifdef ESP32
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
#else
        CMDPreferences sysPrefs2;
        sysPrefs2.begin("cmd-c", false);
        sysPrefs2.putUChar("boot_count", 0);
        sysPrefs2.putULong("boot_time", 0);
        sysPrefs2.commit();
#endif
        
        return;
    }
    
#ifdef ESP32
    uint8_t       bootCount     = prefs.getUChar("boot_count", 0);
    unsigned long lastBootTime  = prefs.getULong("boot_time", 0);
    uint64_t      currentTimeUs = esp_timer_get_time();
    uint64_t      currentTimeSec= currentTimeUs / 1000000;
#else
    CMDPreferences sysPrefs3;
    sysPrefs3.begin("cmd-c", false);
    uint8_t       bootCount     = sysPrefs3.getUChar("boot_count", 0);
    unsigned long lastBootTime  = sysPrefs3.getULong("boot_time", 0);
    unsigned long currentTimeSec= millis() / 1000;
#endif
    
    (void)lastBootTime; // se quiser usar depois
    
    bootCount++;
    
    Serial.printf(CMD_DEBUG_PREFIX "Boot %d/%d detectado (reset: ", 
                  bootCount, CMD_BOOT_COUNT_THRESHOLD);
#ifndef ESP32
    Serial.print(ESP.getResetReason());
#endif
    Serial.println(")");
    
#ifdef ESP32
    prefs.putUChar("boot_count", bootCount);
    prefs.putULong("boot_time", (unsigned long)currentTimeSec);
#else
    sysPrefs3.putUChar("boot_count", bootCount);
    sysPrefs3.putULong("boot_time", currentTimeSec);
    sysPrefs3.commit();
#endif
    
    if (bootCount >= CMD_BOOT_COUNT_THRESHOLD) {
        executeFactoryReset();
    }
}

void CMDWiFi::clearBootCountAfterStableRun() {
    static bool cleared = false;
    if (!cleared && millis() > CMD_BOOT_CLEAR_MS) {

#ifdef ESP32
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
#else
        CMDPreferences sysPrefs;
        sysPrefs.begin("cmd-c", false);
        sysPrefs.putUChar("boot_count", 0);
        sysPrefs.putULong("boot_time", 0);
        sysPrefs.commit();
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
    
#ifdef ESP32
    prefs.putUChar("boot_count", 0);
    prefs.putULong("boot_time", 0);
    prefs.clear();
#else
    // ESP8266: limpa todos os namespaces relevantes
    CMDPreferences sysPrefs;
    sysPrefs.begin("cmd-c", false);
    sysPrefs.clear();
    sysPrefs.commit();
    
    CMDPreferences wifiPrefs;
    wifiPrefs.begin("wifi", false);
    wifiPrefs.clear();
    wifiPrefs.commit();
    
    CMDPreferences authPrefs;
    authPrefs.begin("auth", false);
    authPrefs.clear();
    authPrefs.commit();
    
    CMDPreferences mqttPrefs;
    mqttPrefs.begin("cmd-mqtt", false);
    mqttPrefs.clear();
    mqttPrefs.commit();
#endif
    
#ifdef ESP32
    WiFi.disconnect(true, true);
#else
    WiFi.disconnect(true);
    
    Serial.println(CMD_DEBUG_PREFIX "Formatando LittleFS...");
    LittleFS.format();
#endif
    
    savedSSID   = "";
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
