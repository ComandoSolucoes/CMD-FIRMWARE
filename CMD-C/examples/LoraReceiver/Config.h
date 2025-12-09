#ifndef CONFIG_LORA_RECEIVER_H
#define CONFIG_LORA_RECEIVER_H

#include <Arduino.h>

// ==================== IDENTIFICAÇÃO ====================
#define DEVICE_MODEL "LORA-RECEIVER"
#define FIRMWARE_VERSION "1.0.0"

// ==================== PINAGENS ====================
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
// Frequência (deve ser igual ao transmissor)
#define LORA_FREQUENCY         915E6    // 915 MHz

// Parâmetros de modulação padrão (devem ser iguais ao transmissor)
#define DEFAULT_LORA_BANDWIDTH       125E3    // 125 kHz
#define DEFAULT_LORA_SPREADING       7        // SF7
#define DEFAULT_LORA_CODING_RATE     5        // 4/5
#define DEFAULT_LORA_SYNC_WORD       0x12     // Private network

// Limites configuráveis
#define MIN_LORA_SPREADING       6
#define MAX_LORA_SPREADING       12

// ==================== MQTT ====================
// Tópico onde publica mensagens recebidas
#define MQTT_TOPIC_RECEIVED      "lora/received"

// ==================== LOG DE MENSAGENS ====================
// Número de mensagens a armazenar
#define MAX_LOG_MESSAGES         5

// ==================== LED FEEDBACK ====================
// Cores RGB
#define LED_COLOR_OFF         0,   0,   0
#define LED_COLOR_IDLE        0,   200, 0     // Verde = aguardando
#define LED_COLOR_RECEIVING   0,   0,   255   // Azul = recebendo
#define LED_COLOR_ERROR       255, 0,   0     // Vermelho = erro
#define LED_COLOR_WIFI_AP     255, 255, 0     // Amarelo = modo AP

// Tempos de blink (ms)
#define LED_BLINK_ON_MS       80
#define LED_BLINK_OFF_MS      60

// ==================== PREFERENCES (NVS) ====================
#define PREFS_NAMESPACE           "lora-receiver"

// Chaves de configuração LoRa
#define PREFS_LORA_SPREADING      "lora_sf"           // uint8_t
#define PREFS_LORA_BANDWIDTH      "lora_bw"           // uint32_t
#define PREFS_LORA_CODING_RATE    "lora_cr"           // uint8_t
#define PREFS_LORA_SYNC_WORD      "lora_sw"           // uint8_t

// Chaves de configuração MQTT
#define PREFS_AUTO_PUBLISH        "auto_pub"          // bool

// ==================== ESTRUTURAS DE DADOS ====================
struct LoRaConfig {
    uint8_t spreadingFactor; // 6-12
    uint8_t codingRate;      // 5-8 (4/5 a 4/8)
    uint32_t bandwidth;      // Hz
    uint8_t syncWord;        // 0x00-0xFF
};

struct ReceivedMessage {
    String payload;          // Conteúdo da mensagem
    int rssi;               // RSSI do pacote
    float snr;              // SNR do pacote
    unsigned long timestamp; // millis() do recebimento
};

// ==================== LOGS ====================
#define LOG_PREFIX "[LORA-RX] "

#define LOG_INFO(msg)       Serial.println(LOG_PREFIX msg)
#define LOG_INFOF(fmt, ...) Serial.printf(LOG_PREFIX fmt "\n", ##__VA_ARGS__)
#define LOG_ERROR(msg)      Serial.println(LOG_PREFIX "❌ " msg)
#define LOG_ERRORF(fmt, ...) Serial.printf(LOG_PREFIX "❌ " fmt "\n", ##__VA_ARGS__)

#endif // CONFIG_LORA_RECEIVER_H
