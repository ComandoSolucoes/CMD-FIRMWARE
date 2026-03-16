////////////////////////////////////////////////////////////////////////////////////
// ESP32-8CH-CONTROLADO — Escravo I2C para ETH-18CH-MASTER
//
// Recebe do mestre (JSON):
//   Saídas:  {"do1":1,"do2":0,...,"do8":0}
//   Config:  {"do1":1,..., "cfg":{"in1":{"mode":1,"deb":50,"pulse":500},...}}
//            (campo "cfg" é opcional — só enviado quando o mestre quiser configurar)
//
// Envia ao mestre (JSON):
//   Entradas: {"di1":1,"di2":0,...,"di8":0}  ← estado debounced de cada canal
//
// Modos de entrada (campo "mode"):
//   0 = DISABLED   — não processa, só reporta estado
//   1 = TRANSITION — toggle no relé correspondente a cada borda de subida
//   2 = PULSE      — liga o relé por "pulse" ms na borda de subida
//   3 = FOLLOW     — relé segue o estado da entrada
//   4 = INVERTED   — relé é o inverso da entrada
//
// ENDEREÇO I2C: altere I2C_SLAVE_ADDR conforme jumper/posição
//   Escravo 1 → 0x55
//   Escravo 2 → 0x56
////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// ==================== CONFIGURAÇÃO ====================

#define I2C_SLAVE_ADDR   0x55
#define I2C_SDA_PIN      21
#define I2C_SCL_PIN      22

#define NUM_CHANNELS     8
#define PREFS_NAMESPACE  "8ch-ctrl"

// Pinos de saída (relés)
const uint8_t OUTPUT_PINS[NUM_CHANNELS] = { 19, 18, 17, 16, 4, 27, 12, 13 };

// Pinos de entrada
const uint8_t INPUT_PINS[NUM_CHANNELS]  = { 36, 39, 34, 35, 32, 33, 25, 26 };

// ==================== MODOS DE ENTRADA ====================

enum InputMode : uint8_t {
    INPUT_MODE_DISABLED   = 0,
    INPUT_MODE_TRANSITION = 1,
    INPUT_MODE_PULSE      = 2,
    INPUT_MODE_FOLLOW     = 3,
    INPUT_MODE_INVERTED   = 4
};

// ==================== CONFIGURAÇÃO POR CANAL ====================

struct ChannelConfig {
    InputMode mode    = INPUT_MODE_DISABLED;
    uint16_t  debMs   = 50;
    uint16_t  pulseMs = 500;
};

// ==================== ESTADO GLOBAL ====================

bool outputStates[NUM_CHANNELS] = {};
bool inputStates[NUM_CHANNELS]  = {};

bool     lastRawInput[NUM_CHANNELS]  = {};
uint32_t debounceTimer[NUM_CHANNELS] = {};

bool     pulseActive[NUM_CHANNELS] = {};
uint32_t pulseStart[NUM_CHANNELS]  = {};

ChannelConfig chanCfg[NUM_CHANNELS];

// Buffer I2C
volatile bool newDataReceived = false;
char          rxBuffer[512]   = {};
uint16_t      rxLen           = 0;

// NVS: escrita adiada para não bloquear o barramento I2C
bool     nvsDirtyStates = false;
bool     nvsDirtyConfig = false;
uint32_t nvsWriteAfter  = 0;          // millis() a partir do qual pode gravar
#define  NVS_WRITE_DELAY_MS  200      // aguarda 200ms de inatividade no I2C

Preferences prefs;

// ==================== PROTÓTIPOS ====================

void onReceive(int numBytes);
void onRequest();
void processReceivedJson();
void parseConfig(JsonObject cfg);
void processInput(uint8_t ch, bool newState);
void setOutput(uint8_t ch, bool state);
void saveConfig();
void saveOutputStates();
void loadConfig();

// ==================== CALLBACKS I2C ====================

void onReceive(int numBytes) {
    uint16_t len = 0;
    while (Wire.available() && len < (uint16_t)(sizeof(rxBuffer) - 1)) {
        rxBuffer[len++] = (char)Wire.read();
    }
    rxBuffer[len] = '\0';
    rxLen = len;
    newDataReceived = true;
}

void onRequest() {
    JsonDocument doc;
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[5];
        snprintf(key, sizeof(key), "di%d", i + 1);
        doc[key] = inputStates[i] ? 1 : 0;
    }
    String out;
    serializeJson(doc, out);
    Wire.print(out);
}

// ==================== PROCESSAMENTO DO JSON ====================

void processReceivedJson() {
    char* start = strchr(rxBuffer, '{');
    if (!start) return;

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, start);
    if (err) {
        Serial.printf("[CTRL] Erro JSON: %s\n", err.c_str());
        return;
    }

    // 1) Atualiza saídas (do1–do8)
    bool outputChanged = false;
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[5];
        snprintf(key, sizeof(key), "do%d", i + 1);
        if (doc[key].is<int>()) {
            bool state = doc[key].as<int>() != 0;
            if (state != outputStates[i]) outputChanged = true;
            outputStates[i] = state;
            digitalWrite(OUTPUT_PINS[i], state ? HIGH : LOW);
        }
    }
    if (outputChanged) {
        nvsDirtyStates = true;
        nvsWriteAfter  = millis() + NVS_WRITE_DELAY_MS;
    }

    // 2) Atualiza configuração dos canais (campo "cfg" é opcional)
    if (doc["cfg"].is<JsonObject>()) {
        JsonObject cfg = doc["cfg"].as<JsonObject>();
        parseConfig(cfg);
        nvsDirtyConfig = true;
        nvsWriteAfter  = millis() + NVS_WRITE_DELAY_MS;

        Serial.println("[CTRL] Config atualizada e salva:");
        for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
            Serial.printf("  in%d → mode=%d  deb=%dms  pulse=%dms\n",
                i + 1, (uint8_t)chanCfg[i].mode,
                chanCfg[i].debMs, chanCfg[i].pulseMs);
        }
    }

    Serial.print("[CTRL] Saidas: ");
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) Serial.print(outputStates[i] ? "1" : "0");
    Serial.println();
}

void parseConfig(JsonObject cfg) {
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[5];
        snprintf(key, sizeof(key), "in%d", i + 1);
        if (!cfg[key].is<JsonObject>()) continue;
        JsonObject ch = cfg[key].as<JsonObject>();

        if (ch["mode"].is<uint8_t>()) {
            uint8_t m = ch["mode"].as<uint8_t>();
            if (m <= (uint8_t)INPUT_MODE_INVERTED) chanCfg[i].mode = (InputMode)m;
        }
        if (ch["deb"].is<int>())   chanCfg[i].debMs   = (uint16_t)constrain(ch["deb"].as<int>(),   10,   1000);
        if (ch["pulse"].is<int>()) chanCfg[i].pulseMs = (uint16_t)constrain(ch["pulse"].as<int>(), 50, 10000);
    }
}

// ==================== LÓGICA DE ENTRADA ====================

void processInput(uint8_t ch, bool newState) {
    bool prevState  = inputStates[ch];
    inputStates[ch] = newState;
    bool risingEdge = newState && !prevState;

    Serial.printf("[CTRL] IN%d: %s → %s\n",
        ch + 1, prevState ? "ON" : "OFF", newState ? "ON" : "OFF");

    switch (chanCfg[ch].mode) {
        case INPUT_MODE_DISABLED:
            break;

        case INPUT_MODE_TRANSITION:
            if (risingEdge) setOutput(ch, !outputStates[ch]);
            break;

        case INPUT_MODE_PULSE:
            if (risingEdge) {
                setOutput(ch, true);
                pulseActive[ch] = true;
                pulseStart[ch]  = millis();
            }
            break;

        case INPUT_MODE_FOLLOW:
            setOutput(ch, newState);
            break;

        case INPUT_MODE_INVERTED:
            setOutput(ch, !newState);
            break;
    }
}

void setOutput(uint8_t ch, bool state) {
    if (ch >= NUM_CHANNELS) return;
    outputStates[ch] = state;
    digitalWrite(OUTPUT_PINS[ch], state ? HIGH : LOW);
    nvsDirtyStates = true;
    nvsWriteAfter  = millis() + NVS_WRITE_DELAY_MS;
    Serial.printf("[CTRL] OUT%d → %s\n", ch + 1, state ? "ON" : "OFF");
}

// ==================== PERSISTÊNCIA ====================

void saveOutputStates() {
    prefs.begin(PREFS_NAMESPACE, false);
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[8];
        snprintf(key, sizeof(key), "out%d", i);
        prefs.putBool(key, outputStates[i]);
    }
    prefs.end();
}

void saveConfig() {
    prefs.begin(PREFS_NAMESPACE, false);
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "mode%d",  i); prefs.putUChar(key,  (uint8_t)chanCfg[i].mode);
        snprintf(key, sizeof(key), "deb%d",   i); prefs.putUShort(key, chanCfg[i].debMs);
        snprintf(key, sizeof(key), "pulse%d", i); prefs.putUShort(key, chanCfg[i].pulseMs);
    }
    prefs.end();
}

void loadConfig() {
    prefs.begin(PREFS_NAMESPACE, true);
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[12];
        snprintf(key, sizeof(key), "mode%d",  i); chanCfg[i].mode    = (InputMode)prefs.getUChar(key,  INPUT_MODE_TRANSITION);
        snprintf(key, sizeof(key), "deb%d",   i); chanCfg[i].debMs   = prefs.getUShort(key, 50);
        snprintf(key, sizeof(key), "pulse%d", i); chanCfg[i].pulseMs = prefs.getUShort(key, 500);
        // Restaura último estado do relé
        char ks[8];
        snprintf(ks, sizeof(ks), "out%d", i);     outputStates[i]    = prefs.getBool(ks, false);
    }
    prefs.end();
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║   ESP32-8CH-CONTROLADO — Escravo I2C  ║");
    Serial.printf ("║   Endereço: 0x%02X                       ║\n", I2C_SLAVE_ADDR);
    Serial.println("╚════════════════════════════════════════╝");

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        digitalWrite(OUTPUT_PINS[i], LOW);
        pinMode(OUTPUT_PINS[i], OUTPUT);
        outputStates[i] = false;
    }

    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        pinMode(INPUT_PINS[i], INPUT);
        bool raw        = digitalRead(INPUT_PINS[i]);
        lastRawInput[i] = raw;
        inputStates[i]  = raw;
    }

    loadConfig();
    // Aplica estados restaurados nos relés (funciona mesmo sem mestre)
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        digitalWrite(OUTPUT_PINS[i], outputStates[i] ? HIGH : LOW);
    }
    Serial.println("[CTRL] Configuração e estados restaurados da NVS:");
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        Serial.printf("  in%d → mode=%d  deb=%dms  pulse=%dms  |  out%d → %s\n",
            i + 1, (uint8_t)chanCfg[i].mode, chanCfg[i].debMs, chanCfg[i].pulseMs,
            i + 1, outputStates[i] ? "ON" : "OFF");
    }

    Wire.setBufferSize(512); // JSON com cfg ~420 bytes — buffer padrao (128) e insuficiente
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin(I2C_SLAVE_ADDR);
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);

    Serial.println("[CTRL] Pronto — aguardando mestre...");
}

// ==================== LOOP ====================

void loop() {
    uint32_t now = millis();

    // 1) Processa JSON recebido (fora do callback — seguro para Serial/JSON)
    if (newDataReceived) {
        newDataReceived = false;
        processReceivedJson();
    }

    // 2) Debounce das entradas físicas
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        bool raw = digitalRead(INPUT_PINS[i]);
        if (raw != lastRawInput[i]) {
            lastRawInput[i]  = raw;
            debounceTimer[i] = now;
        }
        if ((now - debounceTimer[i]) >= chanCfg[i].debMs) {
            if (raw != inputStates[i]) processInput(i, raw);
        }
    }

    // 3) Expiração de pulsos ativos
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        if (pulseActive[i] && (now - pulseStart[i]) >= chanCfg[i].pulseMs) {
            pulseActive[i] = false;
            setOutput(i, false);
        }
    }

    // 4) Flush NVS adiado — só grava após NVS_WRITE_DELAY_MS de inatividade
    //    Evita bloquear o barramento I2C durante escritas na flash
    if ((nvsDirtyStates || nvsDirtyConfig) && (millis() >= nvsWriteAfter)) {
        if (nvsDirtyStates) { saveOutputStates(); nvsDirtyStates = false; }
        if (nvsDirtyConfig) { saveConfig();       nvsDirtyConfig = false; }
    }

    delay(5);
}
