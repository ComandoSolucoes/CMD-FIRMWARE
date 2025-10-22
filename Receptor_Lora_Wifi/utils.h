#ifndef UTILS_H
#define UTILS_H

#include "globals.h"

// Serial / sistema
void initSerial();
void printSystemInfo();

// Factory reset por múltiplos boots (Tasmota-style)
void handleBootCountReset();
void clearBootCountAfterStableRun();

String macSuffix();

#endif // UTILS_H