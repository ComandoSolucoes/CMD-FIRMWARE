#ifndef IO_MANAGER_H
#define IO_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

// Callback quando um relé muda de estado
typedef std::function<void(uint8_t relay, bool state)> RelayChangeCallback;

// Callback quando uma entrada muda de estado
typedef std::function<void(uint8_t input, bool state)> InputChangeCallback;

class IOManager {
public:
    IOManager();
    
    // Inicialização
    void begin();
    void handle();  // Chamado no loop principal
    
    // ==================== RELÉS ====================
    
    // Controle direto
    void setRelay(uint8_t relay, bool state);
    void toggleRelay(uint8_t relay);
    void setAllRelays(bool state);
    bool getRelayState(uint8_t relay);
    
    // Configuração
    void setRelayLogic(RelayLogic logic);
    RelayLogic getRelayLogic();
    void setInitialState(InitialState state);
    InitialState getInitialState();
    
    // ==================== ENTRADAS ====================
    
    // Leitura
    bool getInputState(uint8_t input);
    
    // Configuração de modo
    void setInputMode(uint8_t input, InputMode mode);
    InputMode getInputMode(uint8_t input);
    
    // Configuração de tempos
    void setDebounceTime(uint8_t input, uint16_t ms);
    uint16_t getDebounceTime(uint8_t input);
    void setPulseTime(uint8_t input, uint16_t ms);
    uint16_t getPulseTime(uint8_t input);
    
    // ==================== CALLBACKS ====================
    
    void setRelayChangeCallback(RelayChangeCallback callback);
    void setInputChangeCallback(InputChangeCallback callback);
    
    // ==================== PERSISTÊNCIA ====================
    
    void saveConfig();       // Salva todas as configurações na flash
    void loadConfig();       // Carrega configurações da flash
    void saveRelayStates();  // Salva estados dos relés
    void loadRelayStates();  // Carrega estados dos relés
    
private:
    Preferences prefs;
    
    // Estados dos relés
    bool relayStates[NUM_OUTPUTS];
    RelayLogic relayLogic;
    InitialState initialState;
    
    // Configuração das entradas
    InputMode inputModes[NUM_INPUTS];
    uint16_t debounceTimes[NUM_INPUTS];
    uint16_t pulseTimes[NUM_INPUTS];
    
    // Estados das entradas (debounce)
    bool inputStates[NUM_INPUTS];
    bool lastInputStates[NUM_INPUTS];
    unsigned long lastDebounceTime[NUM_INPUTS];
    
    // Controle de pulso
    bool pulseActive[NUM_OUTPUTS];
    unsigned long pulseStartTime[NUM_OUTPUTS];
    
    // Callbacks
    RelayChangeCallback onRelayChange;
    InputChangeCallback onInputChange;
    
    // Métodos privados
    void initPins();
    void applyInitialStates();
    void updatePhysicalRelay(uint8_t relay);
    void handleInputs();
    void handlePulses();
    void processInputChange(uint8_t input, bool newState);
    
    // Pinos
    const uint8_t inputPins[NUM_INPUTS] = {INPUT_1_PIN, INPUT_2_PIN, INPUT_3_PIN, INPUT_4_PIN};
    const uint8_t outputPins[NUM_OUTPUTS] = {OUTPUT_1_PIN, OUTPUT_2_PIN, OUTPUT_3_PIN, OUTPUT_4_PIN};
};

#endif // IO_MANAGER_H