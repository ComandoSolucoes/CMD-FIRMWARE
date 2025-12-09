#include "WebInterface.h"
#include <webserver/CMDWebServer.h>
#include <ArduinoJson.h>

WebInterface::WebInterface(IOManager* ioManager, MqttHandler* mqttHandler)
    : io(ioManager), mqtt(mqttHandler), server(nullptr) {
}

void WebInterface::registerRoutes(CMDWebServer* webServer) {
    // Não podemos acessar o server diretamente, então vamos usar addRoute
    // A página /device será tratada como rota customizada
    
    LOG_INFO("Rotas da interface web registradas");
}

// ==================== PÁGINA HTML PRINCIPAL ====================

String WebInterface::generateDevicePage() {
    String html = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dispositivo - 4CH-ETH</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;color:#333}
.container{max-width:900px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:10px;font-size:28px}
.subtitle{color:rgba(255,255,255,0.9);text-align:center;margin-bottom:30px;font-size:16px}
.card{background:white;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 16px rgba(0,0,0,0.1)}
.card h2{color:#667eea;margin-bottom:15px;font-size:18px;border-bottom:2px solid #f0f0f0;padding-bottom:10px}
.relay-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(200px,1fr));gap:15px;margin:20px 0}
.relay-btn{padding:20px;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;transition:all 0.3s;display:flex;flex-direction:column;align-items:center;gap:8px}
.relay-btn.on{background:#4caf50;color:white;box-shadow:0 4px 12px rgba(76,175,80,0.4)}
.relay-btn.off{background:#f5f5f5;color:#666;border:2px solid #e0e0e0}
.relay-btn:hover{transform:translateY(-2px)}
.relay-btn:active{transform:translateY(0)}
.relay-icon{font-size:32px}
.input-status{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:10px;margin:15px 0}
.input-box{padding:12px;border-radius:8px;text-align:center;font-weight:600}
.input-box.high{background:#e8f5e9;color:#2e7d32;border:2px solid #4caf50}
.input-box.low{background:#fafafa;color:#666;border:2px solid #e0e0e0}
.form-group{margin:15px 0}
.form-group label{display:block;font-weight:600;margin-bottom:8px;color:#555}
.form-group select,.form-group input{width:100%;padding:10px;border:2px solid #e0e0e0;border-radius:8px;font-size:14px}
.form-group select:focus,.form-group input:focus{outline:none;border-color:#667eea}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:15px}
button.btn-primary{width:100%;padding:14px;background:#667eea;color:white;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
button.btn-primary:hover{background:#5568d3}
button.btn-secondary{width:100%;padding:14px;background:#ff9800;color:white;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
button.btn-secondary:hover{background:#f57c00}
.btn-back{width:100%;padding:14px;background:#e0e0e0;color:#333;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer}
.btn-back:hover{background:#d0d0d0}
.info-text{font-size:13px;color:#888;margin-top:5px}
.config-section{display:none}
.config-section.active{display:block}
.tabs{display:flex;gap:10px;margin-bottom:20px}
.tab{flex:1;padding:12px;background:#f5f5f5;border:none;border-radius:8px;font-weight:600;cursor:pointer;color:#666}
.tab.active{background:#667eea;color:white}
small{color:#888;font-size:12px}
@media(max-width:600px){.form-row{grid-template-columns:1fr}}
</style>
</head><body>
<div class="container">
<h1>🎛️ 4 Canais + Ethernet</h1>
<div class="subtitle">Controle e Configuração</div>

<div class="tabs">
  <button class="tab active" onclick="showTab('control')">🎮 Controle</button>
  <button class="tab" onclick="showTab('config')">⚙️ Configurações</button>
</div>

<!-- SEÇÃO DE CONTROLE -->
<div class="config-section active" id="controlSection">
  <div class="card">
    <h2>🔌 Controle dos Relés</h2>
    <div class="relay-grid" id="relayGrid"></div>
    <button class="btn-secondary" onclick="toggleAllRelays()">🔄 Alternar Todos</button>
  </div>
  
  <div class="card">
    <h2>📥 Estado das Entradas</h2>
    <div class="input-status" id="inputStatus"></div>
  </div>
</div>

<!-- SEÇÃO DE CONFIGURAÇÃO -->
<div class="config-section" id="configSection">
  <div class="card">
    <h2>⚡ Configurações de Saída</h2>
    
    <div class="form-group">
      <label>Lógica dos Relés:</label>
      <select id="relayLogic">
        <option value="0">Normal (HIGH = ON, LOW = OFF)</option>
        <option value="1">Invertida (HIGH = OFF, LOW = ON)</option>
      </select>
      <small>Define como os relés são acionados fisicamente</small>
    </div>
    
    <div class="form-group">
      <label>Estado Inicial na Inicialização:</label>
      <select id="initialState">
        <option value="0">Sempre Desligados</option>
        <option value="1">Sempre Ligados</option>
        <option value="2">Recuperar Último Estado</option>
      </select>
      <small>Estado dos relés quando o dispositivo reinicia</small>
    </div>
  </div>
  
  <div class="card">
    <h2>📥 Configurações de Entrada</h2>
    <div id="inputConfigs"></div>
  </div>
  
  <div class="card">
    <h2>📡 Configurações MQTT</h2>
    
    <div class="form-group">
      <label>Intervalo de Publicação Periódica:</label>
      <div class="form-row">
        <input type="number" id="publishInterval" min="1" max="300" value="30">
        <select id="publishUnit">
          <option value="1000">Segundos</option>
          <option value="60000">Minutos</option>
        </select>
      </div>
      <small>Frequência de publicação automática do status completo</small>
    </div>
  </div>
  
  <button class="btn-primary" onclick="saveConfig()">💾 Salvar Todas as Configurações</button>
</div>

<button class="btn-back" onclick="location.href='/'">← Voltar ao Menu</button>

<script>
let currentTab = 'control';
let updateInterval;

function showTab(tab) {
  currentTab = tab;
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.config-section').forEach(s => s.classList.remove('active'));
  
  if (tab === 'control') {
    document.querySelector('.tab:nth-child(1)').classList.add('active');
    document.getElementById('controlSection').classList.add('active');
    startAutoUpdate();
  } else {
    document.querySelector('.tab:nth-child(2)').classList.add('active');
    document.getElementById('configSection').classList.add('active');
    stopAutoUpdate();
  }
}

async function loadStatus() {
  try {
    const r = await fetch('/api/device/status');
    const data = await r.json();
    
    updateRelayGrid(data.relays);
    updateInputStatus(data.inputs);
  } catch (e) {
    console.error('Erro ao carregar status:', e);
  }
}

function updateRelayGrid(relays) {
  const grid = document.getElementById('relayGrid');
  grid.innerHTML = '';
  
  for (let i = 0; i < relays.length; i++) {
    const state = relays[i];
    const btn = document.createElement('button');
    btn.className = 'relay-btn ' + (state ? 'on' : 'off');
    btn.innerHTML = `
      <div class="relay-icon">${state ? '💡' : '⚫'}</div>
      <div>Relé ${i + 1}</div>
      <div style="font-size:12px">${state ? 'LIGADO' : 'DESLIGADO'}</div>
    `;
    btn.onclick = () => toggleRelay(i);
    grid.appendChild(btn);
  }
}

function updateInputStatus(inputs) {
  const status = document.getElementById('inputStatus');
  status.innerHTML = '';
  
  for (let i = 0; i < inputs.length; i++) {
    const state = inputs[i];
    const box = document.createElement('div');
    box.className = 'input-box ' + (state ? 'high' : 'low');
    box.innerHTML = `
      <div>Entrada ${i + 1}</div>
      <div style="font-size:18px;margin-top:5px">${state ? '⬆️ HIGH' : '⬇️ LOW'}</div>
    `;
    status.appendChild(box);
  }
}

async function toggleRelay(relay) {
  try {
    await fetch('/api/device/relay/' + relay, {method: 'POST'});
    loadStatus();
  } catch (e) {
    console.error('Erro ao controlar relé:', e);
  }
}

async function toggleAllRelays() {
  try {
    await fetch('/api/device/relay/all', {method: 'POST'});
    loadStatus();
  } catch (e) {
    console.error('Erro ao controlar relés:', e);
  }
}

async function loadConfig() {
  try {
    const r = await fetch('/api/device/config');
    const data = await r.json();
    
    document.getElementById('relayLogic').value = data.relay_logic;
    document.getElementById('initialState').value = data.initial_state;
    document.getElementById('publishInterval').value = data.publish_interval / 1000;
    
    generateInputConfigs(data.inputs);
  } catch (e) {
    console.error('Erro ao carregar config:', e);
  }
}

function generateInputConfigs(inputs) {
  const container = document.getElementById('inputConfigs');
  container.innerHTML = '';
  
  for (let i = 0; i < inputs.length; i++) {
    const input = inputs[i];
    const div = document.createElement('div');
    div.style.borderBottom = '1px solid #f0f0f0';
    div.style.paddingBottom = '15px';
    div.style.marginBottom = '15px';
    div.innerHTML = `
      <h3 style="color:#667eea;margin-bottom:10px">Entrada ${i + 1}</h3>
      <div class="form-group">
        <label>Modo de Operação:</label>
        <select id="inputMode${i}">
          <option value="0">Desabilitada</option>
          <option value="1">Transição (Toggle)</option>
          <option value="2">Pulso</option>
          <option value="3">Seguir (Follow)</option>
          <option value="4">Invertido</option>
        </select>
      </div>
      <div class="form-row">
        <div class="form-group">
          <label>Debounce (ms):</label>
          <input type="number" id="debounce${i}" min="10" max="1000" value="${input.debounce}">
        </div>
        <div class="form-group">
          <label>Tempo de Pulso (ms):</label>
          <input type="number" id="pulse${i}" min="50" max="10000" value="${input.pulse}">
        </div>
      </div>
    `;
    container.appendChild(div);
    
    document.getElementById('inputMode' + i).value = input.mode;
  }
}

async function saveConfig() {
  const config = {
    relay_logic: parseInt(document.getElementById('relayLogic').value),
    initial_state: parseInt(document.getElementById('initialState').value),
    publish_interval: parseInt(document.getElementById('publishInterval').value) * 
                     parseInt(document.getElementById('publishUnit').value),
    inputs: []
  };
  
  for (let i = 0; i < 4; i++) {
    config.inputs.push({
      mode: parseInt(document.getElementById('inputMode' + i).value),
      debounce: parseInt(document.getElementById('debounce' + i).value),
      pulse: parseInt(document.getElementById('pulse' + i).value)
    });
  }
  
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

function startAutoUpdate() {
  if (updateInterval) clearInterval(updateInterval);
  loadStatus();
  updateInterval = setInterval(loadStatus, 2000);
}

function stopAutoUpdate() {
  if (updateInterval) {
    clearInterval(updateInterval);
    updateInterval = null;
  }
}

window.onload = () => {
  loadStatus();
  loadConfig();
  startAutoUpdate();
};

window.onbeforeunload = () => {
  stopAutoUpdate();
};
</script>
</div></body></html>
)rawliteral";
    
    return html;
}

// ==================== HANDLERS DE ROTAS (SERÃO CONECTADAS NO .ino) ====================

// Estas funções serão chamadas via lambda no arquivo principal

String WebInterface::getInputModeName(InputMode mode) {
    switch (mode) {
        case INPUT_MODE_DISABLED: return "Desabilitada";
        case INPUT_MODE_TRANSITION: return "Transição";
        case INPUT_MODE_PULSE: return "Pulso";
        case INPUT_MODE_FOLLOW: return "Seguir";
        case INPUT_MODE_INVERTED: return "Invertido";
        default: return "Desconhecido";
    }
}

String WebInterface::getRelayLogicName(RelayLogic logic) {
    return (logic == RELAY_LOGIC_NORMAL) ? "Normal" : "Invertida";
}

String WebInterface::getInitialStateName(InitialState state) {
    switch (state) {
        case INITIAL_STATE_OFF: return "Desligados";
        case INITIAL_STATE_ON: return "Ligados";
        case INITIAL_STATE_LAST: return "Último Estado";
        default: return "Desconhecido";
    }
}