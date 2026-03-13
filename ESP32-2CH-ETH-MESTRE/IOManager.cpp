#include "IOManager.h"

// ==================== PINOS LOCAIS (do Config.h) ====================

const uint8_t IOManager::OUTPUT_PINS[NUM_LOCAL_OUTPUTS] = {
    LOCAL_OUTPUT_1_PIN,   // GPIO14 — rele1
    LOCAL_OUTPUT_2_PIN    // GPIO12 — rele2
};

const uint8_t IOManager::INPUT_PINS[NUM_LOCAL_INPUTS] = {
    LOCAL_INPUT_1_PIN,    // GPIO39 — InC01
    LOCAL_INPUT_2_PIN     // GPIO36 — InC02
};

// ==================== CONSTRUTOR ====================

IOManager::IOManager(I2CSlaveManager* slaveMgr)
    : slaves(slaveMgr),
      relayLogic(RELAY_LOGIC_NORMAL),
      initialState(INITIAL_STATE_OFF)
{
    memset(outputStates,  0, sizeof(outputStates));
    memset(inputStates,   0, sizeof(inputStates));
    memset(lastInputRaw,  0, sizeof(lastInputRaw));
    memset(debounceTimer, 0, sizeof(debounceTimer));
    memset(pulseTimer,    0, sizeof(pulseTimer));

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS;  i++) {
        inputMode  [i] = INPUT_MODE_DISABLED;
        debounceMs [i] = DEFAULT_DEBOUNCE_MS;
    }
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        pulseMs[i] = DEFAULT_PULSE_MS;
    }
}

// ==================== BEGIN ====================

void IOManager::begin() {
    // Configura pinos de saída
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        pinMode(OUTPUT_PINS[i], OUTPUT);
        digitalWrite(OUTPUT_PINS[i],
            (relayLogic == RELAY_LOGIC_INVERTED) ? HIGH : LOW);
    }

    // Configura pinos de entrada (INPUT_ONLY: 34, 35, 36, 39)
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        pinMode(INPUT_PINS[i], INPUT);
        lastInputRaw[i] = digitalRead(INPUT_PINS[i]);
    }

    loadConfig();
    applyInitialState();

    LOG_INFO("IOManager iniciado (2 saidas + 2 entradas locais + 16 via I2C = 18CH)");
}

void IOManager::applyInitialState() {
    if (initialState == INITIAL_STATE_ON) {
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) setOutput(i, true);
    } else if (initialState == INITIAL_STATE_LAST) {
        prefs.begin(PREFS_NAMESPACE, true);
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
            char key[16];
            snprintf(key, sizeof(key), "%s%d", PREFS_RELAY_STATE_PREFIX, i);
            bool saved = prefs.getBool(key, false);
            setOutput(i, saved);
        }
        prefs.end();
    }
    // INITIAL_STATE_OFF — já zerado no construtor
}

// ==================== SAÍDAS ====================

void IOManager::setOutput(uint8_t index, bool state) {
    if (index >= NUM_TOTAL_OUTPUTS) return;

    outputStates[index] = state;

    if (index < NUM_LOCAL_OUTPUTS) {
        applyOutputHardware(index, state);
    } else {
        // Escravo 1: índices 4–10, Escravo 2: índices 11–17
        uint8_t slaveIndex  = (index - NUM_LOCAL_OUTPUTS) / NUM_SLAVE_CHANNELS;
        uint8_t channelIndex = (index - NUM_LOCAL_OUTPUTS) % NUM_SLAVE_CHANNELS;
        slaves->setOutput(slaveIndex, channelIndex, state);
    }

    if (onOutputChanged) onOutputChanged(index, state);
}

void IOManager::toggleOutput(uint8_t index) {
    if (index < NUM_TOTAL_OUTPUTS)
        setOutput(index, !outputStates[index]);
}

bool IOManager::getOutputState(uint8_t index) {
    return (index < NUM_TOTAL_OUTPUTS) ? outputStates[index] : false;
}

void IOManager::setAllOutputs(bool state) {
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) setOutput(i, state);
}

void IOManager::applyOutputHardware(uint8_t index, bool state) {
    bool level = (relayLogic == RELAY_LOGIC_INVERTED) ? !state : state;
    digitalWrite(OUTPUT_PINS[index], level ? HIGH : LOW);
}

// ==================== ENTRADAS ====================

bool IOManager::getInputState(uint8_t index) {
    return (index < NUM_TOTAL_INPUTS) ? inputStates[index] : false;
}

bool IOManager::readLocalInput(uint8_t index) const {
    return digitalRead(INPUT_PINS[index]);
}

// ==================== HANDLE ====================

void IOManager::handle() {
    uint32_t now = millis();

    // Entradas locais com debounce
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        bool raw = readLocalInput(i);
        if (raw != lastInputRaw[i]) {
            lastInputRaw[i] = raw;
            debounceTimer[i] = now;
        }
        if ((now - debounceTimer[i]) >= debounceMs[i]) {
            if (raw != inputStates[i]) {
                processInput(i, raw);
            }
        }
    }

    // Entradas dos escravos
    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        if (!slaves->isSlaveOnline(s)) continue;
        for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
            uint8_t idx = NUM_LOCAL_INPUTS + s * NUM_SLAVE_CHANNELS + c;
            bool raw = slaves->getInput(s, c);
            if (raw != inputStates[idx]) {
                inputStates[idx] = raw;
                if (onInputChanged) onInputChanged(idx, raw);
            }
        }
    }

    // Pulsos ativos nas saídas locais
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        if (pulseTimer[i] > 0 && (now - pulseTimer[i]) >= pulseMs[i]) {
            pulseTimer[i] = 0;
            setOutput(i, false);
        }
    }
}

void IOManager::processInput(uint8_t index, bool rawState) {
    bool prevState = inputStates[index];
    inputStates[index] = rawState;

    if (onInputChanged) onInputChanged(index, rawState);

    if (inputMode[index] == INPUT_MODE_DISABLED) return;

    // Só atua na borda de subida (exceto FOLLOW/INVERTED)
    bool risingEdge = rawState && !prevState;

    switch (inputMode[index]) {
        case INPUT_MODE_TRANSITION:
            if (risingEdge && index < NUM_LOCAL_OUTPUTS)
                toggleOutput(index);
            break;

        case INPUT_MODE_PULSE:
            if (risingEdge && index < NUM_LOCAL_OUTPUTS) {
                setOutput(index, true);
                pulseTimer[index] = millis();
            }
            break;

        case INPUT_MODE_FOLLOW:
            if (index < NUM_LOCAL_OUTPUTS)
                setOutput(index, rawState);
            break;

        case INPUT_MODE_INVERTED:
            if (index < NUM_LOCAL_OUTPUTS)
                setOutput(index, !rawState);
            break;

        default:
            break;
    }
}

// ==================== CONFIG ====================

void IOManager::setRelayLogic(RelayLogic logic) {
    relayLogic = logic;
    // Reaplicar nos pinos locais
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++)
        applyOutputHardware(i, outputStates[i]);
}

void IOManager::setInitialState(InitialState is) {
    initialState = is;
}

void IOManager::setInputMode(uint8_t index, InputMode mode) {
    if (index < NUM_TOTAL_INPUTS) inputMode[index] = mode;
}

InputMode IOManager::getInputMode(uint8_t index) {
    return (index < NUM_TOTAL_INPUTS) ? inputMode[index] : INPUT_MODE_DISABLED;
}

void IOManager::setDebounceTime(uint8_t index, uint16_t ms) {
    if (index < NUM_TOTAL_INPUTS)
        debounceMs[index] = constrain(ms, MIN_DEBOUNCE_MS, MAX_DEBOUNCE_MS);
}

uint16_t IOManager::getDebounceTime(uint8_t index) {
    return (index < NUM_TOTAL_INPUTS) ? debounceMs[index] : DEFAULT_DEBOUNCE_MS;
}

void IOManager::setPulseTime(uint8_t index, uint16_t ms) {
    if (index < NUM_LOCAL_OUTPUTS)
        pulseMs[index] = constrain(ms, MIN_PULSE_MS, MAX_PULSE_MS);
}

uint16_t IOManager::getPulseTime(uint8_t index) {
    return (index < NUM_LOCAL_OUTPUTS) ? pulseMs[index] : DEFAULT_PULSE_MS;
}

// ==================== PERSISTÊNCIA ====================

void IOManager::saveConfig() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUInt(PREFS_RELAY_LOGIC,   (uint32_t)relayLogic);
    prefs.putUInt(PREFS_INITIAL_STATE, (uint32_t)initialState);
    prefs.putUInt(PREFS_TELE_INTERVAL, 0); // reservado

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        char k1[20], k2[20];
        snprintf(k1, sizeof(k1), "%s%d", PREFS_INPUT_MODE_PREFIX, i);
        snprintf(k2, sizeof(k2), "%s%d", PREFS_DEBOUNCE_PREFIX, i);
        prefs.putUChar(k1, (uint8_t)inputMode[i]);
        prefs.putUShort(k2, debounceMs[i]);
    }
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        char k[20];
        snprintf(k, sizeof(k), "%s%d", PREFS_PULSE_PREFIX, i);
        prefs.putUShort(k, pulseMs[i]);

        // Salva estado atual para INITIAL_STATE_LAST
        char ks[20];
        snprintf(ks, sizeof(ks), "%s%d", PREFS_RELAY_STATE_PREFIX, i);
        prefs.putBool(ks, outputStates[i]);
    }
    prefs.end();
    LOG_INFO("IOManager: config salva");
}

void IOManager::loadConfig() {
    prefs.begin(PREFS_NAMESPACE, true);
    relayLogic   = (RelayLogic)  prefs.getUInt(PREFS_RELAY_LOGIC,   0);
    initialState = (InitialState)prefs.getUInt(PREFS_INITIAL_STATE, 0);

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        char k1[20], k2[20];
        snprintf(k1, sizeof(k1), "%s%d", PREFS_INPUT_MODE_PREFIX, i);
        snprintf(k2, sizeof(k2), "%s%d", PREFS_DEBOUNCE_PREFIX, i);
        inputMode [i] = (InputMode)prefs.getUChar(k1, INPUT_MODE_DISABLED);
        debounceMs[i] = prefs.getUShort(k2, DEFAULT_DEBOUNCE_MS);
    }
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        char k[20];
        snprintf(k, sizeof(k), "%s%d", PREFS_PULSE_PREFIX, i);
        pulseMs[i] = prefs.getUShort(k, DEFAULT_PULSE_MS);
    }
    prefs.end();
}