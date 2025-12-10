#ifndef CMD_PAGE_INFO_H
#define CMD_PAGE_INFO_H

#include <pgmspace.h>

// ==================== PÁGINA DE INFORMAÇÕES ====================

const char HTML_INFO_PAGE[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Informações - CMD-C</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;padding:20px;color:#333}
.container{max-width:600px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:30px;font-size:28px}
.card{background:white;border-radius:12px;padding:20px;margin-bottom:20px;box-shadow:0 4px 16px rgba(0,0,0,0.1)}
.card h2{color:#667eea;margin-bottom:15px;font-size:18px}
.info-row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #f0f0f0}
.info-row:last-child{border-bottom:none}
.label{font-weight:600;color:#555}
.value{color:#333;text-align:right;word-break:break-all;max-width:60%}
.btn-back{width:100%;padding:14px;background:#e0e0e0;border:none;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;color:#333}
.btn-back:hover{background:#d0d0d0}
.status-badge{display:inline-block;padding:4px 10px;border-radius:12px;font-size:12px;font-weight:600}
.badge-connected{background:#e8f5e9;color:#2e7d32}
.badge-disconnected{background:#ffebee;color:#c62828}
.badge-not-configured{background:#f0f0f0;color:#666}
</style>
</head><body>
<div class="container">
<h1>ℹ️ Informações do Sistema</h1>

<div class="card">
  <h2>Hardware</h2>
  <div class="info-row"><span class="label">Chip:</span><span class="value" id="chip">-</span></div>
  <div class="info-row"><span class="label">Cores:</span><span class="value" id="cores">-</span></div>
  <div class="info-row"><span class="label">CPU:</span><span class="value" id="cpu">-</span></div>
  <div class="info-row"><span class="label">RAM Livre:</span><span class="value" id="ram">-</span></div>
  <div class="info-row"><span class="label">Uptime:</span><span class="value" id="uptime">-</span></div>
</div>

<div class="card">
  <h2>Rede</h2>
  <div class="info-row"><span class="label">SSID:</span><span class="value" id="ssid">-</span></div>
  <div class="info-row"><span class="label">IP Local:</span><span class="value" id="ip">-</span></div>
  <div class="info-row"><span class="label">MAC:</span><span class="value" id="mac">-</span></div>
  <div class="info-row"><span class="label">RSSI:</span><span class="value" id="rssi">-</span></div>
</div>

<div class="card">
  <h2>MQTT</h2>
  <div class="info-row">
    <span class="label">Status:</span>
    <span class="value" id="mqttStatus">-</span>
  </div>
  <div class="info-row" id="brokerRow" style="display:none">
    <span class="label">Broker:</span>
    <span class="value" id="mqttBroker">-</span>
  </div>
  <div class="info-row" id="portRow" style="display:none">
    <span class="label">Porta:</span>
    <span class="value" id="mqttPort">-</span>
  </div>
  <div class="info-row" id="clientRow" style="display:none">
    <span class="label">Client ID:</span>
    <span class="value" id="mqttClient">-</span>
  </div>
  <div class="info-row" id="topicRow" style="display:none">
    <span class="label">Base Topic:</span>
    <span class="value" id="mqttTopic">-</span>
  </div>
</div>

<button class="btn-back" onclick="location.href='/'">← Voltar</button>

<script>
Promise.all([
  fetch('/api/info').then(r=>r.json()),
  fetch('/api/wifi').then(r=>r.json()),
  fetch('/api/mqtt/status').then(r=>r.json()),
  fetch('/api/mqtt/config').then(r=>r.json()).catch(()=>({configured:false}))
]).then(([info,wifi,mqttStatus,mqttConfig])=>{
  // Hardware
  document.getElementById('chip').textContent=info.chip_model||'-';
  document.getElementById('cores').textContent='2';
  document.getElementById('cpu').textContent=info.cpu_freq+' MHz';
  document.getElementById('ram').textContent=(info.free_heap/1024).toFixed(1)+' KB';
  document.getElementById('uptime').textContent=formatUptime(info.uptime);
  
  // Rede
  document.getElementById('ssid').textContent=wifi.ssid||'-';
  document.getElementById('ip').textContent=wifi.ip||'-';
  document.getElementById('mac').textContent=wifi.mac||'-';
  document.getElementById('rssi').textContent=wifi.rssi+' dBm';
  
  // MQTT
  const statusEl = document.getElementById('mqttStatus');
  
  if(!mqttConfig.configured){
    // Não configurado
    statusEl.innerHTML='<span class="status-badge badge-not-configured">⚙️ Não configurado</span>';
  }else{
    // Configurado - mostra se está conectado ou não
    if(mqttStatus.connected){
      statusEl.innerHTML='<span class="status-badge badge-connected">✅ Conectado</span>';
    }else{
      statusEl.innerHTML='<span class="status-badge badge-disconnected">❌ Desconectado</span>';
    }
    
    // Mostra informações do broker
    document.getElementById('mqttBroker').textContent=mqttConfig.broker||'-';
    document.getElementById('mqttPort').textContent=mqttConfig.port||'-';
    document.getElementById('mqttClient').textContent=mqttConfig.client_id||'-';
    document.getElementById('mqttTopic').textContent=mqttStatus.base_topic||'-';
    
    // Exibe as linhas
    document.getElementById('brokerRow').style.display='flex';
    document.getElementById('portRow').style.display='flex';
    document.getElementById('clientRow').style.display='flex';
    document.getElementById('topicRow').style.display='flex';
  }
}).catch(e=>{
  console.error('Erro ao carregar informações:', e);
});

function formatUptime(s){
  const d=Math.floor(s/86400);
  const h=Math.floor((s%86400)/3600);
  const m=Math.floor((s%3600)/60);
  return d>0?`${d}d ${h}h ${m}m`:`${h}h ${m}m`;
}
</script>
</div></body></html>
)RAW";

#endif // CMD_PAGE_INFO_H