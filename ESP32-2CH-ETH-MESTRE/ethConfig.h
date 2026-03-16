#ifndef ETH_CONFIG_H
#define ETH_CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <ETH.h>
#include "Config.h"

class EthConfig {
public:
    EthConfig();

    // Carrega config da flash e aplica ao ETH (sempre haverá um IP — padrão ou customizado)
    void begin();

    // Salva IP fixo definido pelo usuário — chame antes de reiniciar para aplicar
    bool saveStaticIP(const String& ip, const String& gateway,
                      const String& subnet, const String& dns = "");

    // Remove IP customizado — próximo boot usará o IP padrão (192.168.1.100)
    void clearStaticIP();

    // true se o usuário salvou um IP customizado (false = usando IP padrão)
    bool    hasStaticIP()   { return useStatic; }

    String  getIP()         { return staticIP;  }
    String  getGateway()    { return staticGW;  }
    String  getSubnet()     { return staticSN;  }
    String  getDNS()        { return staticDNS; }

private:
    Preferences prefs;
    bool   useStatic;   // true = IP customizado pelo usuário; false = IP padrão
    String staticIP;
    String staticGW;
    String staticSN;
    String staticDNS;

    bool validateIP(const String& ip);
};

#endif // ETH_CONFIG_H