#include "IOManager.h"

IOManager::IOManager() 
    : relayLogic(RELAY_LOGIC_NORMAL), 
      initialState(INITIAL_STATE_OFF),
      onRelayChange(nullptr),
      onInputChange(nullptr) {
    
    // Inicializa arrays
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        relayStates[i] = false;
        pulseActive[i] = false;
        pulseStartTime[i] = 0;
    }
    
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        inputModes[i] = INPUT_MODE_DISABLED;
        debounceTimes[i] = DEFAULT_DEBOUNCE_MS;
        pulseTimes[i] = DEFAULT_PULSE_MS;
        inputStates[i] = false;
        lastInputStates[i] = false;
        lastDebounceTime[i] = 0;
    }
}

void IOManager::begin() {
    LOG_INFO("Inicializando IOManager...");
    
    // Abre Preferences
    prefs.begin(PREFS_NAMESPACE, false);
    
    // Carrega configurações
    loadConfig();
    
    // Configura pinos
    initPins();
    
    // Aplica estados iniciais
    applyInitialStates();
    
    LOG_INFO("IOManager inicializado!");
}

void IOManager::initPins() {
    // Configura pinos de saída
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        pinMode(outputPins[i], OUTPUT);
    }
    
    // Configura pinos de entrada
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        pinMode(inputPins[i], INPUT);
        // Lê estado inicial
        inputStates[i] = digitalRead(inputPins[i]);
        lastInputStates[i] = inputStates[i];
    }
}

void IOManager::applyInitialStates() {
    switch (initialState) {
        case INITIAL_STATE_OFF:
            LOG_INFO("Estados iniciais: TODOS OFF");
            for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
                setRelay(i, false);
            }
            break;
            
        case INITIAL_STATE_ON:
            LOG_INFO("Estados iniciais: TODOS ON");
            for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
                setRelay(i, true);
            }
            break;
            
        case INITIAL_STATE_LAST:
            LOG_INFO("Estados iniciais: RECUPERANDO ÚLTIMOS");
            loadRelayStates();
            for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
                updatePhysicalRelay(i);
            }
            break;
    }
}

void IOManager::handle() {
    handleInputs();
    handlePulses();
}

// ==================== RELÉS ====================

void IOManager::setRelay(uint8_t relay, bool state) {
    if (relay >= NUM_OUTPUTS) return;
    
    // Se está em modo pulso, cancela o pulso
    if (pulseActive[relay]) {
        pulseActive[relay] = false;
    }
    
    // Atualiza estado
    bool changed = (relayStates[relay] != state);
    relayStates[relay] = state;
    
    // Atualiza saída física
    updatePhysicalRelay(relay);
    
    // Chama callback se mudou
    if (changed && onRelayChange) {
        onRelayChange(relay, state);
    }
}

void IOManager::toggleRelay(uint8_t relay) {
    if (relay >= NUM_OUTPUTS) return;
    setRelay(relay, !relayStates[relay]);
}

void IOManager::setAllRelays(bool state) {
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        setRelay(i, state);
    }
}

bool IOManager::getRelayState(uint8_t relay) {
    if (relay >= NUM_OUTPUTS) return false;
    return relayStates[relay];
}

void IOManager::updatePhysicalRelay(uint8_t relay) {
    if (relay >= NUM_OUTPUTS) return;
    
    bool physicalState = relayStates[relay];
    
    // Aplica lógica invertida se configurado
    if (relayLogic == RELAY_LOGIC_INVERTED) {
        physicalState = !physicalState;
    }
    
    digitalWrite(outputPins[relay], physicalState ? HIGH : LOW);
}

// ==================== ENTRADAS ====================

bool IOManager::getInputState(uint8_t input) {
    if (input >= NUM_INPUTS) return false;
    return inputStates[input];
}

void IOManager::handleInputs() {
    unsigned long now = millis();
    
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        // Se modo desabilitado, ignora
        if (inputModes[i] == INPUT_MODE_DISABLED) continue;
        
        // Lê entrada física
        bool reading = digitalRead(inputPins[i]);
        
        // Debounce
        if (reading != lastInputStates[i]) {
            lastDebounceTime[i] = now;
        }
        
        if ((now - lastDebounceTime[i]) > debounceTimes[i]) {
            // Se estado mudou após debounce
            if (reading != inputStates[i]) {
                inputStates[i] = reading;
                processInputChange(i, reading);
            }
        }
        
        lastInputStates[i] = reading;
    }
}

void IOManager::processInputChange(uint8_t input, bool newState) {
    // Chama callback
    if (onInputChange) {
        onInputChange(input, newState);
    }
    
    // Processa de acordo com o modo
    switch (inputModes[input]) {
        case INPUT_MODE_TRANSITION:
            // Toggle na borda de subida
            if (newState) {
                toggleRelay(input);
            }
            break;
            
        case INPUT_MODE_PULSE:
            // Pulso na borda de subida
            if (newState) {
                setRelay(input, true);
                pulseActive[input] = true;
                pulseStartTime[input] = millis();
            }
            break;
            
        case INPUT_MODE_FOLLOW:
            // Segue o estado da entrada
            setRelay(input, newState);
            break;
            
        case INPUT_MODE_INVERTED:
            // Inverte o estado da entrada
            setRelay(input, !newState);
            break;
            
        default:
            break;
    }
}

void IOManager::handlePulses() {
    unsigned long now = millis();
    
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        if (pulseActive[i]) {
            if (now - pulseStartTime[i] >= pulseTimes[i]) {
                pulseActive[i] = false;
                setRelay(i, false);
            }
        }
    }
}

// ==================== CONFIGURAÇÃO ====================

void IOManager::setRelayLogic(RelayLogic logic) {
    relayLogic = logic;
    
    // Atualiza todas as saídas físicas
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        updatePhysicalRelay(i);
    }
}

RelayLogic IOManager::getRelayLogic() {
    return relayLogic;
}

void IOManager::setInitialState(InitialState state) {
    initialState = state;
}

InitialState IOManager::getInitialState() {
    return initialState;
}

void IOManager::setInputMode(uint8_t input, InputMode mode) {
    if (input >= NUM_INPUTS) return;
    inputModes[input] = mode;
}

InputMode IOManager::getInputMode(uint8_t input) {
    if (input >= NUM_INPUTS) return INPUT_MODE_DISABLED;
    return inputModes[input];
}

void IOManager::setDebounceTime(uint8_t input, uint16_t ms) {
    if (input >= NUM_INPUTS) return;
    debounceTimes[input] = constrain(ms, MIN_DEBOUNCE_MS, MAX_DEBOUNCE_MS);
}

uint16_t IOManager::getDebounceTime(uint8_t input) {
    if (input >= NUM_INPUTS) return DEFAULT_DEBOUNCE_MS;
    return debounceTimes[input];
}

void IOManager::setPulseTime(uint8_t input, uint16_t ms) {
    if (input >= NUM_INPUTS) return;
    pulseTimes[input] = constrain(ms, MIN_PULSE_MS, MAX_PULSE_MS);
}

uint16_t IOManager::getPulseTime(uint8_t input) {
    if (input >= NUM_INPUTS) return DEFAULT_PULSE_MS;
    return pulseTimes[input];
}

// ==================== CALLBACKS ====================

void IOManager::setRelayChangeCallback(RelayChangeCallback callback) {
    onRelayChange = callback;
}

void IOManager::setInputChangeCallback(InputChangeCallback callback) {
    onInputChange = callback;
}

// ==================== PERSISTÊNCIA ====================

void IOManager::saveConfig() {
    prefs.putUChar(PREFS_RELAY_LOGIC, (uint8_t)relayLogic);
    prefs.putUChar(PREFS_INITIAL_STATE, (uint8_t)initialState);
    
    // Salva configurações de cada entrada
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        String keyMode = String(PREFS_INPUT_MODE_PREFIX) + String(i + 1);
        String keyDebounce = String(PREFS_DEBOUNCE_PREFIX) + String(i + 1);
        String keyPulse = String(PREFS_PULSE_PREFIX) + String(i + 1);
        
        prefs.putUChar(keyMode.c_str(), (uint8_t)inputModes[i]);
        prefs.putUShort(keyDebounce.c_str(), debounceTimes[i]);
        prefs.putUShort(keyPulse.c_str(), pulseTimes[i]);
    }
    
    LOG_INFO("Configurações salvas na flash");
}

void IOManager::loadConfig() {
    relayLogic = (RelayLogic)prefs.getUChar(PREFS_RELAY_LOGIC, RELAY_LOGIC_NORMAL);
    initialState = (InitialState)prefs.getUChar(PREFS_INITIAL_STATE, INITIAL_STATE_OFF);
    
    // Carrega configurações de cada entrada
    for (uint8_t i = 0; i < NUM_INPUTS; i++) {
        String keyMode = String(PREFS_INPUT_MODE_PREFIX) + String(i + 1);
        String keyDebounce = String(PREFS_DEBOUNCE_PREFIX) + String(i + 1);
        String keyPulse = String(PREFS_PULSE_PREFIX) + String(i + 1);
        
        inputModes[i] = (InputMode)prefs.getUChar(keyMode.c_str(), INPUT_MODE_DISABLED);
        debounceTimes[i] = prefs.getUShort(keyDebounce.c_str(), DEFAULT_DEBOUNCE_MS);
        pulseTimes[i] = prefs.getUShort(keyPulse.c_str(), DEFAULT_PULSE_MS);
    }
    
    LOG_INFO("Configurações carregadas da flash");
}

void IOManager::saveRelayStates() {
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        String key = String(PREFS_RELAY_STATE_PREFIX) + String(i + 1);
        prefs.putBool(key.c_str(), relayStates[i]);
    }
}

void IOManager::loadRelayStates() {
    for (uint8_t i = 0; i < NUM_OUTPUTS; i++) {
        String key = String(PREFS_RELAY_STATE_PREFIX) + String(i + 1);
        relayStates[i] = prefs.getBool(key.c_str(), false);
    }
}