#ifndef CONFIG_LORA_SENSOR_H
#define CONFIG_LORA_SENSOR_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL "LORA-SENSOR"
#define FIRMWARE_VERSION "1.0.0"

// ==================== PINAGENS ====================
// Sensor Ultrassônico RCWL-1655 (modo GP10 / HC-SR04)
#define PIN_TRIG  32
#define PIN_ECHO  33

// NeoPixel
#define PIN_LED        2
#define NUMPIXELS      1
#define LED_BRIGHTNESS 60

// LoRa (SX1276/SX1278)
#define LORA_SCK   18
#define LORA_MISO  19
#define LORA_MOSI  23
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  26

// ==================== CONFIGURAÇÕES DO SENSOR ====================
// Timeout para leitura do sensor (microsegundos)
#define SENSOR_TIMEOUT_US      30000    // 30ms

// Temperatura ambiente padrão (°C) para compensação
#define DEFAULT_AMBIENT_TEMP   25.0f

// Limites de temperatura configurável
#define MIN_AMBIENT_TEMP       -10.0f
#define MAX_AMBIENT_TEMP       50.0f

// Intervalo de leitura padrão (milissegundos)
#define DEFAULT_READ_INTERVAL  30000    // 30 segundos
#define MIN_READ_INTERVAL      1000     // 1 segundo
#define MAX_READ_INTERVAL      300000   // 5 minutos

// Número de leituras consecutivas para média
#define NUM_READINGS_AVG       3

// ==================== CONFIGURAÇÕES LORA ====================
// Frequência
#define LORA_FREQUENCY         915E6    // 915 MHz (América)
// Alternativas: 433E6 (Ásia), 868E6 (Europa)

// Parâmetros de modulação padrão
#define DEFAULT_LORA_BANDWIDTH       125E3    // 125 kHz
#define DEFAULT_LORA_SPREADING       7        // SF7
#define DEFAULT_LORA_CODING_RATE     5        // 4/5
#define DEFAULT_LORA_TX_POWER        17       // 17 dBm
#define DEFAULT_LORA_SYNC_WORD       0x12     // Private network

// Limites configuráveis
#define MIN_LORA_TX_POWER        2
#define MAX_LORA_TX_POWER        20
#define MIN_LORA_SPREADING       6
#define MAX_LORA_SPREADING       12

// ==================== MQTT ====================
// Tópicos MQTT (relativos ao base topic: cmd-c/{MAC}/)
#define MQTT_TOPIC_DISTANCE      "sensor/distance"      // Publica: distância em cm
#define MQTT_TOPIC_STATUS        "sensor/status"        // Publica: JSON completo
#define MQTT_TOPIC_CONFIG        "sensor/config/set"    // Subscreve: configurações

// Intervalo de publicação MQTT padrão
#define DEFAULT_MQTT_PUBLISH_INTERVAL  30000   // 30 segundos
#define MIN_MQTT_PUBLISH_INTERVAL      5000    // 5 segundos
#define MAX_MQTT_PUBLISH_INTERVAL      300000  // 5 minutos

// ==================== LED FEEDBACK ====================
// Cores RGB
#define LED_COLOR_OFF         0,   0,   0
#define LED_COLOR_IDLE        0,   200, 0     // Verde = OK
#define LED_COLOR_READING     0,   0,   255   // Azul = lendo
#define LED_COLOR_TRANSMIT    255, 100, 0     // Laranja = transmitindo
#define LED_COLOR_ERROR       255, 0,   0     // Vermelho = erro
#define LED_COLOR_WIFI_AP     255, 255, 0     // Amarelo = modo AP

// Tempos de blink (ms)
#define LED_BLINK_ON_MS       80
#define LED_BLINK_OFF_MS      60

// ==================== PREFERENCES (NVS) ====================
#define PREFS_NAMESPACE           "lora-sensor"

// Chaves de configuração
#define PREFS_AMBIENT_TEMP        "amb_temp"          // float
#define PREFS_READ_INTERVAL       "read_interval"     // uint32_t
#define PREFS_LORA_TX_POWER       "lora_txpwr"        // uint8_t
#define PREFS_LORA_SPREADING      "lora_sf"           // uint8_t
#define PREFS_MQTT_INTERVAL       "mqtt_interval"     // uint32_t
#define PREFS_LED_ENABLED         "led_enabled"       // bool

// ==================== ESTRUTURAS DE DADOS ====================
struct SensorReading {
    float distance_cm;      // Distância medida (cm)
    float temperature;      // Temperatura usada na compensação
    bool valid;             // Leitura válida?
    unsigned long timestamp; // millis() da leitura
};

struct LoRaConfig {
    uint8_t txPower;        // 2-20 dBm
    uint8_t spreadingFactor; // 6-12
    uint8_t codingRate;     // 5-8 (4/5 a 4/8)
    uint32_t bandwidth;     // Hz
    uint8_t syncWord;       // 0x00-0xFF
};

// ==================== LOGS ====================
#define LOG_PREFIX "[LORA-SENSOR] "

#define LOG_INFO(msg)       Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...) Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)      Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)

#endif // CONFIG_LORA_SENSOR_H
