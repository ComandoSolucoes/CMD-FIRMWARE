#include "I2CSlaveManager.h"

I2CSlaveManager::I2CSlaveManager()
    : lastPollTime(0), inputChangedCb(nullptr) {

    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        slaveStatus[s].online      = false;
        slaveStatus[s].lastContact = 0;
        slaveStatus[s].errorCount  = 0;
        slaveStatus[s].totalErrors = 0;
        pendingConfigPush[s]       = false;

        for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
            slaveData[s].outputs[i] = false;
            slaveData[s].inputs[i]  = false;
            slaveConfig[s][i]       = SlaveChannelConfig{};
        }
    }
}

void I2CSlaveManager::begin() {
    LOG_INFO("Inicializando I2C Slave Manager...");

    // IMPORTANTE: setBufferSize ANTES de Wire.begin()
    // O JSON com cfg completo pode chegar a ~420 bytes
    Wire.setBufferSize(I2C_BUF_SIZE);
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin();
    Wire.setClock(I2C_FREQ);

    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        Wire.beginTransmission(slaveAddresses[s]);
        uint8_t err = Wire.endTransmission(true);

        if (err == 0) {
            slaveStatus[s].online = true;
            LOG_INFOF("Escravo %d (0x%02X): ONLINE", s + 1, slaveAddresses[s]);
        } else {
            slaveStatus[s].online = false;
            LOG_WARNF("Escravo %d (0x%02X): OFFLINE (erro %d)", s + 1, slaveAddresses[s], err);
        }
    }

    LOG_INFO("I2C Slave Manager inicializado!");
}

void I2CSlaveManager::handle() {
    unsigned long now = millis();
    if (now - lastPollTime < I2C_POLL_MS) return;
    lastPollTime = now;

    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        communicateWithSlave(s);
    }
}

// ==================== SAÍDAS ====================

void I2CSlaveManager::setOutput(uint8_t slaveIndex, uint8_t outputIndex, bool state) {
    if (slaveIndex >= NUM_SLAVES || outputIndex >= NUM_SLAVE_CHANNELS) return;
    slaveData[slaveIndex].outputs[outputIndex] = state;
}

bool I2CSlaveManager::getOutput(uint8_t slaveIndex, uint8_t outputIndex) {
    if (slaveIndex >= NUM_SLAVES || outputIndex >= NUM_SLAVE_CHANNELS) return false;
    return slaveData[slaveIndex].outputs[outputIndex];
}

// ==================== ENTRADAS ====================

bool I2CSlaveManager::getInput(uint8_t slaveIndex, uint8_t inputIndex) {
    if (slaveIndex >= NUM_SLAVES || inputIndex >= NUM_SLAVE_CHANNELS) return false;
    return slaveData[slaveIndex].inputs[inputIndex];
}

// ==================== STATUS ====================

bool I2CSlaveManager::isSlaveOnline(uint8_t slaveIndex) {
    if (slaveIndex >= NUM_SLAVES) return false;
    return slaveStatus[slaveIndex].online;
}

SlaveStatus I2CSlaveManager::getSlaveStatus(uint8_t slaveIndex) {
    if (slaveIndex >= NUM_SLAVES) return SlaveStatus{false, 0, 0, 0};
    return slaveStatus[slaveIndex];
}

void I2CSlaveManager::setInputChangedCallback(SlaveInputChangedCallback cb) {
    inputChangedCb = cb;
}

// ==================== CONFIG PUSH ====================

void I2CSlaveManager::pushConfig(uint8_t slaveIndex,
                                  const SlaveChannelConfig cfg[NUM_SLAVE_CHANNELS]) {
    if (slaveIndex >= NUM_SLAVES) return;

    for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
        slaveConfig[slaveIndex][i] = cfg[i];
    }
    pendingConfigPush[slaveIndex] = true;

    LOG_INFOF("Config agendada para escravo %d (sera enviada no proximo ciclo I2C)", slaveIndex + 1);
}

// ==================== COMUNICAÇÃO ====================

bool I2CSlaveManager::communicateWithSlave(uint8_t slaveIndex) {
    bool previousInputs[NUM_SLAVE_CHANNELS];
    for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
        previousInputs[i] = slaveData[slaveIndex].inputs[i];
    }

    if (!sendOutputsJson(slaveIndex)) {
        slaveStatus[slaveIndex].errorCount++;
        slaveStatus[slaveIndex].totalErrors++;
        if (slaveStatus[slaveIndex].errorCount >= 5) {
            if (slaveStatus[slaveIndex].online) {
                LOG_WARNF("Escravo %d offline (falhas consecutivas)", slaveIndex + 1);
            }
            slaveStatus[slaveIndex].online = false;
        }
        return false;
    }

    if (!receiveInputsJson(slaveIndex)) {
        slaveStatus[slaveIndex].errorCount++;
        slaveStatus[slaveIndex].totalErrors++;
        return false;
    }

    if (!slaveStatus[slaveIndex].online) {
        LOG_INFOF("Escravo %d voltou online!", slaveIndex + 1);
    }
    slaveStatus[slaveIndex].online      = true;
    slaveStatus[slaveIndex].lastContact = millis();
    slaveStatus[slaveIndex].errorCount  = 0;

    detectInputChanges(slaveIndex, previousInputs);
    return true;
}

bool I2CSlaveManager::sendOutputsJson(uint8_t slaveIndex) {
    // Documento maior quando há cfg pendente (com cfg ~420 bytes, sem cfg ~80 bytes)
    StaticJsonDocument<1024> doc;

    for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
        doc["do" + String(i + 1)] = slaveData[slaveIndex].outputs[i] ? 1 : 0;
    }

    // Embute campo "cfg" apenas quando há push pendente
    if (pendingConfigPush[slaveIndex]) {
        JsonObject cfg = doc.createNestedObject("cfg");
        for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
            JsonObject ch = cfg.createNestedObject("in" + String(i + 1));
            ch["mode"]  = slaveConfig[slaveIndex][i].mode;
            ch["deb"]   = slaveConfig[slaveIndex][i].debMs;
            ch["pulse"] = slaveConfig[slaveIndex][i].pulseMs;
        }
        pendingConfigPush[slaveIndex] = false;  // limpa flag após incluir no JSON

        LOG_INFOF("Escravo %d: enviando estados + cfg completa", slaveIndex + 1);
    }

    String output;
    serializeJson(doc, output);

    Wire.beginTransmission(slaveAddresses[slaveIndex]);
    Wire.print(output);  // sem \r\n — o escravo identifica pelo '{'
    uint8_t err = Wire.endTransmission(true);

    return (err == 0);
}

bool I2CSlaveManager::receiveInputsJson(uint8_t slaveIndex) {
    uint8_t bytesReceived = Wire.requestFrom(slaveAddresses[slaveIndex], (uint8_t)128);
    if (!bytesReceived) return false;

    String data = "";
    unsigned long timeout = millis() + I2C_TIMEOUT_MS;
    while (Wire.available() && millis() < timeout) {
        data += (char)Wire.read();
    }

    if (data.length() == 0) return false;

    StaticJsonDocument<300> doc;
    DeserializationError err = deserializeJson(doc, data);
    if (err) {
        LOG_WARNF("Escravo %d: erro JSON (%s)", slaveIndex + 1, err.c_str());
        return false;
    }

    for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
        String key = "di" + String(i + 1);
        if (doc.containsKey(key)) {
            slaveData[slaveIndex].inputs[i] = doc[key].as<int>() != 0;
        }
    }

    return true;
}

void I2CSlaveManager::detectInputChanges(uint8_t slaveIndex,
                                          bool previousInputs[NUM_SLAVE_CHANNELS]) {
    if (!inputChangedCb) return;

    for (uint8_t i = 0; i < NUM_SLAVE_CHANNELS; i++) {
        if (slaveData[slaveIndex].inputs[i] != previousInputs[i]) {
            inputChangedCb(slaveIndex, i, slaveData[slaveIndex].inputs[i]);
        }
    }
}