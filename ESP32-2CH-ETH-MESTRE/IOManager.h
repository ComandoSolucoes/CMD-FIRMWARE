#ifndef IO_MANAGER_ETH18_H
#define IO_MANAGER_ETH18_H

#include <Arduino.h>
#include <Preferences.h>
#include <functional>
#include "Config.h"
#include "I2CSlaveManager.h"

using OutputChangedCallback = std::function<void(uint8_t index, bool state)>;
using InputChangedCallback  = std::function<void(uint8_t index, bool state)>;

class IOManager {
public:
    explicit IOManager(I2CSlaveManager* slaveMgr);

    void begin();
    void handle();

    // ── Saídas (índice global 0-17) ──────────────────────────
    void setOutput(uint8_t index, bool state);
    void toggleOutput(uint8_t index);
    bool getOutputState(uint8_t index) const;
    void setAllOutputs(bool state);

    // ── Entradas (índice global 0-17) ────────────────────────
    bool getInputState(uint8_t index) const;

    // ── Config geral ─────────────────────────────────────────
    void         setRelayLogic  (RelayLogic   logic);
    RelayLogic   getRelayLogic  () const { return relayLogic;   }
    void         setInitialState(InitialState is);
    InitialState getInitialState() const { return initialState; }

    // ── Config por canal de entrada (0-17) ───────────────────
    void      setInputMode   (uint8_t index, InputMode mode);
    InputMode getInputMode   (uint8_t index) const;
    void      setDebounceTime(uint8_t index, uint16_t ms);
    uint16_t  getDebounceTime(uint8_t index) const;
    void      setPulseTime   (uint8_t index, uint16_t ms);
    uint16_t  getPulseTime   (uint8_t index) const;

    // ── Callbacks ────────────────────────────────────────────
    void setOutputChangedCallback(OutputChangedCallback cb) { onOutputChanged = cb; }
    void setInputChangedCallback (InputChangedCallback  cb) { onInputChanged  = cb; }

    // ── Persistência ─────────────────────────────────────────
    void saveConfig();
    void loadConfig();

    // ── Push de config I2C (boot + save) ─────────────────────
    void pushSlavesConfig();

private:
    I2CSlaveManager* slaves;

    // outputStates[0-1]  = saídas locais (hardware direto)
    // outputStates[2-17] = espelho de confirmed[] dos escravos
    bool     outputStates[NUM_TOTAL_OUTPUTS];
    bool     inputStates [NUM_TOTAL_INPUTS];

    // Debounce das entradas locais
    bool     lastInputRaw [NUM_LOCAL_INPUTS];
    uint32_t debounceTimer[NUM_LOCAL_INPUTS];
    uint32_t pulseTimer   [NUM_LOCAL_INPUTS];

    RelayLogic   relayLogic;
    InitialState initialState;

    InputMode inputMode [NUM_TOTAL_INPUTS];
    uint16_t  debounceMs[NUM_TOTAL_INPUTS];
    uint16_t  pulseMs   [NUM_TOTAL_INPUTS];

    OutputChangedCallback onOutputChanged;
    InputChangedCallback  onInputChanged;

    static const uint8_t OUTPUT_PINS[NUM_LOCAL_OUTPUTS];
    static const uint8_t INPUT_PINS [NUM_LOCAL_INPUTS];

    Preferences prefs;

    void applyInitialState();
    void applyOutputHardware(uint8_t index, bool state);
    bool readLocalInput(uint8_t index) const;
    void processLocalInput(uint8_t index, bool rawState);

    // Converte índice global → (slaveIndex, channelIndex)
    void globalToSlave(uint8_t globalIndex, uint8_t& si, uint8_t& ci) const;
};

#endif // IO_MANAGER_ETH18_H