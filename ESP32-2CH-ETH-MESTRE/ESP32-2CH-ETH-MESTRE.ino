////////////////////////////////////////////////////////////////////////////////////
// ETH-18CH-MASTER v2.0.1
//
// Arquitetura:
//   Core 0 — I2C task: polling dos escravos a cada I2C_POLL_MS
//   Core 1 — loop():  ETH / WiFi / WebServer / MQTT / entradas locais
//
// Lógica de rede:
//   1. ETH é sempre tentado primeiro (timeout 10s)
//   2. Se ETH subiu → WiFi fica desabilitado (economia de energia + evita
//      conflito de IPs no CMD-C)
//   3. Se ETH falhou → core.begin() inicia WiFi normalmente via CMDWiFi
//
// Protocolo I2C:
//   TX (mestre→escravo): desired outputs + cfg (quando pendente)
//   RX (escravo→mestre): confirmed outputs + inputs
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

// ==================== MUTEX I2C ====================

SemaphoreHandle_t i2cMutex = nullptr;

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
            LOG_INFOF("ETH IP: %s  MAC: %s  %dMbps %s",
                ETH.localIP().toString().c_str(),
                ETH.macAddress().c_str(),
                ETH.linkSpeed(),
                ETH.fullDuplex() ? "FD" : "HD");
            ethConnected = true;
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
        case ARDUINO_EVENT_ETH_STOP:
            LOG_WARN("Ethernet desconectado");
            ethConnected = false;
            break;
        default: break;
    }
}

// ==================== I2C TASK (Core 0) ====================

void i2cTask(void*) {
    LOG_INFO("I2C Task iniciada no Core 0");
    for (;;) {
        if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            slaveMgr.handle();
            xSemaphoreGive(i2cMutex);
        }
        vTaskDelay(pdMS_TO_TICKS(I2C_POLL_MS));
    }
}

// ==================== SETUP ====================

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println();
    Serial.println("╔════════════════════════════════════════╗");
    Serial.println("║  ETH-18CH-MASTER v2.0.1 + CMD-C Core  ║");
    Serial.println("╚════════════════════════════════════════╝");
    Serial.println();

    // Watchdog
    esp_task_wdt_deinit();
    esp_task_wdt_config_t wdtCfg = {
        .timeout_ms     = WATCHDOG_TIMEOUT_SEC * 1000,
        .idle_core_mask = 0,
        .trigger_panic  = true
    };
    esp_task_wdt_init(&wdtCfg);
    esp_task_wdt_add(NULL);

    // ── 1) ETHERNET (sempre tentado primeiro) ───────────────────────────────
    WiFi.onEvent(onEthEvent);
    ethCfg.begin();
    ETH.begin(ETH_PHY_TYPE, ETH_PHY_ADDR, ETH_PHY_MDC,
              ETH_PHY_MDIO, ETH_PHY_POWER, ETH_CLK_MODE);

    LOG_INFO("Aguardando Ethernet (10s)...");
    unsigned long t = millis();
    while (!ethConnected && millis() - t < 10000) {
        esp_task_wdt_reset();
        delay(100);
    }

    // ── 2) CMD-C Core — WiFi só sobe se ETH não está disponível ─────────────
    if (ethConnected) {
        LOG_INFOF("ETH OK: %s — WiFi desabilitado", ETH.localIP().toString().c_str());

        // Desativa rádio WiFi para evitar conflito e economizar energia.
        // CMD-C ainda funciona: NVS, WebServer via ETH, MQTT via ETH.
        WiFi.mode(WIFI_OFF);

        // Inicializa CMD-C sem tentar conectar WiFi.
        // O WebServer do CMD-C vai responder via ETH.
        core.begin();

    } else {
        LOG_WARN("ETH indisponivel — iniciando WiFi via CMD-C");

        // ETH não subiu: CMD-C inicializa normalmente com WiFi AP/STA
        core.begin();
    }

    // ── 3) Escravos I2C ──────────────────────────────────────────────────────
    slaveMgr.begin();

    // ── 4) IOManager ────────────────────────────────────────────────────────
    ioMgr.begin();

    ioMgr.setOutputChangedCallback([](uint8_t index, bool state) {
        mqttHdl.publishOutputState(index, state);
    });

    ioMgr.setInputChangedCallback([](uint8_t index, bool state) {
        mqttHdl.publishInputState(index, state);
    });

    // ── 5) MQTT ──────────────────────────────────────────────────────────────
    // Sempre via ethClient (MqttHandler.h) — funciona independente do WiFi
    mqttHdl.begin(&core);

    // ── 6) Web ───────────────────────────────────────────────────────────────
    webIface.registerRoutes(&core.webServer);

    // ── 7) Mutex + Task I2C no Core 0 ───────────────────────────────────────
    i2cMutex = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(i2cTask, "i2c_task", 4096, nullptr, 1, nullptr, 0);

    // ── Status ───────────────────────────────────────────────────────────────
    Serial.println();
    Serial.println(LOG_PREFIX "════════════════════════════════════════");
    Serial.println(LOG_PREFIX "Sistema inicializado!");
    if (ethConnected)
        Serial.println(LOG_PREFIX "ETH:  http://" + ETH.localIP().toString() + "/device");
    else
        Serial.println(LOG_PREFIX "WiFi: http://" + core.getIP() + "/device");
    Serial.println(LOG_PREFIX "════════════════════════════════════════");
    Serial.println();
}

// ==================== LOOP (Core 1) ====================

void loop() {
    esp_task_wdt_reset();

    core.handle();

    if (xSemaphoreTake(i2cMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        ioMgr.handle();
        xSemaphoreGive(i2cMutex);
    }

    mqttHdl.handle();

    delay(1);
}
