#include "EthConfig.h"

// IP padrão usado quando nenhum IP fixo foi configurado pelo usuário
#define ETH_DEFAULT_IP       "192.168.1.100"
#define ETH_DEFAULT_GATEWAY  "192.168.1.1"
#define ETH_DEFAULT_SUBNET   "255.255.255.0"
#define ETH_DEFAULT_DNS      "8.8.8.8"

EthConfig::EthConfig()
    : useStatic(false) {}

void EthConfig::begin() {
    prefs.begin(PREFS_NAMESPACE, false);

    bool savedStatic = prefs.getBool("eth_static", false);

    if (savedStatic) {
        // Usuário configurou um IP fixo — usa ele
        staticIP  = prefs.getString("eth_ip",  ETH_DEFAULT_IP);
        staticGW  = prefs.getString("eth_gw",  ETH_DEFAULT_GATEWAY);
        staticSN  = prefs.getString("eth_sn",  ETH_DEFAULT_SUBNET);
        staticDNS = prefs.getString("eth_dns", ETH_DEFAULT_DNS);
        useStatic = true;
        LOG_INFOF("ETH: IP fixo configurado pelo usuario — %s", staticIP.c_str());
    } else {
        // Sem configuração salva — aplica IP padrão
        staticIP  = ETH_DEFAULT_IP;
        staticGW  = ETH_DEFAULT_GATEWAY;
        staticSN  = ETH_DEFAULT_SUBNET;
        staticDNS = ETH_DEFAULT_DNS;
        useStatic = true;
        LOG_INFOF("ETH: usando IP padrao — %s (configure em /device para alterar)", ETH_DEFAULT_IP);
    }

    prefs.end();

    // Aplica IP ao ETH antes do begin()
    IPAddress ip, gw, sn, dns;
    if (ip.fromString(staticIP) && gw.fromString(staticGW) &&
        sn.fromString(staticSN) && dns.fromString(staticDNS)) {
        ETH.config(ip, gw, sn, dns);
    } else {
        LOG_WARN("ETH: IP invalido na flash — voltando para DHCP");
        useStatic = false;
    }
}

bool EthConfig::saveStaticIP(const String& ip, const String& gateway,
                              const String& subnet,  const String& dns) {
    if (!validateIP(ip) || !validateIP(gateway) || !validateIP(subnet)) {
        LOG_WARN("ETH: IP invalido, nao salvo");
        return false;
    }

    String dnsVal = (dns.length() > 0 && validateIP(dns)) ? dns : ETH_DEFAULT_DNS;

    prefs.begin(PREFS_NAMESPACE, false);
    prefs.putBool  ("eth_static", true);
    prefs.putString("eth_ip",  ip);
    prefs.putString("eth_gw",  gateway);
    prefs.putString("eth_sn",  subnet);
    prefs.putString("eth_dns", dnsVal);
    prefs.end();

    staticIP  = ip;
    staticGW  = gateway;
    staticSN  = subnet;
    staticDNS = dnsVal;
    useStatic = true;

    LOG_INFOF("ETH: IP fixo salvo — %s (reinicie para aplicar)", ip.c_str());
    return true;
}

void EthConfig::clearStaticIP() {
    // Remove o flag de "configurado pelo usuario" — no proximo boot volta para o IP padrao
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.remove("eth_static");
    prefs.remove("eth_ip");
    prefs.remove("eth_gw");
    prefs.remove("eth_sn");
    prefs.remove("eth_dns");
    prefs.end();

    // Mantém IP padrão na memória
    staticIP  = ETH_DEFAULT_IP;
    staticGW  = ETH_DEFAULT_GATEWAY;
    staticSN  = ETH_DEFAULT_SUBNET;
    staticDNS = ETH_DEFAULT_DNS;
    useStatic = true;

    LOG_INFOF("ETH: IP customizado removido. Proximo boot usara IP padrao (%s)", ETH_DEFAULT_IP);
}

bool EthConfig::validateIP(const String& ip) {
    IPAddress addr;
    return addr.fromString(ip);
}