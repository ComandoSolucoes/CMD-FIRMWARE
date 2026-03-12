#ifndef IO_MANAGER_ETH18_H
#define IO_MANAGER_ETH18_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "I2CSlaveManager.h"

// Callbacks
typedef std::function<void(uint8_t channelIndex, bool state)> OutputChangedCallback;
typedef std::function<void(uint8_t channelIndex, bool state)> InputChangedCallback;

// ==================== MAPEAMENTO UNIFICADO ====================
// O IOManager expõe 18 outputs (0-17) e 18 inputs (0-17):
//   outputs 0-1   → relés locais do master
//   outputs 2-9   → escravo 1, do1-do8
//   outputs 10-17 → escravo 2, do1-do8
//
//   inputs  0-1   → entradas locais do master
//   inputs  2-9   → escravo 1, di1-di8
//   inputs  10-17 → escravo 2, di1-di8

class IOManager {
public:
    IOManager(I2CSlaveManager* slaveMgr);

    void begin();
    void handle();

    // ==================== SAÍDAS (OUTPUTS) ====================

    void setOutput(uint8_t index, bool state);
    void toggleOutput(uint8_t index);
    void setAllOutputs(bool state);
    bool getOutputState(uint8_t index);

    // Configuração
    void setRelayLogic(RelayLogic logic);
    RelayLogic getRelayLogic();
    void setInitialState(InitialState state);
    InitialState getInitialState();

    // ==================== ENTRADAS (INPUTS) ====================

    bool getInputState(uint8_t index);

    // Configuração por canal
    void setInputMode(uint8_t index, InputMode mode);
    InputMode getInputMode(uint8_t index);
    void setDebounceTime(uint8_t index, uint16_t ms);
    uint16_t getDebounceTime(uint8_t index);
    void setPulseTime(uint8_t index, uint16_t ms);
    uint16_t getPulseTime(uint8_t index);

    // ==================== CALLBACKS ====================

    void setOutputChangedCallback(OutputChangedCallback cb);
    void setInputChangedCallback(InputChangedCallback cb);

    // ==================== PERSISTÊNCIA ====================

    void saveConfig();
    void loadConfig();
    void saveOutputStates();
    void loadOutputStates();

    // ==================== INFORMAÇÕES ====================

    bool isLocalOutput(uint8_t index);   // true se index < NUM_LOCAL_OUTPUTS
    bool isLocalInput(uint8_t index);    // true se index < NUM_LOCAL_INPUTS
    uint8_t getSlaveIndex(uint8_t index);        // qual escravo (0 ou 1)
    uint8_t getSlaveChannelIndex(uint8_t index); // canal dentro do escravo (0-7)

private:
    I2CSlaveManager* slaves;
    Preferences prefs;

    // Estados locais
    bool localOutputStates[NUM_LOCAL_OUTPUTS];
    bool localInputStates[NUM_LOCAL_INPUTS];
    bool lastLocalInputStates[NUM_LOCAL_INPUTS];
    unsigned long lastLocalDebounceTime[NUM_LOCAL_INPUTS];

    // Config global
    RelayLogic relayLogic;
    InitialState initialState;

    // Config por canal de entrada (todos os 18)
    InputMode inputModes[NUM_TOTAL_INPUTS];
    uint16_t debounceTimes[NUM_TOTAL_INPUTS];
    uint16_t pulseTimes[NUM_TOTAL_INPUTS];

    // Controle de pulso de outputs (todos os 18)
    bool pulseActive[NUM_TOTAL_OUTPUTS];
    unsigned long pulseStartTime[NUM_TOTAL_OUTPUTS];

    // Callbacks
    OutputChangedCallback outputChangedCb;
    InputChangedCallback  inputChangedCb;

    // Pinos físicos locais
    const uint8_t localOutputPins[NUM_LOCAL_OUTPUTS] = {
        LOCAL_OUTPUT_1_PIN, LOCAL_OUTPUT_2_PIN
    };
    const uint8_t localInputPins[NUM_LOCAL_INPUTS] = {
        LOCAL_INPUT_1_PIN, LOCAL_INPUT_2_PIN
    };

    // Métodos internos
    void initLocalPins();
    void applyInitialStates();
    void handleLocalInputs();
    void handlePulses();
    void processInputChange(uint8_t index, bool newState);
    void setLocalOutput(uint8_t localIndex, bool state);
    void onSlaveInputChanged(uint8_t slaveIndex, uint8_t inputIndex, bool state);
};

#endif // IO_MANAGER_ETH18_H