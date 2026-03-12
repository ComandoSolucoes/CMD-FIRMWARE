#ifndef CONFIG_ETH18CH_H
#define CONFIG_ETH18CH_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL     "ETH-18CH-MASTER"
#define FIRMWARE_VERSION "1.0.0"

// ==================== ETHERNET (WT32-ETH01 / LAN8720) ====================
// Pinos fixos do WT32-ETH01 — NÃO alterar
#define ETH_PHY_ADDR    1
#define ETH_PHY_TYPE    ETH_PHY_LAN8720
#define ETH_PHY_POWER   16
#define ETH_PHY_MDC     23
#define ETH_PHY_MDIO    18
#define ETH_CLK_MODE    ETH_CLOCK_GPIO0_IN

// ==================== I2C (BARRAMENTO) ====================
#define I2C_SDA_PIN     4      // Pino SDA (igual ao exemplo do fornecedor)
#define I2C_SCL_PIN     2      // Pino SCL (igual ao exemplo do fornecedor)
#define I2C_FREQ        100000 // 100 kHz

// Endereços I2C dos escravos 8CH
#define I2C_SLAVE1_ADDR 0x08   // Escravo 1 — altere conforme seu hardware
#define I2C_SLAVE2_ADDR 0x09   // Escravo 2 — altere conforme seu hardware

// Timeout de comunicação I2C (ms)
#define I2C_TIMEOUT_MS  50
// Intervalo entre ciclos de polling I2C (ms)
#define I2C_POLL_MS     20

// ==================== PINOS LOCAIS (MASTER) ====================
// Saídas locais (2 relés no próprio master)
#define LOCAL_OUTPUT_1_PIN  14  // GPIO14
#define LOCAL_OUTPUT_2_PIN  15  // GPIO15

// Entradas locais (2 entradas digitais no próprio master)
#define LOCAL_INPUT_1_PIN   34  // GPIO34 (somente entrada)
#define LOCAL_INPUT_2_PIN   35  // GPIO35 (somente entrada)

// ==================== CANAIS TOTAIS ====================
#define NUM_LOCAL_OUTPUTS   2
#define NUM_LOCAL_INPUTS    2
#define NUM_SLAVE_CHANNELS  8   // Canais por escravo (8 in + 8 out)
#define NUM_SLAVES          2
#define NUM_TOTAL_OUTPUTS   (NUM_LOCAL_OUTPUTS + NUM_SLAVES * NUM_SLAVE_CHANNELS)  // 18
#define NUM_TOTAL_INPUTS    (NUM_LOCAL_INPUTS  + NUM_SLAVES * NUM_SLAVE_CHANNELS)  // 18

// ==================== MAPEAMENTO DE CANAIS ====================
// Saídas:  1-2   → locais
//          3-10  → escravo 1 (do1-do8)
//          11-18 → escravo 2 (do1-do8)
//
// Entradas: 1-2   → locais
//           3-10  → escravo 1 (di1-di8)
//           11-18 → escravo 2 (di1-di8)

// ==================== CONFIGURAÇÕES DE ENTRADA ====================
enum InputMode {
    INPUT_MODE_DISABLED   = 0,
    INPUT_MODE_TRANSITION = 1,  // Toggle na borda de subida
    INPUT_MODE_PULSE      = 2,  // Pulso por tempo definido
    INPUT_MODE_FOLLOW     = 3,  // Segue estado da entrada
    INPUT_MODE_INVERTED   = 4   // Inverte estado da entrada
};

#define DEFAULT_DEBOUNCE_MS  50
#define DEFAULT_PULSE_MS     500
#define MIN_DEBOUNCE_MS      10
#define MAX_DEBOUNCE_MS      1000
#define MIN_PULSE_MS         50
#define MAX_PULSE_MS         10000

// ==================== CONFIGURAÇÕES DE SAÍDA ====================
enum RelayLogic {
    RELAY_LOGIC_NORMAL   = 0,   // HIGH = ON
    RELAY_LOGIC_INVERTED = 1    // HIGH = OFF
};

enum InitialState {
    INITIAL_STATE_OFF  = 0,
    INITIAL_STATE_ON   = 1,
    INITIAL_STATE_LAST = 2      // Recupera da flash
};

// ==================== MQTT — FORMATO TASMOTA ====================
#define DEFAULT_TELEMETRY_INTERVAL  30000   // 30 s
#define MIN_TELEMETRY_INTERVAL      5000    // 5 s
#define MAX_TELEMETRY_INTERVAL      300000  // 5 min

// Prefixos Tasmota
#define MQTT_CMD_PREFIX   "cmd-c/"
#define MQTT_STAT_PREFIX  "stat/cmd-c/"
#define MQTT_TELE_PREFIX  "tele/cmd-c/"

// ==================== PREFERENCES (NVS) ====================
#define PREFS_NAMESPACE          "eth18ch"
#define PREFS_RELAY_LOGIC        "relay_logic"
#define PREFS_INITIAL_STATE      "init_state"
#define PREFS_RELAY_STATE_PREFIX "relay_"
#define PREFS_INPUT_MODE_PREFIX  "in_mode_"
#define PREFS_DEBOUNCE_PREFIX    "debounce_"
#define PREFS_PULSE_PREFIX       "pulse_"
#define PREFS_TELE_INTERVAL      "tele_interval"

// ==================== LOGS ====================
#define LOG_PREFIX "[ETH-18CH] "
#define LOG_INFO(msg)        Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...)  Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)       Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)
#define LOG_WARN(msg)        Serial.println(LOG_PREFIX "⚠️ " msg)

#endif // CONFIG_ETH18CH_H