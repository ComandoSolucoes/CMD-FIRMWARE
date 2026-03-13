////////////////////////////////////////////////////////////////////////////////////
// ETH-18CH-MASTER — Integrado com CMD-C Core
// Módulo mestre com Ethernet (WT32-ETH01 / LAN8720)
// 2 entradas + 2 saídas locais
// 2 escravos I2C 8CH (total 18 entradas + 18 saídas)
// MQTT formato Tasmota via Ethernet (PubSubClient)
// Gerenciamento WiFi/Auth/Web via CMD-C Core
////////////////////////////////////////////////////////////////////////////////////

#include <CMDCore.h>
#include <ETH.h>
#include <esp_task_wdt.h>
#include "Config.h"
#include "EthConfig.h"
#include "I2CSlaveManager.h"
#include "IOManager.h"
#include "MqttHandler.h"
#include "WebInterface.h"

// ==================== OBJETOS GLOBAIS ====================

CMDCore         core(DEVICE_MODEL);
EthConfig       ethCfg;
I2CSlaveManager slaveMgr;
IOManager       ioMgr(&slaveMgr);
MqttHandler     mqttHdl(&ioMgr);
WebInterface    webIface(&ioMgr, &mqttHdl, &slaveMgr, &ethCfg);

static bool ethConnected = false;

// ==================== EVENTOS ETHERNET ====================

void onEthEvent(WiFiEvent_t event) {
    switch (event) {
        case ARDUINO_EVENT_ETH_START:
            LOG_INFO("Ethernet iniciado");
            ETH.setHostname(DEVICE_MODEL);
            break;

        case ARDUINO_EVENT_ETH_CONNECTED:
            LOG_INFO("Cabo Ethernet conectado");
            break;

        case ARDUINO_EVENT_ETH_GOT_IP:
            LOG_INFOF("ETH IP: %s  MAC: %s  Speed: %d Mbps  %s",
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

    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║      ETH-18CH-MASTER + CMD-C Core      ║");
    Serial.println("║      Firmware v" FIRMWARE_VERSION "                    ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    // Watchdog (API core 3.x — usa struct)
    esp_task_wdt_config_t wdtConfig = {
        .timeout_ms     = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic  = true
    };
    esp_task_wdt_init(&wdtConfig);
    esp_task_wdt_add(NULL);
    LOG_INFO("Watchdog iniciado");

    // 1) CMD-C Core — sobe AP WiFi e WebServer primeiro
    //    Garante acesso à interface mesmo se ETH travar
    core.begin();

    // 2) Inicializa Ethernet (WT32-ETH01 — LAN8720)
    //    Core 3.x: primeiro argumento é o tipo, depois addr
    WiFi.onEvent(onEthEvent);
    ethCfg.begin(); // aplica IP fixo se configurado
    ETH.begin(ETH_PHY_TYPE,
              ETH_PHY_ADDR,
              ETH_PHY_MDC,
              ETH_PHY_MDIO,
              ETH_PHY_POWER,
              ETH_CLK_MODE);

    LOG_INFO("Aguardando IP Ethernet...");
    unsigned long t = millis();
    while (!ethConnected && millis() - t < 10000) delay(100);

    if (ethConnected) {
        LOG_INFOF("Ethernet OK! IP: %s", ETH.localIP().toString().c_str());
    } else {
        LOG_WARN("Ethernet sem IP — continuando (MQTT desabilitado até obter IP)");
    }

    // 3) Escravos I2C
    slaveMgr.begin();

    // 4) IOManager (pinos locais + callbacks)
    ioMgr.begin();

    ioMgr.setOutputChangedCallback([](uint8_t index, bool state) {
        mqttHdl.publishOutputState(index, state);
    });

    ioMgr.setInputChangedCallback([](uint8_t index, bool state) {
        mqttHdl.publishInputState(index, state);
    });

    // 5) MQTT Handler (usa Ethernet + config salva pelo CMD-C /mqtt)
    mqttHdl.begin(&core);

    // 6) Interface web — registra rotas /device e APIs no CMDCore WebServer
    webIface.registerRoutes(&core.webServer);

    // Log de endereços
    Serial.println();
    Serial.println(LOG_PREFIX "════════════════════════════════════════");
    Serial.println(LOG_PREFIX "✅ Sistema inicializado!");
    if (ethConnected) {
        Serial.println(LOG_PREFIX "🌐 Dashboard ETH: http://" + ETH.localIP().toString() + "/device");
    }
    if (core.isAPMode()) {
        Serial.println(LOG_PREFIX "📶 Config WiFi AP: http://" + core.getIP());
    } else {
        Serial.println(LOG_PREFIX "📶 Config WiFi STA: http://" + core.getIP() + "/device");
    }
    Serial.println(LOG_PREFIX "════════════════════════════════════════");
    Serial.println();
}

// ==================== LOOP ====================

void loop() {
    esp_task_wdt_reset();

    // CMD-C Core (WiFi + WebServer)
    core.handle();

    // Polling I2C dos escravos
    slaveMgr.handle();

    // Entradas locais + pulsos
    ioMgr.handle();

    // MQTT via Ethernet
    mqttHdl.handle();

    delay(1);
}
