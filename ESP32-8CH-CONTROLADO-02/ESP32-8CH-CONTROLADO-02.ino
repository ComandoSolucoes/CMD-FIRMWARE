////////////////////////////////////////////////////////////////////////////////////
// ESP32-8CH-CONTROLADO — Escravo I2C para ETH-18CH-MASTER
//
// Recebe do mestre (JSON):
//   {"do1":1,"do2":0,...,"do8":0}
//   {"do1":1,..., "cfg":{"in1":{"mode":1,"deb":50,"pulse":500},...}}
//
// Envia ao mestre (JSON):
//   {"di1":1,...,"di8":0,"do1":1,...,"do8":0}
//   ↑ inclui do1-do8 para o mestre aprender o estado real após acionamento local
//
// Proteção de acionamento local:
//   Quando uma entrada física aciona um relé, o escravo ignora o comando
//   do mestre por LOCAL_OVERRIDE_MS. Nesse intervalo o mestre lê o feedback
//   (do1-do8) e atualiza slaveData — no próximo envio já manda o valor correto.
//
// SEM persistência — o mestre é a única fonte de verdade.
//   Estados e config chegam via I2C no boot e sempre que necessário.
//
// ENDEREÇO I2C: altere I2C_SLAVE_ADDR conforme posição
//   Escravo 1 → 0x55
//   Escravo 2 → 0x56
////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>

// ==================== CONFIGURAÇÃO ====================

#define I2C_SLAVE_ADDR      0x56
#define I2C_SDA_PIN         21
#define I2C_SCL_PIN         22
#define NUM_CHANNELS        8

// Tempo em que o escravo ignora o mestre após acionamento local.
// Deve ser maior que um ciclo de polling do mestre (I2C_POLL_MS = 50ms).
// 300ms garante que o mestre leia o feedback antes de tentar sobrescrever.
#define LOCAL_OVERRIDE_MS   300

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

// ==================== CONFIG POR CANAL ====================

struct ChannelConfig {
    InputMode mode    = INPUT_MODE_DISABLED;
    uint16_t  debMs   = 50;
    uint16_t  pulseMs = 500;
};

// ==================== ESTADO GLOBAL ====================

bool          outputStates  [NUM_CHANNELS] = {};
bool          inputStates   [NUM_CHANNELS] = {};
bool          lastRawInput  [NUM_CHANNELS] = {};
uint32_t      debounceTimer [NUM_CHANNELS] = {};
bool          pulseActive   [NUM_CHANNELS] = {};
uint32_t      pulseStart    [NUM_CHANNELS] = {};
ChannelConfig chanCfg       [NUM_CHANNELS];

// Proteção de override local — millis() do último acionamento por entrada física.
// Zero = sem proteção ativa.
uint32_t      localOverrideAt[NUM_CHANNELS] = {};

// Buffer I2C RX
volatile bool newDataReceived = false;
char          rxBuffer[512]   = {};

// Buffer I2C TX — pré-montado fora do ISR.
// onRequest() é chamado em interrupção de hardware: sem String, sem JsonDocument.
static char     txBuffer[200] = {};
static uint16_t txLen         = 0;

// ==================== PROTÓTIPOS ====================

void buildTxBuffer();
void onReceive(int numBytes);
void onRequest();
void processReceivedJson();
void parseConfig(JsonObject cfg);
void processInput(uint8_t ch, bool newState);
void setOutput(uint8_t ch, bool state, bool local = false);

// ==================== BUFFER TX ====================

// Monta JSON com entradas (di1-di8) E saídas (do1-do8).
// O mestre lê do1-do8 e atualiza slaveData.outputs para não sobrescrever
// com valor antigo no próximo ciclo de envio.
// Deve ser chamado sempre que inputStates ou outputStates mudar.
void buildTxBuffer() {
    StaticJsonDocument<256> doc;
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[5];
        snprintf(key, sizeof(key), "di%d", i + 1);
        doc[key] = inputStates[i] ? 1 : 0;
        snprintf(key, sizeof(key), "do%d", i + 1);
        doc[key] = outputStates[i] ? 1 : 0;
    }
    String out;
    serializeJson(doc, out);
    txLen = (uint16_t)out.length();
    if (txLen >= sizeof(txBuffer)) txLen = sizeof(txBuffer) - 1;
    memcpy(txBuffer, out.c_str(), txLen);
    txBuffer[txLen] = '\0';
}

// ==================== CALLBACKS I2C ====================

void onReceive(int numBytes) {
    uint16_t len = 0;
    while (Wire.available() && len < (uint16_t)(sizeof(rxBuffer) - 1)) {
        rxBuffer[len++] = (char)Wire.read();
    }
    rxBuffer[len] = '\0';
    newDataReceived = true;
}

// ISR-safe: apenas copia buffer pré-montado, sem alocações dinâmicas
void onRequest() {
    Wire.write((uint8_t*)txBuffer, txLen);
}

// ==================== PROCESSAMENTO DO JSON ====================

void processReceivedJson() {
    char* start = strchr(rxBuffer, '{');
    if (!start) return;

    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, start);
    if (err) {
        Serial.printf("[CTRL] Erro JSON: %s\n", err.c_str());
        return;
    }

    // Atualiza saídas (do1-do8)
    // Se o canal estiver em override local (entrada física acionou recentemente),
    // ignora o comando do mestre. O mestre vai ler o feedback no próximo ciclo
    // e enviar o valor correto — sem sobrescrever o acionamento local.
    uint32_t now = millis();
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        char key[5];
        snprintf(key, sizeof(key), "do%d", i + 1);
        if (!doc[key].is<int>()) continue;

        bool state = doc[key].as<int>() != 0;

        if (localOverrideAt[i] > 0 && (now - localOverrideAt[i]) < LOCAL_OVERRIDE_MS) {
            // Override ativo — ignora mestre, mantém estado local
            continue;
        }

        // Override expirado ou inexistente — aplica comando do mestre normalmente
        localOverrideAt[i] = 0;
        outputStates[i]    = state;
        digitalWrite(OUTPUT_PINS[i], state ? HIGH : LOW);
    }

    // Atualiza configuração dos canais (opcional — só quando mestre envia "cfg")
    if (doc["cfg"].is<JsonObject>()) {
        JsonObject cfg = doc["cfg"].as<JsonObject>();
        parseConfig(cfg);
        Serial.println("[CTRL] Config recebida:");
        for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
            Serial.printf("  in%d → mode=%d  deb=%dms  pulse=%dms\n",
                i + 1, (uint8_t)chanCfg[i].mode,
                chanCfg[i].debMs, chanCfg[i].pulseMs);
        }
    }

    Serial.print("[CTRL] Saidas: ");
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) Serial.print(outputStates[i] ? "1" : "0");
    Serial.println();

    // Atualiza buffer TX — mestre lerá do1-do8 corretos na próxima requisição
    buildTxBuffer();
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
        if (ch["deb"].is<int>())
            chanCfg[i].debMs = (uint16_t)constrain(ch["deb"].as<int>(), 10, 1000);
        if (ch["pulse"].is<int>())
            chanCfg[i].pulseMs = (uint16_t)constrain(ch["pulse"].as<int>(), 50, 10000);
    }
}

// ==================== LÓGICA DE ENTRADA ====================

void processInput(uint8_t ch, bool newState) {
    bool prevState  = inputStates[ch];
    inputStates[ch] = newState;
    bool risingEdge = newState && !prevState;

    Serial.printf("[CTRL] IN%d: %s → %s  (mode=%d)\n",
        ch + 1,
        prevState ? "ON" : "OFF",
        newState  ? "ON" : "OFF",
        (uint8_t)chanCfg[ch].mode);

    switch (chanCfg[ch].mode) {
        case INPUT_MODE_DISABLED:
            break;

        case INPUT_MODE_TRANSITION:
            if (risingEdge) setOutput(ch, !outputStates[ch], true);
            break;

        case INPUT_MODE_PULSE:
            if (risingEdge) {
                setOutput(ch, true, true);
                pulseActive[ch] = true;
                pulseStart[ch]  = millis();
            }
            break;

        case INPUT_MODE_FOLLOW:
            setOutput(ch, newState, true);
            break;

        case INPUT_MODE_INVERTED:
            setOutput(ch, !newState, true);
            break;
    }

    // Atualiza buffer TX — entradas e saídas atualizadas para o mestre
    buildTxBuffer();
}

// local=true → acionamento por entrada física → ativa proteção de override
// local=false → acionamento pelo mestre → sem proteção
void setOutput(uint8_t ch, bool state, bool local) {
    if (ch >= NUM_CHANNELS) return;
    outputStates[ch] = state;
    digitalWrite(OUTPUT_PINS[ch], state ? HIGH : LOW);

    if (local) {
        // Marca timestamp do acionamento local.
        // processReceivedJson() vai ignorar comandos do mestre para este canal
        // até LOCAL_OVERRIDE_MS expirar — tempo suficiente para o mestre ler
        // o feedback e atualizar slaveData.outputs.
        localOverrideAt[ch] = millis();
    }

    buildTxBuffer();
    Serial.printf("[CTRL] OUT%d → %s%s\n",
        ch + 1, state ? "ON" : "OFF", local ? " (local)" : "");
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(300);

    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║   ESP32-8CH-CONTROLADO — Escravo I2C  ║");
    Serial.printf ("║   Endereço: 0x%02X                       ║\n", I2C_SLAVE_ADDR);
    Serial.println("║   SEM persistencia — mestre e master  ║");
    Serial.println("╚════════════════════════════════════════╝");

    // Saídas — todas desligadas ao ligar
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        digitalWrite(OUTPUT_PINS[i], LOW);
        pinMode(OUTPUT_PINS[i], OUTPUT);
    }

    // Entradas
    for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
        pinMode(INPUT_PINS[i], INPUT);
        lastRawInput[i] = digitalRead(INPUT_PINS[i]);
        inputStates[i]  = lastRawInput[i];
    }

    // Monta buffer TX inicial antes de ativar I2C
    buildTxBuffer();

    // Liga I2C como escravo
    Wire.setBufferSize(512);
    Wire.setPins(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.begin(I2C_SLAVE_ADDR);
    Wire.onReceive(onReceive);
    Wire.onRequest(onRequest);

    Serial.println("[CTRL] Pronto — aguardando mestre...");
}

// ==================== LOOP ====================

void loop() {
    uint32_t now = millis();

    // 1) Processa JSON recebido fora do ISR
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
            setOutput(i, false, false); // fim de pulso — não é local, mestre já sabe
        }
    }

    delay(5);
}
