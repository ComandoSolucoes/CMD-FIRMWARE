#ifndef I2C_SLAVE_MANAGER_H
#define I2C_SLAVE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "Config.h"

// ==================== CONFIG DE CANAL DO ESCRAVO ====================

struct SlaveChannelConfig {
    uint8_t  mode    = INPUT_MODE_TRANSITION;
    uint16_t debMs   = DEFAULT_DEBOUNCE_MS;
    uint16_t pulseMs = DEFAULT_PULSE_MS;
};

// ==================== STATUS ====================

struct SlaveStatus {
    bool          online;
    unsigned long lastContact;
    uint32_t      errorCount;
    uint32_t      totalErrors;
};

// ==================== DADOS DO ESCRAVO ====================
// desired  = o que o mestre QUER enviar ao escravo
// confirmed = o que o escravo REPORTOU (fonte da verdade)
// inputs   = entradas digitais do escravo

struct SlaveData {
    bool desired  [NUM_SLAVE_CHANNELS];
    bool confirmed[NUM_SLAVE_CHANNELS];
    bool inputs   [NUM_SLAVE_CHANNELS];
};

// ==================== CALLBACKS ====================

// Disparado quando escravo muda saída localmente (botão físico)
// → NÃO disparado quando mestre confirma seu próprio comando
typedef std::function<void(uint8_t slaveIndex, uint8_t channelIndex, bool state)> SlaveOutputChangedCallback;
typedef std::function<void(uint8_t slaveIndex, uint8_t channelIndex, bool state)> SlaveInputChangedCallback;

// ==================== CLASSE ====================

class I2CSlaveManager {
public:
    I2CSlaveManager();

    void begin();
    void handle();

    // ── Saídas ──────────────────────────────────────────────
    // setDesiredOutput: mestre quer mudar estado → vai no TX
    void setDesiredOutput(uint8_t slaveIndex, uint8_t ch, bool state);

    // getConfirmedOutput: último estado confirmado pelo escravo (verdade)
    bool getConfirmedOutput(uint8_t slaveIndex, uint8_t ch) const;

    // ── Entradas ─────────────────────────────────────────────
    bool getInput(uint8_t slaveIndex, uint8_t ch) const;

    // ── Status ───────────────────────────────────────────────
    bool        isSlaveOnline(uint8_t slaveIndex) const;
    SlaveStatus getSlaveStatus(uint8_t slaveIndex) const;

    // ── Callbacks ────────────────────────────────────────────
    void setOutputChangedCallback(SlaveOutputChangedCallback cb) { outputChangedCb = cb; }
    void setInputChangedCallback (SlaveInputChangedCallback  cb) { inputChangedCb  = cb; }

    // ── Config push ──────────────────────────────────────────
    // Agenda envio de config completa embutida no próximo ciclo I2C
    void pushConfig(uint8_t slaveIndex, const SlaveChannelConfig cfg[NUM_SLAVE_CHANNELS]);

private:
    SlaveData   slaveData  [NUM_SLAVES];
    SlaveStatus slaveStatus[NUM_SLAVES];
    unsigned long lastPollTime;

    bool               pendingConfigPush[NUM_SLAVES];
    SlaveChannelConfig slaveConfig[NUM_SLAVES][NUM_SLAVE_CHANNELS];

    SlaveOutputChangedCallback outputChangedCb;
    SlaveInputChangedCallback  inputChangedCb;

    const uint8_t slaveAddresses[NUM_SLAVES] = { I2C_SLAVE1_ADDR, I2C_SLAVE2_ADDR };

    bool communicateWithSlave(uint8_t slaveIndex);
    bool sendOutputsJson(uint8_t slaveIndex);
    bool receiveStateJson(uint8_t slaveIndex);
    void detectChanges(uint8_t slaveIndex,
                       bool prevInputs [NUM_SLAVE_CHANNELS],
                       bool prevConfirmed[NUM_SLAVE_CHANNELS]);
};

#endif // I2C_SLAVE_MANAGER_H