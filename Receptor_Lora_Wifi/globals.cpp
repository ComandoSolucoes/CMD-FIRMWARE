#include "globals.h"

// Objetos
Preferences prefs;
WebServer server(80);

// Flags / estados globais

// Reset/boot tracking
// Reset/boot tracking
// Reset/boot tracking
RTC_DATA_ATTR uint32_t rstFlag = 0;
RTC_DATA_ATTR uint8_t bootCount = 0;
RTC_DATA_ATTR uint32_t lastBootTime = 0;  // ✅ ADICIONE ESTA LINHA

bool armWindow = false;
uint32_t armStartMs = 0;
// Wi-Fi
String savedSsid = "";
String savedPass = "";
bool haveSavedCreds = false;

// Estados
bool routesReady = false;

volatile ConnState connState = IDLE;

bool apActive = false;
wl_status_t lastWiFiStatus = WL_IDLE_STATUS;

String targetSSID, targetPASS;

bool pulseConnecting = false;

// WiFi events
WiFiEventId_t evGotIp = 0, evDisc = 0;

// AP IP
IPAddress AP_IP(192,168,4,1), AP_GW(192,168,4,1), AP_MASK(255,255,255,0);

// NeoPixel (usa defines de config.h)
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);
