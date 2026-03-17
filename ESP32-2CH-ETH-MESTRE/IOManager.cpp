#include "IOManager.h"

const uint8_t IOManager::OUTPUT_PINS[NUM_LOCAL_OUTPUTS] = {
    LOCAL_OUTPUT_1_PIN,
    LOCAL_OUTPUT_2_PIN
};
const uint8_t IOManager::INPUT_PINS[NUM_LOCAL_INPUTS] = {
    LOCAL_INPUT_1_PIN,
    LOCAL_INPUT_2_PIN
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

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        inputMode [i] = INPUT_MODE_TRANSITION;
        debounceMs[i] = DEFAULT_DEBOUNCE_MS;
        pulseMs   [i] = DEFAULT_PULSE_MS;
    }
}

// ==================== BEGIN ====================

void IOManager::begin() {
    // Pinos locais
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        pinMode(OUTPUT_PINS[i], OUTPUT);
        digitalWrite(OUTPUT_PINS[i], (relayLogic == RELAY_LOGIC_INVERTED) ? HIGH : LOW);
    }
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        pinMode(INPUT_PINS[i], INPUT);
        lastInputRaw[i] = digitalRead(INPUT_PINS[i]);
    }

    // Carrega NVS
    loadConfig();

    // Aplica estado inicial das saídas locais
    applyInitialState();

    // Registra callback de saída dos escravos no SlaveManager.
    // Quando o escravo muda um relé localmente (botão físico), o
    // SlaveManager chama este callback → IOManager atualiza outputStates[]
    // e dispara onOutputChanged (MQTT).
    slaves->setOutputChangedCallback([this](uint8_t si, uint8_t ci, bool state) {
        uint8_t globalIdx = NUM_LOCAL_OUTPUTS + si * NUM_SLAVE_CHANNELS + ci;
        if (globalIdx >= NUM_TOTAL_OUTPUTS) return;

        outputStates[globalIdx] = state;
        if (onOutputChanged) onOutputChanged(globalIdx, state);
    });

    // Registra callback de entrada dos escravos
    slaves->setInputChangedCallback([this](uint8_t si, uint8_t ci, bool state) {
        uint8_t globalIdx = NUM_LOCAL_INPUTS + si * NUM_SLAVE_CHANNELS + ci;
        if (globalIdx >= NUM_TOTAL_INPUTS) return;

        inputStates[globalIdx] = state;
        if (onInputChanged) onInputChanged(globalIdx, state);
    });

    // Envia config para os escravos no boot
    pushSlavesConfig();

    LOG_INFO("IOManager pronto (2 locais + 16 I2C = 18CH)");
}

// ==================== ESTADO INICIAL ====================

void IOManager::applyInitialState() {
    if (initialState == INITIAL_STATE_ON) {
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) setOutput(i, true);

    } else if (initialState == INITIAL_STATE_LAST) {
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++)
            applyOutputHardware(i, outputStates[i]);

    } else {
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
            outputStates[i] = false;
            applyOutputHardware(i, false);
        }
    }
}

// ==================== SAÍDAS ====================

void IOManager::globalToSlave(uint8_t idx, uint8_t& si, uint8_t& ci) const {
    si = (idx - NUM_LOCAL_OUTPUTS) / NUM_SLAVE_CHANNELS;
    ci = (idx - NUM_LOCAL_OUTPUTS) % NUM_SLAVE_CHANNELS;
}

void IOManager::setOutput(uint8_t index, bool state) {
    if (index >= NUM_TOTAL_OUTPUTS) return;

    outputStates[index] = state;

    if (index < NUM_LOCAL_OUTPUTS) {
        // Saída local — aplica diretamente no hardware
        applyOutputHardware(index, state);
    } else {
        // Saída do escravo — registra como DESIRED no SlaveManager.
        // O SlaveManager enviará no próximo ciclo I2C.
        // NÃO sobrescrevemos confirmed aqui — ele só muda quando o
        // escravo reportar de volta.
        uint8_t si, ci;
        globalToSlave(index, si, ci);
        slaves->setDesiredOutput(si, ci, state);
    }

    // Dispara callback imediatamente para MQTT (confirmação do comando)
    if (onOutputChanged) onOutputChanged(index, state);
}

void IOManager::toggleOutput(uint8_t index) {
    if (index < NUM_TOTAL_OUTPUTS)
        setOutput(index, !outputStates[index]);
}

bool IOManager::getOutputState(uint8_t index) const {
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

bool IOManager::getInputState(uint8_t index) const {
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
            lastInputRaw[i]  = raw;
            debounceTimer[i] = now;
        }
        if ((now - debounceTimer[i]) >= debounceMs[i]) {
            if (raw != inputStates[i]) {
                processLocalInput(i, raw);
            }
        }

        // Expiração de pulso local
        if (pulseTimer[i] > 0 && (now - pulseTimer[i]) >= pulseMs[i]) {
            pulseTimer[i] = 0;
            if (i < NUM_LOCAL_OUTPUTS) setOutput(i, false);
        }
    }

    // NOTA: entradas e saídas dos escravos são tratadas pelos callbacks
    // registrados em begin() → não há polling aqui para evitar race conditions.
    // SlaveManager.handle() (rodando no Core 0) chama os callbacks quando
    // detecta mudanças no ciclo I2C.
}

void IOManager::processLocalInput(uint8_t index, bool rawState) {
    bool prevState     = inputStates[index];
    inputStates[index] = rawState;

    if (onInputChanged) onInputChanged(index, rawState);

    if (inputMode[index] == INPUT_MODE_DISABLED) return;

    bool risingEdge = rawState && !prevState;

    switch (inputMode[index]) {
        case INPUT_MODE_TRANSITION:
            if (risingEdge && index < NUM_LOCAL_OUTPUTS) toggleOutput(index);
            break;
        case INPUT_MODE_PULSE:
            if (risingEdge && index < NUM_LOCAL_OUTPUTS) {
                setOutput(index, true);
                pulseTimer[index] = millis();
            }
            break;
        case INPUT_MODE_FOLLOW:
            if (index < NUM_LOCAL_OUTPUTS) setOutput(index, rawState);
            break;
        case INPUT_MODE_INVERTED:
            if (index < NUM_LOCAL_OUTPUTS) setOutput(index, !rawState);
            break;
        default: break;
    }
}

// ==================== CONFIG ====================

void IOManager::setRelayLogic(RelayLogic logic) {
    relayLogic = logic;
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++)
        applyOutputHardware(i, outputStates[i]);
}

void IOManager::setInitialState(InitialState is) { initialState = is; }

void IOManager::setInputMode(uint8_t i, InputMode mode) {
    if (i < NUM_TOTAL_INPUTS) inputMode[i] = mode;
}
InputMode IOManager::getInputMode(uint8_t i) const {
    return (i < NUM_TOTAL_INPUTS) ? inputMode[i] : INPUT_MODE_DISABLED;
}

void IOManager::setDebounceTime(uint8_t i, uint16_t ms) {
    if (i < NUM_TOTAL_INPUTS)
        debounceMs[i] = constrain(ms, MIN_DEBOUNCE_MS, MAX_DEBOUNCE_MS);
}
uint16_t IOManager::getDebounceTime(uint8_t i) const {
    return (i < NUM_TOTAL_INPUTS) ? debounceMs[i] : DEFAULT_DEBOUNCE_MS;
}

void IOManager::setPulseTime(uint8_t i, uint16_t ms) {
    if (i < NUM_TOTAL_INPUTS)
        pulseMs[i] = constrain(ms, MIN_PULSE_MS, MAX_PULSE_MS);
}
uint16_t IOManager::getPulseTime(uint8_t i) const {
    return (i < NUM_TOTAL_INPUTS) ? pulseMs[i] : DEFAULT_PULSE_MS;
}

// ==================== PUSH CONFIG ESCRAVOS ====================

void IOManager::pushSlavesConfig() {
    for (uint8_t s = 0; s < NUM_SLAVES; s++) {
        SlaveChannelConfig cfg[NUM_SLAVE_CHANNELS];
        for (uint8_t c = 0; c < NUM_SLAVE_CHANNELS; c++) {
            uint8_t inputIdx = NUM_LOCAL_INPUTS + s * NUM_SLAVE_CHANNELS + c;
            cfg[c].mode    = (uint8_t)inputMode [inputIdx];
            cfg[c].debMs   = debounceMs[inputIdx];
            cfg[c].pulseMs = pulseMs   [inputIdx];
        }
        slaves->pushConfig(s, cfg);
    }
    LOG_INFO("pushSlavesConfig: config agendada para S1 e S2");
}

// ==================== PERSISTÊNCIA ====================

void IOManager::saveConfig() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUInt(PREFS_RELAY_LOGIC,   (uint32_t)relayLogic);
    prefs.putUInt(PREFS_INITIAL_STATE, (uint32_t)initialState);

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        char k1[24], k2[24], k3[24];
        snprintf(k1, sizeof(k1), "%s%d", PREFS_INPUT_MODE_PREFIX, i);
        snprintf(k2, sizeof(k2), "%s%d", PREFS_DEBOUNCE_PREFIX,   i);
        snprintf(k3, sizeof(k3), "%s%d", PREFS_PULSE_PREFIX,      i);
        prefs.putUChar (k1, (uint8_t)inputMode[i]);
        prefs.putUShort(k2, debounceMs[i]);
        prefs.putUShort(k3, pulseMs[i]);
    }

    // Salva apenas saídas locais — estados dos escravos ficam na NVS deles
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        char ks[24];
        snprintf(ks, sizeof(ks), "%s%d", PREFS_RELAY_STATE_PREFIX, i);
        prefs.putBool(ks, outputStates[i]);
    }

    prefs.end();
    LOG_INFO("IOManager: config salva");
}

void IOManager::loadConfig() {
    prefs.begin(PREFS_NAMESPACE, true);
    relayLogic   = (RelayLogic)  prefs.getUInt(PREFS_RELAY_LOGIC,   RELAY_LOGIC_NORMAL);
    initialState = (InitialState)prefs.getUInt(PREFS_INITIAL_STATE, INITIAL_STATE_OFF);

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        char k1[24], k2[24], k3[24];
        snprintf(k1, sizeof(k1), "%s%d", PREFS_INPUT_MODE_PREFIX, i);
        snprintf(k2, sizeof(k2), "%s%d", PREFS_DEBOUNCE_PREFIX,   i);
        snprintf(k3, sizeof(k3), "%s%d", PREFS_PULSE_PREFIX,      i);
        inputMode [i] = (InputMode)prefs.getUChar (k1, INPUT_MODE_TRANSITION);
        debounceMs[i] = prefs.getUShort(k2, DEFAULT_DEBOUNCE_MS);
        pulseMs   [i] = prefs.getUShort(k3, DEFAULT_PULSE_MS);
    }

    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        char ks[24];
        snprintf(ks, sizeof(ks), "%s%d", PREFS_RELAY_STATE_PREFIX, i);
        outputStates[i] = prefs.getBool(ks, false);
    }
    prefs.end();

    LOG_INFO("IOManager: config carregada");
}