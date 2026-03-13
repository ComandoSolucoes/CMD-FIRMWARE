#include "RelayManager.h"

RelayManager::RelayManager() 
    : relayPin(DEFAULT_RELAY_PIN),
      relayLogic(RELAY_LOGIC_NORMAL),
      initialState(INITIAL_STATE_OFF),
      operationMode(MODE_NORMAL),
      timerSeconds(DEFAULT_TIMER_SECONDS),
      pulseMs(DEFAULT_PULSE_MS),
      relayState(false),
      physicalState(false),
      timerActive(false),
      pulseActive(false),
      totalOnTime(0),
      switchCount(0),
      changeCallback(nullptr) {
}

void RelayManager::begin() {
    LOG_INFO("Inicializando RelayManager...");
    
    // Carrega configurações salvas
    loadConfig();
    
    // Valida o pino
    if (!isValidPin(relayPin)) {
        LOG_ERRORF("Pino inválido: %d. Usando padrão GPIO%d", relayPin, DEFAULT_RELAY_PIN);
        relayPin = DEFAULT_RELAY_PIN;
    }
    
    // Configura o pino
    pinMode(relayPin, OUTPUT);
    LOG_INFOF("Pino do relé: GPIO%d", relayPin);
    
    // Define estado inicial
    bool startState = false;
    switch (initialState) {
        case INITIAL_STATE_OFF:
            startState = false;
            LOG_INFO("Estado inicial: OFF");
            break;
        case INITIAL_STATE_ON:
            startState = true;
            LOG_INFO("Estado inicial: ON");
            break;
        case INITIAL_STATE_LAST:
            prefs.begin(PREFS_NAMESPACE, true);
            startState = prefs.getBool(PREFS_RELAY_STATE, false);
            prefs.end();
            LOG_INFOF("Estado inicial: ÚLTIMO (%s)", startState ? "ON" : "OFF");
            break;
    }
    
    relayState = startState;
    lastStateChangeTime = millis();
    statsUpdateTime = millis();
    
    updatePhysicalRelay();
    
    LOG_SUCCESS("RelayManager inicializado!");
}

void RelayManager::handle() {
    // Atualiza estatísticas
    updateStatistics();
    
    // Processa modo temporizador
    if (timerActive) {
        unsigned long elapsed = millis() - timerStartTime;
        if (elapsed >= timerDuration) {
            LOG_INFO("Temporizador expirou, desligando relé");
            setRelay(false);
            timerActive = false;
        }
    }
    
    // Processa modo pulso
    if (pulseActive) {
        unsigned long elapsed = millis() - pulseStartTime;
        if (elapsed >= pulseDuration) {
            LOG_INFO("Pulso concluído, desligando relé");
            setRelay(false);
            pulseActive = false;
        }
    }
}

void RelayManager::setRelay(bool state) {
    if (relayState != state) {
        relayState = state;
        updatePhysicalRelay();
        
        // Atualiza estatísticas
        switchCount++;
        lastStateChangeTime = millis();
        
        // Salva estado se configurado
        if (initialState == INITIAL_STATE_LAST) {
            saveRelayState();
        }
        
        // Chama callback
        if (changeCallback != nullptr) {
            changeCallback(state);
        }
        
        LOG_INFOF("Relé: %s", state ? "ON" : "OFF");
    }
}

void RelayManager::toggleRelay() {
    setRelay(!relayState);
}

bool RelayManager::getRelayState() {
    return relayState;
}

void RelayManager::activatePulse(uint32_t durationMs) {
    if (durationMs == 0) {
        durationMs = pulseMs;
    }
    
    // Valida duração
    if (durationMs < MIN_PULSE_MS) durationMs = MIN_PULSE_MS;
    if (durationMs > MAX_PULSE_MS) durationMs = MAX_PULSE_MS;
    
    pulseActive = true;
    pulseStartTime = millis();
    pulseDuration = durationMs;
    
    setRelay(true);
    LOG_INFOF("Pulso ativado por %dms", durationMs);
}

bool RelayManager::isPulseActive() {
    return pulseActive;
}

void RelayManager::startTimer(uint32_t seconds) {
    if (seconds == 0) {
        seconds = timerSeconds;
    }
    
    // Valida duração
    if (seconds < MIN_TIMER_SECONDS) seconds = MIN_TIMER_SECONDS;
    if (seconds > MAX_TIMER_SECONDS) seconds = MAX_TIMER_SECONDS;
    
    timerActive = true;
    timerStartTime = millis();
    timerDuration = seconds * 1000;
    
    setRelay(true);
    LOG_INFOF("Temporizador iniciado: %d segundos", seconds);
}

void RelayManager::stopTimer() {
    if (timerActive) {
        timerActive = false;
        LOG_INFO("Temporizador cancelado");
    }
}

bool RelayManager::isTimerActive() {
    return timerActive;
}

uint32_t RelayManager::getTimerRemaining() {
    if (!timerActive) return 0;
    
    unsigned long elapsed = millis() - timerStartTime;
    if (elapsed >= timerDuration) return 0;
    
    return (timerDuration - elapsed) / 1000;
}

void RelayManager::setRelayPin(uint8_t pin) {
    if (isValidPin(pin)) {
        relayPin = pin;
        pinMode(relayPin, OUTPUT);
        updatePhysicalRelay();
        LOG_INFOF("Pino alterado para GPIO%d", pin);
    } else {
        LOG_ERRORF("Pino inválido: %d", pin);
    }
}

uint8_t RelayManager::getRelayPin() {
    return relayPin;
}

void RelayManager::setRelayLogic(RelayLogic logic) {
    relayLogic = logic;
    updatePhysicalRelay();
    LOG_INFOF("Lógica alterada: %s", logic == RELAY_LOGIC_NORMAL ? "NORMAL" : "INVERTIDA");
}

RelayLogic RelayManager::getRelayLogic() {
    return relayLogic;
}

void RelayManager::setInitialState(InitialState state) {
    initialState = state;
}

InitialState RelayManager::getInitialState() {
    return initialState;
}

void RelayManager::setOperationMode(OperationMode mode) {
    operationMode = mode;
}

OperationMode RelayManager::getOperationMode() {
    return operationMode;
}

void RelayManager::setTimerSeconds(uint32_t seconds) {
    if (seconds >= MIN_TIMER_SECONDS && seconds <= MAX_TIMER_SECONDS) {
        timerSeconds = seconds;
    }
}

uint32_t RelayManager::getTimerSeconds() {
    return timerSeconds;
}

void RelayManager::setPulseMs(uint32_t ms) {
    if (ms >= MIN_PULSE_MS && ms <= MAX_PULSE_MS) {
        pulseMs = ms;
    }
}

uint32_t RelayManager::getPulseMs() {
    return pulseMs;
}

uint32_t RelayManager::getTotalOnTime() {
    return totalOnTime;
}

uint32_t RelayManager::getSwitchCount() {
    return switchCount;
}

void RelayManager::resetStatistics() {
    totalOnTime = 0;
    switchCount = 0;
    saveStatistics();
    LOG_INFO("Estatísticas resetadas");
}

void RelayManager::saveConfig() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUChar(PREFS_RELAY_PIN, relayPin);
    prefs.putUChar(PREFS_RELAY_LOGIC, (uint8_t)relayLogic);
    prefs.putUChar(PREFS_INITIAL_STATE, (uint8_t)initialState);
    prefs.putUChar(PREFS_OPERATION_MODE, (uint8_t)operationMode);
    prefs.putUInt(PREFS_TIMER_SECONDS, timerSeconds);
    prefs.putUInt(PREFS_PULSE_MS, pulseMs);
    prefs.end();
    LOG_SUCCESS("Configurações salvas!");
}

void RelayManager::loadConfig() {
    prefs.begin(PREFS_NAMESPACE, true);
    relayPin = prefs.getUChar(PREFS_RELAY_PIN, DEFAULT_RELAY_PIN);
    relayLogic = (RelayLogic)prefs.getUChar(PREFS_RELAY_LOGIC, RELAY_LOGIC_NORMAL);
    initialState = (InitialState)prefs.getUChar(PREFS_INITIAL_STATE, INITIAL_STATE_OFF);
    operationMode = (OperationMode)prefs.getUChar(PREFS_OPERATION_MODE, MODE_NORMAL);
    timerSeconds = prefs.getUInt(PREFS_TIMER_SECONDS, DEFAULT_TIMER_SECONDS);
    pulseMs = prefs.getUInt(PREFS_PULSE_MS, DEFAULT_PULSE_MS);
    totalOnTime = prefs.getUInt(PREFS_STATS_TOTAL_ON, 0);
    switchCount = prefs.getUInt(PREFS_STATS_SWITCH_COUNT, 0);
    prefs.end();
}

void RelayManager::saveRelayState() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putBool(PREFS_RELAY_STATE, relayState);
    prefs.end();
}

void RelayManager::setRelayChangeCallback(RelayChangeCallback callback) {
    changeCallback = callback;
}

void RelayManager::updatePhysicalRelay() {
    if (relayLogic == RELAY_LOGIC_NORMAL) {
        physicalState = relayState;
    } else {
        physicalState = !relayState;
    }
    
    digitalWrite(relayPin, physicalState ? HIGH : LOW);
}

void RelayManager::updateStatistics() {
    #if ENABLE_STATISTICS
    unsigned long now = millis();
    
    // Atualiza a cada 1 segundo
    if (now - statsUpdateTime >= 1000) {
        if (relayState) {
            totalOnTime++;
        }
        statsUpdateTime = now;
        
        // Salva estatísticas a cada 60 segundos
        static uint8_t saveCounter = 0;
        if (++saveCounter >= 60) {
            saveStatistics();
            saveCounter = 0;
        }
    }
    #endif
}

void RelayManager::saveStatistics() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUInt(PREFS_STATS_TOTAL_ON, totalOnTime);
    prefs.putUInt(PREFS_STATS_SWITCH_COUNT, switchCount);
    prefs.end();
}

bool RelayManager::isValidPin(uint8_t pin) {
    // Lista de pinos válidos no ESP8266
    return (pin == 0 || pin == 2 || pin == 4 || pin == 5 || 
            pin == 12 || pin == 13 || pin == 14 || pin == 15 || pin == 16);
}
