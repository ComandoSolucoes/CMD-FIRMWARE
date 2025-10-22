#include "globals.h"
#include "config.h"
#include "utils.h"
#include "wifi_functions.h"
#include "led_functions.h"
#include "lora_functions.h"
#include "web_server.h"

// Tick leve
static unsigned long lastTickMs = 0;
static const unsigned long tickIntervalMs = 50;

void setup() {
  initSerial();
  printSystemInfo();
  handleBootCountReset();
  prefs.begin("wifi", false);
  initNeoPixel();
  initWiFi();
  iniciarServidorWeb();
}

void loop() {
  processarServidorWeb();
  clearBootCountAfterStableRun();
  updateLedState();
}
