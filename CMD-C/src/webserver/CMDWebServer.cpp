#include "webserver/CMDWebServer.h"
#include "pages/CMDPages.h"
#include "config/CMDConfig.h"

CMDWebServer::CMDWebServer(CMDWiFi* wifiManager, CMDMqtt* mqttManager, CMDAuth* authManager) 
    : server(80), wifi(wifiManager), mqtt(mqttManager), auth(authManager), running(false), deviceRouteRegistered(false) {}

void CMDWebServer::begin() {
    if (running) {
        Serial.println(CMD_DEBUG_PREFIX "Servidor já está rodando!");
        return;
    }
    
    setupRoutes();
    server.begin();
    running = true;
    
    Serial.println(CMD_DEBUG_PREFIX "Servidor HTTP iniciado na porta 80");
}

void CMDWebServer::handle() {
    if (running) {
        server.handleClient();
    }
}

void CMDWebServer::setupRoutes() {
    // Rotas HTML
    server.on("/", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleRoot(); 
    });

    // ✅ REMOVIDO: NÃO registra /device aqui mais
    // Isso permite que módulos customizados registrem sua própria página
    // Se nenhum módulo registrar, handleNotFound() exibirá a página default
    
    server.on("/info", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleInfoPage(); 
    });
    
    server.on("/network", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleNetworkPage(); 
    });
    
    server.on("/mqtt", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleMqttPage(); 
    });
    
    server.on("/security", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleSecurityPage(); 
    });
    
    server.on("/scan", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleScan(); 
    });
    
    server.on("/connect", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleConnect(); 
    });
    
    server.on("/status", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleStatus(); 
    });
    
    server.on("/forget_wifi", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleForgetWiFi(); 
    });
    
    server.on("/wifi_info", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleWiFiInfo(); 
    });
    
    // API REST - WiFi
    server.on("/api/info", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiInfo(); 
    });
    
    server.on("/api/wifi", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiWiFi(); 
    });
    
    server.on("/api/wifi/scan", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiWiFiScan(); 
    });
    
    server.on("/api/wifi/connect", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiWiFiConnect(); 
    });
    
    server.on("/api/wifi/forget", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiWiFiForget(); 
    });
    
    server.on("/api/reboot", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiReboot(); 
    });
    
    server.on("/api/network/static", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiGetStaticIP(); 
    });
    
    server.on("/api/network/static", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiSetStaticIP(); 
    });
    
    server.on("/api/network/static", HTTP_DELETE, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiClearStaticIP(); 
    });
    
    // API REST - MQTT
    server.on("/api/mqtt/status", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttStatus(); 
    });
    
    server.on("/api/mqtt/config", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttConfig(); 
    });
    
    server.on("/api/mqtt/config", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttSaveConfig(); 
    });
    
    server.on("/api/mqtt/config", HTTP_DELETE, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttClearConfig(); 
    });
    
    server.on("/api/mqtt/connect", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttConnect(); 
    });
    
    server.on("/api/mqtt/disconnect", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttDisconnect(); 
    });
    
    server.on("/api/mqtt/publish", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttPublish(); 
    });
    
    server.on("/api/mqtt/messages", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiMqttMessages(); 
    });
    
    // API REST - Auth
    server.on("/api/auth/status", HTTP_GET, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiAuthStatus(); 
    });
    
    server.on("/api/auth/password", HTTP_POST, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiAuthSetPassword(); 
    });
    
    server.on("/api/auth/password", HTTP_DELETE, [this]() { 
        if (!auth->authenticate(server)) return;
        handleApiAuthClearPassword(); 
    });
    
    server.onNotFound([this]() { handleNotFound(); });
    
    Serial.println(CMD_DEBUG_PREFIX "Rotas registradas");
}

void CMDWebServer::addRoute(const char* uri, WebServer::THandlerFunction handler) {
    server.on(uri, HTTP_GET, handler);
    Serial.print(CMD_DEBUG_PREFIX "Rota customizada adicionada: ");
    Serial.println(uri);
}

void CMDWebServer::addRoutePost(const char* uri, WebServer::THandlerFunction handler) {
    server.on(uri, HTTP_POST, handler);
    Serial.print(CMD_DEBUG_PREFIX "Rota POST customizada adicionada: ");
    Serial.println(uri);
}

// ✅ NOVO: Método específico para registrar rota /device com autenticação integrada
void CMDWebServer::registerDeviceRoute(WebServer::THandlerFunction handler) {
    server.on("/device", HTTP_GET, [this, handler]() {
        if (!auth->authenticate(server)) return;
        handler();
    });
    deviceRouteRegistered = true;
    Serial.println(CMD_DEBUG_PREFIX "Rota customizada /device registrada pelo módulo");
}

bool CMDWebServer::isRunning() {
    return running;
}

// ==================== HANDLERS HTML ====================

void CMDWebServer::handleRoot() {
    Serial.println(CMD_DEBUG_PREFIX "Requisição em /");
    
    // Verifica se está conectado ao WiFi (modo STA)
    if (WiFi.status() == WL_CONNECTED) {
        sendDashboardPage();
    } else {
        sendConfigPage();
    }
}

void CMDWebServer::sendDashboardPage() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(HTML_DASHBOARD_INICIO);
    server.sendContent_P(HTML_DASHBOARD_BUTTONS);
    server.sendContent_P(HTML_DASHBOARD_FIM);
}

void CMDWebServer::sendConfigPage() {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent_P(HTML_INICIO);
    server.sendContent_P(HTML_ESTILOS);
    server.sendContent_P(HTML_BODY_ABRE);
    server.sendContent_P(HTML_CARD_SCAN);
    server.sendContent_P(HTML_CARD_FORM);
    server.sendContent_P(HTML_CARD_STATUS);
    server.sendContent_P(HTML_SCRIPT);
    server.sendContent_P(HTML_FIM);
}

void CMDWebServer::handleInfoPage() {
    server.send_P(200, "text/html", HTML_INFO_PAGE);
}

void CMDWebServer::handleNetworkPage() {
    server.send_P(200, "text/html", HTML_NETWORK_PAGE);
}

void CMDWebServer::handleMqttPage() {
    server.send_P(200, "text/html", HTML_MQTT_PAGE);
}

void CMDWebServer::handleSecurityPage() {
    server.send_P(200, "text/html", HTML_SECURITY_PAGE);
}

void CMDWebServer::handleScan() {
    Serial.println(CMD_DEBUG_PREFIX "Scan de redes solicitado");
    
    int n = scanNetworks();
    
    if (n == 0) {
        server.send(200, "application/json", "[]");
        return;
    }
    
    String json = "[";
    for (int i = 0; i < n; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN ? "true" : "false");
        json += "}";
    }
    json += "]";
    
    server.send(200, "application/json", json);
    WiFi.scanDelete();
}

void CMDWebServer::handleConnect() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");
    
    Serial.print(CMD_DEBUG_PREFIX "Tentativa de conexão: ");
    Serial.println(ssid);
    
    if (ssid.length() == 0) {
        server.send(400, "text/plain", "SSID vazio");
        return;
    }
    
    // Usa AP+STA para manter comunicação durante conexão
    WiFi.mode(WIFI_AP_STA);
    WiFi.disconnect(true, false);
    delay(100);
    
    // Aplica IP fixo se configurado
    if (wifi->hasStaticIP()) {
        Serial.println(CMD_DEBUG_PREFIX "Aplicando IP fixo...");
        IPAddress staticIP, gateway, subnet;
        staticIP.fromString(wifi->getStaticIP());
        gateway.fromString(wifi->getStaticGateway());
        subnet.fromString(wifi->getStaticSubnet());
        WiFi.config(staticIP, gateway, subnet);
    }
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000) {
        delay(250);
        yield();
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi->saveCredentials(ssid, pass);
        String ip = WiFi.localIP().toString();
        
        Serial.print(CMD_DEBUG_PREFIX "Conexão bem-sucedida! IP: ");
        Serial.println(ip);
        
        // Envia resposta COM o IP antes de fazer qualquer coisa
        server.send(200, "text/plain", "Conectado! IP: " + ip);
        
        // Pequeno delay para garantir envio da resposta
        delay(100);
        
        Serial.println(CMD_DEBUG_PREFIX "Resposta HTTP enviada. AP permanece ativo até reboot.");
        
    } else {
        server.send(200, "text/plain", "Falha ao conectar. Verifique a senha.");
        Serial.println(CMD_DEBUG_PREFIX "Falha na conexão");
        
        // Volta para modo AP puro se falhou
        WiFi.mode(WIFI_AP);
    }
}

void CMDWebServer::handleStatus() {
    if (WiFi.status() == WL_CONNECTED) {
        String ip = WiFi.localIP().toString();
        String json = "{\"state\":\"connected\",\"text\":\"Conectado em " 
                    + WiFi.SSID() + " | IP: " + ip + "\"}";
        server.send(200, "application/json", json);
    } else {
        server.send(200, "application/json", 
                    "{\"state\":\"idle\",\"text\":\"Aguardando conexão...\"}");
    }
}

void CMDWebServer::handleForgetWiFi() {
    Serial.println(CMD_DEBUG_PREFIX "Solicitação para esquecer WiFi");
    
    wifi->forgetCredentials();
    
    server.send(200, "text/plain", 
        "Credenciais apagadas!\nReinicie o dispositivo para reconfigurar.");
}

void CMDWebServer::handleWiFiInfo() {
    String json = "{";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI());
    json += "}";
    
    server.send(200, "application/json", json);
}

// ✅ MODIFICADO: Detecta /device e mostra página default SOMENTE se nenhum módulo registrou
void CMDWebServer::handleNotFound() {
    // Se a requisição é para /device e nenhum módulo registrou uma rota customizada
    if (server.uri() == "/device" && !deviceRouteRegistered) {
        Serial.println(CMD_DEBUG_PREFIX "Rota /device não registrada por módulo, exibindo página default");
        
        // Verifica autenticação
        if (!auth->authenticate(server)) return;
        
        // Mostra página default
        handleDeviceDefault();
        return;
    }
    
    // Para outras rotas, retorna 404 normal
    Serial.print(CMD_DEBUG_PREFIX "Rota não encontrada: ");
    Serial.println(server.uri());
    server.send(404, "text/plain", "404 - Página não encontrada");
}

// ==================== API REST - WiFi ====================

void CMDWebServer::handleApiInfo() {
    String json = "{";
    json += "\"version\":\"" CMD_VERSION "\",";
    json += "\"device\":\"" + wifi->getAPName() + "\",";
    json += "\"mac\":\"" + CMDWiFi::getMacSuffix() + "\",";
    json += "\"uptime\":" + String(millis() / 1000) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"chip_model\":\"" + String(ESP.getChipModel()) + "\",";
    json += "\"cpu_freq\":" + String(ESP.getCpuFreqMHz());
    json += "}";
    
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiWiFi() {
    String json = "{";
    json += "\"connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"ssid\":\"" + WiFi.SSID() + "\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
    json += "\"rssi\":" + String(WiFi.RSSI()) + ",";
    json += "\"mac\":\"" + WiFi.macAddress() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiWiFiScan() {
    handleScan(); // Reutiliza implementação
}

void CMDWebServer::handleApiWiFiConnect() {
    handleConnect(); // Reutiliza implementação
}

void CMDWebServer::handleApiWiFiForget() {
    wifi->forgetCredentials();
    server.send(200, "application/json", "{\"success\":true}");
}

void CMDWebServer::handleApiReboot() {
    Serial.println(CMD_DEBUG_PREFIX "Reiniciando por solicitação web...");
    
    // Envia resposta de sucesso
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Reiniciando...\"}");
    
    // Aguarda envio completo da resposta
    delay(500);
    
    Serial.println(CMD_DEBUG_PREFIX "Executando reboot...");
    ESP.restart();
}

void CMDWebServer::handleApiGetStaticIP() {
    String json = "{";
    json += "\"enabled\":" + String(wifi->hasStaticIP() ? "true" : "false") + ",";
    json += "\"ip\":\"" + wifi->getStaticIP() + "\",";
    json += "\"gateway\":\"" + wifi->getStaticGateway() + "\",";
    json += "\"subnet\":\"" + wifi->getStaticSubnet() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiSetStaticIP() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Body vazio\"}");
        return;
    }
    
    String body = server.arg("plain");
    
    // Parse JSON simples
    int ipStart = body.indexOf("\"ip\":\"") + 6;
    int ipEnd = body.indexOf("\"", ipStart);
    String ip = body.substring(ipStart, ipEnd);
    
    int gwStart = body.indexOf("\"gateway\":\"") + 11;
    int gwEnd = body.indexOf("\"", gwStart);
    String gateway = body.substring(gwStart, gwEnd);
    
    int snStart = body.indexOf("\"subnet\":\"") + 10;
    int snEnd = body.indexOf("\"", snStart);
    String subnet = body.substring(snStart, snEnd);
    
    Serial.print(CMD_DEBUG_PREFIX "Salvando IP fixo: ");
    Serial.println(ip);
    
    if (wifi->saveStaticIP(ip, gateway, subnet)) {
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"IP inválido\"}");
    }
}

void CMDWebServer::handleApiClearStaticIP() {
    Serial.println(CMD_DEBUG_PREFIX "Removendo IP fixo...");
    wifi->clearStaticIP();
    server.send(200, "application/json", "{\"success\":true}");
}

// ==================== API REST - MQTT ====================

void CMDWebServer::handleApiMqttStatus() {
    String json = "{";
    json += "\"configured\":" + String(mqtt->hasConfig() ? "true" : "false") + ",";
    json += "\"connected\":" + String(mqtt->isConnected() ? "true" : "false") + ",";
    json += "\"status\":\"" + mqtt->getStatusText() + "\",";
    json += "\"base_topic\":\"" + mqtt->getBaseTopic() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiMqttConfig() {
    if (!mqtt->hasConfig()) {
        server.send(200, "application/json", "{\"configured\":false}");
        return;
    }
    
    String json = "{";
    json += "\"configured\":true,";
    json += "\"broker\":\"" + mqtt->getBroker() + "\",";
    json += "\"port\":" + String(mqtt->getPort()) + ",";
    json += "\"user\":\"" + mqtt->getUser() + "\",";
    json += "\"client_id\":\"" + mqtt->getClientId() + "\",";
    json += "\"qos\":" + String(mqtt->getQoS()) + ",";
    json += "\"base_topic\":\"" + mqtt->getBaseTopic() + "\"";
    json += "}";
    
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiMqttSaveConfig() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Body vazio\"}");
        return;
    }
    
    String body = server.arg("plain");
    
    // Parse JSON simples
    int brokerStart = body.indexOf("\"broker\":\"") + 10;
    int brokerEnd = body.indexOf("\"", brokerStart);
    String broker = body.substring(brokerStart, brokerEnd);
    
    int portStart = body.indexOf("\"port\":") + 7;
    int portEnd = body.indexOf(",", portStart);
    if (portEnd == -1) portEnd = body.indexOf("}", portStart);
    uint16_t port = body.substring(portStart, portEnd).toInt();
    
    int userStart = body.indexOf("\"user\":\"") + 8;
    int userEnd = body.indexOf("\"", userStart);
    String user = (userStart > 7) ? body.substring(userStart, userEnd) : "";
    
    int passStart = body.indexOf("\"pass\":\"") + 8;
    int passEnd = body.indexOf("\"", passStart);
    String pass = (passStart > 7) ? body.substring(passStart, passEnd) : "";
    
    int clientStart = body.indexOf("\"client_id\":\"") + 13;
    int clientEnd = body.indexOf("\"", clientStart);
    String clientId = body.substring(clientStart, clientEnd);
    
    int qosStart = body.indexOf("\"qos\":") + 6;
    int qosEnd = body.indexOf(",", qosStart);
    if (qosEnd == -1) qosEnd = body.indexOf("}", qosStart);
    uint8_t qos = body.substring(qosStart, qosEnd).toInt();
    
    Serial.print(CMD_DEBUG_PREFIX "Salvando config MQTT: ");
    Serial.print(broker);
    Serial.print(":");
    Serial.println(port);
    
    if (mqtt->saveConfig(broker, port, user, pass, clientId, qos)) {
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Erro ao salvar\"}");
    }
}

void CMDWebServer::handleApiMqttClearConfig() {
    Serial.println(CMD_DEBUG_PREFIX "Removendo config MQTT...");
    mqtt->clearConfig();
    server.send(200, "application/json", "{\"success\":true}");
}

void CMDWebServer::handleApiMqttConnect() {
    if (!mqtt->hasConfig()) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"MQTT não configurado\"}");
        return;
    }
    
    bool success = mqtt->connect();
    String json = "{\"success\":" + String(success ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiMqttDisconnect() {
    mqtt->disconnect();
    server.send(200, "application/json", "{\"success\":true}");
}

void CMDWebServer::handleApiMqttPublish() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Body vazio\"}");
        return;
    }
    
    String body = server.arg("plain");
    
    int topicStart = body.indexOf("\"topic\":\"") + 9;
    int topicEnd = body.indexOf("\"", topicStart);
    String topic = body.substring(topicStart, topicEnd);
    
    int payloadStart = body.indexOf("\"payload\":\"") + 11;
    int payloadEnd = body.indexOf("\"", payloadStart);
    String payload = body.substring(payloadStart, payloadEnd);
    
    int retainStart = body.indexOf("\"retain\":") + 9;
    bool retain = false;
    if (retainStart > 8) {
        String retainStr = body.substring(retainStart, retainStart + 4);
        retain = (retainStr == "true");
    }
    
    bool success = mqtt->publish(topic, payload, retain);
    String json = "{\"success\":" + String(success ? "true" : "false") + "}";
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiMqttMessages() {
    int count;
    const CMDMqtt::Message* messages = mqtt->getLastMessages(count);
    
    String json = "{\"count\":" + String(count) + ",\"messages\":[";
    
    for (int i = 0; i < count; i++) {
        if (i > 0) json += ",";
        json += "{";
        json += "\"topic\":\"" + messages[i].topic + "\",";
        json += "\"payload\":\"" + messages[i].payload + "\",";
        json += "\"timestamp\":" + String(messages[i].timestamp);
        json += "}";
    }
    
    json += "]}";
    
    server.send(200, "application/json", json);
}

// ==================== API REST - Auth ====================

void CMDWebServer::handleApiAuthStatus() {
    String json = "{";
    json += "\"enabled\":" + String(auth->hasPassword() ? "true" : "false");
    json += "}";
    
    server.send(200, "application/json", json);
}

void CMDWebServer::handleApiAuthSetPassword() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Body vazio\"}");
        return;
    }
    
    String body = server.arg("plain");
    
    // Parse JSON simples
    int passStart = body.indexOf("\"password\":\"") + 12;
    int passEnd = body.indexOf("\"", passStart);
    String password = body.substring(passStart, passEnd);
    
    if (auth->setPassword(password)) {
        server.send(200, "application/json", "{\"success\":true}");
    } else {
        server.send(400, "application/json", "{\"success\":false,\"message\":\"Senha inválida (4-32 caracteres)\"}");
    }
}

void CMDWebServer::handleApiAuthClearPassword() {
    auth->clearPassword();
    server.send(200, "application/json", "{\"success\":true}");
}

// ==================== UTILITÁRIOS ====================

int CMDWebServer::scanNetworks() {
    Serial.println(CMD_DEBUG_PREFIX "Escaneando redes...");
    int n = WiFi.scanNetworks();
    Serial.printf(CMD_DEBUG_PREFIX "%d redes encontradas\n", n);
    return n;
}

// ==================== PÁGINA PADRÃO /DEVICE ====================

void CMDWebServer::handleDeviceDefault() {
    server.send_P(200, "text/html", HTML_DEVICE_DEFAULT);
}