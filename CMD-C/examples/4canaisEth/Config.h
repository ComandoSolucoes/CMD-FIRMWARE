#ifndef CONFIG_4CH_H
#define CONFIG_4CH_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL "4CH-ETH"
#define FIRMWARE_VERSION "1.0.0"

// ==================== PINAGENS ====================
// Entradas Digitais
#define INPUT_1_PIN   39
#define INPUT_2_PIN   36
#define INPUT_3_PIN   15
#define INPUT_4_PIN   35

// Saídas (Relés)
#define OUTPUT_1_PIN  14
#define OUTPUT_2_PIN  12
#define OUTPUT_3_PIN  4
#define OUTPUT_4_PIN  2

#define NUM_INPUTS    4
#define NUM_OUTPUTS   4

// ==================== CONFIGURAÇÕES DE ENTRADA ====================
// Modos de operação das entradas
enum InputMode {
    INPUT_MODE_DISABLED = 0,      // Entrada desabilitada
    INPUT_MODE_TRANSITION = 1,    // Toggle: cada borda muda o estado
    INPUT_MODE_PULSE = 2,         // Pulso: ativa relé por X ms
    INPUT_MODE_FOLLOW = 3,        // Segue: HIGH=ON, LOW=OFF
    INPUT_MODE_INVERTED = 4       // Invertido: HIGH=OFF, LOW=ON
};

// Tempos padrão (ms)
#define DEFAULT_DEBOUNCE_MS       50    // Tempo de debounce padrão
#define DEFAULT_PULSE_MS          500   // Tempo de pulso padrão
#define MIN_DEBOUNCE_MS           10
#define MAX_DEBOUNCE_MS           1000
#define MIN_PULSE_MS              50
#define MAX_PULSE_MS              10000

// ==================== CONFIGURAÇÕES DE SAÍDA ====================
// Lógica de acionamento dos relés
enum RelayLogic {
    RELAY_LOGIC_NORMAL = 0,       // HIGH = ON, LOW = OFF
    RELAY_LOGIC_INVERTED = 1      // HIGH = OFF, LOW = ON
};

// Estado inicial dos relés
enum InitialState {
    INITIAL_STATE_OFF = 0,        // Sempre desligados
    INITIAL_STATE_ON = 1,         // Sempre ligados
    INITIAL_STATE_LAST = 2        // Recupera último estado da flash
};

// ==================== MQTT ====================
// Intervalo de publicação periódica
#define DEFAULT_PUBLISH_INTERVAL  30000  // 30 segundos
#define MIN_PUBLISH_INTERVAL      1000   // 1 segundo
#define MAX_PUBLISH_INTERVAL      300000 // 5 minutos

// Tópicos MQTT (relativos ao base topic: cmd-c/{MAC}/)
#define MQTT_TOPIC_RELAY_STATE    "relay/%d/state"    // Publicação: "ON" ou "OFF"
#define MQTT_TOPIC_RELAY_SET      "relay/%d/set"      // Subscrição: "ON", "OFF", "TOGGLE"
#define MQTT_TOPIC_RELAY_ALL_SET  "relay/all/set"     // Subscrição: "ON", "OFF"
#define MQTT_TOPIC_INPUT_STATE    "input/%d/state"    // Publicação: "HIGH" ou "LOW"
#define MQTT_TOPIC_STATUS         "status"            // Publicação: JSON completo

// ==================== WATCHDOG ====================
#define WATCHDOG_TIMEOUT_SEC      30    // Reinicia se travar por 30s

// ==================== PREFERENCES (NVS) ====================
#define PREFS_NAMESPACE           "4ch-eth"

// Chaves de configuração
#define PREFS_RELAY_LOGIC         "relay_logic"       // RelayLogic (uint8_t)
#define PREFS_INITIAL_STATE       "init_state"        // InitialState (uint8_t)
#define PREFS_RELAY_STATE_PREFIX  "relay_"            // relay_1, relay_2... (bool)
#define PREFS_INPUT_MODE_PREFIX   "in_mode_"          // in_mode_1, in_mode_2... (uint8_t)
#define PREFS_DEBOUNCE_PREFIX     "debounce_"         // debounce_1, debounce_2... (uint16_t)
#define PREFS_PULSE_PREFIX        "pulse_"            // pulse_1, pulse_2... (uint16_t)
#define PREFS_PUBLISH_INTERVAL    "pub_interval"      // uint32_t

// ==================== LOGS SIMPLIFICADOS ====================
#define LOG_PREFIX "[4CH] "

// Macros de log simples (sempre ativo, sem níveis)
#define LOG_INFO(msg)       Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...) Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)      Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)

#endif // CONFIG_4CH_H