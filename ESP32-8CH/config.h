#ifndef CONFIG_8CH_H
#define CONFIG_8CH_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL "8CH-IO"
#define FIRMWARE_VERSION "1.0.0"

// ==================== PINAGENS ====================
// Entradas Digitais (8 canais)
#define INPUT_1_PIN   36  // GPIO36 (ADC1_CH0, somente entrada)
#define INPUT_2_PIN   39  // GPIO39 (ADC1_CH3, somente entrada)
#define INPUT_3_PIN   34  // GPIO34 (ADC1_CH6, somente entrada)
#define INPUT_4_PIN   35  // GPIO35 (ADC1_CH7, somente entrada)
#define INPUT_5_PIN   32  // GPIO32 (ADC1_CH4)
#define INPUT_6_PIN   33  // GPIO33 (ADC1_CH5)
#define INPUT_7_PIN   25  // GPIO25 (ADC2_CH8, DAC1)
#define INPUT_8_PIN   26  // GPIO26 (ADC2_CH9, DAC2)

// Saídas (Relés) (8 canais)
#define OUTPUT_1_PIN  19  // GPIO19
#define OUTPUT_2_PIN  18  // GPIO18
#define OUTPUT_3_PIN  17  // GPIO17
#define OUTPUT_4_PIN  16  // GPIO16
#define OUTPUT_5_PIN  4   // GPIO4
#define OUTPUT_6_PIN  27  // GPIO27
#define OUTPUT_7_PIN  12  // GPIO12
#define OUTPUT_8_PIN  13  // GPIO13

#define NUM_INPUTS    8
#define NUM_OUTPUTS   8

// ==================== CONFIGURAÇÕES DE ENTRADA ====================
enum InputMode {
    INPUT_MODE_DISABLED = 0,
    INPUT_MODE_TRANSITION = 1,
    INPUT_MODE_PULSE = 2,
    INPUT_MODE_FOLLOW = 3,
    INPUT_MODE_INVERTED = 4
};

#define DEFAULT_DEBOUNCE_MS       50
#define DEFAULT_PULSE_MS          500
#define MIN_DEBOUNCE_MS           10
#define MAX_DEBOUNCE_MS           1000
#define MIN_PULSE_MS              50
#define MAX_PULSE_MS              10000

// ==================== CONFIGURAÇÕES DE SAÍDA ====================
enum RelayLogic {
    RELAY_LOGIC_NORMAL = 0,
    RELAY_LOGIC_INVERTED = 1
};

enum InitialState {
    INITIAL_STATE_OFF = 0,
    INITIAL_STATE_ON = 1,
    INITIAL_STATE_LAST = 2
};

// ==================== MQTT - FORMATO TASMOTA ====================
// Intervalo de telemetria (tele/STATE)
#define DEFAULT_TELEMETRY_INTERVAL  30000  // 30 segundos
#define MIN_TELEMETRY_INTERVAL      5000   // 5 segundos
#define MAX_TELEMETRY_INTERVAL      300000 // 5 minutos

// Prefixos dos tópicos (MAC é adicionado automaticamente)
// Comandos: cmd-c/{MAC}/POWER, cmd-c/{MAC}/POWER2, etc
#define MQTT_CMD_PREFIX    "cmd-c/"
#define MQTT_STAT_PREFIX   "stat/cmd-c/"
#define MQTT_TELE_PREFIX   "tele/cmd-c/"

// ==================== PREFERENCES (NVS) ====================
#define PREFS_NAMESPACE           "8ch-io"
#define PREFS_RELAY_LOGIC         "relay_logic"
#define PREFS_INITIAL_STATE       "init_state"
#define PREFS_RELAY_STATE_PREFIX  "relay_"
#define PREFS_INPUT_MODE_PREFIX   "in_mode_"
#define PREFS_DEBOUNCE_PREFIX     "debounce_"
#define PREFS_PULSE_PREFIX        "pulse_"
#define PREFS_TELEMETRY_INTERVAL  "tele_interval"

// ==================== LOGS ====================
#define LOG_PREFIX "[8CH] "
#define LOG_INFO(msg)       Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...) Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)      Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)

#endif // CONFIG_8CH_H