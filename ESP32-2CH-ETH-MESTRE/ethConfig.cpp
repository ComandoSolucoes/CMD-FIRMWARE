#include "EthConfig.h"

EthConfig::EthConfig()
    : useStatic(false) {}

void EthConfig::begin() {
    prefs.begin(PREFS_NAMESPACE, false);

    useStatic = prefs.getBool("eth_static", false);

    if (!useStatic) {
        LOG_INFO("ETH: modo DHCP");
        prefs.end();
        return;
    }

    staticIP  = prefs.getString("eth_ip",  "");
    staticGW  = prefs.getString("eth_gw",  "");
    staticSN  = prefs.getString("eth_sn",  "255.255.255.0");
    staticDNS = prefs.getString("eth_dns", "8.8.8.8");

    prefs.end();

    // Aplica IP fixo ao ETH antes do begin()
    IPAddress ip, gw, sn, dns;
    if (ip.fromString(staticIP) && gw.fromString(staticGW) &&
        sn.fromString(staticSN) && dns.fromString(staticDNS)) {
        ETH.config(ip, gw, sn, dns);
        LOG_INFOF("ETH: IP fixo configurado — %s", staticIP.c_str());
    } else {
        LOG_WARN("ETH: IP fixo invalido na flash, usando DHCP");
        useStatic = false;
    }
}

bool EthConfig::saveStaticIP(const String& ip, const String& gateway,
                              const String& subnet,  const String& dns) {
    if (!validateIP(ip) || !validateIP(gateway) || !validateIP(subnet)) {
        LOG_WARN("ETH: IP invalido, nao salvo");
        return false;
    }

    String dnsVal = (dns.length() > 0 && validateIP(dns)) ? dns : "8.8.8.8";

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
    prefs.begin(PREFS_NAMESPACE, false);
    prefs.remove("eth_static");
    prefs.remove("eth_ip");
    prefs.remove("eth_gw");
    prefs.remove("eth_sn");
    prefs.remove("eth_dns");
    prefs.end();

    useStatic = false;
    staticIP = staticGW = staticSN = staticDNS = "";

    LOG_INFO("ETH: IP fixo removido, voltando para DHCP (reinicie para aplicar)");
}

bool EthConfig::validateIP(const String& ip) {
    IPAddress addr;
    return addr.fromString(ip);
}