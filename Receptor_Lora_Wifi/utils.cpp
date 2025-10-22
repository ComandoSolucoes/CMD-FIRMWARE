#include "utils.h"
#include "config.h"
#include "led_functions.h"

#include <esp_system.h>
#include <Preferences.h>

void initSerial() {
  Serial.begin(115200);
  delay(100);
  Serial.println();
  Serial.println("=== Serial inicializado ===");
}

void printSystemInfo() {
  Serial.println();
  Serial.println(F("===== Sistema ====="));
  Serial.printf("SDK: %s\n", ESP.getSdkVersion());
  Serial.printf("Flash: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("CPU Freq: %u MHz\n", ESP.getCpuFreqMHz());
  uint64_t mac = ESP.getEfuseMac();
  Serial.printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
    (uint8_t)(mac>>40), (uint8_t)(mac>>32), (uint8_t)(mac>>24),
    (uint8_t)(mac>>16), (uint8_t)(mac>>8), (uint8_t)mac);
  Serial.println("====================");
}

void handleBootCountReset() {
  auto reason = esp_reset_reason();
  
  // Obtém timestamp atual (ms desde boot)
  uint32_t currentTime = millis();
  
  // Se não for power-on ou reset externo, ignora
  if (reason != ESP_RST_POWERON && reason != ESP_RST_EXT) {
    Serial.println("[BOOT] Reset por software/brownout - ignorando contagem");
    return;
  }

  // Calcula tempo desde último boot
  // Nota: após power-off completo, lastBootTime será 0
  uint32_t timeSinceLastBoot = currentTime - lastBootTime;
  
  // Se passou muito tempo desde o último boot ou é o primeiro, reseta o contador
  if (lastBootTime == 0 || timeSinceLastBoot > BOOT_WINDOW_MS) {
    bootCount = 1;
    lastBootTime = currentTime;
    Serial.printf("[BOOT] Primeira inicializacao ou timeout. Contador: %d/%d\n", 
                  bootCount, BOOT_COUNT_THRESHOLD);
    return;
  }

  // Incrementa contador
  bootCount++;
  lastBootTime = currentTime;
  
  Serial.printf("[BOOT] Boot rapido detectado! Contador: %d/%d (janela: %.1fs)\n", 
                bootCount, BOOT_COUNT_THRESHOLD, BOOT_WINDOW_MS/1000.0);

  // Se atingiu o threshold, executa factory reset
  if (bootCount >= BOOT_COUNT_THRESHOLD) {
    Serial.println();
    Serial.println("=============================================");
    Serial.println("  FACTORY RESET: 6 BOOTS DETECTADOS");
    Serial.println("  APAGANDO CREDENCIAIS WI-FI");
    Serial.println("=============================================");

    // Reset do contador
    bootCount = 0;
    lastBootTime = 0;

    // Apaga credenciais
    Preferences p;
    p.begin("wifi", false);
    p.clear();
    p.end();

    WiFi.disconnect(true, true);

    savedSsid = "";
    savedPass = "";
    haveSavedCreds = false;

    // Feedback LED: vermelho piscando 6x
    for (int i = 0; i < 6; i++) {
      ledShowRGB(255, 0, 0);
      delay(200);
      ledOff();
      delay(200);
    }

    Serial.println("[BOOT] Credenciais apagadas. Reiniciando em modo AP...");
    delay(500);
    ESP.restart();
  } else {
    // LED azul pisca brevemente para mostrar contagem
    for (int i = 0; i < bootCount; i++) {
      ledShowRGB(0, 0, 255);
      delay(100);
      ledOff();
      delay(100);
    }
  }
}

void clearBootCountAfterStableRun() {
  // Chame esta função no loop após BOOT_CLEAR_MS
  static bool cleared = false;
  if (!cleared && millis() > BOOT_CLEAR_MS) {
    bootCount = 0;
    lastBootTime = 0;
    cleared = true;
    Serial.println("[BOOT] Sistema estavel. Contador de boots zerado.");
  }
}

void handleResetLogic() {
  // Função legada - pode ser removida se usar handleBootCountReset()
  // Mantida para compatibilidade
}

void checkFactoryResetOnBoot() {
  // Função legada - não mais usada com boot count
  // Mantida para compatibilidade
}

String macSuffix() {
  uint64_t mac = ESP.getEfuseMac();
  char buf[7];
  snprintf(buf, sizeof(buf), "%02X%02X%02X",
           (uint8_t)(mac >> 16), (uint8_t)(mac >> 8), (uint8_t)mac);
  return String(buf);
}