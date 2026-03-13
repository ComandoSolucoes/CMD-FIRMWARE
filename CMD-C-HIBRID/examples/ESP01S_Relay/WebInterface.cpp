#include "WebInterface.h"

WebInterface::WebInterface(RelayManager* relayManager, MqttHandler* mqttHandler)
    : relay(relayManager), mqtt(mqttHandler) {
}

String WebInterface::generateDevicePage() {
    String html = R"rawliteral(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ESP01S Relay - Controle</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;color:#333}
.container{max-width:800px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:10px;font-size:28px}
.subtitle{color:rgba(255,255,255,0.9);text-align:center;margin-bottom:30px;font-size:16px}
.card{background:white;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 16px rgba(0,0,0,0.1)}
.card h2{color:#667eea;margin-bottom:15px;font-size:18px;border-bottom:2px solid #f0f0f0;padding-bottom:10px}

/* Controle do Relé */
.relay-control{display:flex;flex-direction:column;align-items:center;gap:20px;padding:20px 0}
.relay-status{width:200px;height:200px;border-radius:50%;display:flex;align-items:center;justify-content:center;font-size:48px;transition:all 0.3s;cursor:pointer;user-select:none}
.relay-status.on{background:linear-gradient(135deg,#4caf50,#45a049);box-shadow:0 8px 24px rgba(76,175,80,0.5);animation:pulse 2s infinite}
.relay-status.off{background:#f5f5f5;color:#666;border:4px dashed #ccc}
.relay-status:hover{transform:scale(1.05)}
@keyframes pulse{0%,100%{box-shadow:0 8px 24px rgba(76,175,80,0.5)}50%{box-shadow:0 8px 32px rgba(76,175,80,0.7)}}
.relay-label{font-size:24px;font-weight:700;text-align:center}
.relay-label.on{color:#4caf50}
.relay-label.off{color:#999}

/* Botões de Ação */
.action-buttons{display:grid;grid-template-columns:1fr 1fr;gap:15px;width:100%}
.btn{padding:16px;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;transition:all 0.3s;display:flex;align-items:center;justify-content:center;gap:8px}
.btn-primary{background:#667eea;color:white}
.btn-primary:hover{background:#5568d3;transform:translateY(-2px)}
.btn-secondary{background:#ff9800;color:white}
.btn-secondary:hover{background:#f57c00;transform:translateY(-2px)}
.btn-danger{background:#f44336;color:white}
.btn-danger:hover{background:#d32f2f;transform:translateY(-2px)}
.btn-full{grid-column:1 / -1}

/* Estatísticas */
.stats-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:15px}
.stat-box{padding:20px;background:linear-gradient(135deg,#f5f5f5,#e8e8e8);border-radius:8px;text-align:center}
.stat-value{font-size:32px;font-weight:700;color:#667eea;margin-bottom:5px}
.stat-label{font-size:14px;color:#666;text-transform:uppercase}

/* Formulários */
.form-group{margin:15px 0}
.form-group label{display:block;font-weight:600;margin-bottom:8px;color:#555}
.form-group select,.form-group input{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;font-size:14px}
.form-group select:focus,.form-group input:focus{outline:none;border-color:#667eea}
.form-row{display:grid;grid-template-columns:1fr 1fr;gap:15px}
small{color:#888;font-size:12px;display:block;margin-top:5px}

/* Tabs */
.tabs{display:flex;gap:10px;margin-bottom:20px}
.tab{flex:1;padding:12px;background:#f5f5f5;border:none;border-radius:8px;font-weight:600;cursor:pointer;color:#666;transition:all 0.3s}
.tab.active{background:#667eea;color:white}
.tab:hover{transform:translateY(-2px)}
.config-section{display:none}
.config-section.active{display:block}

/* Timer Info */
.timer-info{background:#fff3cd;border-left:4px solid #ffc107;padding:12px;border-radius:4px;margin:15px 0}
.timer-info.active{background:#d4edda;border-left-color:#28a745}

/* Botão Voltar */
.btn-back{width:100%;padding:14px;background:#e0e0e0;color:#333;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin-top:10px}
.btn-back:hover{background:#d0d0d0}

@media(max-width:600px){
  .form-row,.action-buttons{grid-template-columns:1fr}
  .relay-status{width:160px;height:160px;font-size:40px}
  .relay-label{font-size:20px}
}
</style>
</head><body>
<div class="container">
<h1>⚡ ESP01S Relay</h1>
<div class="subtitle">Controle e Configuração</div>

<div class="tabs">
  <button class="tab active" onclick="showTab('control')">🎮 Controle</button>
  <button class="tab" onclick="showTab('stats')">📊 Estatísticas</button>
  <button class="tab" onclick="showTab('config')">⚙️ Config</button>
</div>

<!-- CONTROLE -->
<div class="config-section active" id="controlSection">
  <div class="card">
    <h2>🔌 Controle do Relé</h2>
    <div class="relay-control">
      <div class="relay-status" id="relayStatus" onclick="toggleRelay()">💡</div>
      <div class="relay-label" id="relayLabel">CARREGANDO...</div>
    </div>
    <div class="action-buttons">
      <button class="btn btn-primary" onclick="setRelay(true)">🟢 Ligar</button>
      <button class="btn btn-danger" onclick="setRelay(false)">🔴 Desligar</button>
      <button class="btn btn-secondary btn-full" onclick="toggleRelay()">🔄 Alternar</button>
      <button class="btn btn-secondary" onclick="showPulseDialog()">⚡ Pulso</button>
      <button class="btn btn-secondary" onclick="showTimerDialog()">⏱️ Timer</button>
    </div>
    <div id="timerInfo" class="timer-info" style="display:none"></div>
  </div>
</div>

<!-- ESTATÍSTICAS -->
<div class="config-section" id="statsSection">
  <div class="card">
    <h2>📊 Estatísticas de Uso</h2>
    <div class="stats-grid">
      <div class="stat-box">
        <div class="stat-value" id="statTotalOn">0</div>
        <div class="stat-label">Tempo Ligado (h)</div>
      </div>
      <div class="stat-box">
        <div class="stat-value" id="statSwitchCount">0</div>
        <div class="stat-label">Acionamentos</div>
      </div>
      <div class="stat-box">
        <div class="stat-value" id="statUptime">0</div>
        <div class="stat-label">Uptime (h)</div>
      </div>
      <div class="stat-box">
        <div class="stat-value" id="statFreeHeap">0</div>
        <div class="stat-label">RAM Livre (KB)</div>
      </div>
    </div>
    <button class="btn btn-danger btn-full" onclick="resetStats()" style="margin-top:20px">🗑️ Resetar Estatísticas</button>
  </div>
</div>

<!-- CONFIGURAÇÕES -->
<div class="config-section" id="configSection">
  <div class="card">
    <h2>⚙️ Configurações do Relé</h2>
    
    <div class="form-group">
      <label>Pino GPIO:</label>
      <select id="relayPin">
        <option value="0">GPIO 0 (padrão ESP01S)</option>
        <option value="2">GPIO 2</option>
        <option value="4">GPIO 4 (D2)</option>
        <option value="5">GPIO 5 (D1)</option>
        <option value="12">GPIO 12 (D6)</option>
        <option value="13">GPIO 13 (D7)</option>
        <option value="14">GPIO 14 (D5)</option>
        <option value="15">GPIO 15 (D8)</option>
        <option value="16">GPIO 16 (D0)</option>
      </select>
      <small>Selecione o pino do ESP8266 conectado ao relé</small>
    </div>
    
    <div class="form-group">
      <label>Lógica do Relé:</label>
      <select id="relayLogic">
        <option value="0">Normal (HIGH = ON, LOW = OFF)</option>
        <option value="1">Invertida (HIGH = OFF, LOW = ON)</option>
      </select>
      <small>Defina como o relé é acionado fisicamente</small>
    </div>
    
    <div class="form-group">
      <label>Estado Inicial:</label>
      <select id="initialState">
        <option value="0">Sempre Desligado</option>
        <option value="1">Sempre Ligado</option>
        <option value="2">Recuperar Último Estado</option>
      </select>
      <small>Estado do relé ao ligar o dispositivo</small>
    </div>
    
    <div class="form-group">
      <label>Tempo Padrão de Pulso (ms):</label>
      <input type="number" id="pulseMs" min="100" max="60000" value="1000">
      <small>Duração do pulso quando ativado (100-60000ms)</small>
    </div>
    
    <div class="form-group">
      <label>Tempo Padrão de Timer (segundos):</label>
      <input type="number" id="timerSeconds" min="1" max="86400" value="300">
      <small>Duração do temporizador (1-86400s = 24h)</small>
    </div>
  </div>
  
  <div class="card">
    <h2>📡 MQTT</h2>
    <div class="form-group">
      <label>Intervalo de Publicação:</label>
      <div class="form-row">
        <input type="number" id="publishInterval" min="1" max="300" value="30">
        <select id="publishUnit">
          <option value="1000">Segundos</option>
          <option value="60000">Minutos</option>
        </select>
      </div>
      <small>Frequência de publicação automática</small>
    </div>
  </div>
  
  <button class="btn btn-primary btn-full" onclick="saveConfig()">💾 Salvar Configurações</button>
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
  } else if (tab === 'stats') {
    document.querySelector('.tab:nth-child(2)').classList.add('active');
    document.getElementById('statsSection').classList.add('active');
    startAutoUpdate();
  } else if (tab === 'config') {
    document.querySelector('.tab:nth-child(3)').classList.add('active');
    document.getElementById('configSection').classList.add('active');
    stopAutoUpdate();
    loadConfig();
  }
}

function startAutoUpdate() {
  stopAutoUpdate();
  updateStatus();
  updateInterval = setInterval(updateStatus, 2000);
}

function stopAutoUpdate() {
  if (updateInterval) {
    clearInterval(updateInterval);
    updateInterval = null;
  }
}

async function updateStatus() {
  try {
    const response = await fetch('/api/device/status');
    const data = await response.json();
    
    const relayState = data.relay === 1;
    const statusEl = document.getElementById('relayStatus');
    const labelEl = document.getElementById('relayLabel');
    
    statusEl.className = 'relay-status ' + (relayState ? 'on' : 'off');
    labelEl.className = 'relay-label ' + (relayState ? 'on' : 'off');
    labelEl.textContent = relayState ? 'LIGADO' : 'DESLIGADO';
    
    if (data.timer_active) {
      const timerInfo = document.getElementById('timerInfo');
      timerInfo.style.display = 'block';
      timerInfo.className = 'timer-info active';
      timerInfo.innerHTML = '⏱️ Timer ativo: ' + data.timer_remaining + 's restantes';
    } else {
      document.getElementById('timerInfo').style.display = 'none';
    }
    
    if (currentTab === 'stats') {
      document.getElementById('statTotalOn').textContent = (data.total_on_time / 3600).toFixed(1);
      document.getElementById('statSwitchCount').textContent = data.switch_count;
      document.getElementById('statUptime').textContent = (data.uptime / 3600).toFixed(1);
      document.getElementById('statFreeHeap').textContent = (data.free_heap / 1024).toFixed(1);
    }
  } catch (err) {
    console.error('Erro ao atualizar status:', err);
  }
}

async function setRelay(state) {
  try {
    await fetch('/api/device/relay', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({state: state})
    });
    updateStatus();
  } catch (err) {
    alert('Erro ao controlar relé');
  }
}

async function toggleRelay() {
  try {
    await fetch('/api/device/relay/toggle', {method: 'POST'});
    updateStatus();
  } catch (err) {
    alert('Erro ao alternar relé');
  }
}

function showPulseDialog() {
  const duration = prompt('Duração do pulso em milissegundos (100-60000):', '1000');
  if (duration && !isNaN(duration)) {
    activatePulse(parseInt(duration));
  }
}

async function activatePulse(duration) {
  try {
    await fetch('/api/device/pulse', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({duration: duration})
    });
    updateStatus();
  } catch (err) {
    alert('Erro ao ativar pulso');
  }
}

function showTimerDialog() {
  const duration = prompt('Duração do timer em segundos (1-86400):', '300');
  if (duration && !isNaN(duration)) {
    startTimer(parseInt(duration));
  }
}

async function startTimer(seconds) {
  try {
    await fetch('/api/device/timer', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({seconds: seconds})
    });
    updateStatus();
  } catch (err) {
    alert('Erro ao iniciar timer');
  }
}

async function loadConfig() {
  try {
    const response = await fetch('/api/device/config');
    const data = await response.json();
    
    document.getElementById('relayPin').value = data.relay_pin;
    document.getElementById('relayLogic').value = data.relay_logic;
    document.getElementById('initialState').value = data.initial_state;
    document.getElementById('pulseMs').value = data.pulse_ms;
    document.getElementById('timerSeconds').value = data.timer_seconds;
    
    const pubInterval = data.publish_interval;
    if (pubInterval >= 60000) {
      document.getElementById('publishInterval').value = pubInterval / 60000;
      document.getElementById('publishUnit').value = '60000';
    } else {
      document.getElementById('publishInterval').value = pubInterval / 1000;
      document.getElementById('publishUnit').value = '1000';
    }
  } catch (err) {
    console.error('Erro ao carregar config:', err);
  }
}

async function saveConfig() {
  const config = {
    relay_pin: parseInt(document.getElementById('relayPin').value),
    relay_logic: parseInt(document.getElementById('relayLogic').value),
    initial_state: parseInt(document.getElementById('initialState').value),
    pulse_ms: parseInt(document.getElementById('pulseMs').value),
    timer_seconds: parseInt(document.getElementById('timerSeconds').value),
    publish_interval: parseInt(document.getElementById('publishInterval').value) * 
                     parseInt(document.getElementById('publishUnit').value)
  };
  
  try {
    await fetch('/api/device/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify(config)
    });
    alert('✅ Configurações salvas! O dispositivo será reiniciado.');
    setTimeout(() => location.reload(), 2000);
  } catch (err) {
    alert('❌ Erro ao salvar configurações');
  }
}

async function resetStats() {
  if (confirm('Deseja resetar todas as estatísticas?')) {
    try {
      await fetch('/api/device/stats/reset', {method: 'POST'});
      alert('✅ Estatísticas resetadas!');
      updateStatus();
    } catch (err) {
      alert('❌ Erro ao resetar estatísticas');
    }
  }
}

startAutoUpdate();
</script>
</body></html>
)rawliteral";
    
    return html;
}

String WebInterface::getRelayLogicName(RelayLogic logic) {
    return logic == RELAY_LOGIC_NORMAL ? "Normal" : "Invertida";
}

String WebInterface::getInitialStateName(InitialState state) {
    switch (state) {
        case INITIAL_STATE_OFF: return "Desligado";
        case INITIAL_STATE_ON: return "Ligado";
        case INITIAL_STATE_LAST: return "Último Estado";
        default: return "Desconhecido";
    }
}

String WebInterface::getOperationModeName(OperationMode mode) {
    switch (mode) {
        case MODE_NORMAL: return "Normal";
        case MODE_TIMER: return "Temporizador";
        case MODE_PULSE: return "Pulso";
        case MODE_SCHEDULE: return "Agendamento";
        default: return "Desconhecido";
    }
}
