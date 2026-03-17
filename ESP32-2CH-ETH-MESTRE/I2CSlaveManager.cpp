#include "I2CSlaveManager.h"

I2CSlaveManager::I2CSlaveManager()
    : lastPollTime(0), outputChangedCb(nullptr), inputChangedCb(nullptr) {

    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        slaveStatus[s]       = { false, 0, 0, 0 };
        pendingConfigPush[s] = false;

        for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
            slaveData[s].desired  [c] = false;
            slaveData[s].confirmed[c] = false;
            slaveData[s].inputs   [c] = false;
            slaveConfig[s][c]         = SlaveChannelConfig{};
        }
    }
}

// ==================== BEGIN ====================

void I2CSlaveManager::begin() {
    LOG_INFO("Inicializando I2C Slave Manager...");

    // Bus Recovery — libera SDA caso mestre tenha resetado durante transação
    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, OUTPUT);

    bool sdaFree = digitalRead(I2C_SDA_PIN);
    if (!sdaFree) {
        LOG_WARN("I2C: SDA preso — iniciando bus recovery...");
        for (uint8_t pulse = 0; pulse < 9 && !digitalRead(I2C_SDA_PIN); pulse++) {
            digitalWrite(I2C_SCL_PIN, LOW);  delayMicroseconds(5);
            digitalWrite(I2C_SCL_PIN, HIGH); delayMicroseconds(5);
        }
        pinMode(I2C_SDA_PIN, OUTPUT);
        digitalWrite(I2C_SDA_PIN, LOW);  delayMicroseconds(5);
        digitalWrite(I2C_SCL_PIN, HIGH); delayMicroseconds(5);
        digitalWrite(I2C_SDA_PIN, HIGH); delayMicroseconds(5);
        LOG_INFO("I2C: bus recovery concluido");
    }

    pinMode(I2C_SDA_PIN, INPUT_PULLUP);
    pinMode(I2C_SCL_PIN, INPUT_PULLUP);
    delay(10);

    Wire.setBufferSize(I2C_BUF_SIZE);
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(I2C_FREQ);

    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        Wire.beginTransmission(slaveAddresses[s]);
        uint8_t err = Wire.endTransmission(true);
        slaveStatus[s].online = (err == 0);
        if (err == 0) LOG_INFOF("Escravo %d (0x%02X): ONLINE",  s+1, slaveAddresses[s]);
        else          LOG_WARNF("Escravo %d (0x%02X): OFFLINE (err %d)", s+1, slaveAddresses[s], err);
    }

    LOG_INFO("I2C Slave Manager pronto");
}

// ==================== HANDLE ====================

void I2CSlaveManager::handle() {
    unsigned long now = millis();
    if (now - lastPollTime < I2C_POLL_MS) return;
    lastPollTime = now;

    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        communicateWithSlave(s);
    }
}

// ==================== API PUBLICA ====================

void I2CSlaveManager::setDesiredOutput(uint8_t s, uint8_t c, bool state) {
    if (s >= NUM_SLAVES || c >= NUM_SLAVE_CHANNELS) return;
    slaveData[s].desired[c] = state;
}

bool I2CSlaveManager::getConfirmedOutput(uint8_t s, uint8_t c) const {
    if (s >= NUM_SLAVES || c >= NUM_SLAVE_CHANNELS) return false;
    return slaveData[s].confirmed[c];
}

bool I2CSlaveManager::getInput(uint8_t s, uint8_t c) const {
    if (s >= NUM_SLAVES || c >= NUM_SLAVE_CHANNELS) return false;
    return slaveData[s].inputs[c];
}

bool I2CSlaveManager::isSlaveOnline(uint8_t s) const {
    return (s < NUM_SLAVES) && slaveStatus[s].online;
}

SlaveStatus I2CSlaveManager::getSlaveStatus(uint8_t s) const {
    if (s >= NUM_SLAVES) return { false, 0, 0, 0 };
    return slaveStatus[s];
}

void I2CSlaveManager::pushConfig(uint8_t s, const SlaveChannelConfig cfg[NUM_SLAVE_CHANNELS]) {
    if (s >= NUM_SLAVES) return;
    for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) slaveConfig[s][c] = cfg[c];
    pendingConfigPush[s] = true;
    LOG_INFOF("Config agendada para escravo %d", s+1);
}

// ==================== COMUNICACAO INTERNA ====================

bool I2CSlaveManager::communicateWithSlave(uint8_t s) {
    bool prevInputs   [NUM_SLAVE_CHANNELS];
    bool prevConfirmed[NUM_SLAVE_CHANNELS];
    bool wasOnline = slaveStatus[s].online;

    for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
        prevInputs   [c] = slaveData[s].inputs   [c];
        prevConfirmed[c] = slaveData[s].confirmed[c];
    }

    if (!sendOutputsJson(s)) {
        slaveStatus[s].errorCount++;
        slaveStatus[s].totalErrors++;
        if (slaveStatus[s].errorCount >= 5 && slaveStatus[s].online) {
            slaveStatus[s].online = false;
            LOG_WARNF("Escravo %d OFFLINE (falhas consecutivas)", s+1);
        }
        return false;
    }

    if (!receiveStateJson(s)) {
        slaveStatus[s].errorCount++;
        slaveStatus[s].totalErrors++;
        return false;
    }

    // Escravo voltou online — reagenda cfg para garantir que ele
    // receba a configuração atual (pode ter reiniciado com defaults)
    bool justCameOnline = !wasOnline && slaveStatus[s].errorCount > 0;
    slaveStatus[s].online      = true;
    slaveStatus[s].lastContact = millis();
    slaveStatus[s].errorCount  = 0;

    if (justCameOnline) {
        LOG_INFOF("Escravo %d voltou ONLINE — reenviando cfg", s+1);
        pendingConfigPush[s] = true;
    }

    detectChanges(s, prevInputs, prevConfirmed);
    return true;
}

// TX: envia desired[c] + cfg se pendente
// pendingConfigPush so e limpo apos confirmacao de envio bem-sucedido
bool I2CSlaveManager::sendOutputsJson(uint8_t s) {
    JsonDocument doc;

    for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
        doc["do" + String(c + 1)] = slaveData[s].desired[c] ? 1 : 0;
    }

    bool hasCfg = pendingConfigPush[s];
    if (hasCfg) {
        JsonObject cfg = doc["cfg"].to<JsonObject>();
        for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
            JsonObject ch = cfg["in" + String(c + 1)].to<JsonObject>();
            ch["mode"]  = slaveConfig[s][c].mode;
            ch["deb"]   = slaveConfig[s][c].debMs;
            ch["pulse"] = slaveConfig[s][c].pulseMs;
        }
        LOG_INFOF("Escravo %d: enviando estados + cfg", s+1);
    }

    String payload;
    serializeJson(doc, payload);

    Wire.beginTransmission(slaveAddresses[s]);
    Wire.print(payload);
    bool ok = (Wire.endTransmission(true) == 0);

    // Limpa flag SO SE o envio foi confirmado pelo ACK do escravo.
    // Se falhou, mantém true para reenviar no proximo ciclo.
    if (ok && hasCfg) pendingConfigPush[s] = false;

    return ok;
}

// RX: le di1-di8 e do1-do8
bool I2CSlaveManager::receiveStateJson(uint8_t s) {
    if (!Wire.requestFrom(slaveAddresses[s], (uint8_t)192)) return false;

    String data;
    unsigned long deadline = millis() + I2C_TIMEOUT_MS;
    while (Wire.available() && millis() < deadline) data += (char)Wire.read();

    if (data.isEmpty()) return false;

    JsonDocument doc;
    if (deserializeJson(doc, data) != DeserializationError::Ok) {
        LOG_WARNF("Escravo %d: JSON invalido", s+1);
        return false;
    }

    for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
        String diKey = "di" + String(c + 1);
        String doKey = "do" + String(c + 1);

        if (doc[diKey].is<int>())
            slaveData[s].inputs[c] = doc[diKey].as<int>() != 0;

        if (doc[doKey].is<int>())
            slaveData[s].confirmed[c] = doc[doKey].as<int>() != 0;
    }

    return true;
}

// Detecta mudancas e dispara callbacks
void I2CSlaveManager::detectChanges(uint8_t s,
                                     bool prevInputs   [NUM_SLAVE_CHANNELS],
                                     bool prevConfirmed[NUM_SLAVE_CHANNELS]) {
    for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {

        if (inputChangedCb && slaveData[s].inputs[c] != prevInputs[c]) {
            inputChangedCb(s, c, slaveData[s].inputs[c]);
        }

        if (slaveData[s].confirmed[c] != prevConfirmed[c]) {
            // Atualiza desired para acompanhar o escravo
            slaveData[s].desired[c] = slaveData[s].confirmed[c];

            if (outputChangedCb) {
                outputChangedCb(s, c, slaveData[s].confirmed[c]);
            }
        }
    }
}