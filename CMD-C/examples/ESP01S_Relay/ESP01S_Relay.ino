#include <CMDCore.h>
#include <ArduinoJson.h>
#include "Config.h"
#include "RelayManager.h"
#include "MqttHandler.h"
#include "WebInterface.h"

// ==================== OBJETOS GLOBAIS ====================
CMDCore core(DEVICE_MODEL);
RelayManager relayManager;
MqttHandler mqttHandler(&relayManager);
WebInterface webInterface(&relayManager, &mqttHandler);

// ==================== CALLBACKS ====================

void onRelayChange(bool state) {
  mqttHandler.publishRelayState();
  LOG_INFOF("Relé: %s", state ? "ON" : "OFF");
}

// ==================== HANDLERS DA API REST ====================

// GET /api/device/status
void handleApiDeviceStatus() {
  JsonDocument doc;
  
  doc["relay"] = relayManager.getRelayState() ? 1 : 0;
  doc["relay_pin"] = relayManager.getRelayPin();
  doc["timer_active"] = relayManager.isTimerActive();
  doc["timer_remaining"] = relayManager.getTimerRemaining();
  doc["pulse_active"] = relayManager.isPulseActive();
  
  #if ENABLE_STATISTICS
  doc["total_on_time"] = relayManager.getTotalOnTime();
  doc["switch_count"] = relayManager.getSwitchCount();
  #endif
  
  doc["uptime"] = millis() / 1000;
  doc["free_heap"] = ESP.getFreeHeap();
  
  String response;
  serializeJson(doc, response);
  core.webServer.getServer().send(200, "application/json", response);
}

// POST /api/device/relay
void handleApiRelaySet() {
  String body = core.webServer.getServer().arg("plain");
  JsonDocument doc;
  
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  if (doc.containsKey("state")) {
    bool state = doc["state"].as<bool>();
    relayManager.setRelay(state);
    core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
  } else {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
  }
}

// POST /api/device/relay/toggle
void handleApiRelayToggle() {
  relayManager.toggleRelay();
  core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
}

// POST /api/device/pulse
void handleApiPulse() {
  String body = core.webServer.getServer().arg("plain");
  JsonDocument doc;
  
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  if (doc.containsKey("duration")) {
    uint32_t duration = doc["duration"].as<uint32_t>();
    relayManager.activatePulse(duration);
    core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
  } else {
    relayManager.activatePulse();
    core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
  }
}

// POST /api/device/timer
void handleApiTimer() {
  String body = core.webServer.getServer().arg("plain");
  JsonDocument doc;
  
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
    return;
  }
  
  if (doc.containsKey("seconds")) {
    uint32_t seconds = doc["seconds"].as<uint32_t>();
    relayManager.startTimer(seconds);
    core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
  } else {
    core.webServer.getServer().send(400, "application/json", "{\"success\":false}");
  }
}

// GET /api/device/config
void handleApiGetConfig() {
  JsonDocument doc;
  
  doc["relay_pin"] = relayManager.getRelayPin();
  doc["relay_logic"] = (uint8_t)relayManager.getRelayLogic();
  doc["initial_state"] = (uint8_t)relayManager.getInitialState();
  doc["pulse_ms"] = relayManager.getPulseMs();
  doc["timer_seconds"] = relayManager.getTimerSeconds();
  doc["publish_interval"] = mqttHandler.getPublishInterval();
  
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
  
  bool needRestart = false;
  
  if (doc.containsKey("relay_pin")) {
    uint8_t newPin = doc["relay_pin"].as<uint8_t>();
    if (newPin != relayManager.getRelayPin()) {
      relayManager.setRelayPin(newPin);
      needRestart = true;
    }
  }
  
  if (doc.containsKey("relay_logic")) {
    relayManager.setRelayLogic((RelayLogic)doc["relay_logic"].as<uint8_t>());
  }
  
  if (doc.containsKey("initial_state")) {
    relayManager.setInitialState((InitialState)doc["initial_state"].as<uint8_t>());
  }
  
  if (doc.containsKey("pulse_ms")) {
    relayManager.setPulseMs(doc["pulse_ms"].as<uint32_t>());
  }
  
  if (doc.containsKey("timer_seconds")) {
    relayManager.setTimerSeconds(doc["timer_seconds"].as<uint32_t>());
  }
  
  if (doc.containsKey("publish_interval")) {
    mqttHandler.setPublishInterval(doc["publish_interval"].as<uint32_t>());
    mqttHandler.savePublishInterval();
  }
  
  relayManager.saveConfig();
  
  core.webServer.getServer().send(200, "application/json", "{\"success\":true}");
  
  if (needRestart) {
    LOG_INFO("Reiniciando dispositivo em 2 segundos...");
    delay(2000);
    ESP.restart();
  }
}

// POST /api/device/stats/reset
void handleApiResetStats() {
  relayManager.resetStatistics();
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
  core.addRoutePost("/api/device/relay", handleApiRelaySet);
  core.addRoutePost("/api/device/relay/toggle", handleApiRelayToggle);
  core.addRoutePost("/api/device/pulse", handleApiPulse);
  core.addRoutePost("/api/device/timer", handleApiTimer);
  core.addRoutePost("/api/device/config", handleApiSaveConfig);
  core.addRoutePost("/api/device/stats/reset", handleApiResetStats);
  
  LOG_SUCCESS("Rotas customizadas registradas!");
}

// ==================== SETUP ====================

void setup() {
  Serial.begin(115200);
  delay(100);
  
  Serial.println();
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║         ESP01S RELAY CONTROL           ║");
  Serial.println("║         Firmware v" FIRMWARE_VERSION "                 ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  
  // CMD-Core (WiFi, WebServer, MQTT, Auth)
  core.begin();
  
  // Relay Manager
  relayManager.begin();
  relayManager.setRelayChangeCallback(onRelayChange);
  
  // MQTT Handler
  mqttHandler.begin(&core.mqtt);
  
  // Registra rotas customizadas
  registerCustomRoutes();
  
  // Publica estados iniciais
  if (core.mqtt.isConnected()) {
    mqttHandler.publishAllStates();
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
  core.handle();
  relayManager.handle();
  mqttHandler.handle();
  delay(1);
}
