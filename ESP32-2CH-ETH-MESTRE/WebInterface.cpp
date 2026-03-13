#include "WebInterface.h"
#include <webserver/CMDWebServer.h>
#include <ETH.h>

WebInterface::WebInterface(IOManager* ioMgr, MqttHandler* mqttHdl,
                           I2CSlaveManager* slaveMgr, EthConfig* ethCfg)
    : io(ioMgr), mqtt(mqttHdl), slaves(slaveMgr), eth(ethCfg), server(nullptr) {}

// ==================== REGISTRO DE ROTAS ====================

void WebInterface::registerRoutes(CMDWebServer* webServer) {
    server = &webServer->getServer();

    webServer->registerDeviceRoute([this]() {
        server->send(200, "text/html", buildDashboard());
    });

    server->on("/api/device/outputs", HTTP_GET,  [this]() { handleApiOutputs(); });
    server->on("/api/device/outputs", HTTP_POST, [this]() { handleApiOutputControl(); });
    server->on("/api/device/inputs",  HTTP_GET,  [this]() { handleApiInputs(); });
    server->on("/api/device/status",  HTTP_GET,  [this]() { handleApiStatus(); });
    server->on("/api/device/config",  HTTP_GET,  [this]() { handleApiGetConfig(); });
    server->on("/api/device/config",  HTTP_POST, [this]() { handleApiSaveConfig(); });

    // IP fixo ETH
    server->on("/api/device/eth", HTTP_GET,    [this]() { handleApiGetEthConfig(); });
    server->on("/api/device/eth", HTTP_POST,   [this]() { handleApiSaveEthConfig(); });
    server->on("/api/device/eth", HTTP_DELETE, [this]() { handleApiClearEthConfig(); });

    LOG_INFO("Rotas WebInterface registradas (ETH-18CH)");
}

// ==================== HANDLERS EXISTENTES ====================

void WebInterface::handleApiOutputs() {
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (uint8_t i = 0; i < NUM_TOTAL_OUTPUTS; i++) {
        JsonObject o = arr.createNestedObject();
        o["index"] = i;
        o["state"] = io->getOutputState(i);
        if (i < NUM_LOCAL_OUTPUTS) {
            o["label"] = "Local " + String(i + 1);
        } else if (i < NUM_LOCAL_OUTPUTS + NUM_SLAVE_CHANNELS) {
            o["label"] = "S1-DO" + String(i - NUM_LOCAL_OUTPUTS + 1);
        } else {
            o["label"] = "S2-DO" + String(i - NUM_LOCAL_OUTPUTS - NUM_SLAVE_CHANNELS + 1);
        }
    }

    String payload;
    serializeJson(doc, payload);
    server->send(200, "application/json", payload);
}

void WebInterface::handleApiOutputControl() {
    String body = server->arg("plain");
    StaticJsonDocument<128> doc;

    if (deserializeJson(doc, body)) {
        server->send(400, "application/json", "{\"error\":\"JSON invalido\"}");
        return;
    }

    int idx = doc["index"].as<int>();
    if (idx < 0 || idx >= (int)NUM_TOTAL_OUTPUTS) {
        server->send(400, "application/json", "{\"error\":\"Indice invalido\"}");
        return;
    }

    if (doc.containsKey("toggle") && doc["toggle"].as<bool>()) {
        io->toggleOutput((uint8_t)idx);
    } else {
        io->setOutput((uint8_t)idx, doc["state"].as<bool>());
    }

    bool st = io->getOutputState((uint8_t)idx);
    server->send(200, "application/json",
        "{\"index\":" + String(idx) + ",\"state\":" + (st ? "true" : "false") + "}");
}

void WebInterface::handleApiInputs() {
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        JsonObject o = arr.createNestedObject();
        o["index"] = i;
        o["state"] = io->getInputState(i);
        if (i < NUM_LOCAL_INPUTS) {
            o["label"] = "Local " + String(i + 1);
        } else if (i < NUM_LOCAL_INPUTS + NUM_SLAVE_CHANNELS) {
            o["label"] = "S1-DI" + String(i - NUM_LOCAL_INPUTS + 1);
        } else {
            o["label"] = "S2-DI" + String(i - NUM_LOCAL_INPUTS - NUM_SLAVE_CHANNELS + 1);
        }
    }

    String payload;
    serializeJson(doc, payload);
    server->send(200, "application/json", payload);
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
    s1["online"] = slaves->isSlaveOnline(0);
    s1["errors"] = slaves->getSlaveStatus(0).totalErrors;

    JsonObject s2 = doc.createNestedObject("slave2");
    s2["online"] = slaves->isSlaveOnline(1);
    s2["errors"] = slaves->getSlaveStatus(1).totalErrors;

    String payload;
    serializeJson(doc, payload);
    server->send(200, "application/json", payload);
}

void WebInterface::handleApiGetConfig() {
    StaticJsonDocument<512> doc;
    doc["relay_logic"]   = (uint8_t)io->getRelayLogic();
    doc["initial_state"] = (uint8_t)io->getInitialState();
    doc["tele_interval"] = mqtt->getTelemetryInterval() / 1000;
    doc["mqtt_device"]   = mqtt->getDeviceName();

    JsonArray inputs = doc.createNestedArray("inputs");
    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        JsonObject inp = inputs.createNestedObject();
        inp["mode"]     = (uint8_t)io->getInputMode(i);
        inp["debounce"] = io->getDebounceTime(i);
        inp["pulse"]    = io->getPulseTime(i);
    }

    String payload;
    serializeJson(doc, payload);
    server->send(200, "application/json", payload);
}

void WebInterface::handleApiSaveConfig() {
    String body = server->arg("plain");
    StaticJsonDocument<512> doc;

    if (deserializeJson(doc, body)) {
        server->send(400, "application/json", "{\"error\":\"JSON invalido\"}");
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
    server->send(200, "application/json", "{\"ok\":true}");
}

// ==================== HANDLERS IP FIXO ETH ====================

void WebInterface::handleApiGetEthConfig() {
    StaticJsonDocument<256> doc;
    doc["enabled"] = eth->hasStaticIP();
    doc["ip"]      = eth->getIP();
    doc["gateway"] = eth->getGateway();
    doc["subnet"]  = eth->getSubnet();
    doc["dns"]     = eth->getDNS();

    String payload;
    serializeJson(doc, payload);
    server->send(200, "application/json", payload);
}

void WebInterface::handleApiSaveEthConfig() {
    String body = server->arg("plain");
    StaticJsonDocument<256> doc;

    if (deserializeJson(doc, body)) {
        server->send(400, "application/json", "{\"success\":false,\"message\":\"JSON invalido\"}");
        return;
    }

    String ip      = doc["ip"]     .as<String>();
    String gateway = doc["gateway"].as<String>();
    String subnet  = doc["subnet"] .as<String>();
    String dns     = doc["dns"]    | "8.8.8.8";

    if (eth->saveStaticIP(ip, gateway, subnet, dns)) {
        server->send(200, "application/json",
            "{\"success\":true,\"message\":\"IP fixo salvo! Reinicie para aplicar.\"}");
    } else {
        server->send(400, "application/json",
            "{\"success\":false,\"message\":\"IP invalido\"}");
    }
}

void WebInterface::handleApiClearEthConfig() {
    eth->clearStaticIP();
    server->send(200, "application/json",
        "{\"success\":true,\"message\":\"Voltando para DHCP. Reinicie para aplicar.\"}");
}

// ==================== DASHBOARD HTML ====================

String WebInterface::buildDashboard() {
    String html = R"rawhtml(<!DOCTYPE html>
<html lang="pt-BR">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ETH-18CH-MASTER — CMD-C</title>
<style>
  *{box-sizing:border-box;margin:0;padding:0}
  body{font-family:Arial,sans-serif;background:#1a1a2e;color:#eee;padding:12px}
  h1{color:#00d4ff;text-align:center;margin-bottom:4px;font-size:22px}
  .subtitle{text-align:center;color:#888;font-size:12px;margin-bottom:14px}
  .tabs{display:flex;gap:8px;margin-bottom:16px}
  .tab{flex:1;padding:10px;background:#2a2a4a;border:none;border-radius:8px;color:#888;font-size:13px;font-weight:bold;cursor:pointer}
  .tab.active{background:#00d4ff;color:#000}
  .section{display:none}
  .section.active{display:block}
  .status-bar{display:flex;gap:8px;flex-wrap:wrap;margin-bottom:12px;font-size:12px;justify-content:center}
  .badge{padding:4px 10px;border-radius:12px;background:#2a2a4a}
  .badge.ok{background:#0a4a0a;color:#4caf50}
  .badge.err{background:#4a0a0a;color:#f44}
  .section-title{color:#aaa;font-size:11px;text-transform:uppercase;letter-spacing:1px;margin:12px 0 6px}
  .grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(88px,1fr));gap:8px;margin-bottom:8px}
  .btn{padding:12px 4px;border:none;border-radius:8px;cursor:pointer;font-size:12px;font-weight:bold;transition:.2s;white-space:pre-line;line-height:1.4}
  .btn-on{background:#00d4ff;color:#000}
  .btn-off{background:#2a2a4a;color:#888;border:1px solid #3a3a6a}
  .btn:hover{opacity:.85;transform:scale(1.03)}
  .input-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(88px,1fr));gap:8px}
  .input-cell{padding:10px 4px;border-radius:8px;font-size:11px;text-align:center;background:#2a2a4a;color:#888;border:1px solid #3a3a6a;white-space:pre-line;line-height:1.4}
  .input-cell.on{background:#1a3a1a;color:#4caf50;border-color:#2a6a2a;font-weight:bold}
  .card{background:#2a2a4a;border-radius:10px;padding:16px;margin-bottom:14px}
  .card h3{color:#00d4ff;margin-bottom:12px;font-size:14px}
  label{display:block;color:#aaa;font-size:12px;margin-bottom:4px;margin-top:10px}
  input[type=text]{width:100%;padding:9px 10px;background:#1a1a2e;border:1px solid #3a3a6a;border-radius:6px;color:#eee;font-size:13px}
  input[type=text]:focus{outline:none;border-color:#00d4ff}
  .row{display:grid;grid-template-columns:1fr 1fr;gap:10px}
  .btn-save{width:100%;padding:11px;background:#00d4ff;color:#000;border:none;border-radius:8px;font-size:14px;font-weight:bold;cursor:pointer;margin-top:12px}
  .btn-save:hover{opacity:.85}
  .btn-danger{width:100%;padding:11px;background:#4a0a0a;color:#f88;border:1px solid #f44;border-radius:8px;font-size:13px;font-weight:bold;cursor:pointer;margin-top:8px}
  .btn-danger:hover{background:#6a1a1a}
  .msg{padding:10px;border-radius:6px;margin-top:10px;font-size:13px;text-align:center;display:none}
  .msg.ok{background:#0a4a0a;color:#4caf50}
  .msg.err{background:#4a0a0a;color:#f88}
  .nav-btn{display:block;width:100%;margin-top:16px;padding:12px;background:#667eea;color:white;border:none;border-radius:8px;font-size:14px;font-weight:bold;cursor:pointer;text-align:center;text-decoration:none}
  .nav-btn:hover{background:#5568d3}
  @media(max-width:480px){.grid,.input-grid{grid-template-columns:repeat(3,1fr)}.row{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1>🎛️ ETH-18CH-MASTER</h1>
<div class="subtitle">CMD-C v)rawhtml" FIRMWARE_VERSION R"rawhtml( — IP: <span id="ip">...</span></div>

<div class="tabs">
  <button class="tab active" onclick="showTab('control')">🎮 Controle</button>
  <button class="tab"        onclick="showTab('eth')">🌐 Ethernet</button>
</div>

<!-- ==================== ABA CONTROLE ==================== -->
<div class="section active" id="tab-control">
  <div class="status-bar">
    <span class="badge" id="eth-badge">ETH ...</span>
    <span class="badge" id="mqtt-badge">MQTT ...</span>
    <span class="badge" id="s1-badge">S1 ...</span>
    <span class="badge" id="s2-badge">S2 ...</span>
  </div>

  <div class="section-title">Saídas (18)</div>
  <div class="grid" id="outputs-grid"></div>

  <div class="section-title">Entradas (18)</div>
  <div class="input-grid" id="inputs-grid"></div>
</div>

<!-- ==================== ABA ETHERNET ==================== -->
<div class="section" id="tab-eth">
  <div class="card">
    <h3>🌐 IP Fixo Ethernet</h3>
    <p style="color:#888;font-size:12px;margin-bottom:8px">
      Deixe em branco para usar DHCP. Após salvar, reinicie o dispositivo.
    </p>

    <label>IP do Dispositivo</label>
    <input type="text" id="eth-ip" placeholder="ex: 192.168.1.100">

    <div class="row">
      <div>
        <label>Gateway</label>
        <input type="text" id="eth-gw" placeholder="ex: 192.168.1.1">
      </div>
      <div>
        <label>Máscara</label>
        <input type="text" id="eth-sn" placeholder="ex: 255.255.255.0">
      </div>
    </div>

    <label>DNS (opcional)</label>
    <input type="text" id="eth-dns" placeholder="ex: 8.8.8.8">

    <button class="btn-save" onclick="saveEthConfig()">💾 Salvar IP Fixo</button>
    <button class="btn-danger" onclick="clearEthConfig()">🔄 Voltar para DHCP</button>
    <div class="msg" id="eth-msg"></div>
  </div>

  <div class="card">
    <h3>📊 Status Ethernet</h3>
    <div id="eth-status" style="font-size:13px;color:#aaa;line-height:1.8">
      Carregando...
    </div>
  </div>
</div>

<a class="nav-btn" href="/">← Menu CMD-C</a>

<script>
let autoUpdate;

function showTab(name) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
  document.getElementById('tab-' + name).classList.add('active');
  event.target.classList.add('active');

  if (name === 'eth') {
    clearInterval(autoUpdate);
    loadEthConfig();
    loadEthStatus();
  } else {
    startAutoUpdate();
  }
}

// ==================== CONTROLE ====================

function toggleOutput(idx, label) {
  fetch('/api/device/outputs', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({index: idx, toggle: true})
  }).then(r => r.json()).then(d => {
    const btn = document.getElementById('out-' + idx);
    if (btn) {
      btn.className = 'btn ' + (d.state ? 'btn-on' : 'btn-off');
      btn.textContent = label + '\n' + (d.state ? '● ON' : '○ OFF');
    }
  }).catch(console.error);
}

function loadOutputs() {
  fetch('/api/device/outputs').then(r => r.json()).then(data => {
    const grid = document.getElementById('outputs-grid');
    grid.innerHTML = '';
    data.forEach(o => {
      const btn = document.createElement('button');
      btn.id = 'out-' + o.index;
      btn.className = 'btn ' + (o.state ? 'btn-on' : 'btn-off');
      btn.textContent = o.label + '\n' + (o.state ? '● ON' : '○ OFF');
      btn.onclick = () => toggleOutput(o.index, o.label);
      grid.appendChild(btn);
    });
  }).catch(console.error);
}

function loadInputs() {
  fetch('/api/device/inputs').then(r => r.json()).then(data => {
    const grid = document.getElementById('inputs-grid');
    grid.innerHTML = '';
    data.forEach(inp => {
      const div = document.createElement('div');
      div.id = 'in-' + inp.index;
      div.className = 'input-cell' + (inp.state ? ' on' : '');
      div.textContent = inp.label + '\n' + (inp.state ? '⬆ HIGH' : '⬇ LOW');
      grid.appendChild(div);
    });
  }).catch(console.error);
}

function loadStatus() {
  fetch('/api/device/status').then(r => r.json()).then(d => {
    document.getElementById('ip').textContent = d.ip || '---';
    setbadge('eth-badge',  'ETH',  d.eth_link);
    setbadge('mqtt-badge', 'MQTT', d.mqtt);
    setbadge('s1-badge',   'S1',   d.slave1.online);
    setbadge('s2-badge',   'S2',   d.slave2.online);
  }).catch(console.error);
}

function setbadge(id, label, ok) {
  const el = document.getElementById(id);
  el.textContent = label + ' ' + (ok ? 'OK' : 'OFF');
  el.className = 'badge ' + (ok ? 'ok' : 'err');
}

function startAutoUpdate() {
  clearInterval(autoUpdate);
  loadOutputs(); loadInputs(); loadStatus();
  autoUpdate = setInterval(() => { loadInputs(); loadStatus(); }, 2000);
  setInterval(loadOutputs, 5000);
}

// ==================== ETH CONFIG ====================

function loadEthConfig() {
  fetch('/api/device/eth').then(r => r.json()).then(d => {
    document.getElementById('eth-ip').value  = d.enabled ? d.ip      : '';
    document.getElementById('eth-gw').value  = d.enabled ? d.gateway : '';
    document.getElementById('eth-sn').value  = d.enabled ? d.subnet  : '255.255.255.0';
    document.getElementById('eth-dns').value = d.enabled ? d.dns     : '8.8.8.8';
  }).catch(console.error);
}

function loadEthStatus() {
  fetch('/api/device/status').then(r => r.json()).then(d => {
    document.getElementById('eth-status').innerHTML =
      '<b>IP atual:</b> '   + (d.ip || '---')      + '<br>' +
      '<b>MAC:</b> '        + (d.mac || '---')      + '<br>' +
      '<b>Link:</b> '       + (d.eth_link ? '✅ Conectado' : '❌ Sem cabo') + '<br>' +
      '<b>Uptime:</b> '     + formatUptime(d.uptime);
  }).catch(console.error);
}

function saveEthConfig() {
  const ip  = document.getElementById('eth-ip').value.trim();
  const gw  = document.getElementById('eth-gw').value.trim();
  const sn  = document.getElementById('eth-sn').value.trim();
  const dns = document.getElementById('eth-dns').value.trim();

  if (!ip || !gw || !sn) {
    showMsg('eth-msg', 'Preencha IP, Gateway e Máscara', false);
    return;
  }

  fetch('/api/device/eth', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({ip, gateway: gw, subnet: sn, dns})
  }).then(r => r.json()).then(d => {
    showMsg('eth-msg', d.message, d.success);
  }).catch(e => showMsg('eth-msg', 'Erro: ' + e.message, false));
}

function clearEthConfig() {
  if (!confirm('Voltar para DHCP?\n\nO IP será obtido automaticamente após reiniciar.')) return;

  fetch('/api/device/eth', {method: 'DELETE'})
    .then(r => r.json())
    .then(d => {
      showMsg('eth-msg', d.message, d.success);
      document.getElementById('eth-ip').value  = '';
      document.getElementById('eth-gw').value  = '';
      document.getElementById('eth-sn').value  = '255.255.255.0';
      document.getElementById('eth-dns').value = '8.8.8.8';
    }).catch(e => showMsg('eth-msg', 'Erro: ' + e.message, false));
}

function showMsg(id, text, ok) {
  const el = document.getElementById(id);
  el.textContent = text;
  el.className = 'msg ' + (ok ? 'ok' : 'err');
  el.style.display = 'block';
  setTimeout(() => el.style.display = 'none', 4000);
}

function formatUptime(s) {
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  return h + 'h ' + m + 'm';
}

// Inicia aba de controle
startAutoUpdate();
</script>
</body>
</html>)rawhtml";
    return html;
}