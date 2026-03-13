#ifndef RELAY_MANAGER_H
#define RELAY_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"

class RelayManager {
public:
    RelayManager();
    
    // Inicialização
    void begin();
    void handle();
    
    // Controle do relé
    void setRelay(bool state);
    void toggleRelay();
    bool getRelayState();
    
    // Modo pulso
    void activatePulse(uint32_t durationMs = 0);
    bool isPulseActive();
    
    // Modo temporizador
    void startTimer(uint32_t seconds = 0);
    void stopTimer();
    bool isTimerActive();
    uint32_t getTimerRemaining();
    
    // Configuração de pino
    void setRelayPin(uint8_t pin);
    uint8_t getRelayPin();
    
    // Configuração de lógica
    void setRelayLogic(RelayLogic logic);
    RelayLogic getRelayLogic();
    
    // Configuração de estado inicial
    void setInitialState(InitialState state);
    InitialState getInitialState();
    
    // Configuração de modo de operação
    void setOperationMode(OperationMode mode);
    OperationMode getOperationMode();
    
    // Configuração de temporizador
    void setTimerSeconds(uint32_t seconds);
    uint32_t getTimerSeconds();
    
    // Configuração de pulso
    void setPulseMs(uint32_t ms);
    uint32_t getPulseMs();
    
    // Estatísticas
    uint32_t getTotalOnTime();
    uint32_t getSwitchCount();
    void resetStatistics();
    
    // Persistência
    void saveConfig();
    void loadConfig();
    void saveRelayState();
    
    // Callback
    typedef void (*RelayChangeCallback)(bool state);
    void setRelayChangeCallback(RelayChangeCallback callback);
    
private:
    Preferences prefs;
    
    // Configuração
    uint8_t relayPin;
    RelayLogic relayLogic;
    InitialState initialState;
    OperationMode operationMode;
    uint32_t timerSeconds;
    uint32_t pulseMs;
    
    // Estado
    bool relayState;
    bool physicalState;
    
    // Temporizador
    bool timerActive;
    unsigned long timerStartTime;
    uint32_t timerDuration;
    
    // Pulso
    bool pulseActive;
    unsigned long pulseStartTime;
    uint32_t pulseDuration;
    
    // Estatísticas
    uint32_t totalOnTime;
    uint32_t switchCount;
    unsigned long lastStateChangeTime;
    unsigned long statsUpdateTime;
    
    // Callback
    RelayChangeCallback changeCallback;
    
    // Métodos privados
    void updatePhysicalRelay();
    void updateStatistics();
    void saveStatistics();
    bool isValidPin(uint8_t pin);
};

#endif // RELAY_MANAGER_H
