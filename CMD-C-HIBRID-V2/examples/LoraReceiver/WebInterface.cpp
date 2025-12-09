#include "WebInterface.h"
#include <webserver/CMDWebServer.h>
#include <ArduinoJson.h>

WebInterface::WebInterface(LoRaReceiver* receiver, MqttPublisher* publisher)
    : lora(receiver), mqtt(publisher), server(nullptr) {
}

void WebInterface::registerRoutes(CMDWebServer* webServer) {
    server = &webServer->getServer();
    
    // Usa método específico que já inclui autenticação
    webServer->registerDeviceRoute([this]() { handleDevicePage(); });
    
    // APIs REST
    server->on("/api/device/status", HTTP_GET, [this]() { handleApiStatus(); });
    server->on("/api/device/messages", HTTP_GET, [this]() { handleApiMessages(); });
    server->on("/api/device/clear-log", HTTP_POST, [this]() { handleApiClearLog(); });
    server->on("/api/device/config", HTTP_GET, [this]() { handleApiConfig(); });
    server->on("/api/device/config", HTTP_POST, [this]() { handleApiSaveConfig(); });
    
    LOG_INFO("Rotas da interface web registradas");
}

// ==================== PÁGINA HTML ====================

void WebInterface::handleDevicePage() {
    server->send(200, "text/html", generateDevicePage());
}

String WebInterface::generateDevicePage() {
    return R"rawliteral(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>LoRa Receiver - Log</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;color:#333}
.container{max-width:900px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:10px;font-size:28px}
.subtitle{color:rgba(255,255,255,0.9);text-align:center;margin-bottom:30px;font-size:16px}
.card{background:white;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 16px rgba(0,0,0,0.1)}
.card h2{color:#667eea;margin-bottom:15px;font-size:18px;border-bottom:2px solid #f0f0f0;padding-bottom:10px}
.stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px;margin:20px 0}
.stat-box{background:#f5f5f5;padding:15px;border-radius:8px;text-align:center}
.stat-value{font-size:28px;font-weight:bold;color:#667eea;margin:5px 0}
.stat-label{font-size:13px;color:#666}
.message-log{margin:20px 0}
.message{background:#f5f5f5;padding:15px;margin:10px 0;border-radius:8px;border-left:4px solid #667eea}
.message-header{display:flex;justify-content:space-between;margin-bottom:8px;font-size:13px;color:#666}
.message-payload{font-family:monospace;background:white;padding:10px;border-radius:4px;margin:8px 0;word-break:break-all}
.message-meta{display:flex;gap:15px;font-size:12px;color:#888;margin-top:8px}
.no-messages{text-align:center;color:#888;padding:40px;font-style:italic}
.btn{width:100%;padding:14px;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
.btn-primary{background:#667eea;color:white}
.btn-primary:hover{background:#5568d3}
.btn-secondary{background:#ff9800;color:white}
.btn-secondary:hover{background:#f57c00}
.btn-danger{background:#f44336;color:white}
.btn-danger:hover{background:#d32f2f}
.btn-back{background:#e0e0e0;color:#333}
.btn-back:hover{background:#d0d0d0}
.tabs{display:flex;gap:10px;margin-bottom:20px}
.tab{flex:1;padding:12px;background:#f5f5f5;border:none;border-radius:8px;font-weight:600;cursor:pointer;color:#666}
.tab.active{background:#667eea;color:white}
.config-section{display:none}
.config-section.active{display:block}
.form-group{margin:15px 0}
.form-group label{display:block;font-weight:600;margin-bottom:8px;color:#555}
.form-group input,.form-group select{width:100%;padding:10px;border:2px solid #e0e0e0;border-radius:8px;font-size:14px}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:15px}
small{color:#888;font-size:12px}
.badge{display:inline-block;padding:4px 8px;border-radius:4px;font-size:11px;font-weight:600}
.badge-good{background:#e8f5e9;color:#2e7d32}
.badge-medium{background:#fff3e0;color:#e65100}
.badge-poor{background:#ffebee;color:#c62828}
@media(max-width:600px){.stats-grid{grid-template-columns:1fr}}
</style>
</head><body>
<div class="container">
<h1>📡 LoRa Receiver</h1>
<div class="subtitle">Monitor de Mensagens Recebidas</div>

<div class="tabs">
  <button class="tab active" onclick="showTab('log')">📝 Log</button>
  <button class="tab" onclick="showTab('config')">⚙️ Configurações</button>
  <button class="tab" onclick="showTab('stats')">📊 Estatísticas</button>
</div>

<!-- SEÇÃO LOG -->
<div class="config-section active" id="logSection">
  <div class="card">
    <h2>📥 Últimas Mensagens Recebidas</h2>
    <button class="btn btn-secondary" onclick="refreshLog()">🔄 Atualizar</button>
    <div class="message-log" id="messageLog">
      <div class="no-messages">Nenhuma mensagem recebida ainda...</div>
    </div>
    <button class="btn btn-danger" onclick="clearLog()">🗑️ Limpar Log</button>
  </div>
</div>

<!-- SEÇÃO CONFIGURAÇÕES -->
<div class="config-section" id="configSection">
  <div class="card">
    <h2>📡 LoRa</h2>
    <div class="form-row">
      <div class="form-group">
        <label>Spreading Factor:</label>
        <select id="loraSpreading">
          <option value="6">SF6</option>
          <option value="7">SF7</option>
          <option value="8">SF8</option>
          <option value="9">SF9</option>
          <option value="10">SF10</option>
          <option value="11">SF11</option>
          <option value="12">SF12</option>
        </select>
      </div>
      <div class="form-group">
        <label>Bandwidth:</label>
        <select id="loraBandwidth">
          <option value="125000">125 kHz</option>
          <option value="250000">250 kHz</option>
          <option value="500000">500 kHz</option>
        </select>
      </div>
    </div>
    <small>⚠️ Configurações devem ser iguais ao transmissor</small>
  </div>
  
  <div class="card">
    <h2>🌐 MQTT</h2>
    <div class="form-group">
      <label>
        <input type="checkbox" id="autoPublish" style="width:auto;display:inline;margin-right:8px">
        Publicar automaticamente no MQTT
      </label>
      <small>Publica em: cmd-c/{MAC}/lora/received</small>
    </div>
  </div>
  
  <button class="btn btn-primary" onclick="saveConfig()">💾 Salvar Configurações</button>
</div>

<!-- SEÇÃO ESTATÍSTICAS -->
<div class="config-section" id="statsSection">
  <div class="card">
    <h2>📊 Estatísticas</h2>
    <div class="stats-grid">
      <div class="stat-box">
        <div class="stat-label">Total Recebidas</div>
        <div class="stat-value" id="statTotal">0</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">Erro CRC</div>
        <div class="stat-value" id="statErrors">0</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">RSSI Médio</div>
        <div class="stat-value" id="statRssi">---</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">SNR Médio</div>
        <div class="stat-value" id="statSnr">---</div>
      </div>
    </div>
  </div>
</div>

<button class="btn btn-back" onclick="location.href='/'">← Voltar ao Menu</button>

<script>
let updateInterval;

function showTab(tab) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.config-section').forEach(s => s.classList.remove('active'));
  
  if (tab === 'log') {
    document.querySelector('.tab:nth-child(1)').classList.add('active');
    document.getElementById('logSection').classList.add('active');
    startAutoUpdate();
  } else if (tab === 'config') {
    document.querySelector('.tab:nth-child(2)').classList.add('active');
    document.getElementById('configSection').classList.add('active');
    stopAutoUpdate();
    loadConfig();
  } else if (tab === 'stats') {
    document.querySelector('.tab:nth-child(3)').classList.add('active');
    document.getElementById('statsSection').classList.add('active');
    stopAutoUpdate();
    loadStats();
  }
}

async function refreshLog() {
  try {
    const r = await fetch('/api/device/messages');
    const data = await r.json();
    updateLogDisplay(data);
  } catch (e) {
    console.error('Erro:', e);
  }
}

function updateLogDisplay(data) {
  const logDiv = document.getElementById('messageLog');
  
  if (data.count === 0) {
    logDiv.innerHTML = '<div class="no-messages">Nenhuma mensagem recebida ainda...</div>';
    return;
  }
  
  let html = '';
  for (let i = data.messages.length - 1; i >= 0; i--) {
    const msg = data.messages[i];
    const time = formatTime(msg.timestamp);
    const rssiClass = msg.rssi > -50 ? 'badge-good' : msg.rssi > -80 ? 'badge-medium' : 'badge-poor';
    
    html += `<div class="message">
      <div class="message-header">
        <span>#${data.count - i}</span>
        <span>${time}</span>
      </div>
      <div class="message-payload">${msg.payload}</div>
      <div class="message-meta">
        <span class="badge ${rssiClass}">RSSI: ${msg.rssi} dBm</span>
        <span class="badge badge-good">SNR: ${msg.snr.toFixed(1)} dB</span>
      </div>
    </div>`;
  }
  
  logDiv.innerHTML = html;
}

async function clearLog() {
  if (!confirm('Limpar log de mensagens?')) return;
  try {
    await fetch('/api/device/clear-log', {method: 'POST'});
    refreshLog();
  } catch (e) {
    console.error('Erro:', e);
  }
}

async function loadConfig() {
  try {
    const r = await fetch('/api/device/config');
    const data = await r.json();
    
    document.getElementById('loraSpreading').value = data.lora_spreading || 7;
    document.getElementById('loraBandwidth').value = data.lora_bandwidth || 125000;
    document.getElementById('autoPublish').checked = data.auto_publish !== false;
  } catch (e) {
    console.error('Erro:', e);
  }
}

async function saveConfig() {
  const config = {
    lora_spreading: parseInt(document.getElementById('loraSpreading').value),
    lora_bandwidth: parseInt(document.getElementById('loraBandwidth').value),
    auto_publish: document.getElementById('autoPublish').checked
  };
  
  try {
    const r = await fetch('/api/device/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(config)
    });
    const data = await r.json();
    if (data.success) {
      alert('✅ Configurações salvas!');
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function loadStats() {
  try {
    const r = await fetch('/api/device/status');
    const data = await r.json();
    
    document.getElementById('statTotal').textContent = data.total_received || 0;
    document.getElementById('statErrors').textContent = data.error_count || 0;
    
    if (data.avg_rssi) {
      document.getElementById('statRssi').textContent = data.avg_rssi.toFixed(0) + ' dBm';
    }
    if (data.avg_snr) {
      document.getElementById('statSnr').textContent = data.avg_snr.toFixed(1) + ' dB';
    }
  } catch (e) {
    console.error('Erro:', e);
  }
}

function formatTime(timestamp) {
  const sec = Math.floor((Date.now() - timestamp) / 1000);
  if (sec < 60) return 'há ' + sec + 's';
  if (sec < 3600) return 'há ' + Math.floor(sec / 60) + 'm';
  return 'há ' + Math.floor(sec / 3600) + 'h';
}

function startAutoUpdate() {
  if (updateInterval) clearInterval(updateInterval);
  refreshLog();
  updateInterval = setInterval(refreshLog, 3000);
}

function stopAutoUpdate() {
  if (updateInterval) {
    clearInterval(updateInterval);
    updateInterval = null;
  }
}

window.onload = () => {
  refreshLog();
  startAutoUpdate();
};

window.onbeforeunload = () => {
  stopAutoUpdate();
};
</script>
</div></body></html>
)rawliteral";
}

// ==================== API REST ====================

void WebInterface::handleApiStatus() {
    JsonDocument doc;
    
    doc["device"] = DEVICE_MODEL;
    doc["version"] = FIRMWARE_VERSION;
    doc["uptime"] = millis() / 1000;
    doc["lora_initialized"] = lora->isInitialized();
    doc["total_received"] = lora->getTotalReceived();
    doc["error_count"] = lora->getErrorCount();
    
    // Calcula RSSI e SNR médios
    int count;
    const ReceivedMessage* messages = lora->getLastMessages(count);
    
    if (count > 0) {
        int totalRssi = 0;
        float totalSnr = 0;
        for (int i = 0; i < count; i++) {
            totalRssi += messages[i].rssi;
            totalSnr += messages[i].snr;
        }
        doc["avg_rssi"] = totalRssi / count;
        doc["avg_snr"] = totalSnr / count;
    }
    
    String json;
    serializeJson(doc, json);
    server->send(200, "application/json", json);
}

void WebInterface::handleApiMessages() {
    int count;
    const ReceivedMessage* messages = lora->getLastMessages(count);
    
    JsonDocument doc;
    doc["count"] = count;
    
    JsonArray arr = doc["messages"].to<JsonArray>();
    for (int i = 0; i < count; i++) {
        JsonObject msg = arr.add<JsonObject>();
        msg["payload"] = messages[i].payload;
        msg["rssi"] = messages[i].rssi;
        msg["snr"] = messages[i].snr;
        msg["timestamp"] = messages[i].timestamp;
    }
    
    String json;
    serializeJson(doc, json);
    server->send(200, "application/json", json);
}

void WebInterface::handleApiClearLog() {
    lora->clearLog();
    server->send(200, "application/json", "{\"success\":true}");
}

void WebInterface::handleApiConfig() {
    JsonDocument doc;
    
    doc["lora_spreading"] = lora->getSpreadingFactor();
    doc["lora_bandwidth"] = lora->getBandwidth();
    doc["auto_publish"] = mqtt->isAutoPublishEnabled();
    
    String json;
    serializeJson(doc, json);
    server->send(200, "application/json", json);
}

void WebInterface::handleApiSaveConfig() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    JsonDocument doc;
    if (deserializeJson(doc, server->arg("plain"))) {
        server->send(400, "application/json", "{\"success\":false}");
        return;
    }
    
    if (doc.containsKey("lora_spreading")) {
        lora->setSpreadingFactor(doc["lora_spreading"]);
    }
    if (doc.containsKey("lora_bandwidth")) {
        lora->setBandwidth(doc["lora_bandwidth"]);
    }
    if (doc.containsKey("auto_publish")) {
        mqtt->setAutoPublish(doc["auto_publish"]);
    }
    
    lora->saveConfig();
    mqtt->saveConfig();
    
    server->send(200, "application/json", "{\"success\":true}");
}
