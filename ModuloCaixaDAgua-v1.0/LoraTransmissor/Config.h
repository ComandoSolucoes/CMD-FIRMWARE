#ifndef CONFIG_LORA_SENSOR_H
#define CONFIG_LORA_SENSOR_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL "LORA-SENSOR"
#define FIRMWARE_VERSION "1.0.0"

// ==================== PINAGENS ====================
// Sensor Ultrassônico
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

// ==================== CONFIGURAÇÕES LORA ====================
// ⚠️ ATENÇÃO: DEVEM SER IDÊNTICOS NO TRANSMISSOR E RECEPTOR!

#define LORA_FREQUENCY         915E6    // 915 MHz (América do Sul)
                                        // 433E6 para Ásia
                                        // 868E6 para Europa

// Parâmetros de Modulação
#define DEFAULT_LORA_SPREADING       7        // SF7 (6-12)
#define DEFAULT_LORA_BANDWIDTH       125E3    // 125 kHz
#define DEFAULT_LORA_CODING_RATE     5        // 4/5 (5-8)
#define DEFAULT_LORA_TX_POWER        17       // 17 dBm (2-20)
#define DEFAULT_LORA_SYNC_WORD       0x12     // 0x12 = rede privada

// Limites
#define MIN_LORA_TX_POWER        2
#define MAX_LORA_TX_POWER        20
#define MIN_LORA_SPREADING       6
#define MAX_LORA_SPREADING       12

// ==================== SENSOR ====================
#define SENSOR_TIMEOUT_US      30000
#define DEFAULT_AMBIENT_TEMP   25.0f
#define MIN_AMBIENT_TEMP       -10.0f
#define MAX_AMBIENT_TEMP       50.0f
#define DEFAULT_READ_INTERVAL  30000
#define MIN_READ_INTERVAL      1000
#define MAX_READ_INTERVAL      300000
#define NUM_READINGS_AVG       3

// ==================== MQTT ====================
#define MQTT_TOPIC_DISTANCE      "sensor/distance"
#define MQTT_TOPIC_STATUS        "sensor/status"
#define MQTT_TOPIC_CONFIG        "sensor/config/set"
#define DEFAULT_MQTT_PUBLISH_INTERVAL  30000
#define MIN_MQTT_PUBLISH_INTERVAL      5000
#define MAX_MQTT_PUBLISH_INTERVAL      300000

// ==================== LED FEEDBACK ====================
#define LED_COLOR_OFF         0,   0,   0
#define LED_COLOR_IDLE        0,   200, 0
#define LED_COLOR_READING     0,   0,   255
#define LED_COLOR_TRANSMIT    255, 100, 0
#define LED_COLOR_ERROR       255, 0,   0
#define LED_COLOR_WIFI_AP     255, 255, 0

#define LED_BLINK_ON_MS       80
#define LED_BLINK_OFF_MS      60

// ==================== PREFERENCES ====================
#define PREFS_NAMESPACE           "lora-sensor"
#define PREFS_AMBIENT_TEMP        "amb_temp"
#define PREFS_READ_INTERVAL       "read_interval"
#define PREFS_LORA_TX_POWER       "lora_txpwr"
#define PREFS_LORA_SPREADING      "lora_sf"
#define PREFS_MQTT_INTERVAL       "mqtt_interval"
#define PREFS_LED_ENABLED         "led_enabled"

// ==================== ESTRUTURAS ====================
struct SensorReading {
    float distance_cm;
    float temperature;
    bool valid;
    unsigned long timestamp;
};

struct LoRaConfig {
    uint8_t txPower;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint32_t bandwidth;
    uint8_t syncWord;
};

// ==================== LOGS ====================
#define LOG_PREFIX "[LORA-SENSOR] "

#define LOG_INFO(msg)       Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...) Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)      Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)

#endif