#include "WebInterface.h"
#include <ETH.h>

WebInterface::WebInterface(IOManager* ioMgr, MqttHandler* mqttHdl, I2CSlaveManager* slaveMgr)
    : server(80), io(ioMgr), mqtt(mqttHdl), slaves(slaveMgr) {}

void WebInterface::begin() {
    setupRoutes();
    server.begin();
    LOG_INFO("Interface web iniciada na porta 80");
}

void WebInterface::handle() {
    server.handleClient();
}

void WebInterface::setupRoutes() {
    server.on("/",             HTTP_GET,  [this]() { handleRoot(); });
    server.on("/api/outputs",  HTTP_GET,  [this]() { handleApiOutputs(); });
    server.on("/api/outputs",  HTTP_POST, [this]() { handleApi(); });
    server.on("/api/inputs",   HTTP_GET,  [this]() { handleApiInputs(); });
    server.on("/api/status",   HTTP_GET,  [this]() { handleApiStatus(); });
    server.on("/api/config",   HTTP_GET,  [this]() { handleApiConfig(); });
    server.on("/api/config",   HTTP_POST, [this]() { handleApiConfig(); });
    server.onNotFound([this]() { handleNotFound(); });
}

// ==================== AUTENTICAÇÃO ====================

bool WebInterface::authenticate() {
    if (!server.authenticate(wwwUser, wwwPass)) {
        server.requestAuthentication();
        return false;
    }
    return true;
}

// ==================== PÁGINAS ====================

void WebInterface::handleRoot() {
    if (!authenticate()) return;
    server.send(200, "text/html", buildDashboard());
}

void WebInterface::handleApi() {
    // POST /api/outputs  → {"index":0,"state":true}  ou  {"index":0,"toggle":true}
    if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        StaticJsonDocument<128> doc;
        if (deserializeJson(doc, body)) {
            server.send(400, "application/json", "{\"error\":\"JSON inválido\"}");
            return;
        }
        uint8_t idx = doc["index"].as<int>();
        if (idx >= NUM_TOTAL_OUTPUTS) {
            server.send(400, "application/json", "{\"error\":\"Índice inválido\"}");
            return;
        }
        if (doc.containsKey("toggle") && doc["toggle"].as<bool>()) {
            io->toggleOutput(idx);
        } else {
            io->setOutput(idx, doc["state"].as<bool>());
        }
        bool st = io->getOutputState(idx);
        mqtt->publishOutputState(idx, st);

        String resp = "{\"index\":" + String(idx) + ",\"state\":" + (st ? "true" : "false") + "}";
        server.send(200, "application/json", resp);
    }
}

void WebInterface::handleApiOutputs() {
    // GET /api/outputs → array com estado de todas as 18 saídas
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        JsonObject o = arr.createNestedObject();
        o["index"] = i;
        o["state"] = io->getOutputState(i);
        o["label"] = (i < NUM_LOCAL_OUTPUTS)
            ? ("Local " + String(i + 1))
            : ((i < NUM_LOCAL_OUTPUTS + NUM_SLAVE_CHANNELS)
                ? ("S1-DO" + String(i - NUM_LOCAL_OUTPUTS + 1))
                : ("S2-DO" + String(i - NUM_LOCAL_OUTPUTS - NUM_SLAVE_CHANNELS + 1)));
    }
    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}

void WebInterface::handleApiInputs() {
    // GET /api/inputs → array com estado de todas as 18 entradas
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        JsonObject o = arr.createNestedObject();
        o["index"] = i;
        o["state"] = io->getInputState(i);
        o["label"] = (i < NUM_LOCAL_INPUTS)
            ? ("Local " + String(i + 1))
            : ((i < NUM_LOCAL_INPUTS + NUM_SLAVE_CHANNELS)
                ? ("S1-DI" + String(i - NUM_LOCAL_INPUTS + 1))
                : ("S2-DI" + String(i - NUM_LOCAL_INPUTS - NUM_SLAVE_CHANNELS + 1)));
    }
    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}

void WebInterface::handleApiStatus() {
    StaticJsonDocument<512> doc;
    doc["device"]   = DEVICE_MODEL;
    doc["firmware"] = FIRMWARE_VERSION;
    doc["ip"]       = ETH.localIP().toString();
    doc["mac"]      = ETH.macAddress();
    doc["uptime"]   = millis() / 1000;
    doc["mqtt"]     = mqtt->isConnected();
    doc["eth_link"] = ETH.linkUp();

    JsonObject s1 = doc.createNestedObject("slave1");
    s1["online"]     = slaves->isSlaveOnline(0);
    s1["errors"]     = slaves->getSlaveStatus(0).totalErrors;

    JsonObject s2 = doc.createNestedObject("slave2");
    s2["online"]     = slaves->isSlaveOnline(1);
    s2["errors"]     = slaves->getSlaveStatus(1).totalErrors;

    String payload;
    serializeJson(doc, payload);
    server.send(200, "application/json", payload);
}

void WebInterface::handleApiConfig() {
    if (server.method() == HTTP_GET) {
        StaticJsonDocument<512> doc;
        doc["relay_logic"]    = (uint8_t)io->getRelayLogic();
        doc["initial_state"]  = (uint8_t)io->getInitialState();
        doc["tele_interval"]  = mqtt->getTelemetryInterval() / 1000;
        doc["mqtt_device"]    = mqtt->getDeviceName();

        JsonArray inputs = doc.createNestedArray("inputs");
        for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
            JsonObject inp = inputs.createNestedObject();
            inp["mode"]     = (uint8_t)io->getInputMode(i);
            inp["debounce"] = io->getDebounceTime(i);
            inp["pulse"]    = io->getPulseTime(i);
        }
        String payload;
        serializeJson(doc, payload);
        server.send(200, "application/json", payload);

    } else if (server.method() == HTTP_POST) {
        String body = server.arg("plain");
        StaticJsonDocument<512> doc;
        if (deserializeJson(doc, body)) {
            server.send(400, "application/json", "{\"error\":\"JSON inválido\"}");
            return;
        }
        if (doc.containsKey("relay_logic"))   io->setRelayLogic((RelayLogic)doc["relay_logic"].as<int>());
        if (doc.containsKey("initial_state")) io->setInitialState((InitialState)doc["initial_state"].as<int>());
        if (doc.containsKey("tele_interval")) mqtt->setTelemetryInterval(doc["tele_interval"].as<uint32_t>() * 1000);
        if (doc.containsKey("mqtt_device"))   mqtt->setDeviceName(doc["mqtt_device"].as<String>());

        if (doc.containsKey("inputs")) {
            JsonArray arr = doc["inputs"].as<JsonArray>();
            uint8_t i = 0;
            for (JsonObject inp : arr) {
                if (i >= NUM_TOTAL_INPUTS) break;
                if (inp.containsKey("mode"))     io->setInputMode(i, (InputMode)inp["mode"].as<int>());
                if (inp.containsKey("debounce")) io->setDebounceTime(i, inp["debounce"].as<uint16_t>());
                if (inp.containsKey("pulse"))    io->setPulseTime(i, inp["pulse"].as<uint16_t>());
                i++;
            }
        }
        io->saveConfig();
        server.send(200, "application/json", "{\"ok\":true}");
    }
}

void WebInterface::handleNotFound() {
    server.send(404, "text/plain", "Not found");
}

// ==================== DASHBOARD HTML ====================

String WebInterface::buildDashboard() {
    String html = R"rawhtml(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ETH-18CH-MASTER</title>
<style>
  body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;margin:0;padding:10px}
  h1{color:#00d4ff;text-align:center;margin-bottom:4px}
  .subtitle{text-align:center;color:#888;font-size:12px;margin-bottom:16px}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(90px,1fr));gap:8px;margin-bottom:16px}
  .btn{padding:12px 6px;border:none;border-radius:8px;cursor:pointer;font-size:13px;font-weight:bold;transition:.2s}
  .btn-on {background:#00d4ff;color:#000}
  .btn-off{background:#2a2a4a;color:#888}
  .btn:hover{opacity:.85}
  .section-title{color:#aaa;font-size:12px;text-transform:uppercase;letter-spacing:1px;margin:12px 0 6px}
  .status-bar{display:flex;gap:12px;flex-wrap:wrap;margin-bottom:12px;font-size:12px}
  .badge{padding:4px 10px;border-radius:12px;background:#2a2a4a}
  .badge.ok{background:#0a4a0a;color:#4caf50}
  .badge.err{background:#4a0a0a;color:#f44}
  .input-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(90px,1fr));gap:8px}
  .input-btn{padding:10px 6px;border-radius:8px;font-size:12px;text-align:center;background:#2a2a4a;color:#888}
  .input-btn.active{background:#1a3a1a;color:#4caf50;font-weight:bold}
</style>
</head>
<body>
<h1>ETH-18CH-MASTER</h1>
<div class="subtitle">Firmware v)rawhtml" FIRMWARE_VERSION R"rawhtml( — <span id="ip">carregando...</span></div>

<div class="status-bar">
  <span class="badge" id="eth-badge">ETH ...</span>
  <span class="badge" id="mqtt-badge">MQTT ...</span>
  <span class="badge" id="s1-badge">Escravo 1 ...</span>
  <span class="badge" id="s2-badge">Escravo 2 ...</span>
</div>

<div class="section-title">Saídas (18)</div>
<div class="grid" id="outputs-grid"></div>

<div class="section-title">Entradas (18)</div>
<div class="input-grid" id="inputs-grid"></div>

<script>
const labels_out = [
  'Local 1','Local 2',
  'S1-DO1','S1-DO2','S1-DO3','S1-DO4','S1-DO5','S1-DO6','S1-DO7','S1-DO8',
  'S2-DO1','S2-DO2','S2-DO3','S2-DO4','S2-DO5','S2-DO6','S2-DO7','S2-DO8'
];
const labels_in = [
  'Local 1','Local 2',
  'S1-DI1','S1-DI2','S1-DI3','S1-DI4','S1-DI5','S1-DI6','S1-DI7','S1-DI8',
  'S2-DI1','S2-DI2','S2-DI3','S2-DI4','S2-DI5','S2-DI6','S2-DI7','S2-DI8'
];

function toggleOutput(idx) {
  fetch('/api/outputs', {
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body: JSON.stringify({index:idx, toggle:true})
  }).then(r => r.json()).then(d => {
    const btn = document.getElementById('out-' + idx);
    if (btn) {
      btn.className = 'btn ' + (d.state ? 'btn-on' : 'btn-off');
      btn.textContent = labels_out[idx] + '\n' + (d.state ? 'ON' : 'OFF');
    }
  });
}

function loadOutputs() {
  fetch('/api/outputs').then(r => r.json()).then(data => {
    const grid = document.getElementById('outputs-grid');
    grid.innerHTML = '';
    data.forEach(o => {
      const btn = document.createElement('button');
      btn.id = 'out-' + o.index;
      btn.className = 'btn ' + (o.state ? 'btn-on' : 'btn-off');
      btn.textContent = labels_out[o.index] + '\n' + (o.state ? 'ON' : 'OFF');
      btn.onclick = () => toggleOutput(o.index);
      grid.appendChild(btn);
    });
  });
}

function loadInputs() {
  fetch('/api/inputs').then(r => r.json()).then(data => {
    const grid = document.getElementById('inputs-grid');
    grid.innerHTML = '';
    data.forEach(inp => {
      const div = document.createElement('div');
      div.id = 'in-' + inp.index;
      div.className = 'input-btn ' + (inp.state ? 'active' : '');
      div.textContent = labels_in[inp.index] + '\n' + (inp.state ? '● ON' : '○ OFF');
      grid.appendChild(div);
    });
  });
}

function loadStatus() {
  fetch('/api/status').then(r => r.json()).then(d => {
    document.getElementById('ip').textContent = d.ip;
    const eth = document.getElementById('eth-badge');
    eth.textContent = 'ETH ' + (d.eth_link ? 'ON' : 'OFF');
    eth.className = 'badge ' + (d.eth_link ? 'ok' : 'err');
    const mq = document.getElementById('mqtt-badge');
    mq.textContent = 'MQTT ' + (d.mqtt ? 'OK' : 'OFF');
    mq.className = 'badge ' + (d.mqtt ? 'ok' : 'err');
    const s1 = document.getElementById('s1-badge');
    s1.textContent = 'Escravo 1 ' + (d.slave1.online ? 'ON' : 'OFF');
    s1.className = 'badge ' + (d.slave1.online ? 'ok' : 'err');
    const s2 = document.getElementById('s2-badge');
    s2.textContent = 'Escravo 2 ' + (d.slave2.online ? 'ON' : 'OFF');
    s2.className = 'badge ' + (d.slave2.online ? 'ok' : 'err');
  });
}

// Atualiza periodicamente
loadOutputs(); loadInputs(); loadStatus();
setInterval(() => { loadInputs(); loadStatus(); }, 2000);
setInterval(loadOutputs, 5000);
</script>
</body>
</html>)rawhtml";
    return html;
}