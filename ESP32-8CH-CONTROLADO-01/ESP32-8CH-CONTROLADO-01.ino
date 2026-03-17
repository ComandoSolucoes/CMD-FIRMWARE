////////////////////////////////////////////////////////////////////////////////////
// ESP32-8CH-SLAVE — Firmware Monolítico v1.0.4
//
// FIX v1.0.4: onI2CReceive() faz APENAS cópia de bytes.
//   Alocar JsonDocument dentro da callback Wire causa stack overflow
//   (a task Wire tem stack reduzida). Todo processamento vai para loop().
//
//   Para garantir que onI2CRequest() sirva txBuffer atualizado mesmo com
//   o mestre fazendo endTransmission()+requestFrom() em sequência:
//   → processamento do rxBuffer é feito no INÍCIO do loop(), antes de
//     qualquer delay(), com prioridade máxima.
//   → txBuffer é atualizado imediatamente após aplicar os outputs.
//   → O mestre pode ter 1 ciclo de "stale" no pior caso, mas com o
//     modelo desired/confirmed do novo mestre isso é tolerado.
//
// ⚠️  Para o 2º escravo: altere I2C_SLAVE_ADDR para 0x56
////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <esp_task_wdt.h>

#define FIRMWARE_VERSION      "1.0.5"
#define DEVICE_MODEL          "8CH-SLAVE"

#define I2C_SLAVE_ADDR        0x55
#define I2C_SDA_PIN           21
#define I2C_SCL_PIN           22
#define I2C_BUF_SIZE          512

#define NUM_CHANNELS          8

// Tempo que o escravo ignora o mestre após acionamento físico.
// Deve cobrir pelo menos 2 ciclos I2C do mestre (I2C_POLL_MS=20ms).
#define LOCAL_OVERRIDE_MS     200

const uint8_t OUTPUT_PINS[NUM_CHANNELS] = { 19, 18, 17, 16,  4, 27, 12, 13 };
const uint8_t INPUT_PINS [NUM_CHANNELS] = { 36, 39, 34, 35, 32, 33, 25, 26 };

enum InputMode : uint8_t {
    INPUT_MODE_DISABLED   = 0,
    INPUT_MODE_TRANSITION = 1,
    INPUT_MODE_PULSE      = 2,
    INPUT_MODE_FOLLOW     = 3,
    INPUT_MODE_INVERTED   = 4
};

enum RelayLogic : uint8_t {
    RELAY_LOGIC_NORMAL   = 0,
    RELAY_LOGIC_INVERTED = 1
};

struct ChannelConfig {
    InputMode mode    = INPUT_MODE_TRANSITION;
    uint16_t  debMs   = 50;
    uint16_t  pulseMs = 500;
};

#define LOG_PREFIX "[8CH-SLAVE] "
#define LOG_INFO(m)         Serial.println(LOG_PREFIX m)
#define LOG_INFOF(f, ...)   Serial.printf(LOG_PREFIX f "\n", ##__VA_ARGS__)
#define LOG_WARN(m)         Serial.println(LOG_PREFIX "! " m)

#define WATCHDOG_TIMEOUT_SEC  30
#define PREFS_NAMESPACE       "8ch-slave"

// ==================== ESTADO GLOBAL ====================

bool          outputStates      [NUM_CHANNELS] = {};
RelayLogic    relayLogic                       = RELAY_LOGIC_NORMAL;
bool          inputStates       [NUM_CHANNELS] = {};
bool          lastInputRaw      [NUM_CHANNELS] = {};
uint32_t      debounceTimer     [NUM_CHANNELS] = {};
uint32_t      pulseTimer        [NUM_CHANNELS] = {};
ChannelConfig channelCfg        [NUM_CHANNELS];

// Override local — impede mestre de sobrescrever acionamento físico
bool     localOverride      [NUM_CHANNELS] = {};
uint32_t localOverrideTimer [NUM_CHANNELS] = {};

// ==================== BUFFERS I2C ====================
// rxBuffer: preenchido na ISR (apenas cópia de bytes)
// txBuffer: lido pela ISR onI2CRequest
// rxReady:  flag atômica ISR→loop

volatile char     rxBuffer[I2C_BUF_SIZE];
volatile bool     rxReady = false;

char     txBuffer[300];
uint16_t txLen = 0;

Preferences prefs;

// ==================== PROTÓTIPOS ====================

void applyOutputHardware(uint8_t index, bool state);
void setOutputLocal(uint8_t index, bool state);
void toggleOutputLocal(uint8_t index);
void buildTxJson();
void processRxBuffer();
void applyOutputsFromBuffer(JsonDocument& doc);
void processCfgFromBuffer(JsonDocument& doc);
void handleInputs();
void processInput(uint8_t index, bool rawState);
void applyInitialOutputs();
void saveConfig();
void saveOutputStates();
void loadConfig();

// ==================== I2C CALLBACKS ====================
// REGRA: callbacks Wire têm stack limitada.
// onI2CReceive → APENAS copia bytes, seta flag.
// onI2CRequest → APENAS escreve txBuffer já preparado pelo loop().

void onI2CReceive(int numBytes) {
    uint16_t i = 0;
    while (Wire.available() && i < (I2C_BUF_SIZE - 1))
        rxBuffer[i++] = (char)Wire.read();
    rxBuffer[i] = '\0';
    rxReady = true;
}

void onI2CRequest() {
    Wire.write((const uint8_t*)txBuffer, txLen);
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("=====================================");
    Serial.printf("[%s] Firmware v%s  addr=0x%02X\n",
                  DEVICE_MODEL, FIRMWARE_VERSION, I2C_SLAVE_ADDR);
    Serial.println("=====================================\n");

    esp_task_wdt_deinit();
    esp_task_wdt_config_t wdtCfg = {
        .timeout_ms     = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic  = true
    };
    esp_task_wdt_init(&wdtCfg);
    esp_task_wdt_add(NULL);

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        pinMode(OUTPUT_PINS[i], OUTPUT);
        digitalWrite(OUTPUT_PINS[i], LOW);
    }
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        uint8_t pin    = INPUT_PINS[i];
        bool inputOnly = (pin==36||pin==39||pin==34||pin==35);
        pinMode(pin, inputOnly ? INPUT : INPUT_PULLUP);
        lastInputRaw[i] = digitalRead(pin);
        inputStates[i]  = lastInputRaw[i];
    }

    loadConfig();
    applyInitialOutputs();

    Wire.setBufferSize(I2C_BUF_SIZE);
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin((uint8_t)I2C_SLAVE_ADDR);
    Wire.onReceive(onI2CReceive);
    Wire.onRequest(onI2CRequest);

    buildTxJson();  // txBuffer inicial antes do mestre chegar
    LOG_INFO("Pronto - aguardando mestre...\n");
}

// ==================== LOOP ====================

void loop() {
    esp_task_wdt_reset();

    // 1) Processa RX — PRIMEIRO, antes de qualquer delay
    //    Garante que txBuffer seja atualizado o mais rápido possível
    //    para o próximo onI2CRequest
    if (rxReady) {
        rxReady = false;
        processRxBuffer();
    }

    // 2) Expira localOverride
    uint32_t now = millis();
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (localOverride[i] && (now - localOverrideTimer[i]) >= LOCAL_OVERRIDE_MS) {
            localOverride[i] = false;
        }
    }

    // 3) Entradas físicas
    handleInputs();

    delay(1);
}

// ==================== PROCESSA RX ====================

void processRxBuffer() {
    JsonDocument doc;
    if (deserializeJson(doc, (const char*)rxBuffer) != DeserializationError::Ok) {
        LOG_WARN("JSON invalido no rxBuffer");
        return;
    }

    // Aplica outputs — respeita localOverride
    applyOutputsFromBuffer(doc);

    // Reconstrói txBuffer com estado atualizado
    buildTxJson();

    // Processa cfg se presente (salva na NVS)
    if (doc["cfg"].is<JsonObject>()) {
        processCfgFromBuffer(doc);
    }
}

// ==================== APPLY OUTPUTS ====================

void applyOutputsFromBuffer(JsonDocument& doc) {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        // Canal protegido por acionamento físico — ignora comando do mestre
        if (localOverride[i]) continue;

        String key = "do" + String(i + 1);
        if (doc[key].is<int>()) {
            bool newState = doc[key].as<int>() != 0;
            if (newState != outputStates[i]) {
                outputStates[i] = newState;
                applyOutputHardware(i, newState);
            }
        }
    }
}

// ==================== PROCESS CFG ====================

void processCfgFromBuffer(JsonDocument& doc) {
    JsonObject cfg  = doc["cfg"].as<JsonObject>();
    bool cfgChanged = false;

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        String key = "in" + String(i + 1);
        if (!cfg[key].is<JsonObject>()) continue;
        JsonObject ch = cfg[key].as<JsonObject>();
        if (ch["mode"].is<int>())
            channelCfg[i].mode    = (InputMode)ch["mode"].as<int>();
        if (ch["deb"].is<int>())
            channelCfg[i].debMs   = constrain(ch["deb"].as<int>(),   10, 1000);
        if (ch["pulse"].is<int>())
            channelCfg[i].pulseMs = constrain(ch["pulse"].as<int>(), 50, 10000);
        cfgChanged = true;
    }

    if (cfgChanged) {
        saveConfig();
        LOG_INFO("Config atualizada e salva");
        for (uint8_t i = 0; i < NUM_CHANNELS; i++)
            LOG_INFOF("  CH%d: mode=%d deb=%dms pulse=%dms",
                i+1, channelCfg[i].mode, channelCfg[i].debMs, channelCfg[i].pulseMs);
    }
}

// ==================== TX ====================

void buildTxJson() {
    JsonDocument doc;
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        doc["di" + String(i+1)] = inputStates[i]  ? 1 : 0;
        doc["do" + String(i+1)] = outputStates[i] ? 1 : 0;
    }
    txLen = serializeJson(doc, txBuffer, sizeof(txBuffer));
}

// ==================== SAIDAS ====================

void applyOutputHardware(uint8_t index, bool state) {
    bool level = (relayLogic == RELAY_LOGIC_INVERTED) ? !state : state;
    digitalWrite(OUTPUT_PINS[index], level ? HIGH : LOW);
}

// Acionamento local (botão físico) — ativa override
void setOutputLocal(uint8_t index, bool state) {
    if (index >= NUM_CHANNELS) return;
    outputStates[index]       = state;
    localOverride[index]      = true;
    localOverrideTimer[index] = millis();
    applyOutputHardware(index, state);
}

void toggleOutputLocal(uint8_t index) {
    if (index >= NUM_CHANNELS) return;
    setOutputLocal(index, !outputStates[index]);
}

// ==================== ENTRADAS ====================

void handleInputs() {
    uint32_t now = millis();
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        bool raw = digitalRead(INPUT_PINS[i]);
        if (raw != lastInputRaw[i]) {
            lastInputRaw[i]  = raw;
            debounceTimer[i] = now;
        }
        if ((now - debounceTimer[i]) >= channelCfg[i].debMs && raw != inputStates[i])
            processInput(i, raw);

        if (pulseTimer[i] > 0 && (now - pulseTimer[i]) >= channelCfg[i].pulseMs) {
            pulseTimer[i] = 0;
            setOutputLocal(i, false);
            buildTxJson();
        }
    }
}

void processInput(uint8_t index, bool rawState) {
    bool prevState     = inputStates[index];
    inputStates[index] = rawState;
    buildTxJson();

    if (channelCfg[index].mode == INPUT_MODE_DISABLED) return;
    bool risingEdge = rawState && !prevState;

    switch (channelCfg[index].mode) {
        case INPUT_MODE_TRANSITION:
            if (risingEdge) { toggleOutputLocal(index); buildTxJson(); }
            break;
        case INPUT_MODE_PULSE:
            if (risingEdge) {
                setOutputLocal(index, true);
                pulseTimer[index] = millis();
                buildTxJson();
            }
            break;
        case INPUT_MODE_FOLLOW:
            setOutputLocal(index, rawState);  buildTxJson();
            break;
        case INPUT_MODE_INVERTED:
            setOutputLocal(index, !rawState); buildTxJson();
            break;
        default: break;
    }
}

// ==================== ESTADO INICIAL ====================

void applyInitialOutputs() {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
        applyOutputHardware(i, outputStates[i]);
    LOG_INFO("Estados restaurados da NVS");
}

// ==================== NVS ====================

void saveConfig() {
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putUChar("relay_logic", (uint8_t)relayLogic);
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char k[20];
        snprintf(k,sizeof(k),"mode_%d", i);  prefs.putUChar (k,(uint8_t)channelCfg[i].mode);
        snprintf(k,sizeof(k),"deb_%d",  i);  prefs.putUShort(k,channelCfg[i].debMs);
        snprintf(k,sizeof(k),"pulse_%d",i);  prefs.putUShort(k,channelCfg[i].pulseMs);
        snprintf(k,sizeof(k),"out_%d",  i);  prefs.putBool  (k,outputStates[i]);
    }
    prefs.end();
}

void saveOutputStates() {
    prefs.begin(PREFS_NAMESPACE, false);
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char k[20];
        snprintf(k,sizeof(k),"out_%d",i);
        prefs.putBool(k,outputStates[i]);
    }
    prefs.end();
}

void loadConfig() {
    prefs.begin(PREFS_NAMESPACE, true);
    relayLogic = (RelayLogic)prefs.getUChar("relay_logic", RELAY_LOGIC_NORMAL);
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char k[20];
        snprintf(k,sizeof(k),"mode_%d", i); channelCfg[i].mode    = (InputMode)prefs.getUChar (k,INPUT_MODE_TRANSITION);
        snprintf(k,sizeof(k),"deb_%d",  i); channelCfg[i].debMs   =            prefs.getUShort(k,50);
        snprintf(k,sizeof(k),"pulse_%d",i); channelCfg[i].pulseMs =            prefs.getUShort(k,500);
        snprintf(k,sizeof(k),"out_%d",  i); outputStates[i]       =            prefs.getBool  (k,false);
    }
    prefs.end();
    LOG_INFO("Config carregada da NVS:");
    for (uint8_t i = 0; i < NUM_CHANNELS; i++)
        LOG_INFOF("  CH%d: mode=%d deb=%3dms pulse=%4dms out=%s",
            i+1,(uint8_t)channelCfg[i].mode,
            channelCfg[i].debMs,channelCfg[i].pulseMs,
            outputStates[i]?"ON":"OFF");
}
