////////////////////////////////////////////////////////////////////////////////////
// ETH-18CH-MASTER
// Módulo mestre com Ethernet (WT32-ETH01 / LAN8720)
// 2 entradas + 2 saídas locais
// 2 escravos I2C 8CH (total 18 entradas + 18 saídas)
// MQTT formato Tasmota
////////////////////////////////////////////////////////////////////////////////////

#include <Arduino.h>
#include <ETH.h>
#include <Preferences.h>
#include "Config.h"
#include "I2CSlaveManager.h"
#include "IOManager.h"
#include "MqttHandler.h"
#include "WebInterface.h"

// ==================== CONFIGURAÇÕES DO USUÁRIO ====================
// Altere conforme sua rede

const char* MQTT_BROKER   = "192.168.1.10";  // IP do broker MQTT
const uint16_t MQTT_PORT  = 1883;
const char* MQTT_USER     = "";              // Vazio se não usar autenticação
const char* MQTT_PASS     = "";
const char* DEVICE_NAME   = "ETH-18CH-01";  // Nome único do dispositivo

// ==================== OBJETOS GLOBAIS ====================

I2CSlaveManager slaveMgr;
IOManager       ioMgr(&slaveMgr);
MqttHandler     mqttHdl(&ioMgr);
WebInterface    webIface(&ioMgr, &mqttHdl, &slaveMgr);

Preferences prefs;

// Controle de evento Ethernet
static bool ethConnected = false;

// ==================== CALLBACKS ETHERNET ====================

void onEthEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            LOG_INFO("Ethernet iniciado");
            ETH.setHostname(DEVICE_NAME);
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            LOG_INFO("Cabo Ethernet conectado");
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            LOG_INFOF("IP: %s  MAC: %s  Speed: %d Mbps  %s",
                ETH.localIP().toString().c_str(),
                ETH.macAddress().c_str(),
                ETH.linkSpeed(),
                ETH.fullDuplex() ? "Full Duplex" : "Half Duplex");
            ethConnected = true;
            break;

        case ARDUINO_EVENT_ETH_DISCONNECTED:
            LOG_WARN("Ethernet desconectado");
            ethConnected = false;
            break;

        case ARDUINO_EVENT_ETH_STOP:
            LOG_WARN("Ethernet parado");
            ethConnected = false;
            break;

        default:
            break;
    }
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(500);

    LOG_INFO("=============================================");
    LOG_INFOF("  %s  v%s", DEVICE_MODEL, FIRMWARE_VERSION);
    LOG_INFO("=============================================");

    // 1) Inicializa Ethernet (WT32-ETH01 — LAN8720)
    WiFi.onEvent(onEthEvent);
    ETH.begin(ETH_PHY_ADDR,
              ETH_PHY_POWER,
              ETH_PHY_MDC,
              ETH_PHY_MDIO,
              ETH_PHY_TYPE,
              ETH_CLK_MODE);

    // Aguarda IP (máx 10s para não travar o boot)
    LOG_INFO("Aguardando IP Ethernet...");
    unsigned long t = millis();
    while (!ethConnected && millis() - t < 10000) delay(100);

    if (ethConnected) {
        LOG_INFOF("Ethernet OK! IP: %s", ETH.localIP().toString().c_str());
    } else {
        LOG_WARN("Ethernet sem IP — continuando offline (MQTT desabilitado)");
    }

    // 2) Inicializa escravos I2C
    slaveMgr.begin();

    // 3) Inicializa IO Manager (pinos locais + callbacks)
    ioMgr.begin();

    // Callback: quando IOManager muda uma saída → publica MQTT
    ioMgr.setOutputChangedCallback([](uint8_t index, bool state) {
        mqttHdl.publishOutputState(index, state);
    });

    // Callback: quando IOManager detecta mudança de entrada → publica MQTT
    ioMgr.setInputChangedCallback([](uint8_t index, bool state) {
        mqttHdl.publishInputState(index, state);
    });

    // 4) Inicializa MQTT
    mqttHdl.begin(MQTT_BROKER, MQTT_PORT, MQTT_USER, MQTT_PASS, DEVICE_NAME);

    // 5) Inicializa interface web
    webIface.begin();

    LOG_INFO("Boot completo! Sistema operacional.");
    LOG_INFOF("Dashboard: http://%s", ETH.localIP().toString().c_str());
}

// ==================== LOOP ====================

void loop() {
    // Polling I2C dos escravos + leitura de entradas locais
    slaveMgr.handle();
    ioMgr.handle();

    // MQTT
    mqttHdl.handle();

    // Web server
    webIface.handle();
}
