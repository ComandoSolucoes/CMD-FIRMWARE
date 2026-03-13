#ifndef CONFIG_ESP01S_RELAY_H
#define CONFIG_ESP01S_RELAY_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL "ESP01S-RELAY"
#define FIRMWARE_VERSION "1.0.0"

// ==================== PINAGENS ESP8266 ====================
// GPIO disponíveis no ESP01S: GPIO0, GPIO2
// GPIO0 é o padrão para o relé (pode ser alterado via configuração)
#define DEFAULT_RELAY_PIN     0     // GPIO0 (padrão)

// Opções de pinagem disponíveis no ESP8266
#define GPIO_PIN_0            0
#define GPIO_PIN_1            1     // TX (geralmente usado para serial)
#define GPIO_PIN_2            2
#define GPIO_PIN_3            3     // RX (geralmente usado para serial)
#define GPIO_PIN_4            4     // D2 (disponível em outros modelos ESP)
#define GPIO_PIN_5            5     // D1
#define GPIO_PIN_12           12    // D6
#define GPIO_PIN_13           13    // D7
#define GPIO_PIN_14           14    // D5
#define GPIO_PIN_15           15    // D8
#define GPIO_PIN_16           16    // D0

// ==================== CONFIGURAÇÕES DE RELÉ ====================
// Lógica de acionamento do relé
enum RelayLogic {
    RELAY_LOGIC_NORMAL = 0,       // HIGH = ON, LOW = OFF
    RELAY_LOGIC_INVERTED = 1      // HIGH = OFF, LOW = ON (comum em módulos relé)
};

// Estado inicial do relé
enum InitialState {
    INITIAL_STATE_OFF = 0,        // Sempre desligado
    INITIAL_STATE_ON = 1,         // Sempre ligado
    INITIAL_STATE_LAST = 2        // Recupera último estado da flash
};

// Modo de operação (para futuras expansões)
enum OperationMode {
    MODE_NORMAL = 0,              // Controle manual
    MODE_TIMER = 1,               // Temporizador (liga e desliga automaticamente)
    MODE_PULSE = 2,               // Pulso (ativa por tempo definido)
    MODE_SCHEDULE = 3             // Agendamento (futuro)
};

// ==================== MQTT ====================
// Intervalo de publicação periódica
#define DEFAULT_PUBLISH_INTERVAL  30000  // 30 segundos
#define MIN_PUBLISH_INTERVAL      1000   // 1 segundo
#define MAX_PUBLISH_INTERVAL      300000 // 5 minutos

// Tópicos MQTT (relativos ao base topic: cmd-c/{MAC}/)
#define MQTT_TOPIC_RELAY_STATE    "relay/state"       // Publicação: "ON" ou "OFF"
#define MQTT_TOPIC_RELAY_SET      "relay/set"         // Subscrição: "ON", "OFF", "TOGGLE"
#define MQTT_TOPIC_STATUS         "status"            // Publicação: JSON completo
#define MQTT_TOPIC_UPTIME         "uptime"            // Publicação: tempo ligado
#define MQTT_TOPIC_PULSE          "relay/pulse"       // Subscrição: tempo em ms

// ==================== TEMPORIZADOR ====================
#define MIN_TIMER_SECONDS         1       // 1 segundo
#define MAX_TIMER_SECONDS         86400   // 24 horas
#define DEFAULT_TIMER_SECONDS     300     // 5 minutos

// ==================== PULSE MODE ====================
#define MIN_PULSE_MS              100     // 100ms
#define MAX_PULSE_MS              60000   // 60 segundos
#define DEFAULT_PULSE_MS          1000    // 1 segundo

// ==================== WATCHDOG ====================
#define WATCHDOG_TIMEOUT_SEC      30      // Reinicia se travar por 30s

// ==================== PREFERENCES (EEPROM/NVS) ====================
#define PREFS_NAMESPACE           "esp01s-relay"

// Chaves de configuração
#define PREFS_RELAY_PIN           "relay_pin"         // uint8_t
#define PREFS_RELAY_LOGIC         "relay_logic"       // RelayLogic (uint8_t)
#define PREFS_INITIAL_STATE       "init_state"        // InitialState (uint8_t)
#define PREFS_RELAY_STATE         "relay_state"       // bool
#define PREFS_OPERATION_MODE      "op_mode"           // OperationMode (uint8_t)
#define PREFS_TIMER_SECONDS       "timer_sec"         // uint32_t
#define PREFS_PULSE_MS            "pulse_ms"          // uint32_t
#define PREFS_PUBLISH_INTERVAL    "pub_interval"      // uint32_t
#define PREFS_STATS_TOTAL_ON      "total_on"          // uint32_t (segundos total ligado)
#define PREFS_STATS_SWITCH_COUNT  "switch_count"      // uint32_t (quantidade de acionamentos)

// ==================== LOGS ====================
#define LOG_PREFIX "[ESP01S-RELAY] "

// Macros de log
#define LOG_INFO(msg)       Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...) Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)      Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)
#define LOG_SUCCESS(msg)    Serial.println(LOG_PREFIX "✅ " msg)

// ==================== RECURSOS ADICIONAIS ====================
// Habilitar/desabilitar recursos
#define ENABLE_STATISTICS         true    // Estatísticas de uso
#define ENABLE_TIMER_MODE         true    // Modo temporizador
#define ENABLE_PULSE_MODE         true    // Modo pulso
#define ENABLE_ENERGY_MONITOR     false   // Monitor de energia (futuro - requer sensor)

#endif // CONFIG_ESP01S_RELAY_H
