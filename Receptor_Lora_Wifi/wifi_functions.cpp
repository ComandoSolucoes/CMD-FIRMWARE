#include "utils.h"
#include "config.h"
#include "wifi_functions.h"
#include "led_functions.h"
#include "globals.h"
#include "web_server.h"

#include <WiFi.h>
#include <Preferences.h>


void initWiFi() {
  loadWiFiCredentials();
  condicionalAP();
}

void loadWiFiCredentials() {
  savedSsid = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  haveSavedCreds = savedSsid.length() > 0;
  Serial.println("\n SSID:" + savedSsid + "\n PASS:" + savedPass + "\n haveSavedCreds: " + haveSavedCreds);
}

void condicionalAP() {
  if(haveSavedCreds) {
    Serial.println("Há dados de wifi salvo!");
    initWiFiConnection();
  } else {
    Serial.println("Sem dados wifi, inicializando AP");
    startAPServer();
  }
}

void initWiFiConnection() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(savedSsid.c_str(), savedPass.c_str());
    while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    pulseConnectingTick();
  }
  Serial.println("Conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

int scanWifi() {
  Serial.println("[SCAN] Iniciando escaneamento de redes...");
  
  int n = WiFi.scanNetworks();
  
  if (n == 0) {
    Serial.println("[SCAN] Nenhuma rede encontrada.");
  } else {
    Serial.printf("[SCAN] %d redes encontradas.\n", n);
  }
  
  return n; // Retorna apenas a quantidade
}

void startAPServer() {
  String ssid = "ESPLora-" + macSuffix();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(AP_IP, AP_GW, AP_MASK);
  bool ok = WiFi.softAP(ssid.c_str(), AP_PASSWORD);
  Serial.println(ok ? "[AP] OK" : "[AP] Falha");
  Serial.print("[AP] SSID: "); Serial.println(ssid);
  Serial.print("[AP] IP: ");   Serial.println(WiFi.softAPIP());
  apActive = true;
  showBaseByWifi();
}

bool salvarCredenciaisRede(const String& ssid, const String& pass) {
  // Grava no NVS
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);

  // Atualiza cache em RAM
  savedSsid = ssid;
  savedPass = pass;
  haveSavedCreds = ssid.length() > 0;

  // Evite imprimir a senha em produção
  Serial.println("[WiFi] Credenciais salvas (SSID: " + ssid + ").");


  return true;
}

bool esquecerCredenciaisWiFi() {
  Serial.println("[WiFi] Esquecendo credenciais...");
  
  // Apaga do NVS (memória não-volátil)
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
  
  // Limpa cache RAM
  savedSsid = "";
  savedPass = "";
  haveSavedCreds = false;
  
  // Desconecta do Wi-Fi
  WiFi.disconnect(true, true);
  delay(100);
  
  Serial.println("[WiFi] Credenciais apagadas com sucesso!");
  
  delay(500);
  ledOff();
  
  return true;
}