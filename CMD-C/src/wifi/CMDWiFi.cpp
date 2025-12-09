#include "CMDWiFi.h"
#include <esp_system.h>
#include <esp_timer.h>

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
    prefs.putString("ssid", ssid);
    prefs.putString("pass", password);
    
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
    
    savedSSID = "";
    savedPassword = "";
    credentials = false;
    useStaticIP = false;
    
    WiFi.disconnect(true, true);
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
    
    // Valida IPs
    IPAddress ipAddr = parseIP(ip);
    IPAddress gwAddr = parseIP(gateway);
    IPAddress snAddr = parseIP(subnet);
    
    if (ipAddr == IPAddress(0,0,0,0) || gwAddr == IPAddress(0,0,0,0) || snAddr == IPAddress(0,0,0,0)) {
        Serial.println(CMD_DEBUG_PREFIX "IP inválido!");
        return false;
    }
    
    // Salva no NVS
    prefs.putString("static_ip", ip);
    prefs.putString("static_gw", gateway);
    prefs.putString("static_sn", subnet);
    prefs.putBool("use_static", true);
    
    // Atualiza variáveis
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
    // ✅ Modo STA puro no boot (AP não será iniciado)
    WiFi.mode(WIFI_STA);
    
    // Aplica IP fixo se configurado
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
    auto reason = esp_reset_reason();
    
    // Ignora resets por software (reboot via código)
    if (reason == ESP_RST_SW) {
        Serial.println(CMD_DEBUG_PREFIX "Reset por software - ignorando contagem");
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
        return;
    }
    
    // Lê contador e timestamp do último boot do NVS
    uint8_t bootCount = prefs.getUChar("boot_count", 0);
    unsigned long lastBootTime = prefs.getULong("boot_time", 0);
    unsigned long currentTime = millis() / 1000; // Tempo atual em segundos desde epoch
    
    // Pega o tempo Unix atual (segundos desde 1970) - mas ESP32 sem RTC usa millis
    // Vamos usar esp_timer_get_time() que é mais confiável
    uint64_t currentTimeUs = esp_timer_get_time(); // microsegundos desde boot
    uint64_t currentTimeSec = currentTimeUs / 1000000; // segundos
    
    // Como não temos RTC, vamos usar uma abordagem diferente:
    // Se lastBootTime == 0, é primeiro boot
    // Incrementamos sempre e salvamos
    // clearBootCountAfterStableRun() vai zerar após 10 segundos
    
    bootCount++;
    
    Serial.printf(CMD_DEBUG_PREFIX "Boot detectado! Contador: %d/%d\n", 
                  bootCount, CMD_BOOT_COUNT_THRESHOLD);
    
    // Salva contador no NVS
    prefs.putUChar("boot_count", bootCount);
    prefs.putULong("boot_time", currentTimeSec);
    
    // Se atingiu o threshold, executa factory reset
    if (bootCount >= CMD_BOOT_COUNT_THRESHOLD) {
        executeFactoryReset();
    }
}

void CMDWiFi::clearBootCountAfterStableRun() {
    static bool cleared = false;
    if (!cleared && millis() > CMD_BOOT_CLEAR_MS) {
        prefs.putUChar("boot_count", 0);
        prefs.putULong("boot_time", 0);
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
    
    // Limpa contador do NVS
    prefs.putUChar("boot_count", 0);
    prefs.putULong("boot_time", 0);
    
    // ✅ Limpa TODAS as configurações (WiFi, IP fixo, MQTT, SENHA, etc)
    prefs.clear();
    WiFi.disconnect(true, true);
    
    savedSSID = "";
    savedPassword = "";
    credentials = false;
    useStaticIP = false;
    
    Serial.println(CMD_DEBUG_PREFIX "Todas as configurações apagadas!");
    Serial.println(CMD_DEBUG_PREFIX "  - WiFi");
    Serial.println(CMD_DEBUG_PREFIX "  - IP Fixo");
    Serial.println(CMD_DEBUG_PREFIX "  - MQTT");
    Serial.println(CMD_DEBUG_PREFIX "  - Senha Web");  // ✅ NOVO
    Serial.println(CMD_DEBUG_PREFIX "Reiniciando em modo AP...");
    
    delay(1000);
    ESP.restart();
}

String CMDWiFi::getMacSuffix() {
    uint64_t mac = ESP.getEfuseMac();
    char buf[7];
    snprintf(buf, sizeof(buf), "%02X%02X%02X",
             (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
    return String(buf);
}