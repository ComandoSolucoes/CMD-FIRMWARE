#include "MqttHandler.h"

MqttHandler* MqttHandler::instance = nullptr;

MqttHandler::MqttHandler(IOManager* ioMgr)
    : io(ioMgr),
      core(nullptr),
      mqttClient(ethClient),
      port(1883),
      telemetryInterval(DEFAULT_TELEMETRY_INTERVAL),
      lastTelemetryTime(0),
      lastReconnectAttempt(0) {
    instance = this;
}

// ==================== INICIALIZAÇÃO ====================

void MqttHandler::begin(CMDCore* corePtr,
                        const char* fallbackBroker,
                        uint16_t    fallbackPort,
                        const char* fallbackUser,
                        const char* fallbackPass) {
    core = corePtr;

    deviceName = "ETH-18CH-" + core->getMacAddress();

    loadConfig(fallbackBroker, fallbackPort, fallbackUser, fallbackPass);

    mqttClient.setServer(broker.c_str(), port);
    mqttClient.setCallback(MqttHandler::messageCallback);
    mqttClient.setBufferSize(512);

    LOG_INFOF("MQTT configurado: broker=%s:%d  device=%s",
              broker.c_str(), port, deviceName.c_str());
}

void MqttHandler::loadConfig(const char* fallbackBroker, uint16_t fallbackPort,
                              const char* fallbackUser,   const char* fallbackPass) {
    if (core && core->mqtt.hasConfig()) {
        broker   = core->mqtt.getBroker();
        port     = core->mqtt.getPort();
        mqttUser = core->mqtt.getUser();
        mqttPass = core->mqtt.getPass();   // ← usa getter correto
        LOG_INFO("MQTT: configuracao carregada do CMD-C (/mqtt)");
        return;
    }

    broker   = String(fallbackBroker);
    port     = fallbackPort;
    mqttUser = String(fallbackUser);
    mqttPass = String(fallbackPass);

    if (broker.length() > 0) {
        LOG_INFOF("MQTT: usando configuracao hardcoded (broker=%s)", broker.c_str());
    } else {
        LOG_WARN("MQTT: sem configuracao — configure em http://<IP>/mqtt");
    }
}

// ==================== LOOP ====================

void MqttHandler::handle() {
    if (broker.length() == 0) return;
    if (!ETH.linkUp()) return;

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        mqttClient.loop();
        if (millis() - lastTelemetryTime >= telemetryInterval) {
            lastTelemetryTime = millis();
            publishTelemetry();
        }
    }
}

// ==================== RECONEXÃO ====================

void MqttHandler::reconnect() {
    LOG_INFOF("Conectando MQTT %s:%d ...", broker.c_str(), port);

    String clientId = deviceName + "-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    bool ok;
    if (mqttUser.length() > 0) {
        ok = mqttClient.connect(clientId.c_str(), mqttUser.c_str(), mqttPass.c_str());
    } else {
        ok = mqttClient.connect(clientId.c_str());
    }

    if (ok) {
        LOG_INFO("MQTT conectado!");
        subscribe();
        publishTelemetry();
    } else {
        LOG_ERRORF("Falha MQTT rc=%d — tentando novamente em 5s", mqttClient.state());
    }
}

void MqttHandler::subscribe() {
    String topic = "cmnd/" + deviceName + "/#";
    mqttClient.subscribe(topic.c_str());
    LOG_INFOF("Subscrito: %s", topic.c_str());
}

// ==================== PUBLICAÇÃO ====================

void MqttHandler::publishOutputState(uint8_t index, bool state) {
    if (!mqttClient.connected()) return;

    String topic = statTopic("POWER" + String(index + 1));
    mqttClient.publish(topic.c_str(), state ? "ON" : "OFF", true);

    JsonDocument doc;
    doc["POWER" + String(index + 1)] = state ? "ON" : "OFF";
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(statTopic("RESULT").c_str(), payload.c_str());
}

void MqttHandler::publishInputState(uint8_t index, bool state) {
    if (!mqttClient.connected()) return;

    JsonDocument doc;
    doc["INPUT" + String(index + 1)]["State"] = state ? 1 : 0;
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(teleTopic("SENSOR").c_str(), payload.c_str());
}

void MqttHandler::publishTelemetry() {
    if (!mqttClient.connected()) return;

    JsonDocument doc;
    doc["Time"]   = millis() / 1000;
    doc["Uptime"] = millis() / 1000;

    JsonObject outputs = doc["Outputs"].to<JsonObject>();
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++)
        outputs["POWER" + String(i + 1)] = io->getOutputState(i) ? "ON" : "OFF";

    JsonObject inputs = doc["Inputs"].to<JsonObject>();
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++)
        inputs["INPUT" + String(i + 1)] = io->getInputState(i) ? 1 : 0;

    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(teleTopic("STATE").c_str(), payload.c_str(), true);
}

bool MqttHandler::isConnected() {
    return mqttClient.connected();
}

void MqttHandler::setTelemetryInterval(uint32_t ms) {
    telemetryInterval = constrain(ms, MIN_TELEMETRY_INTERVAL, MAX_TELEMETRY_INTERVAL);
    LOG_INFOF("Intervalo de telemetria: %u ms", telemetryInterval);
}

// ==================== CALLBACK ====================

void MqttHandler::messageCallback(char* topic, byte* payload, unsigned int length) {
    if (!instance) return;
    String t(topic);
    String p;
    for (unsigned int i = 0; i < length; i++) p += (char)payload[i];
    p.toUpperCase();
    instance->handleMessage(t, p);
}

void MqttHandler::handleMessage(const String& topic, const String& payload) {
    String prefix = "cmnd/" + deviceName + "/";
    if (!topic.startsWith(prefix)) return;

    String cmd = topic.substring(prefix.length());
    LOG_INFOF("MQTT <- %s: %s", cmd.c_str(), payload.c_str());

    if (cmd == "POWER") {
        handleAllPowerCommand(payload);
        return;
    }
    if (cmd.startsWith("POWER")) {
        int n = cmd.substring(5).toInt();
        if (n >= 1 && n <= (int)NUM_TOTAL_OUTPUTS)
            handlePowerCommand(n - 1, payload);
        return;
    }
    if (cmd == "TELEPERIOD") {
        setTelemetryInterval(payload.toInt() * 1000);
        return;
    }
    if (cmd == "STATE") {
        publishTelemetry();
        return;
    }
}

void MqttHandler::handlePowerCommand(uint8_t outputIndex, const String& payload) {
    if (outputIndex >= NUM_TOTAL_OUTPUTS) return;
    if      (payload == "ON"  || payload == "1")  io->setOutput(outputIndex, true);
    else if (payload == "OFF" || payload == "0")  io->setOutput(outputIndex, false);
    else if (payload == "TOGGLE")                 io->toggleOutput(outputIndex);
    else return;
    publishOutputState(outputIndex, io->getOutputState(outputIndex));
}

void MqttHandler::handleAllPowerCommand(const String& payload) {
    if (payload == "ON" || payload == "1") {
        io->setAllOutputs(true);
        for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) publishOutputState(i, true);
    } else if (payload == "OFF" || payload == "0") {
        io->setAllOutputs(false);
        for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) publishOutputState(i, false);
    }
}

// ==================== TÓPICOS ====================

String MqttHandler::cmdTopic(const String& cmd)  { return "cmnd/" + deviceName + "/" + cmd; }
String MqttHandler::statTopic(const String& key)  { return "stat/" + deviceName + "/" + key; }
String MqttHandler::teleTopic(const String& key)  { return "tele/" + deviceName + "/" + key; }