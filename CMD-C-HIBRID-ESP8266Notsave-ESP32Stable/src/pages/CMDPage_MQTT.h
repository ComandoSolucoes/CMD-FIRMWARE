#ifndef CMD_PAGE_MQTT_H
#define CMD_PAGE_MQTT_H

#include <pgmspace.h>

// ==================== PÁGINA MQTT FUNCIONAL ====================

const char HTML_MQTT_PAGE[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MQTT - CMD-C</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;color:#333}
.container{max-width:600px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:30px;font-size:28px}
.card{background:white;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 16px rgba(0,0,0,0.1)}
.card h2{color:#667eea;margin-bottom:15px;font-size:18px}
button{width:100%;padding:14px;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;margin:8px 0}
.btn-primary{background:#667eea;color:white}
.btn-primary:hover{background:#5568d3}
.btn-primary:disabled{background:#ccc;cursor:not-allowed}
.btn-secondary{background:#ff9800;color:white}
.btn-secondary:hover{background:#f57c00}
.btn-danger{background:#ff5252;color:white}
.btn-danger:hover{background:#ff1744}
.btn-back{background:#e0e0e0;color:#333}
.btn-back:hover{background:#d0d0d0}
input,select{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;margin:8px 0}
input:focus,select:focus{outline:none;border-color:#667eea}
label{display:block;margin-top:12px;font-weight:600;color:#555}
.info{background:#f0f4ff;padding:12px;border-radius:8px;margin:12px 0;font-size:14px;text-align:center}
.success{background:#e8f5e9;color:#2e7d32}
.warning{background:#fff3e0;color:#e65100}
.error{background:#ffebee;color:#c62828}
.status-badge{display:inline-block;padding:6px 12px;border-radius:20px;font-size:13px;font-weight:600;margin:8px 0}
.status-connected{background:#e8f5e9;color:#2e7d32}
.status-disconnected{background:#ffebee;color:#c62828}
.status-reconnecting{background:#fff3e0;color:#e65100}
.messages{max-height:300px;overflow-y:auto;background:#f5f5f5;padding:12px;border-radius:8px;margin-top:12px}
.message{background:white;padding:10px;margin:8px 0;border-radius:6px;border-left:4px solid #667eea;font-family:monospace;font-size:13px}
.message-topic{font-weight:600;color:#667eea;margin-bottom:4px}
.message-payload{color:#333;word-break:break-all}
.message-time{color:#888;font-size:11px;margin-top:4px}
.no-messages{text-align:center;color:#888;padding:20px;font-style:italic}
.topic-display{background:#f0f4ff;padding:12px;border-radius:8px;font-family:monospace;color:#667eea;font-weight:600;text-align:center;margin:12px 0;word-break:break-all}
small{color:#888;font-size:13px;display:block;margin-top:4px}
.grid{display:grid;grid-template-columns:2fr 1fr;gap:8px}
@media(max-width:500px){.grid{grid-template-columns:1fr}}
</style>
</head><body>
<div class="container">
<h1>📡 MQTT</h1>

<div class="card">
  <h2>Status da Conexão</h2>
  <div style="text-align:center">
    <span class="status-badge" id="statusBadge">Carregando...</span>
  </div>
  <div class="topic-display" id="baseTopic">cmd-c/XXXXXX</div>
  <small style="text-align:center">Este é o tópico raiz do seu dispositivo</small>
  
  <h2 style="margin-top:20px">📥 Monitor de Mensagens Recebidas</h2>
  <button class="btn-secondary" onclick="refreshMessages()">🔄 Atualizar</button>
  <div class="messages" id="messagesArea">
    <div class="no-messages">Nenhuma mensagem recebida ainda</div>
  </div>
</div>

<div class="card">
  <h2>Configuração do Broker</h2>
  <label>Broker MQTT:</label>
  <input type="text" id="broker" placeholder="broker.hivemq.com">
  
  <label>Porta:</label>
  <input type="number" id="port" value="1883">
  
  <label>Client ID (opcional):</label>
  <input type="text" id="clientId" placeholder="Deixe vazio para gerar automaticamente">
  <small>Se não preencher, será gerado automaticamente: cmd-c-{MAC}</small>
  
  <label>Usuário (opcional):</label>
  <input type="text" id="user" placeholder="username">
  
  <label>Senha (opcional):</label>
  <input type="password" id="pass" placeholder="password">
  
  <label>QoS (Quality of Service):</label>
  <select id="qos">
    <option value="0">0 - Fire and Forget</option>
    <option value="1">1 - At Least Once</option>
    <option value="2">2 - Exactly Once</option>
  </select>
  
  <button class="btn-primary" onclick="saveConfig()">💾 Salvar Configuração</button>
  <button class="btn-secondary" onclick="testConnection()">🔌 Testar Conexão</button>
  <button class="btn-danger" onclick="clearConfig()">🗑️ Limpar Configuração</button>
</div>

<div class="card">
  <h2>Publicar Mensagem (Teste)</h2>
  <label>Tópico (relativo ao base topic):</label>
  <input type="text" id="pubTopic" placeholder="test/message">
  <small>Será publicado em: <span id="fullTopicPreview">cmd-c/XXXXXX/test/message</span></small>
  
  <label>Payload:</label>
  <input type="text" id="pubPayload" placeholder="Hello MQTT!">
  
  <label>
    <input type="checkbox" id="retain" style="width:auto;display:inline;margin-right:8px">
    Retain (manter última mensagem)
  </label>
  
  <button class="btn-primary" onclick="publishMessage()">📤 Publicar</button>
</div>

<button class="btn-back" onclick="location.href='/'">← Voltar</button>

<script>
let baseTopic = '';
let updateInterval;

async function loadStatus() {
  try {
    const r = await fetch('/api/mqtt/status');
    const data = await r.json();
    
    baseTopic = data.base_topic || 'cmd-c/XXXXXX';
    document.getElementById('baseTopic').textContent = baseTopic;
    
    const badge = document.getElementById('statusBadge');
    if (data.connected) {
      badge.textContent = '✅ Conectado';
      badge.className = 'status-badge status-connected';
    } else if (data.configured) {
      badge.textContent = '🔄 ' + (data.status || 'Reconectando...');
      badge.className = 'status-badge status-reconnecting';
    } else {
      badge.textContent = '⚙️ Não configurado';
      badge.className = 'status-badge status-disconnected';
    }
  } catch (e) {
    console.error('Erro ao carregar status:', e);
  }
}

async function loadConfig() {
  try {
    const r = await fetch('/api/mqtt/config');
    const data = await r.json();
    
    if (data.configured) {
      document.getElementById('broker').value = data.broker || '';
      document.getElementById('port').value = data.port || 1883;
      document.getElementById('user').value = data.user || '';
      document.getElementById('clientId').value = data.client_id || '';
      document.getElementById('qos').value = data.qos || 0;
      // Não carrega senha por segurança
    }
  } catch (e) {
    console.error('Erro ao carregar config:', e);
  }
}

async function saveConfig() {
  const broker = document.getElementById('broker').value.trim();
  const port = parseInt(document.getElementById('port').value);
  const user = document.getElementById('user').value.trim();
  const pass = document.getElementById('pass').value.trim();
  let clientId = document.getElementById('clientId').value.trim();
  const qos = parseInt(document.getElementById('qos').value);
  
  if (!broker) {
    alert('⚠️ Digite o endereço do broker!');
    return;
  }
  
  // Se Client ID vazio, gera automaticamente baseado no MAC
  if (!clientId) {
    const mac = baseTopic.split('/')[1]; // Extrai MAC do base topic
    clientId = 'cmd-c-' + mac;
    document.getElementById('clientId').value = clientId;
  }
  
  try {
    const r = await fetch('/api/mqtt/config', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({broker, port, user, pass, client_id: clientId, qos})
    });
    
    const data = await r.json();
    if (data.success) {
      alert('✅ Configuração salva com sucesso!\nClient ID: ' + clientId + '\n\nConectando ao broker...');
      await testConnection();
    } else {
      alert('❌ Erro ao salvar configuração');
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function testConnection() {
  try {
    const r = await fetch('/api/mqtt/connect', {method: 'POST'});
    const data = await r.json();
    
    if (data.success) {
      alert('✅ Conectado ao broker MQTT!');
      loadStatus();
    } else {
      alert('❌ Falha ao conectar.\n\nVerifique:\n• Broker e porta corretos\n• Credenciais (se necessário)\n• Broker acessível na rede');
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function clearConfig() {
  if (!confirm('⚠️ Limpar configuração MQTT?\n\nO dispositivo será desconectado do broker.')) {
    return;
  }
  
  try {
    await fetch('/api/mqtt/config', {method: 'DELETE'});
    alert('✅ Configuração removida!');
    
    document.getElementById('broker').value = '';
    document.getElementById('port').value = '1883';
    document.getElementById('user').value = '';
    document.getElementById('pass').value = '';
    document.getElementById('clientId').value = '';
    document.getElementById('qos').value = '0';
    
    loadStatus();
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function publishMessage() {
  const topic = document.getElementById('pubTopic').value.trim();
  const payload = document.getElementById('pubPayload').value;
  const retain = document.getElementById('retain').checked;
  
  if (!topic) {
    alert('⚠️ Digite um tópico!');
    return;
  }
  
  try {
    const r = await fetch('/api/mqtt/publish', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({topic, payload, retain})
    });
    
    const data = await r.json();
    if (data.success) {
      alert('✅ Mensagem publicada!\n\nTópico: ' + baseTopic + '/' + topic + '\nPayload: ' + payload);
      document.getElementById('pubPayload').value = '';
    } else {
      alert('❌ Falha ao publicar.\n\nVerifique se está conectado ao broker.');
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function refreshMessages() {
  try {
    const r = await fetch('/api/mqtt/messages');
    const data = await r.json();
    
    const area = document.getElementById('messagesArea');
    
    if (data.count === 0) {
      area.innerHTML = '<div class="no-messages">Nenhuma mensagem recebida ainda</div>';
      return;
    }
    
    let html = '';
    // Inverte para mostrar mais recente primeiro
    for (let i = data.messages.length - 1; i >= 0; i--) {
      const msg = data.messages[i];
      const time = formatTime(msg.timestamp);
      html += `<div class="message">
        <div class="message-topic">${msg.topic}</div>
        <div class="message-payload">${msg.payload}</div>
        <div class="message-time">${time}</div>
      </div>`;
    }
    
    area.innerHTML = html;
  } catch (e) {
    console.error('Erro ao carregar mensagens:', e);
  }
}

function formatTime(millis) {
  const seconds = Math.floor((Date.now() - millis) / 1000);
  if (seconds < 60) return 'há ' + seconds + 's';
  if (seconds < 3600) return 'há ' + Math.floor(seconds / 60) + 'm';
  return 'há ' + Math.floor(seconds / 3600) + 'h';
}

function updateTopicPreview() {
  const topic = document.getElementById('pubTopic').value.trim();
  const preview = document.getElementById('fullTopicPreview');
  if (topic) {
    preview.textContent = baseTopic + '/' + topic;
  } else {
    preview.textContent = baseTopic + '/...';
  }
}

// Event listeners
document.getElementById('pubTopic').addEventListener('input', updateTopicPreview);

// Inicialização
window.onload = async () => {
  await loadStatus();
  await loadConfig();
  await refreshMessages();
  
  // Atualiza status e mensagens a cada 3 segundos
  updateInterval = setInterval(() => {
    loadStatus();
    refreshMessages();
  }, 3000);
};

// Limpa interval ao sair da página
window.onbeforeunload = () => {
  if (updateInterval) clearInterval(updateInterval);
};
</script>
</div></body></html>
)RAW";

#endif // CMD_PAGE_MQTT_H
