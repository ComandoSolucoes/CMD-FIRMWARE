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

    // Saídas
    void setOutput(uint8_t index, bool state);
    void toggleOutput(uint8_t index);
    bool getOutputState(uint8_t index);
    void setAllOutputs(bool state);

    // Entradas
    bool getInputState(uint8_t index);

    // Configuração
    void         setRelayLogic(RelayLogic logic);
    RelayLogic   getRelayLogic() const { return relayLogic; }

    void         setInitialState(InitialState is);
    InitialState getInitialState() const { return initialState; }

    void      setInputMode(uint8_t index, InputMode mode);
    InputMode getInputMode(uint8_t index);

    void     setDebounceTime(uint8_t index, uint16_t ms);
    uint16_t getDebounceTime(uint8_t index);

    void     setPulseTime(uint8_t index, uint16_t ms);
    uint16_t getPulseTime(uint8_t index);

    // Callbacks
    void setOutputChangedCallback(OutputChangedCallback cb) { onOutputChanged = cb; }
    void setInputChangedCallback (InputChangedCallback  cb) { onInputChanged  = cb; }

    // Persistência
    void saveConfig();
    void loadConfig();

private:
    I2CSlaveManager* slaves;

    bool outputStates[NUM_TOTAL_OUTPUTS];
    bool inputStates [NUM_TOTAL_INPUTS];
    bool lastInputRaw[NUM_TOTAL_INPUTS];

    uint32_t debounceTimer[NUM_TOTAL_INPUTS];
    uint32_t pulseTimer   [NUM_LOCAL_OUTPUTS];

    RelayLogic   relayLogic;
    InitialState initialState;
    InputMode    inputMode   [NUM_TOTAL_INPUTS];
    uint16_t     debounceMs  [NUM_TOTAL_INPUTS];
    uint16_t     pulseMs     [NUM_LOCAL_OUTPUTS];

    OutputChangedCallback onOutputChanged;
    InputChangedCallback  onInputChanged;

    static const uint8_t OUTPUT_PINS[NUM_LOCAL_OUTPUTS];
    static const uint8_t INPUT_PINS [NUM_LOCAL_INPUTS];

    Preferences prefs;

    void applyInitialState();
    void applyOutputHardware(uint8_t index, bool state);
    bool readLocalInput(uint8_t index) const;
    void processInput(uint8_t index, bool rawState);
};

#endif // IO_MANAGER_ETH18_H