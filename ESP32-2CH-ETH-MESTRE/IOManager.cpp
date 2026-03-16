#include "IOManager.h"

const uint8_t IOManager::OUTPUT_PINS[NUM_LOCAL_OUTPUTS] = {
    LOCAL_OUTPUT_1_PIN,   // GPIO14 — rele1
    LOCAL_OUTPUT_2_PIN    // GPIO12 — rele2
};

const uint8_t IOManager::INPUT_PINS[NUM_LOCAL_INPUTS] = {
    LOCAL_INPUT_1_PIN,    // GPIO39 — InC01
    LOCAL_INPUT_2_PIN     // GPIO37 — InC02
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
        inputMode [i] = INPUT_MODE_TRANSITION;  // padrão: ativo, faz toggle
        debounceMs[i] = DEFAULT_DEBOUNCE_MS;
        pulseMs   [i] = DEFAULT_PULSE_MS;
    }
}

// ==================== BEGIN ====================

void IOManager::begin() {
    // Pinos de saída local
    for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
        pinMode(OUTPUT_PINS[i], OUTPUT);
        digitalWrite(OUTPUT_PINS[i],
            (relayLogic == RELAY_LOGIC_INVERTED) ? HIGH : LOW);
    }

    // Pinos de entrada local (INPUT_ONLY GPIOs 37 e 39)
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        pinMode(INPUT_PINS[i], INPUT);
        lastInputRaw[i] = digitalRead(INPUT_PINS[i]);
    }

    // Carrega config + estados da NVS
    loadConfig();

    // Aplica estado inicial nas saídas locais
    applyInitialState();

    // Empurra config completa + estados para os escravos no boot
    // Isso garante que os escravos partam com a última configuração salva
    pushSlavesConfig();

    LOG_INFO("IOManager iniciado (2 saidas + 2 entradas locais + 16 via I2C = 18CH)");
}

void IOManager::applyInitialState() {
    if (initialState == INITIAL_STATE_ON) {
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) setOutput(i, true);

    } else if (initialState == INITIAL_STATE_LAST) {
        // Aplica estados locais restaurados pelo loadConfig() no hardware
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
            applyOutputHardware(i, outputStates[i]);
        }
        // Estados dos escravos já foram pré-carregados em loadConfig() via slaves->setOutput()
        // O pushSlavesConfig() no boot enviará tudo via I2C

    } else {
        // INITIAL_STATE_OFF — zera saídas locais
        for (uint8_t i = 0; i < NUM_LOCAL_OUTPUTS; i++) {
            outputStates[i] = false;
            applyOutputHardware(i, false);
        }
    }
}

// ==================== SAÍDAS ====================

void IOManager::setOutput(uint8_t index, bool state) {
    if (index >= NUM_TOTAL_OUTPUTS) return;

    outputStates[index] = state;

    if (index < NUM_LOCAL_OUTPUTS) {
        applyOutputHardware(index, state);
    } else {
        uint8_t si = (index - NUM_LOCAL_OUTPUTS) / NUM_SLAVE_CHANNELS;
        uint8_t ci = (index - NUM_LOCAL_OUTPUTS) % NUM_SLAVE_CHANNELS;
        slaves->setOutput(si, ci, state);
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
            lastInputRaw[i]  = raw;
            debounceTimer[i] = now;
        }
        if ((now - debounceTimer[i]) >= debounceMs[i]) {
            if (raw != inputStates[i]) {
                processInput(i, raw);
            }
        }
    }

    // Entradas dos escravos (mestre só detecta mudança para MQTT; escravo já atuou localmente)
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

    // Pulsos ativos nas entradas locais
    for (uint8_t i = 0; i < NUM_LOCAL_INPUTS; i++) {
        if (pulseTimer[i] > 0 && (now - pulseTimer[i]) >= pulseMs[i]) {
            pulseTimer[i] = 0;
            if (i < NUM_LOCAL_OUTPUTS) setOutput(i, false);
        }
    }
}

void IOManager::processInput(uint8_t index, bool rawState) {
    if (index >= NUM_LOCAL_INPUTS) return;

    bool prevState = inputStates[index];
    inputStates[index] = rawState;

    if (onInputChanged) onInputChanged(index, rawState);

    if (inputMode[index] == INPUT_MODE_DISABLED) return;

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
    if (index < NUM_TOTAL_INPUTS)
        pulseMs[index] = constrain(ms, MIN_PULSE_MS, MAX_PULSE_MS);
}

uint16_t IOManager::getPulseTime(uint8_t index) {
    return (index < NUM_TOTAL_INPUTS) ? pulseMs[index] : DEFAULT_PULSE_MS;
}

// ==================== PUSH CONFIG AOS ESCRAVOS ====================

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

    // Modos, debounce e pulse de TODAS as 18 entradas
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        char k1[24], k2[24], k3[24];
        snprintf(k1, sizeof(k1), "%s%d", PREFS_INPUT_MODE_PREFIX, i);
        snprintf(k2, sizeof(k2), "%s%d", PREFS_DEBOUNCE_PREFIX,   i);
        snprintf(k3, sizeof(k3), "%s%d", PREFS_PULSE_PREFIX,      i);
        prefs.putUChar (k1, (uint8_t)inputMode [i]);
        prefs.putUShort(k2, debounceMs[i]);
        prefs.putUShort(k3, pulseMs   [i]);
    }

    // Estado atual de TODAS as 18 saídas (para INITIAL_STATE_LAST)
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        char ks[24];
        snprintf(ks, sizeof(ks), "%s%d", PREFS_RELAY_STATE_PREFIX, i);
        prefs.putBool(ks, outputStates[i]);
    }

    prefs.end();
    LOG_INFO("IOManager: config salva (18 canais de entrada + 18 estados de saida)");
}

void IOManager::loadConfig() {
    prefs.begin(PREFS_NAMESPACE, true);

    relayLogic   = (RelayLogic)  prefs.getUInt(PREFS_RELAY_LOGIC,   RELAY_LOGIC_NORMAL);
    initialState = (InitialState)prefs.getUInt(PREFS_INITIAL_STATE, INITIAL_STATE_OFF);

    // Modos, debounce e pulse de todas as 18 entradas
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        char k1[24], k2[24], k3[24];
        snprintf(k1, sizeof(k1), "%s%d", PREFS_INPUT_MODE_PREFIX, i);
        snprintf(k2, sizeof(k2), "%s%d", PREFS_DEBOUNCE_PREFIX,   i);
        snprintf(k3, sizeof(k3), "%s%d", PREFS_PULSE_PREFIX,      i);
        // Padrão TRANSITION: garante que funcione sem cfg prévia
        inputMode [i] = (InputMode)prefs.getUChar (k1, INPUT_MODE_TRANSITION);
        debounceMs[i] = prefs.getUShort(k2, DEFAULT_DEBOUNCE_MS);
        pulseMs   [i] = prefs.getUShort(k3, DEFAULT_PULSE_MS);
    }

    // Estados de todas as 18 saídas
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        char ks[24];
        snprintf(ks, sizeof(ks), "%s%d", PREFS_RELAY_STATE_PREFIX, i);
        outputStates[i] = prefs.getBool(ks, false);
    }

    prefs.end();

    // Pré-carrega estados dos escravos no SlaveManager
    // (serão enviados via I2C pelo pushSlavesConfig() no boot)
    for (uint8_t i = NUM_LOCAL_OUTPUTS; i < NUM_TOTAL_OUTPUTS; i++) {
        uint8_t si = (i - NUM_LOCAL_OUTPUTS) / NUM_SLAVE_CHANNELS;
        uint8_t ci = (i - NUM_LOCAL_OUTPUTS) % NUM_SLAVE_CHANNELS;
        slaves->setOutput(si, ci, outputStates[i]);
    }

    LOG_INFO("IOManager: config carregada:");
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        LOG_INFOF("  IN%02d: mode=%d  deb=%dms  pulse=%dms  | OUT%02d: %s",
            i + 1,
            (uint8_t)inputMode[i], debounceMs[i], pulseMs[i],
            i + 1,
            (i < NUM_TOTAL_OUTPUTS && outputStates[i]) ? "ON" : "OFF");
    }
}