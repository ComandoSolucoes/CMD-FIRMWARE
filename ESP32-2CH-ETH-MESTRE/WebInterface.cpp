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

    server->on("/api/device/eth", HTTP_GET,    [this]() { handleApiGetEthConfig(); });
    server->on("/api/device/eth", HTTP_POST,   [this]() { handleApiSaveEthConfig(); });
    server->on("/api/device/eth", HTTP_DELETE, [this]() { handleApiClearEthConfig(); });

    LOG_INFO("Rotas WebInterface registradas (ETH-18CH)");
}

// ==================== OUTPUTS ====================

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

// ==================== INPUTS ====================

void WebInterface::handleApiInputs() {
    StaticJsonDocument<512> doc;
    JsonArray arr = doc.to<JsonArray>();

    for (uint8_t i = 0; i < NUM_TOTAL_INPUTS; i++) {
        JsonObject o = arr.createNestedObject();
        o["index"] = i;
        o["state"] = io->getInputState(i);
        if (i < NUM_LOCAL_INPUTS) {
            o["label"] = "Local IN" + String(i + 1);
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

// ==================== STATUS ====================

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

// ==================== CONFIG GET ====================

void WebInterface::handleApiGetConfig() {
    // Documento maior: 18 entradas × 3 campos + campos gerais
    StaticJsonDocument<1024> doc;
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

// ==================== CONFIG SAVE ====================

void WebInterface::handleApiSaveConfig() {
    String body = server->arg("plain");
    StaticJsonDocument<1024> doc;  // maior para suportar 18 canais

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

    // Persiste na NVS
    io->saveConfig();

    // Envia nova config para os escravos imediatamente
    io->pushSlavesConfig();

    server->send(200, "application/json", "{\"ok\":true}");
}

// ==================== ETH CONFIG ====================

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
.tabs{display:flex;gap:6px;margin-bottom:14px;flex-wrap:wrap}
.tab{flex:1;min-width:80px;padding:9px 4px;background:#2a2a4a;border:none;border-radius:8px;color:#888;font-size:12px;font-weight:bold;cursor:pointer}
.tab.active{background:#00d4ff;color:#000}
.section{display:none}
.section.active{display:block}
.status-bar{display:flex;gap:6px;flex-wrap:wrap;margin-bottom:10px;font-size:11px;justify-content:center}
.badge{padding:3px 8px;border-radius:10px;background:#2a2a4a}
.badge.ok{background:#0a4a0a;color:#4caf50}
.badge.err{background:#4a0a0a;color:#f44}
.sec-title{color:#aaa;font-size:10px;text-transform:uppercase;letter-spacing:1px;margin:10px 0 5px}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(84px,1fr));gap:7px;margin-bottom:6px}
.btn{padding:11px 4px;border:none;border-radius:8px;cursor:pointer;font-size:11px;font-weight:bold;transition:.2s;white-space:pre-line;line-height:1.4}
.btn-on{background:#00d4ff;color:#000}
.btn-off{background:#2a2a4a;color:#888;border:1px solid #3a3a6a}
.btn:hover{opacity:.85;transform:scale(1.03)}
.inp-grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(84px,1fr));gap:7px}
.inp-cell{padding:9px 4px;border-radius:8px;font-size:10px;text-align:center;background:#2a2a4a;color:#888;border:1px solid #3a3a6a;white-space:pre-line;line-height:1.4}
.inp-cell.on{background:#1a3a1a;color:#4caf50;border-color:#2a6a2a;font-weight:bold}
.card{background:#2a2a4a;border-radius:10px;padding:14px;margin-bottom:12px}
.card h3{color:#00d4ff;margin-bottom:10px;font-size:13px}
label{display:block;color:#aaa;font-size:11px;margin-bottom:3px;margin-top:8px}
input[type=text],input[type=number],select{width:100%;padding:7px;background:#1a1a2e;border:1px solid #3a3a6a;border-radius:6px;color:#eee;font-size:12px}
input:focus,select:focus{outline:none;border-color:#00d4ff}
.row2{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.btn-save{width:100%;padding:10px;background:#00d4ff;color:#000;border:none;border-radius:8px;font-size:13px;font-weight:bold;cursor:pointer;margin-top:10px}
.btn-save:hover{opacity:.85}
.btn-danger{width:100%;padding:10px;background:#4a0a0a;color:#f88;border:1px solid #f44;border-radius:8px;font-size:12px;font-weight:bold;cursor:pointer;margin-top:6px}
.btn-danger:hover{background:#6a1a1a}
.msg{padding:9px;border-radius:6px;margin-top:8px;font-size:12px;text-align:center;display:none}
.msg.ok{background:#0a4a0a;color:#4caf50}
.msg.err{background:#4a0a0a;color:#f88}
.nav-btn{display:block;width:100%;margin-top:14px;padding:11px;background:#667eea;color:white;border:none;border-radius:8px;font-size:13px;font-weight:bold;cursor:pointer;text-align:center;text-decoration:none}
.nav-btn:hover{background:#5568d3}
/* Tabela de config de entradas */
.cfg-table{width:100%;border-collapse:collapse;font-size:11px}
.cfg-table th{color:#00d4ff;padding:5px 3px;text-align:left;border-bottom:1px solid #3a3a6a;font-size:10px;text-transform:uppercase}
.cfg-table td{padding:4px 3px;border-bottom:1px solid #222;vertical-align:middle}
.cfg-table tr:last-child td{border-bottom:none}
.cfg-table tr.grp td{background:#1a1a3a;color:#aaa;font-size:10px;font-weight:bold;padding:4px}
.cfg-table select,.cfg-table input{padding:3px 5px;font-size:11px;margin:0}
.ch-lbl{font-weight:bold;color:#eee;white-space:nowrap}
@media(max-width:480px){.grid,.inp-grid{grid-template-columns:repeat(3,1fr)}.row2{grid-template-columns:1fr}}
</style>
</head>
<body>
<h1>🎛️ ETH-18CH-MASTER</h1>
<div class="subtitle">v)rawhtml" FIRMWARE_VERSION R"rawhtml( — <span id="ip">...</span></div>

<div class="tabs">
  <button class="tab active" onclick="showTab('control',this)">🎮 Controle</button>
  <button class="tab"        onclick="showTab('inputs',this)">⚙️ Entradas</button>
  <button class="tab"        onclick="showTab('eth',this)">🌐 Ethernet</button>
</div>

<!-- ===== ABA: CONTROLE ===== -->
<div class="section active" id="tab-control">
  <div class="status-bar">
    <span class="badge" id="eth-badge">ETH ...</span>
    <span class="badge" id="mqtt-badge">MQTT ...</span>
    <span class="badge" id="s1-badge">S1 ...</span>
    <span class="badge" id="s2-badge">S2 ...</span>
  </div>
  <div class="sec-title">Saídas (18)</div>
  <div class="grid" id="outputs-grid"></div>
  <div class="sec-title">Entradas (18)</div>
  <div class="inp-grid" id="inputs-grid"></div>
</div>

<!-- ===== ABA: CONFIG ENTRADAS ===== -->
<div class="section" id="tab-inputs">
  <div class="card">
    <h3>⚙️ Configuração das Entradas por Canal</h3>
    <p style="color:#888;font-size:11px;margin-bottom:8px">
      Ao salvar, a configuração é enviada imediatamente para os escravos via I2C.
    </p>
    <table class="cfg-table">
      <thead>
        <tr>
          <th>Canal</th><th>Modo</th><th>Deb(ms)</th><th>Pulse(ms)</th>
        </tr>
      </thead>
      <tbody id="cfg-body"></tbody>
    </table>
  </div>

  <div class="card">
    <h3>🔧 Configuração Geral</h3>
    <label>Lógica dos Relés</label>
    <select id="relay-logic">
      <option value="0">Normal (HIGH=ON)</option>
      <option value="1">Invertida (HIGH=OFF)</option>
    </select>
    <label>Estado na Inicialização</label>
    <select id="initial-state">
      <option value="0">Sempre Desligados</option>
      <option value="1">Sempre Ligados</option>
      <option value="2">Recuperar Último Estado</option>
    </select>
    <label>Intervalo Telemetria MQTT (s)</label>
    <input type="number" id="tele-interval" min="5" max="300" value="30">
  </div>

  <button class="btn-save" onclick="saveInputConfig()">💾 Salvar e Enviar para Escravos</button>
  <div class="msg" id="cfg-msg"></div>
</div>

<!-- ===== ABA: ETHERNET ===== -->
<div class="section" id="tab-eth">
  <div class="card">
    <h3>🌐 IP Fixo Ethernet</h3>
    <p style="color:#888;font-size:11px;margin-bottom:8px">Deixe vazio para DHCP. Reinicie após salvar.</p>
    <label>IP do Dispositivo</label>
    <input type="text" id="eth-ip" placeholder="192.168.1.100">
    <div class="row2">
      <div><label>Gateway</label><input type="text" id="eth-gw" placeholder="192.168.1.1"></div>
      <div><label>Máscara</label><input type="text" id="eth-sn" placeholder="255.255.255.0"></div>
    </div>
    <label>DNS (opcional)</label>
    <input type="text" id="eth-dns" placeholder="8.8.8.8">
    <button class="btn-save"   onclick="saveEthConfig()">💾 Salvar IP Fixo</button>
    <button class="btn-danger" onclick="clearEthConfig()">🔄 Voltar para DHCP</button>
    <div class="msg" id="eth-msg"></div>
  </div>
  <div class="card">
    <h3>📊 Status Ethernet</h3>
    <div id="eth-status" style="font-size:12px;color:#aaa;line-height:1.8">Carregando...</div>
  </div>
</div>

<a class="nav-btn" href="/">← Menu CMD-C</a>

<script>
const MODES = ['Desabilitado','Transição (Toggle)','Pulso','Seguir','Invertido'];
const IN_LABELS = [
  'Local IN1','Local IN2',
  'S1-DI1','S1-DI2','S1-DI3','S1-DI4','S1-DI5','S1-DI6','S1-DI7','S1-DI8',
  'S2-DI1','S2-DI2','S2-DI3','S2-DI4','S2-DI5','S2-DI6','S2-DI7','S2-DI8'
];
let autoTmr;

function showTab(name, btn) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
  document.getElementById('tab-' + name).classList.add('active');
  btn.classList.add('active');
  clearInterval(autoTmr);
  if (name === 'control') startAuto();
  else if (name === 'inputs') loadCfg();
  else if (name === 'eth') { loadEthCfg(); loadEthStatus(); }
}

/* ========== CONTROLE ========== */
function toggleOut(idx, lbl) {
  fetch('/api/device/outputs', {
    method: 'POST', headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({index: idx, toggle: true})
  }).then(r => r.json()).then(d => {
    const b = document.getElementById('out-' + idx);
    if (b) { b.className = 'btn ' + (d.state ? 'btn-on' : 'btn-off');
              b.textContent = lbl + '\n' + (d.state ? '● ON' : '○ OFF'); }
  }).catch(console.error);
}

function loadOutputs() {
  fetch('/api/device/outputs').then(r => r.json()).then(data => {
    const g = document.getElementById('outputs-grid');
    g.innerHTML = '';
    data.forEach(o => {
      const b = document.createElement('button');
      b.id = 'out-' + o.index;
      b.className = 'btn ' + (o.state ? 'btn-on' : 'btn-off');
      b.textContent = o.label + '\n' + (o.state ? '● ON' : '○ OFF');
      b.onclick = () => toggleOut(o.index, o.label);
      g.appendChild(b);
    });
  }).catch(console.error);
}

function loadInputs() {
  fetch('/api/device/inputs').then(r => r.json()).then(data => {
    const g = document.getElementById('inputs-grid');
    g.innerHTML = '';
    data.forEach(inp => {
      const d = document.createElement('div');
      d.id = 'in-' + inp.index;
      d.className = 'inp-cell' + (inp.state ? ' on' : '');
      d.textContent = inp.label + '\n' + (inp.state ? '⬆ HIGH' : '⬇ LOW');
      g.appendChild(d);
    });
  }).catch(console.error);
}

function loadStatus() {
  fetch('/api/device/status').then(r => r.json()).then(d => {
    document.getElementById('ip').textContent = d.ip || '---';
    badge('eth-badge',  'ETH',  d.eth_link);
    badge('mqtt-badge', 'MQTT', d.mqtt);
    badge('s1-badge',   'S1',   d.slave1.online);
    badge('s2-badge',   'S2',   d.slave2.online);
  }).catch(console.error);
}

function badge(id, lbl, ok) {
  const el = document.getElementById(id);
  el.textContent = lbl + ' ' + (ok ? 'OK' : 'OFF');
  el.className = 'badge ' + (ok ? 'ok' : 'err');
}

function startAuto() {
  clearInterval(autoTmr);
  loadOutputs(); loadInputs(); loadStatus();
  autoTmr = setInterval(() => { loadInputs(); loadStatus(); }, 2000);
  setInterval(loadOutputs, 5000);
}

/* ========== CONFIG ENTRADAS ========== */
function loadCfg() {
  fetch('/api/device/config').then(r => r.json()).then(d => {
    document.getElementById('relay-logic').value   = d.relay_logic   || 0;
    document.getElementById('initial-state').value = d.initial_state || 0;
    document.getElementById('tele-interval').value = d.tele_interval || 30;

    const tb = document.getElementById('cfg-body');
    tb.innerHTML = '';

    const groups = [
      { lbl: '📍 Local (Master)', s: 0,  n: 2  },
      { lbl: '🔌 Escravo 1',      s: 2,  n: 8  },
      { lbl: '🔌 Escravo 2',      s: 10, n: 8  }
    ];

    groups.forEach(g => {
      // cabeçalho de grupo
      const hdr = document.createElement('tr');
      hdr.className = 'grp';
      hdr.innerHTML = '<td colspan="4">' + g.lbl + '</td>';
      tb.appendChild(hdr);

      for (let i = g.s; i < g.s + g.n; i++) {
        const inp = (d.inputs && d.inputs[i]) ? d.inputs[i] : {mode:1, debounce:50, pulse:500};
        const opts = MODES.map((m, x) =>
          '<option value="' + x + '"' + (inp.mode == x ? ' selected' : '') + '>' + m + '</option>'
        ).join('');
        const tr = document.createElement('tr');
        tr.innerHTML =
          '<td class="ch-lbl">' + IN_LABELS[i] + '</td>' +
          '<td><select id="mode-' + i + '">' + opts + '</select></td>' +
          '<td><input type="number" id="deb-' + i + '" value="' + inp.debounce + '" min="10" max="1000" style="width:60px"></td>' +
          '<td><input type="number" id="pulse-' + i + '" value="' + inp.pulse + '" min="50" max="10000" style="width:68px"' +
              (inp.mode != 2 ? ' disabled' : '') + '></td>';
        tb.appendChild(tr);

        document.getElementById('mode-' + i).onchange = (function(idx) {
          return function() {
            document.getElementById('pulse-' + idx).disabled = (parseInt(this.value) != 2);
          };
        })(i);
      }
    });
  }).catch(console.error);
}

async function saveInputConfig() {
  const inputs = [];
  for (let i = 0; i < 18; i++) {
    const m = document.getElementById('mode-'  + i);
    const d = document.getElementById('deb-'   + i);
    const p = document.getElementById('pulse-' + i);
    if (!m) break;
    inputs.push({ mode: parseInt(m.value), debounce: parseInt(d.value), pulse: parseInt(p.value) });
  }
  const payload = {
    relay_logic:   parseInt(document.getElementById('relay-logic').value),
    initial_state: parseInt(document.getElementById('initial-state').value),
    tele_interval: parseInt(document.getElementById('tele-interval').value),
    inputs
  };
  try {
    const r = await fetch('/api/device/config', {
      method: 'POST', headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(payload)
    });
    const d = await r.json();
    showMsg('cfg-msg', d.ok ? '✅ Configuração salva e enviada para os escravos!' : '❌ Erro ao salvar', !!d.ok);
  } catch(e) { showMsg('cfg-msg', '❌ ' + e.message, false); }
}

/* ========== ETHERNET ========== */
function loadEthCfg() {
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
      '<b>IP:</b> ' + (d.ip||'---') + '<br>' +
      '<b>MAC:</b> ' + (d.mac||'---') + '<br>' +
      '<b>Link:</b> ' + (d.eth_link ? '✅ Conectado' : '❌ Sem cabo') + '<br>' +
      '<b>Uptime:</b> ' + fmtUp(d.uptime);
  }).catch(console.error);
}
function saveEthConfig() {
  const ip=document.getElementById('eth-ip').value.trim();
  const gw=document.getElementById('eth-gw').value.trim();
  const sn=document.getElementById('eth-sn').value.trim();
  const dns=document.getElementById('eth-dns').value.trim();
  if (!ip||!gw||!sn) { showMsg('eth-msg','Preencha IP, Gateway e Máscara',false); return; }
  fetch('/api/device/eth', {method:'POST', headers:{'Content-Type':'application/json'},
    body: JSON.stringify({ip, gateway:gw, subnet:sn, dns})})
    .then(r=>r.json()).then(d=>showMsg('eth-msg',d.message,d.success))
    .catch(e=>showMsg('eth-msg','Erro: '+e.message,false));
}
function clearEthConfig() {
  if (!confirm('Voltar para DHCP?')) return;
  fetch('/api/device/eth',{method:'DELETE'}).then(r=>r.json()).then(d=>{
    showMsg('eth-msg',d.message,d.success);
    ['eth-ip','eth-gw'].forEach(id=>document.getElementById(id).value='');
    document.getElementById('eth-sn').value='255.255.255.0';
    document.getElementById('eth-dns').value='8.8.8.8';
  }).catch(e=>showMsg('eth-msg','Erro: '+e.message,false));
}
function showMsg(id, txt, ok) {
  const el=document.getElementById(id);
  el.textContent=txt; el.className='msg '+(ok?'ok':'err'); el.style.display='block';
  setTimeout(()=>el.style.display='none',4500);
}
function fmtUp(s) { return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m'; }

startAuto();
</script>
</body>
</html>)rawhtml";
    return html;
}