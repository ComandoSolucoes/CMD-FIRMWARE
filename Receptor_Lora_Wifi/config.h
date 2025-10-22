#ifndef CONFIG_H
#define CONFIG_H

// ============================ Wi-Fi ============================
#define AP_PASSWORD "Comando@123"

// ============================ LoRa (pinos/915 MHz) ============================
#define LORA_FREQUENCY 915E6
#define LORA_SS_PIN    5
#define LORA_RST_PIN   14
#define LORA_DIO0_PIN  26

// ============================ LED ============================
#define PIN_LED         2
#define NUMPIXELS       1
#define LED_BRIGHTNESS  60


// ============================ Factory Reset ============================
#define BOOT_COUNT_THRESHOLD  6        // Número de boots para factory reset
#define BOOT_WINDOW_MS        10000    // Janela de tempo (10 segundos)
#define BOOT_CLEAR_MS         10000    // Tempo para zerar contador após boot normal

#endif // CONFIG_H
