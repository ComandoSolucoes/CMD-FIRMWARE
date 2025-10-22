#include "led_functions.h"
#include "config.h"
#include "globals.h"
#include <math.h>

void initNeoPixel() {
  pixels.begin();
  pixels.clear();
  pixels.setBrightness(LED_BRIGHTNESS);
  pixels.show();
}

void ledShowRGB(uint8_t r, uint8_t g, uint8_t b) {
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}

void ledOff() { 
  ledShowRGB(0, 0, 0); 
}

void showBaseByWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    ledShowRGB(0, 200, 0);   // verde
  } else {
    ledShowRGB(255, 0, 0);   // vermelho
  }
}

void blinkBlue(uint8_t times, uint16_t onMs, uint16_t offMs) {
  for (uint8_t i = 0; i < times; i++) {
    ledShowRGB(0, 0, 255);
    delay(onMs);
    showBaseByWifi();
    delay(offMs);
  }
}

void pulseConnectingTick() {
  const uint32_t period = 1800;
  float t = (millis() % period) / (float)period; // 0..1
  float s = 0.2f + 0.8f * (0.5f * (sinf(t * 6.28318530718f) + 1.0f)); // 0.2..1.0

  uint8_t r = (uint8_t)(200 * s);
  uint8_t g = (uint8_t)(140 * s);
  uint8_t b = 0;

  ledShowRGB(r, g, b);
}

void pulseAPModeTick() {
  const uint32_t period = 2000; // 2 segundos por ciclo
  float t = (millis() % period) / (float)period; // 0..1
  
  // Senoide suave: min 20%, max 100%
  float s = 0.2f + 0.8f * (0.5f * (sinf(t * 6.28318530718f) + 1.0f));
  
  uint8_t r = (uint8_t)(255 * s); // Vermelho pulsante
  uint8_t g = 0;
  uint8_t b = 0;
  
  ledShowRGB(r, g, b);
}

void blinkArmedTick() {
  // Implementação futura se necessário
}

void maybeRefreshBaseLed() {
  static unsigned long last = 0;
  if (millis() - last >= 1000) {
    last = millis();
    showBaseByWifi();
  }
}

// ========== GERENCIADOR CENTRAL DE LED ==========
void updateLedState() {
  static unsigned long lastUpdate = 0;
  
  // Atualiza a cada 50ms para animações suaves
  if (millis() - lastUpdate < 50) {
    return;
  }
  lastUpdate = millis();
  
  // Prioridade de estados (do mais específico ao mais geral)
  
  // 1. Modo AP ativo (sem credenciais salvas)
  if (apActive) {
    pulseAPModeTick();
    return;
  }
  
  // 2. Tentando conectar ao Wi-Fi
  if (WiFi.status() != WL_CONNECTED && haveSavedCreds) {
    pulseConnectingTick();
    return;
  }
  
  // 3. Conectado com sucesso
  if (WiFi.status() == WL_CONNECTED) {
    static bool greenSet = false;
    if (!greenSet) {
      ledShowRGB(0, 200, 0); // Verde fixo
      greenSet = true;
    }
    return;
  }
  
  // 4. Estado padrão (desconhecido)
  ledOff();
}