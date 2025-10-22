#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>
#include <SPI.h>
#include <LoRa.h>
#include "esp_system.h"

#include "config.h"

// ============================ Objetos globais ============================
extern Preferences prefs;
extern WebServer server;

// ============================ Estados globais ============================
extern bool routesReady;

enum ConnState { IDLE, CONNECTING, CONNECTED, FAILED };
extern volatile ConnState connState;

extern bool apActive;
extern wl_status_t lastWiFiStatus;

extern String savedSsid;
extern String savedPass;
extern bool haveSavedCreds;

extern String targetSSID;
extern String targetPASS;

extern bool pulseConnecting;   // efeito "respirar" durante CONNECTING (se não estiver na janela armada)
extern bool armWindow;         // janela armada para duplo reset
extern unsigned long armStartMs;

// Reset em RTC
extern RTC_DATA_ATTR uint32_t rstFlag;
extern RTC_DATA_ATTR uint8_t bootCount;         // ← NOVO: contador de boots
extern RTC_DATA_ATTR uint32_t lastBootTime;     // ← NOVO: timestamp do último boot

// WiFi events
extern WiFiEventId_t evGotIp, evDisc;

// AP IP
extern IPAddress AP_IP, AP_GW, AP_MASK;

// NeoPixel (apenas declaração; a definição fica no .cpp)
extern Adafruit_NeoPixel pixels;

#endif // GLOBALS_H
