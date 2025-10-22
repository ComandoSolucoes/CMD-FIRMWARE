#ifndef LED_FUNCTIONS_H
#define LED_FUNCTIONS_H

#include "globals.h"

void initNeoPixel();
void ledShowRGB(uint8_t r, uint8_t g, uint8_t b);
void ledOff();
void showBaseByWifi();
void blinkBlue(uint8_t times = 2, uint16_t onMs = 100, uint16_t offMs = 80);
void pulseConnectingTick();
void blinkArmedTick();
void pulseAPModeTick();
void maybeRefreshBaseLed();

void updateLedState();

#endif
