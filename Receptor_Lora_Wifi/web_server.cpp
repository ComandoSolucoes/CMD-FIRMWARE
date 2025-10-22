#include "web_server.h"
#include "globals.h"
#include "paginas_Html.h"
#include "wifi_functions.h"
#include <WiFi.h>

static bool rotasRegistradas = false;
static bool serverisON = false;

// Declarações das funções
static void handleRoot();
static void handleConnect();
static void handleStatus();
static void handleScan();
static void handleForgetWifi();
static void handleWifiInfo();
static void handleLoRaStream();

// Funções de renderização (apenas estruturam HTML)
static void renderPaginaConfig();
static void renderPaginaPainel();

static void garantirRotas() {
  if(rotasRegistradas) {
    Serial.println("Rotas ja registradas.");
    return;
  }

  server.on("/", HTTP_GET, handleRoot);              // ← Rota inteligente
  server.on("/dashboard", HTTP_GET, paginaPainel);   
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/scan", HTTP_GET, handleScan);
  server.on("/forget_wifi", HTTP_POST, handleForgetWifi);
  server.on("/wifi_info", HTTP_GET, handleWifiInfo);
  server.on("/lora_stream", HTTP_GET, handleLoRaStream);

  rotasRegistradas = true;
  Serial.println("Rotas registradas.");
}

void iniciarServidorWeb() {
  if(serverisON) {
    Serial.println("Servidor http ja iniciado!");
    return;
  }
  garantirRotas();
  server.begin();
  serverisON = true;
  Serial.println("Servidor http inicializado.");
}

void processarServidorWeb() {
  server.handleClient();
}

void pararServidorWeb() {
  server.stop();
}

// ========== ROTEADOR INTELIGENTE ==========
// Decide qual página renderizar baseado no estado do Wi-Fi
static void handleRoot() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[WEB] Requisição em / - redirecionando para /dashboard");
    server.sendHeader("Location", "/dashboard");
    server.send(302, "text/plain", "Redirecionando para painel...");
  } else {
    Serial.println("[WEB] Requisição em / - exibindo página de configuração");
    renderPaginaConfig();
  }
}

// ========== RENDERIZAÇÃO: PÁGINA DE CONFIGURAÇÃO ==========
static void renderPaginaConfig() {
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

// ========== RENDERIZAÇÃO: PAINEL PRINCIPAL ==========
void paginaPainel() {
  // Pode adicionar verificação aqui se quiser
  if (WiFi.status() != WL_CONNECTED) {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "Não conectado, redirecionando...");
    return;
  }
  
  renderPaginaPainel();
}

static void renderPaginaPainel() {
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  server.sendContent_P(HTML_PAINEL_INICIO);
  server.sendContent_P(HTML_PAINEL_ESTILOS);
  server.sendContent_P(HTML_PAINEL_BODY);
  server.sendContent_P(HTML_PAINEL_SCRIPT);
  server.sendContent_P(HTML_PAINEL_FIM);
}

// ========== HANDLERS DE API ==========

static void handleScan() {
  int n = scanWifi();
  
  if (n == 0) {
    server.send(200, "application/json", "[]");
    WiFi.scanDelete();
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

static void handleConnect() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  if (ssid.length() == 0) {
    server.send(400, "text/plain", "SSID vazio");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);
  WiFi.begin(ssid.c_str(), pass.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < 15000UL) {
    delay(250);
    yield();
  }

  if (WiFi.status() == WL_CONNECTED) {
    salvarCredenciaisRede(ssid, pass);
    String ip = WiFi.localIP().toString();
    server.send(200, "text/plain", "Conectado! IP: " + ip + " (credenciais salvas)");
  } else {
    server.send(200, "text/plain", "Falha ao conectar (timeout/credenciais).");
  }
}

static void handleStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    String ip = WiFi.localIP().toString();
    String json = String("{\"state\":\"connected\",\"text\":\"Conectado em ")
                + WiFi.SSID() + " | IP: " + ip + "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(200, "application/json",
                "{\"state\":\"idle\",\"text\":\"Aguardando conexao...\"}");
  }
}

static void handleForgetWifi() {
  bool sucesso = esquecerCredenciaisWiFi();
  
  if (sucesso) {
    Serial.println("[WEB] Credenciais Wi-Fi apagadas via interface web");
    server.send(200, "text/plain", 
      "✅ Credenciais apagadas com sucesso!\n\n"
      "Reinicie o dispositivo para entrar em modo AP e reconfigurar o Wi-Fi.");
  } else {
    Serial.println("[WEB] Erro ao apagar credenciais");
    server.send(500, "text/plain", 
      "❌ Erro ao apagar credenciais.\n\n"
      "Tente novamente ou reinicie o dispositivo.");
  }
}

static void handleWifiInfo() {
  String json = "{";
  json += "\"ssid\":\"" + WiFi.SSID() + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"rssi\":" + String(WiFi.RSSI());
  json += "}";
  
  server.send(200, "application/json", json);
}

static void handleLoRaStream() {
  WiFiClient client = server.client();
  
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/event-stream");
  client.println("Cache-Control: no-cache");
  client.println("Connection: keep-alive");
  client.println();
  
  // TODO: Implementar envio de dados LoRa
  // Exemplo:
  // String payload = "data: {\"time\":\"12:34:56\",\"payload\":\"Hello\",\"rssi\":-45,\"snr\":8.5}\n\n";
  // client.print(payload);
  // client.flush();
}