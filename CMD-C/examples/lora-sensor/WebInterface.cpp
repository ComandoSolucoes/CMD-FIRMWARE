#include "WebInterface.h"
#include <webserver/CMDWebServer.h>
#include <ArduinoJson.h>

WebInterface::WebInterface(SensorManager* sensorManager, LoRaHandler* loraHandler, MqttHandler* mqttHandler)
    : sensor(sensorManager), lora(loraHandler), mqtt(mqttHandler), server(nullptr) {
}

void WebInterface::registerRoutes(CMDWebServer* webServer) {
    server = &webServer->getServer();
    
    // ✅ NOVO: Usa método específico que já inclui autenticação
    webServer->registerDeviceRoute([this]() { handleDevicePage(); });
    
    // APIs REST
    server->on("/api/device/status", HTTP_GET, [this]() { handleApiStatus(); });
    server->on("/api/device/config", HTTP_GET, [this]() { handleApiConfig(); });
    server->on("/api/device/config", HTTP_POST, [this]() { handleApiSaveConfig(); });
    server->on("/api/device/trigger", HTTP_POST, [this]() { handleApiTriggerReading(); });
    server->on("/api/device/reset-stats", HTTP_POST, [this]() { handleApiResetStats(); });
    
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
<title>LoRa Sensor - Dashboard</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;color:#333}
.container{max-width:900px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:10px;font-size:28px}
.subtitle{color:rgba(255,255,255,0.9);text-align:center;margin-bottom:30px;font-size:16px}
.card{background:white;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 16px rgba(0,0,0,0.1)}
.card h2{color:#667eea;margin-bottom:15px;font-size:18px;border-bottom:2px solid #f0f0f0;padding-bottom:10px}
.sensor-display{text-align:center;padding:30px;background:linear-gradient(135deg,#667eea,#764ba2);border-radius:12px;color:white;margin:20px 0}
.distance-value{font-size:64px;font-weight:bold;margin:10px 0}
.distance-unit{font-size:24px;opacity:0.9}
.status-indicator{display:inline-block;width:12px;height:12px;border-radius:50%;margin-right:8px}
.status-indicator.ok{background:#4caf50}
.status-indicator.error{background:#f44336}
.status-indicator.warning{background:#ff9800}
.stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px;margin:20px 0}
.stat-box{background:#f5f5f5;padding:15px;border-radius:8px;text-align:center}
.stat-value{font-size:28px;font-weight:bold;color:#667eea;margin:5px 0}
.stat-label{font-size:13px;color:#666}
.form-group{margin:15px 0}
.form-group label{display:block;font-weight:600;margin-bottom:8px;color:#555}
.form-group input,.form-group select{width:100%;padding:10px;border:2px solid #e0e0e0;border-radius:8px;font-size:14px}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:15px}
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
small{color:#888;font-size:12px}
.info-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid #f0f0f0}
.info-row:last-child{border-bottom:none}
.info-label{font-weight:600;color:#666}
.info-value{color:#333}
@media(max-width:600px){.form-row,.stats-grid{grid-template-columns:1fr}.distance-value{font-size:48px}}
</style>
</head><body>
<div class="container">
<h1>📡 LoRa Sensor</h1>
<div class="subtitle">Monitor de Distância Ultrassônico</div>

<div class="tabs">
  <button class="tab active" onclick="showTab('monitor')">📊 Monitor</button>
  <button class="tab" onclick="showTab('config')">⚙️ Configurações</button>
  <button class="tab" onclick="showTab('info')">ℹ️ Info</button>
</div>

<!-- SEÇÃO MONITOR -->
<div class="config-section active" id="monitorSection">
  <div class="card">
    <h2>📏 Leitura Atual</h2>
    <div class="sensor-display" id="sensorDisplay">
      <div style="opacity:0.8;font-size:14px">Distância Medida</div>
      <div class="distance-value" id="distanceValue">---</div>
      <div class="distance-unit">centímetros</div>
      <div style="margin-top:15px;font-size:14px;opacity:0.8" id="lastUpdate">Aguardando leitura...</div>
    </div>
    <button class="btn btn-secondary" onclick="triggerReading()">🔄 Ler Agora</button>
  </div>
  
  <div class="card">
    <h2>📈 Estatísticas</h2>
    <div class="stats-grid">
      <div class="stat-box">
        <div class="stat-label">Total de Leituras</div>
        <div class="stat-value" id="statTotal">0</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">Leituras Válidas</div>
        <div class="stat-value" id="statValid">0</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">Taxa de Sucesso</div>
        <div class="stat-value" id="statSuccess">0%</div>
      </div>
      <div class="stat-box">
        <div class="stat-label">Pacotes LoRa</div>
        <div class="stat-value" id="statLora">0</div>
      </div>
    </div>
    <button class="btn btn-danger" onclick="resetStats()">🗑️ Resetar Estatísticas</button>
  </div>
</div>

<!-- SEÇÃO CONFIGURAÇÕES -->
<div class="config-section" id="configSection">
  <div class="card">
    <h2>🌡️ Sensor</h2>
    <div class="form-group">
      <label>Temperatura Ambiente (°C):</label>
      <input type="number" id="tempC" min="-10" max="50" step="0.5" value="25">
      <small>Para compensação da velocidade do som</small>
    </div>
    <div class="form-group">
      <label>Intervalo de Leitura (segundos):</label>
      <input type="number" id="readInterval" min="1" max="300" value="30">
      <small>Tempo entre medições automáticas</small>
    </div>
    <div class="form-group">
      <label>LED de Feedback:</label>
      <select id="ledEnabled">
        <option value="true">Habilitado</option>
        <option value="false">Desabilitado</option>
      </select>
    </div>
  </div>
  
  <div class="card">
    <h2>📡 LoRa</h2>
    <div class="form-row">
      <div class="form-group">
        <label>TX Power (dBm):</label>
        <input type="number" id="loraTxPower" min="2" max="20" value="17">
      </div>
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
    </div>
    <small>SF maior = maior alcance, menor taxa de dados</small>
  </div>
  
  <div class="card">
    <h2>🌐 MQTT</h2>
    <div class="form-group">
      <label>Intervalo de Publicação (segundos):</label>
      <input type="number" id="mqttInterval" min="5" max="300" value="30">
      <small>Tempo entre publicações automáticas no broker MQTT</small>
    </div>
  </div>
  
  <button class="btn btn-primary" onclick="saveConfig()">💾 Salvar Configurações</button>
</div>

<!-- SEÇÃO INFO -->
<div class="config-section" id="infoSection">
  <div class="card">
    <h2>ℹ️ Informações do Sistema</h2>
    <div id="systemInfo"></div>
  </div>
</div>

<button class="btn btn-back" onclick="location.href='/'">← Voltar ao Menu</button>

<script>
let currentTab = 'monitor';
let updateInterval;

function showTab(tab) {
  currentTab = tab;
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.config-section').forEach(s => s.classList.remove('active'));
  
  if (tab === 'monitor') {
    document.querySelector('.tab:nth-child(1)').classList.add('active');
    document.getElementById('monitorSection').classList.add('active');
    startAutoUpdate();
  } else if (tab === 'config') {
    document.querySelector('.tab:nth-child(2)').classList.add('active');
    document.getElementById('configSection').classList.add('active');
    stopAutoUpdate();
    loadConfig();
  } else if (tab === 'info') {
    document.querySelector('.tab:nth-child(3)').classList.add('active');
    document.getElementById('infoSection').classList.add('active');
    stopAutoUpdate();
    loadSystemInfo();
  }
}

async function loadStatus() {
  try {
    const r = await fetch('/api/device/status');
    const data = await r.json();
    updateDisplay(data);
  } catch (e) {
    console.error('Erro ao carregar status:', e);
  }
}

function updateDisplay(data) {
  const sensor = data.sensor || {};
  const lora = data.lora || {};
  
  // Distância
  if (sensor.distance_cm !== null && sensor.distance_cm !== undefined) {
    document.getElementById('distanceValue').textContent = sensor.distance_cm.toFixed(1);
    document.getElementById('sensorDisplay').style.background = 'linear-gradient(135deg,#4caf50,#45a049)';
  } else {
    document.getElementById('distanceValue').textContent = 'Sem Eco';
    document.getElementById('sensorDisplay').style.background = 'linear-gradient(135deg,#f44336,#d32f2f)';
  }
  
  // Timestamp
  if (sensor.timestamp) {
    const sec = Math.floor((Date.now() - sensor.timestamp) / 1000);
    document.getElementById('lastUpdate').textContent = 'Última leitura: há ' + sec + 's';
  }
  
  // Estatísticas
  if (sensor.stats) {
    document.getElementById('statTotal').textContent = sensor.stats.total || 0;
    document.getElementById('statValid').textContent = sensor.stats.valid || 0;
    document.getElementById('statSuccess').textContent = (sensor.stats.success_rate || 0).toFixed(1) + '%';
  }
  
  document.getElementById('statLora').textContent = lora.packets_sent || 0;
}

async function loadConfig() {
  try {
    const r = await fetch('/api/device/config');
    const data = await r.json();
    
    document.getElementById('tempC').value = data.temp_c || 25;
    document.getElementById('readInterval').value = (data.read_interval_ms || 30000) / 1000;
    document.getElementById('ledEnabled').value = data.led_enabled ? 'true' : 'false';
    document.getElementById('loraTxPower').value = data.lora_tx_power || 17;
    document.getElementById('loraSpreading').value = data.lora_spreading || 7;
    document.getElementById('mqttInterval').value = (data.mqtt_interval_ms || 30000) / 1000;
  } catch (e) {
    console.error('Erro ao carregar config:', e);
  }
}

async function saveConfig() {
  const config = {
    temp_c: parseFloat(document.getElementById('tempC').value),
    read_interval_ms: parseInt(document.getElementById('readInterval').value) * 1000,
    led_enabled: document.getElementById('ledEnabled').value === 'true',
    lora_tx_power: parseInt(document.getElementById('loraTxPower').value),
    lora_spreading: parseInt(document.getElementById('loraSpreading').value),
    mqtt_interval_ms: parseInt(document.getElementById('mqttInterval').value) * 1000
  };
  
  try {
    const r = await fetch('/api/device/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(config)
    });
    const data = await r.json();
    if (data.success) {
      alert('✅ Configurações salvas com sucesso!');
    } else {
      alert('❌ Erro ao salvar configurações');
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function triggerReading() {
  try {
    await fetch('/api/device/trigger', {method: 'POST'});
    setTimeout(loadStatus, 1000);
  } catch (e) {
    console.error('Erro:', e);
  }
}

async function resetStats() {
  if (!confirm('Resetar todas as estatísticas?')) return;
  try {
    await fetch('/api/device/reset-stats', {method: 'POST'});
    loadStatus();
  } catch (e) {
    console.error('Erro:', e);
  }
}

async function loadSystemInfo() {
  try {
    const r = await fetch('/api/device/status');
    const data = await r.json();
    
    let html = '';
    html += '<div class="info-row"><span class="info-label">Modelo:</span><span class="info-value">' + (data.device || 'N/A') + '</span></div>';
    html += '<div class="info-row"><span class="info-label">Versão:</span><span class="info-value">' + (data.version || 'N/A') + '</span></div>';
    html += '<div class="info-row"><span class="info-label">Uptime:</span><span class="info-value">' + formatUptime(data.uptime || 0) + '</span></div>';
    
    if (data.lora) {
      html += '<div class="info-row"><span class="info-label">LoRa Status:</span><span class="info-value">' + (data.lora.initialized ? '✅ Inicializado' : '❌ Não inicializado') + '</span></div>';
      html += '<div class="info-row"><span class="info-label">LoRa TX Power:</span><span class="info-value">' + data.lora.tx_power + ' dBm</span></div>';
      html += '<div class="info-row"><span class="info-label">Spreading Factor:</span><span class="info-value">SF' + data.lora.spreading + '</span></div>';
      html += '<div class="info-row"><span class="info-label">Taxa LoRa:</span><span class="info-value">' + (data.lora.success_rate || 0).toFixed(1) + '%</span></div>';
    }
    
    document.getElementById('systemInfo').innerHTML = html;
  } catch (e) {
    console.error('Erro:', e);
  }
}

function formatUptime(seconds) {
  const d = Math.floor(seconds / 86400);
  const h = Math.floor((seconds % 86400) / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  
  let str = '';
  if (d > 0) str += d + 'd ';
  if (h > 0) str += h + 'h ';
  if (m > 0) str += m + 'm ';
  str += s + 's';
  return str;
}

function startAutoUpdate() {
  if (updateInterval) clearInterval(updateInterval);
  loadStatus();
  updateInterval = setInterval(loadStatus, 3000);
}

function stopAutoUpdate() {
  if (updateInterval) {
    clearInterval(updateInterval);
    updateInterval = null;
  }
}

window.onload = () => {
  loadStatus();
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
    
    // Info do dispositivo
    doc["device"] = DEVICE_MODEL;
    doc["version"] = FIRMWARE_VERSION;
    doc["uptime"] = millis() / 1000;
    
    // Sensor
    SensorReading reading = sensor->getLastReading();
    if (reading.valid) {
        doc["sensor"]["distance_cm"] = reading.distance_cm;
    } else {
        doc["sensor"]["distance_cm"] = nullptr;
    }
    doc["sensor"]["temperature"] = reading.temperature;
    doc["sensor"]["timestamp"] = reading.timestamp;
    doc["sensor"]["interval_ms"] = sensor->getReadInterval();
    doc["sensor"]["stats"]["total"] = sensor->getTotalReadings();
    doc["sensor"]["stats"]["valid"] = sensor->getValidReadings();
    doc["sensor"]["stats"]["success_rate"] = sensor->getSuccessRate();
    
    // LoRa
    doc["lora"]["initialized"] = lora->isInitialized();
    doc["lora"]["tx_power"] = lora->getTxPower();
    doc["lora"]["spreading"] = lora->getSpreadingFactor();
    doc["lora"]["packets_sent"] = lora->getTotalPackets();
    doc["lora"]["success_rate"] = lora->getSuccessRate();
    
    String json;
    serializeJson(doc, json);
    server->send(200, "application/json", json);
}

void WebInterface::handleApiConfig() {
    JsonDocument doc;
    
    doc["temp_c"] = sensor->getAmbientTemperature();
    doc["read_interval_ms"] = sensor->getReadInterval();
    doc["led_enabled"] = sensor->isLedEnabled();
    doc["lora_tx_power"] = lora->getTxPower();
    doc["lora_spreading"] = lora->getSpreadingFactor();
    doc["mqtt_interval_ms"] = mqtt->getPublishInterval();
    
    String json;
    serializeJson(doc, json);
    server->send(200, "application/json", json);
}

void WebInterface::handleApiSaveConfig() {
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"No JSON\"}");
        return;
    }
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, server->arg("plain"));
    
    if (error) {
        server->send(400, "application/json", "{\"success\":false,\"error\":\"Parse error\"}");
        return;
    }
    
    // Aplica configurações
    if (doc.containsKey("temp_c")) {
        sensor->setAmbientTemperature(doc["temp_c"]);
    }
    if (doc.containsKey("read_interval_ms")) {
        sensor->setReadInterval(doc["read_interval_ms"]);
    }
    if (doc.containsKey("led_enabled")) {
        sensor->setLedEnabled(doc["led_enabled"]);
    }
    if (doc.containsKey("lora_tx_power")) {
        lora->setTxPower(doc["lora_tx_power"]);
    }
    if (doc.containsKey("lora_spreading")) {
        lora->setSpreadingFactor(doc["lora_spreading"]);
    }
    if (doc.containsKey("mqtt_interval_ms")) {
        mqtt->setPublishInterval(doc["mqtt_interval_ms"]);
    }
    
    // Salva configurações
    sensor->saveConfig();
    lora->saveConfig();
    mqtt->savePublishInterval();
    
    server->send(200, "application/json", "{\"success\":true}");
}

void WebInterface::handleApiTriggerReading() {
    sensor->triggerReading();
    server->send(200, "application/json", "{\"success\":true}");
}

void WebInterface::handleApiResetStats() {
    sensor->resetStatistics();
    lora->resetStatistics();
    server->send(200, "application/json", "{\"success\":true}");
}