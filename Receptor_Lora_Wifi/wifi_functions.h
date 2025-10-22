#ifndef WIFI_FUNCTIONS_H
#define WIFI_FUNCTIONS_H

#include <Arduino.h>
#include "globals.h"

// Setup / Wi-Fi
void initWiFi();
void condicionalAP();
void registerWiFiEvents();
void loadWiFiCredentials();
int scanWifi();
void initWiFiConnection();

// AP
void startAPServer();
void stopAPServer();

// Persistência
bool salvarCredenciaisRede(const String& ssid, const String& pass);
bool esquecerCredenciaisWiFi();

// Utils
String macSuffix();
String connStateText();
const char* indexHtml();

#endif // WIFI_FUNCTIONS_H
