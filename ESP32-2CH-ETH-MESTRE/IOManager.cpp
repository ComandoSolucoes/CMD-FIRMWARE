#include "IOManager.h"

IOManager::IOManager(I2CSlaveManager* slaveMgr)
    : slaves(slaveMgr),
      relayLogic(RELAY_LOGIC_NORMAL),
      initialState(INITIAL_STATE_OFF),
      outputChangedCb(nullptr),
      inputChangedCb(nullptr) {

    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        localOutputStates[i] = false;
    }
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        localInputStates[i]      = false;
        lastLocalInputStates[i]  = false;
        lastLocalDebounceTime[i] = 0;
    }
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        pulseActive[i]    = false;
        pulseStartTime[i] = 0;
    }
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        inputModes[i]    = INPUT_MODE_DISABLED;
        debounceTimes[i] = DEFAULT_DEBOUNCE_MS;
        pulseTimes[i]    = DEFAULT_PULSE_MS;
    }
}

void IOManager::begin() {
    LOG_INFO("Inicializando IOManager (18 canais)...");

    prefs.begin(PREFS_NAMESPACE, false);
    loadConfig();
    initLocalPins();

    // Registra callback dos escravos para detectar mudanças nas entradas I2C
    slaves->setInputChangedCallback([this](uint8_t slaveIndex, uint8_t inputIndex, bool state) {
        this->onSlaveInputChanged(slaveIndex, inputIndex, state);
    });

    applyInitialStates();

    LOG_INFO("IOManager inicializado! (2 locais + 8+8 via I2C = 18 canais)");
}

void IOManager::handle() {
    handleLocalInputs();
    handlePulses();
}

// ==================== SAÍDAS ====================

void IOManager::setOutput(uint8_t index, bool state) {
    if (index >= NUM_TOTAL_OUTPUTS) return;

    // Cancela pulso se ativo
    if (pulseActive[index]) pulseActive[index] = false;

    bool changed = false;

    if (isLocalOutput(index)) {
        // Output local
        changed = (localOutputStates[index] != state);
        localOutputStates[index] = state;
        setLocalOutput(index, state);
    } else {
        // Output de escravo
        uint8_t si  = getSlaveIndex(index);
        uint8_t chi = getSlaveChannelIndex(index);
        changed = (slaves->getOutput(si, chi) != state);
        slaves->setOutput(si, chi, state);
    }

    if (changed && outputChangedCb) {
        outputChangedCb(index, state);
    }
}

void IOManager::toggleOutput(uint8_t index) {
    setOutput(index, !getOutputState(index));
}

void IOManager::setAllOutputs(bool state) {
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        setOutput(i, state);
    }
}

bool IOManager::getOutputState(uint8_t index) {
    if (index >= NUM_TOTAL_OUTPUTS) return false;
    if (isLocalOutput(index)) return localOutputStates[index];
    return slaves->getOutput(getSlaveIndex(index), getSlaveChannelIndex(index));
}

// ==================== ENTRADAS ====================

bool IOManager::getInputState(uint8_t index) {
    if (index >= NUM_TOTAL_INPUTS) return false;
    if (isLocalInput(index)) return localInputStates[index];
    return slaves->getInput(getSlaveIndex(index), getSlaveChannelIndex(index));
}

// ==================== CONFIGURAÇÃO ====================

void IOManager::setRelayLogic(RelayLogic logic) {
    relayLogic = logic;
    // Reaplica nas saídas locais
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        setLocalOutput(i, localOutputStates[i]);
    }
}
RelayLogic IOManager::getRelayLogic()           { return relayLogic; }
void IOManager::setInitialState(InitialState s) { initialState = s; }
InitialState IOManager::getInitialState()       { return initialState; }

void IOManager::setInputMode(uint8_t index, InputMode mode) {
    if (index < NUM_TOTAL_INPUTS) inputModes[index] = mode;
}
InputMode IOManager::getInputMode(uint8_t index) {
    if (index >= NUM_TOTAL_INPUTS) return INPUT_MODE_DISABLED;
    return inputModes[index];
}
void IOManager::setDebounceTime(uint8_t index, uint16_t ms) {
    if (index < NUM_TOTAL_INPUTS)
        debounceTimes[index] = constrain(ms, MIN_DEBOUNCE_MS, MAX_DEBOUNCE_MS);
}
uint16_t IOManager::getDebounceTime(uint8_t index) {
    if (index >= NUM_TOTAL_INPUTS) return DEFAULT_DEBOUNCE_MS;
    return debounceTimes[index];
}
void IOManager::setPulseTime(uint8_t index, uint16_t ms) {
    if (index < NUM_TOTAL_INPUTS)
        pulseTimes[index] = constrain(ms, MIN_PULSE_MS, MAX_PULSE_MS);
}
uint16_t IOManager::getPulseTime(uint8_t index) {
    if (index >= NUM_TOTAL_INPUTS) return DEFAULT_PULSE_MS;
    return pulseTimes[index];
}

// ==================== CALLBACKS ====================

void IOManager::setOutputChangedCallback(OutputChangedCallback cb) { outputChangedCb = cb; }
void IOManager::setInputChangedCallback(InputChangedCallback cb)   { inputChangedCb  = cb; }

// ==================== PERSISTÊNCIA ====================

void IOManager::saveConfig() {
    prefs.putUChar(PREFS_RELAY_LOGIC,   (uint8_t)relayLogic);
    prefs.putUChar(PREFS_INITIAL_STATE, (uint8_t)initialState);
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        String km = String(PREFS_INPUT_MODE_PREFIX) + String(i);
        String kd = String(PREFS_DEBOUNCE_PREFIX)   + String(i);
        String kp = String(PREFS_PULSE_PREFIX)       + String(i);
        prefs.putUChar(km.c_str(), (uint8_t)inputModes[i]);
        prefs.putUShort(kd.c_str(), debounceTimes[i]);
        prefs.putUShort(kp.c_str(), pulseTimes[i]);
    }
    LOG_INFO("Configurações salvas na flash");
}

void IOManager::loadConfig() {
    relayLogic   = (RelayLogic)prefs.getUChar(PREFS_RELAY_LOGIC, RELAY_LOGIC_NORMAL);
    initialState = (InitialState)prefs.getUChar(PREFS_INITIAL_STATE, INITIAL_STATE_OFF);
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        String km = String(PREFS_INPUT_MODE_PREFIX) + String(i);
        String kd = String(PREFS_DEBOUNCE_PREFIX)   + String(i);
        String kp = String(PREFS_PULSE_PREFIX)       + String(i);
        inputModes[i]    = (InputMode)prefs.getUChar(km.c_str(), INPUT_MODE_DISABLED);
        debounceTimes[i] = prefs.getUShort(kd.c_str(), DEFAULT_DEBOUNCE_MS);
        pulseTimes[i]    = prefs.getUShort(kp.c_str(), DEFAULT_PULSE_MS);
    }
    LOG_INFO("Configurações carregadas da flash");
}

void IOManager::saveOutputStates() {
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        String key = String(PREFS_RELAY_STATE_PREFIX) + String(i);
        prefs.putBool(key.c_str(), getOutputState(i));
    }
}

void IOManager::loadOutputStates() {
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        String key = String(PREFS_RELAY_STATE_PREFIX) + String(i);
        bool state = prefs.getBool(key.c_str(), false);
        if (isLocalOutput(i)) {
            localOutputStates[i] = state;
        } else {
            slaves->setOutput(getSlaveIndex(i), getSlaveChannelIndex(i), state);
        }
    }
}

// ==================== INFORMAÇÕES DE MAPEAMENTO ====================

bool IOManager::isLocalOutput(uint8_t index) { return index < NUM_LOCAL_OUTPUTS; }
bool IOManager::isLocalInput(uint8_t index)  { return index < NUM_LOCAL_INPUTS; }

uint8_t IOManager::getSlaveIndex(uint8_t index) {
    // outputs 2-9 → slave 0, outputs 10-17 → slave 1
    // inputs  2-9 → slave 0, inputs  10-17 → slave 1
    uint8_t base = isLocalOutput(index) ? NUM_LOCAL_OUTPUTS : NUM_LOCAL_INPUTS;
    return (index - base) / NUM_SLAVE_CHANNELS;
}

uint8_t IOManager::getSlaveChannelIndex(uint8_t index) {
    uint8_t base = isLocalOutput(index) ? NUM_LOCAL_OUTPUTS : NUM_LOCAL_INPUTS;
    return (index - base) % NUM_SLAVE_CHANNELS;
}

// ==================== MÉTODOS PRIVADOS ====================

void IOManager::initLocalPins() {
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        pinMode(localOutputPins[i], OUTPUT);
        digitalWrite(localOutputPins[i], LOW);
    }
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        pinMode(localInputPins[i], INPUT);
        localInputStates[i]     = digitalRead(localInputPins[i]);
        lastLocalInputStates[i] = localInputStates[i];
    }
}

void IOManager::applyInitialStates() {
    switch (initialState) {
        case INITIAL_STATE_OFF:
            LOG_INFO("Estado inicial: TODOS OFF");
            for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) setOutput(i, false);
            break;
        case INITIAL_STATE_ON:
            LOG_INFO("Estado inicial: TODOS ON");
            for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) setOutput(i, true);
            break;
        case INITIAL_STATE_LAST:
            LOG_INFO("Estado inicial: RECUPERANDO ÚLTIMOS");
            loadOutputStates();
            for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
                setLocalOutput(i, localOutputStates[i]);
            }
            break;
    }
}

void IOManager::setLocalOutput(uint8_t localIndex, bool state) {
    if (localIndex >= NUM_LOCAL_OUTPUTS) return;
    bool physical = (relayLogic == RELAY_LOGIC_INVERTED) ? !state : state;
    digitalWrite(localOutputPins[localIndex], physical ? HIGH : LOW);
}

void IOManager::handleLocalInputs() {
    unsigned long now = millis();
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        if (inputModes[i] == INPUT_MODE_DISABLED) continue;
        bool reading = digitalRead(localInputPins[i]);
        if (reading != lastLocalInputStates[i]) {
            lastLocalDebounceTime[i] = now;
        }
        if ((now - lastLocalDebounceTime[i]) > debounceTimes[i]) {
            if (reading != localInputStates[i]) {
                localInputStates[i] = reading;
                processInputChange(i, reading);
            }
        }
        lastLocalInputStates[i] = reading;
    }
}

void IOManager::handlePulses() {
    unsigned long now = millis();
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        if (pulseActive[i] && (now - pulseStartTime[i] >= pulseTimes[i])) {
            pulseActive[i] = false;
            setOutput(i, false);
        }
    }
}

void IOManager::processInputChange(uint8_t index, bool newState) {
    if (inputChangedCb) inputChangedCb(index, newState);

    // Executa lógica de vinculação entrada→saída (mesmo índice)
    if (index < NUM_TOTAL_OUTPUTS) {
        switch (inputModes[index]) {
            case INPUT_MODE_TRANSITION:
                if (newState) toggleOutput(index);
                break;
            case INPUT_MODE_PULSE:
                if (newState) {
                    setOutput(index, true);
                    pulseActive[index]    = true;
                    pulseStartTime[index] = millis();
                }
                break;
            case INPUT_MODE_FOLLOW:
                setOutput(index, newState);
                break;
            case INPUT_MODE_INVERTED:
                setOutput(index, !newState);
                break;
            default:
                break;
        }
    }
}

// Chamado pelo I2CSlaveManager quando detecta mudança de entrada em um escravo
void IOManager::onSlaveInputChanged(uint8_t slaveIndex, uint8_t inputIndex, bool state) {
    // Converte para índice unificado
    uint8_t unifiedIndex = NUM_LOCAL_INPUTS + slaveIndex * NUM_SLAVE_CHANNELS + inputIndex;
    processInputChange(unifiedIndex, state);
}