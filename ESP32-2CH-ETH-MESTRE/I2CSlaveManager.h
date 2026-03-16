#ifndef I2C_SLAVE_MANAGER_H
#define I2C_SLAVE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "Config.h"

// ==================== CONFIG DE CANAL DO ESCRAVO ====================
// Espelha o ChannelConfig do firmware 8CH-IO
struct SlaveChannelConfig {
    uint8_t  mode    = INPUT_MODE_TRANSITION;
    uint16_t debMs   = DEFAULT_DEBOUNCE_MS;
    uint16_t pulseMs = DEFAULT_PULSE_MS;
};

// ==================== STATUS / DADOS DO ESCRAVO ====================

struct SlaveStatus {
    bool          online;
    unsigned long lastContact;
    uint32_t      errorCount;
    uint32_t      totalErrors;
};

struct SlaveData {
    bool outputs[NUM_SLAVE_CHANNELS]; // do1-do8 (enviamos)
    bool inputs [NUM_SLAVE_CHANNELS]; // di1-di8 (recebemos)
};

typedef std::function<void(uint8_t slaveIndex, uint8_t inputIndex, bool state)> SlaveInputChangedCallback;

// ==================== CLASSE ====================

class I2CSlaveManager {
public:
    I2CSlaveManager();

    void begin();
    void handle();

    // Saídas
    void setOutput(uint8_t slaveIndex, uint8_t outputIndex, bool state);
    bool getOutput(uint8_t slaveIndex, uint8_t outputIndex);

    // Entradas
    bool getInput(uint8_t slaveIndex, uint8_t inputIndex);

    // Status
    bool        isSlaveOnline(uint8_t slaveIndex);
    SlaveStatus getSlaveStatus(uint8_t slaveIndex);

    // Callback
    void setInputChangedCallback(SlaveInputChangedCallback cb);

    // ==================== CONFIG PUSH ====================
    // Agenda envio de configuração completa embutida no próximo JSON de saídas.
    // Chamado pelo IOManager no boot e ao salvar configuração.
    void pushConfig(uint8_t slaveIndex, const SlaveChannelConfig cfg[NUM_SLAVE_CHANNELS]);

private:
    SlaveData   slaveData  [NUM_SLAVES];
    SlaveStatus slaveStatus[NUM_SLAVES];
    unsigned long lastPollTime;

    SlaveInputChangedCallback inputChangedCb;

    // Config pendente para cada escravo
    bool               pendingConfigPush[NUM_SLAVES];
    SlaveChannelConfig slaveConfig[NUM_SLAVES][NUM_SLAVE_CHANNELS];

    const uint8_t slaveAddresses[NUM_SLAVES] = { I2C_SLAVE1_ADDR, I2C_SLAVE2_ADDR };

    bool communicateWithSlave(uint8_t slaveIndex);
    bool sendOutputsJson(uint8_t slaveIndex);
    bool receiveInputsJson(uint8_t slaveIndex);
    void detectInputChanges(uint8_t slaveIndex, bool previousInputs[NUM_SLAVE_CHANNELS]);
};

#endif // I2C_SLAVE_MANAGER_H