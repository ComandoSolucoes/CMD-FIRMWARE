#ifndef ETH_CONFIG_H
#define ETH_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <ETH.h>
#include "Config.h"

class EthConfig {
public:
    EthConfig();

    void begin();   // Carrega config da flash e aplica ao ETH

    // Salva IP fixo — chame antes de reiniciar para aplicar
    bool saveStaticIP(const String& ip, const String& gateway,
                      const String& subnet, const String& dns = "");

    // Remove IP fixo — volta para DHCP
    void clearStaticIP();

    // Getters
    bool    hasStaticIP()   { return useStatic; }
    String  getIP()         { return staticIP; }
    String  getGateway()    { return staticGW; }
    String  getSubnet()     { return staticSN; }
    String  getDNS()        { return staticDNS; }

private:
    Preferences prefs;
    bool   useStatic;
    String staticIP;
    String staticGW;
    String staticSN;
    String staticDNS;

    bool validateIP(const String& ip);
};

#endif // ETH_CONFIG_H