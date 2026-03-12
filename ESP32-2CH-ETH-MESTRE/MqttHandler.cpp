#include "MqttHandler.h"

MqttHandler* MqttHandler::instance = nullptr;

MqttHandler::MqttHandler(IOManager* ioMgr)
    : io(ioMgr),
      mqttClient(ethClient),
      port(1883),
      telemetryInterval(DEFAULT_TELEMETRY_INTERVAL),
      lastTelemetryTime(0),
      lastReconnectAttempt(0) {
    instance = this;
}

void MqttHandler::begin(const char* brokerAddr, uint16_t brokerPort,
                        const char* user, const char* pass,
                        const char* devName) {
    broker     = String(brokerAddr);
    port       = brokerPort;
    mqttUser   = String(user);
    mqttPass   = String(pass);
    deviceName = String(devName);

    mqttClient.setServer(broker.c_str(), port);
    mqttClient.setCallback(MqttHandler::messageCallback);
    mqttClient.setBufferSize(512);

    LOG_INFOF("MQTT configurado: broker=%s:%d device=%s", broker.c_str(), port, deviceName.c_str());
}

void MqttHandler::handle() {
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            reconnect();
        }
    } else {
        mqttClient.loop();

        // Telemetria periódica
        if (millis() - lastTelemetryTime >= telemetryInterval) {
            lastTelemetryTime = millis();
            publishTelemetry();
        }
    }
}

bool MqttHandler::isConnected() {
    return mqttClient.connected();
}

void MqttHandler::setTelemetryInterval(uint32_t ms) {
    telemetryInterval = constrain(ms, MIN_TELEMETRY_INTERVAL, MAX_TELEMETRY_INTERVAL);
}
uint32_t MqttHandler::getTelemetryInterval() { return telemetryInterval; }

String MqttHandler::getDeviceName()             { return deviceName; }
void   MqttHandler::setDeviceName(const String& name) { deviceName = name; }

// ==================== PUBLICAÇÃO ====================

void MqttHandler::publishOutputState(uint8_t index, bool state) {
    if (!mqttClient.connected()) return;

    // stat/<device>/POWER<n>  → "ON" ou "OFF"  (formato Tasmota)
    String topic = statTopic("POWER" + String(index + 1));
    mqttClient.publish(topic.c_str(), state ? "ON" : "OFF", true);

    // Publica também JSON de resultado completo
    // stat/<device>/RESULT  → {"POWER1":"ON"}
    String resultTopic = statTopic("RESULT");
    StaticJsonDocument<64> doc;
    doc["POWER" + String(index + 1)] = state ? "ON" : "OFF";
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(resultTopic.c_str(), payload.c_str());
}

void MqttHandler::publishInputState(uint8_t index, bool state) {
    if (!mqttClient.connected()) return;

    // tele/<device>/SENSOR  → {"INPUT1":{"State":1}}
    String topic = teleTopic("SENSOR");
    StaticJsonDocument<128> doc;
    String key = "INPUT" + String(index + 1);
    doc[key]["State"] = state ? 1 : 0;
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void MqttHandler::publishTelemetry() {
    if (!mqttClient.connected()) return;

    // tele/<device>/STATE → estado completo de todas as saídas e entradas
    StaticJsonDocument<512> doc;
    doc["Time"]   = millis() / 1000;
    doc["Uptime"] = millis() / 1000;

    JsonObject outputs = doc.createNestedObject("Outputs");
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        outputs["POWER" + String(i + 1)] = io->getOutputState(i) ? "ON" : "OFF";
    }

    JsonObject inputs = doc.createNestedObject("Inputs");
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        inputs["INPUT" + String(i + 1)] = io->getInputState(i) ? 1 : 0;
    }

    String topic = teleTopic("STATE");
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(topic.c_str(), payload.c_str(), true);
}

// ==================== CALLBACKS ====================

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

    // cmnd/<device>/POWER  → all outputs
    if (cmd == "POWER") {
        handleAllPowerCommand(payload);
        return;
    }

    // cmnd/<device>/POWER<n>  → output n (1-18)
    if (cmd.startsWith("POWER")) {
        int n = cmd.substring(5).toInt();
        if (n >= 1 && n <= NUM_TOTAL_OUTPUTS) {
            handlePowerCommand(n - 1, payload);
        }
        return;
    }

    // cmnd/<device>/TELEPERIOD  → altera intervalo de telemetria
    if (cmd == "TELEPERIOD") {
        uint32_t val = payload.toInt() * 1000;
        setTelemetryInterval(val);
        LOG_INFOF("TelePeriod alterado para %u s", val / 1000);
        return;
    }

    // cmnd/<device>/STATE  → publica estado imediato
    if (cmd == "STATE") {
        publishTelemetry();
        return;
    }
}

void MqttHandler::handlePowerCommand(uint8_t outputIndex, const String& payload) {
    if (outputIndex >= NUM_TOTAL_OUTPUTS) return;

    bool state = false;
    if (payload == "ON" || payload == "1") {
        state = true;
        io->setOutput(outputIndex, true);
    } else if (payload == "OFF" || payload == "0") {
        state = false;
        io->setOutput(outputIndex, false);
    } else if (payload == "TOGGLE") {
        io->toggleOutput(outputIndex);
        state = io->getOutputState(outputIndex);
    } else {
        return; // comando desconhecido
    }

    publishOutputState(outputIndex, io->getOutputState(outputIndex));
}

void MqttHandler::handleAllPowerCommand(const String& payload) {
    bool state = false;
    if (payload == "ON" || payload == "1")  state = true;
    else if (payload == "OFF" || payload == "0") state = false;
    else return;

    io->setAllOutputs(state);
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        publishOutputState(i, state);
    }
}

// ==================== RECONNECT / SUBSCRIBE ====================

void MqttHandler::reconnect() {
    if (!ETH.linkUp()) return;

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
        LOG_ERRORF("Falha MQTT, rc=%d", mqttClient.state());
    }
}

void MqttHandler::subscribe() {
    // cmnd/<device>/POWER      → todas as saídas
    // cmnd/<device>/POWER1-18  → saída individual
    // cmnd/<device>/STATE
    // cmnd/<device>/TELEPERIOD

    String base = "cmnd/" + deviceName + "/#";
    mqttClient.subscribe(base.c_str());
    LOG_INFOF("Subscrito em: %s", base.c_str());
}

// ==================== TÓPICOS ====================

String MqttHandler::cmdTopic(const String& cmd) {
    return "cmnd/" + deviceName + "/" + cmd;
}
String MqttHandler::statTopic(const String& key) {
    return "stat/" + deviceName + "/" + key;
}
String MqttHandler::teleTopic(const String& key) {
    return "tele/" + deviceName + "/" + key;
}