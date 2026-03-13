#include <CMDCore.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "IOManager.h"
#include "MqttHandler.h"
#include "WebInterface.h"

// ==================== OBJETOS GLOBAIS ====================
CMDCore core("4CH-ETH");
IOManager ioManager;
MqttHandler mqttHandler(&ioManager);
WebInterface webInterface(&ioManager, &mqttHandler);

// ==================== CALLBACKS ====================

void onRelayChange(uint8_t relay, bool state) {
  mqttHandler.publishRelayState(relay);
  
  if (ioManager.getInitialState() == INITIAL_STATE_LAST) {
    ioManager.saveRelayStates();
  }
  
  LOG_INFOF("Relé %d: %s", relay + 1, state ? "ON" : "OFF");
}

void onInputChange(uint8_t input, bool state) {
  mqttHandler.publishInputState(input);
  LOG_INFOF("Entrada %d: %s", input + 1, state ? "HIGH" : "LOW");
}

// ==================== HANDLERS DA API REST ====================

// GET /api/device/status
void handleApiDeviceStatus() {
  JsonDocument doc;
  
  JsonArray relays = doc["relays"].to<JsonArray>();
  for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
    relays.add(ioManager.getRelayState(i) ? 1 : 0);
  }
  
  JsonArray inputs = doc["inputs"].to<JsonArray>();
  for (uint8_t i = 0; i < NUM_INPUTS; i++) {
    inputs.add(ioManager.getInputState(i) ? 1 : 0);
  }
  
  String response;
  serializeJson(doc, response);
  core.webServer.getServer().send(200, "application/json", response);
}

// POST /api/device/relay/{id}
void handleApiRelayControl() {
  String uri = core.webServer.getServer().uri();
  int relayId = -1;
  
  int lastSlash = uri.lastIndexOf('/');
  if (lastSlash != -1) {
    String idStr = uri.substring(lastSlash + 1);
    relayId = idStr.toInt();
  }
  
  if (relayId >= 0 && relayId < NUM_OUTPUTS) {
    ioManager.toggleRelay(relayId);
    core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
  } else {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
  }
}

// POST /api/device/relay/all
void handleApiRelaySetAll() {
  bool allOn = true;
  for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
    if (!ioManager.getRelayState(i)) {
      allOn = false;
      break;
    }
  }
  
  ioManager.setAllRelays(!allOn);
  core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
}

// GET /api/device/config
void handleApiGetConfig() {
  JsonDocument doc;
  
  doc["relay_logic"] = (uint8_t)ioManager.getRelayLogic();
  doc["initial_state"] = (uint8_t)ioManager.getInitialState();
  doc["publish_interval"] = mqttHandler.getPublishInterval();
  
  JsonArray inputs = doc["inputs"].to<JsonArray>();
  for (uint8_t i = 0; i < NUM_INPUTS; i++) {
    JsonObject input = inputs.add<JsonObject>();
    input["mode"] = (uint8_t)ioManager.getInputMode(i);
    input["debounce"] = ioManager.getDebounceTime(i);
    input["pulse"] = ioManager.getPulseTime(i);
  }
  
  String response;
  serializeJson(doc, response);
  core.webServer.getServer().send(200, "application/json", response);
}

// POST /api/device/config
void handleApiSaveConfig() {
  String body = core.webServer.getServer().arg("plain");
  JsonDocument doc;
  
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  if (doc.containsKey("relay_logic")) {
    ioManager.setRelayLogic((RelayLogic)doc["relay_logic"].as<uint8_t>());
  }
  
  if (doc.containsKey("initial_state")) {
    ioManager.setInitialState((InitialState)doc["initial_state"].as<uint8_t>());
  }
  
  if (doc.containsKey("publish_interval")) {
    mqttHandler.setPublishInterval(doc["publish_interval"].as<uint32_t>());
    mqttHandler.savePublishInterval();
  }
  
  if (doc.containsKey("inputs")) {
    JsonArray inputs = doc["inputs"].as<JsonArray>();
    for (uint8_t i = 0; i < NUM_INPUTS && i < inputs.size(); i++) {
      JsonObject input = inputs[i];
      
      if (input.containsKey("mode")) {
        ioManager.setInputMode(i, (InputMode)input["mode"].as<uint8_t>());
      }
      if (input.containsKey("debounce")) {
        ioManager.setDebounceTime(i, input["debounce"].as<uint16_t>());
      }
      if (input.containsKey("pulse")) {
        ioManager.setPulseTime(i, input["pulse"].as<uint16_t>());
      }
    }
  }
  
  ioManager.saveConfig();
  core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
}

// GET /device
void handleDevicePage() {
  String html = webInterface.generateDevicePage();
  core.webServer.getServer().send(200, "text/html", html);
}

// ==================== REGISTRO DE ROTAS ====================

void registerCustomRoutes() {
  // Página do dispositivo
  core.addRoute("/device", handleDevicePage);
  
  // APIs GET
  core.addRoute("/api/device/status", handleApiDeviceStatus);
  core.addRoute("/api/device/config", []() {
    if (core.webServer.getServer().method() == HTTP_GET) {
      handleApiGetConfig();
    } else if (core.webServer.getServer().method() == HTTP_POST) {
      handleApiSaveConfig();
    }
  });
  
  // APIs POST
  core.addRoutePost("/api/device/config", handleApiSaveConfig);
  core.addRoutePost("/api/device/relay/all", handleApiRelaySetAll);
  
  // Rotas dinâmicas de relés
  for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
    String path = "/api/device/relay/" + String(i);
    core.addRoutePost(path.c_str(), handleApiRelayControl);
  }
  
  LOG_INFO("Rotas customizadas registradas!");
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║         4 CANAIS + ETHERNET            ║");
  Serial.println("║         Firmware v" FIRMWARE_VERSION "                 ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Watchdog
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);
  LOG_INFO("Watchdog iniciado");
  
  // CMD-Core
  core.begin();
  
  // I/O Manager
  ioManager.begin();
  ioManager.setRelayChangeCallback(onRelayChange);
  ioManager.setInputChangeCallback(onInputChange);
  
  // MQTT Handler
  mqttHandler.begin(&core.mqtt);
  
  // Rotas customizadas
  registerCustomRoutes();
  
  // Publica estados iniciais
  if (core.mqtt.isConnected()) {
    mqttHandler.publishAllStates();
    mqttHandler.publishStatusJson();
  }
  
  Serial.println();
  Serial.println(LOG_PREFIX "════════════════════════════════════════");
  Serial.println(LOG_PREFIX "✅ Sistema inicializado!");
  Serial.println(LOG_PREFIX "🌐 Interface: http://" + core.getIP() + "/device");
  Serial.println(LOG_PREFIX "════════════════════════════════════════");
  Serial.println();
}

// ==================== LOOP ====================

void loop() {
  esp_task_wdt_reset();
  core.handle();
  ioManager.handle();
  mqttHandler.handle();
  delay(1);
}

// ==================== CALLBACKS ====================

void onRelayChange(uint8_t relay, bool state) {
  // Publica mudança no MQTT
  mqttHandler.publishRelayState(relay);
  
  // Salva estado se configurado para recuperar último estado
  if (ioManager.getInitialState() == INITIAL_STATE_LAST) {
    ioManager.saveRelayStates();
  }
  
  LOG_INFOF("Relé %d: %s", relay + 1, state ? "ON" : "OFF");
}

void onInputChange(uint8_t input, bool state) {
  // Publica mudança no MQTT
  mqttHandler.publishInputState(input);
  
  LOG_INFOF("Entrada %d: %s", input + 1, state ? "HIGH" : "LOW");
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║         4 CANAIS + ETHERNET            ║");
  Serial.println("║         Firmware v" FIRMWARE_VERSION "                 ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // Inicializa watchdog (30 segundos)
  esp_task_wdt_init(WATCHDOG_TIMEOUT_SEC, true);
  esp_task_wdt_add(NULL);
  LOG_INFO("Watchdog iniciado (30s)");
  
  // Inicializa CMD-Core (WiFi, WebServer, MQTT, Auth)
  core.begin();
  
  // Inicializa gerenciador de I/O
  ioManager.begin();
  
  // Configura callbacks
  ioManager.setRelayChangeCallback(onRelayChange);
  ioManager.setInputChangeCallback(onInputChange);
  
  // Inicializa handler MQTT
  mqttHandler.begin(&core.mqtt);
  
  // Registra rotas customizadas
  core.addRoute("/device", handleDevicePage);
  core.addRoute("/api/device/status", handleApiDeviceStatus);
  core.addRoute("/api/device/config", []() {
    if (core.webServer.server.method() == HTTP_GET) {
      handleApiGetConfig();
    } else if (core.webServer.server.method() == HTTP_POST) {
      handleApiSaveConfig();
    }
  });
  core.addRoute("/api/device/relay/all", handleApiRelaySetAll);
  
  // Rotas dinâmicas de relés (0-3)
  for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
    String path = "/api/device/relay/" + String(i);
    core.addRoute(path.c_str(), handleApiRelayControl);
  }
  
  // Publica estados iniciais no MQTT se conectado
  if (core.mqtt.isConnected()) {
    mqttHandler.publishAllStates();
    mqttHandler.publishStatusJson();
  }
  
  Serial.println();
  Serial.println(LOG_PREFIX "════════════════════════════════════════");
  Serial.println(LOG_PREFIX "Sistema inicializado com sucesso!");
  Serial.println(LOG_PREFIX "Acesse a interface em: http://" + core.getIP() + "/device");
  Serial.println(LOG_PREFIX "════════════════════════════════════════");
  Serial.println();
}

// ==================== LOOP ====================

void loop() {
  // Reset watchdog
  esp_task_wdt_reset();
  
  // Processa CMD-Core (WiFi + WebServer)
  core.handle();
  
  // Processa gerenciador de I/O
  ioManager.handle();
  
  // Processa handler MQTT
  mqttHandler.handle();
  
  // Pequeno delay para não travar a task
  delay(1);
}
