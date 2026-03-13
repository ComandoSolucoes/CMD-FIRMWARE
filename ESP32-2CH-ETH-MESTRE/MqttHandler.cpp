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

    // Nome do dispositivo = MAC suffix (igual ao base topic do CMD-C)
    deviceName = "ETH-18CH-" + core->getMacAddress();

    // Carrega configuração MQTT — prefere a salva pelo CMD-C, senão usa fallback
    loadConfig(fallbackBroker, fallbackPort, fallbackUser, fallbackPass);

    // Configura PubSubClient (usa WiFiClient que no ESP32 funciona também via ETH)
    mqttClient.setServer(broker.c_str(), port);
    mqttClient.setCallback(MqttHandler::messageCallback);
    mqttClient.setBufferSize(512);

    LOG_INFOF("MQTT configurado: broker=%s:%d  device=%s",
              broker.c_str(), port, deviceName.c_str());
}

void MqttHandler::loadConfig(const char* fallbackBroker, uint16_t fallbackPort,
                              const char* fallbackUser,   const char* fallbackPass) {
    // Tenta usar config salva pelo CMD-C (/mqtt page)
    if (core && core->mqtt.hasConfig()) {
        broker   = core->mqtt.getBroker();
        port     = core->mqtt.getPort();
        mqttUser = core->mqtt.getUser();
        // Senha não é exposta pelo CMDMqtt publicamente, mas a conexão interna usa
        // Para acessar a senha precisaríamos de getter — usamos string vazia como fallback
        mqttPass = "";
        LOG_INFO("MQTT: configuração carregada do CMD-C (/mqtt)");
        return;
    }

    // Fallback: usa valores passados diretamente no .ino
    broker   = String(fallbackBroker);
    port     = fallbackPort;
    mqttUser = String(fallbackUser);
    mqttPass = String(fallbackPass);

    if (broker.length() > 0) {
        LOG_INFOF("MQTT: usando configuração hardcoded (broker=%s)", broker.c_str());
    } else {
        LOG_WARN("MQTT: sem configuração — configure em http://<IP>/mqtt");
    }
}

// ==================== LOOP ====================

void MqttHandler::handle() {
    // Sem broker configurado, nada a fazer
    if (broker.length() == 0) return;

    // Sem link Ethernet, nada a fazer
    if (!ETH.linkUp()) return;

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

// ==================== RECONEXÃO / SUBSCRIBE ====================

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
        LOG_INFO("✅ MQTT conectado!");
        subscribe();
        publishTelemetry();
    } else {
        LOG_ERRORF("Falha MQTT rc=%d — tentando novamente em 5s", mqttClient.state());
    }
}

void MqttHandler::subscribe() {
    // cmnd/<device>/# — subscreve em todos os comandos do dispositivo
    String topic = "cmnd/" + deviceName + "/#";
    mqttClient.subscribe(topic.c_str());
    LOG_INFOF("✅ Subscrito: %s", topic.c_str());
}

// ==================== PUBLICAÇÃO ====================

void MqttHandler::publishOutputState(uint8_t index, bool state) {
    if (!mqttClient.connected()) return;

    // stat/<device>/POWER<n>  → "ON" ou "OFF"
    String topic = statTopic("POWER" + String(index + 1));
    mqttClient.publish(topic.c_str(), state ? "ON" : "OFF", true);

    // stat/<device>/RESULT  → {"POWER1":"ON"}
    StaticJsonDocument<64> doc;
    doc["POWER" + String(index + 1)] = state ? "ON" : "OFF";
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(statTopic("RESULT").c_str(), payload.c_str());
}

void MqttHandler::publishInputState(uint8_t index, bool state) {
    if (!mqttClient.connected()) return;

    // tele/<device>/SENSOR  → {"INPUT1":{"State":1}}
    StaticJsonDocument<128> doc;
    doc["INPUT" + String(index + 1)]["State"] = state ? 1 : 0;
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(teleTopic("SENSOR").c_str(), payload.c_str());
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

// ==================== CALLBACK MQTT ====================

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
    LOG_INFOF("MQTT ← %s: %s", cmd.c_str(), payload.c_str());

    // cmnd/<device>/POWER  → todas as saídas
    if (cmd == "POWER") {
        handleAllPowerCommand(payload);
        return;
    }

    // cmnd/<device>/POWER<n>  → saída individual (1-18)
    if (cmd.startsWith("POWER")) {
        int n = cmd.substring(5).toInt();
        if (n >= 1 && n <= (int)NUM_TOTAL_OUTPUTS) {
            handlePowerCommand(n - 1, payload);
        }
        return;
    }

    // cmnd/<device>/TELEPERIOD  → altera intervalo de telemetria (valor em segundos)
    if (cmd == "TELEPERIOD") {
        uint32_t val = payload.toInt() * 1000;
        setTelemetryInterval(val);
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

    if (payload == "ON" || payload == "1") {
        io->setOutput(outputIndex, true);
    } else if (payload == "OFF" || payload == "0") {
        io->setOutput(outputIndex, false);
    } else if (payload == "TOGGLE") {
        io->toggleOutput(outputIndex);
    } else {
        return;
    }

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

String MqttHandler::cmdTopic(const String& cmd) {
    return "cmnd/" + deviceName + "/" + cmd;
}
String MqttHandler::statTopic(const String& key) {
    return "stat/" + deviceName + "/" + key;
}
String MqttHandler::teleTopic(const String& key) {
    return "tele/" + deviceName + "/" + key;
}