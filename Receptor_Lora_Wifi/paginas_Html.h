#ifndef PAGINAS_HTML_H
#define PAGINAS_HTML_H

#include <pgmspace.h>

// Cabeçalho + <head> (sem estilos ainda) e abertura do <body>
const char HTML_INICIO[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Config Wi-Fi</title>
)RAW";

// Bloco de estilos (CSS)
const char HTML_ESTILOS[] PROGMEM = R"RAW(
<style>
body{font-family:system-ui,Arial,sans-serif;margin:20px;}
button,input{font-size:16px;padding:8px;margin:4px 0;}
.card{border:1px solid #ddd;border-radius:12px;padding:16px;margin-bottom:16px}
.ssid{display:flex;justify-content:space-between;padding:8px;border-bottom:1px solid #eee;cursor:pointer}
.ssid:hover{background:#fafafa}
small{opacity:.7}
</style>
)RAW";

// Abertura do body e título
const char HTML_BODY_ABRE[] PROGMEM = R"RAW(
</head><body>
<h2>Configurar Wi-Fi</h2>
)RAW";

// Card: botão de scan + área de resultados
const char HTML_CARD_SCAN[] PROGMEM = R"RAW(
<div class="card">
  <button id="btnScan">Escanear redes</button>
  <div id="scanArea"><small>Clique em “Escanear redes”.</small></div>
</div>
)RAW";

// Card: formulário SSID/Senha
const char HTML_CARD_FORM[] PROGMEM = R"RAW(
<div class="card">
  <label>SSID</label><br/>
  <input id="ssid" placeholder="Escolha acima ou digite" style="width:100%">
  <label>Senha</label><br/>
  <input id="pass" type="password" placeholder="••••••••" style="width:100%"><br/>
  <button id="btnConnect">Conectar</button>
</div>
)RAW";

// Card: status
const char HTML_CARD_STATUS[] PROGMEM = R"RAW(
<div class="card">
  <b>Status:</b> <span id="status">—</span>
</div>
)RAW";

// Script da página
const char HTML_SCRIPT[] PROGMEM = R"RAW(
<script>
const elScan=document.getElementById('scanArea');
const elSsid=document.getElementById('ssid');
const elPass=document.getElementById('pass');
const elStatus=document.getElementById('status');

document.getElementById('btnScan').onclick=async()=>{
  elScan.innerHTML='Escaneando...';
  try{
    const r=await fetch('/scan');
    const data=await r.json();
    if(!Array.isArray(data)||data.length===0){elScan.innerHTML='Nenhuma rede encontrada.';return;}
    data.sort((a,b)=>b.rssi-a.rssi);
    elScan.innerHTML=data.map((it,i)=>(
      `<div class="ssid" data-i="${i}">
         <div>${i}) ${it.ssid}</div>
         <div>${it.rssi} dBm ${it.secure?'🔒':''}</div>
       </div>`
    )).join('');
    [...elScan.children].forEach((row,i)=>row.onclick=()=>{elSsid.value=data[i].ssid;});
  }catch(e){elScan.textContent='Falha no scan.';}
};

document.getElementById('btnConnect').onclick=async()=>{
  const body=new URLSearchParams({ssid:elSsid.value,pass:elPass.value});
  try{
    const r=await fetch('/connect',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body});
    const t=await r.text();
    elStatus.textContent=t||'Enviando...';
    pollStatus();
  }catch(e){elStatus.textContent='Erro ao enviar conexão.';}
};

async function pollStatus(){
  for(let i=0;i<30;i++){
    try{
      const r=await fetch('/status');
      const s=await r.json();
      elStatus.textContent=s.text||JSON.stringify(s);
      if(s.state==='connected'||s.state==='failed')return;
    }catch(e){}
    await new Promise(res=>setTimeout(res,1000));
  }
}
</script>
)RAW";

// Fechamento do body/html
const char HTML_FIM[] PROGMEM = R"RAW(
</body></html>
)RAW";

// ========== PAINEL PRINCIPAL (quando conectado) ==========

const char HTML_PAINEL_INICIO[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Painel LoRa</title>
)RAW";

const char HTML_PAINEL_ESTILOS[] PROGMEM = R"RAW(
<style>
body{font-family:system-ui,Arial,sans-serif;margin:20px;background:#f5f5f5}
.card{background:white;border-radius:12px;padding:16px;margin-bottom:16px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}
.header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px}
.status{display:flex;gap:20px;flex-wrap:wrap}
.status-item{flex:1;min-width:150px}
.status-item label{font-size:12px;color:#666;display:block;margin-bottom:4px}
.status-item value{font-size:18px;font-weight:bold;color:#333}
.btn-danger{background:#dc3545;color:white;border:none;padding:10px 20px;border-radius:6px;cursor:pointer;font-size:14px}
.btn-danger:hover{background:#c82333}
.packet{border-bottom:1px solid #eee;padding:12px 0;font-family:monospace;font-size:13px}
.packet:last-child{border-bottom:none}
.packet-time{color:#666;font-size:11px;margin-bottom:4px}
.packet-data{color:#333;word-break:break-all}
.packet-rssi{color:#888;font-size:11px;margin-top:4px}
#packets{max-height:400px;overflow-y:auto}
.empty{text-align:center;color:#999;padding:40px;font-style:italic}
.indicator{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px}
.connected{background:#28a745}
.disconnected{background:#dc3545}
</style>
)RAW";

const char HTML_PAINEL_BODY[] PROGMEM = R"RAW(
</head><body>
<div class="header">
  <h2>🌐 Painel LoRa</h2>
  <button class="btn-danger" id="btnForget">Esquecer Rede</button>
</div>

<div class="card">
  <h3>Status da Conexão</h3>
  <div class="status">
    <div class="status-item">
      <label>Status</label>
      <value><span class="indicator connected"></span><span id="wifiStatus">Conectado</span></value>
    </div>
    <div class="status-item">
      <label>SSID</label>
      <value id="ssid">—</value>
    </div>
    <div class="status-item">
      <label>IP Local</label>
      <value id="ip">—</value>
    </div>
    <div class="status-item">
      <label>RSSI</label>
      <value id="rssi">—</value>
    </div>
  </div>
</div>

<div class="card">
  <h3>📡 Pacotes LoRa Recebidos</h3>
  <div id="packets">
    <div class="empty">Aguardando dados do LoRa...</div>
  </div>
</div>
)RAW";

const char HTML_PAINEL_SCRIPT[] PROGMEM = R"RAW(
<script>
const elPackets = document.getElementById('packets');
const elSsid = document.getElementById('ssid');
const elIp = document.getElementById('ip');
const elRssi = document.getElementById('rssi');

// Server-Sent Events para dados LoRa
const evtSource = new EventSource('/lora_stream');

evtSource.onmessage = (e) => {
  try {
    const data = JSON.parse(e.data);
    
    // Remove mensagem vazia
    if(elPackets.querySelector('.empty')) {
      elPackets.innerHTML = '';
    }
    
    // Adiciona novo pacote no topo
    const packet = document.createElement('div');
    packet.className = 'packet';
    packet.innerHTML = `
      <div class="packet-time">${data.time}</div>
      <div class="packet-data">${data.payload}</div>
      <div class="packet-rssi">RSSI: ${data.rssi} dBm | SNR: ${data.snr} dB</div>
    `;
    elPackets.insertBefore(packet, elPackets.firstChild);
    
    // Limita a 50 pacotes
    while(elPackets.children.length > 50) {
      elPackets.removeChild(elPackets.lastChild);
    }
  } catch(err) {
    console.error('Erro ao processar pacote:', err);
  }
};

evtSource.onerror = () => {
  console.error('Conexão SSE perdida');
};

// Busca status Wi-Fi inicial
fetch('/wifi_info')
  .then(r => r.json())
  .then(data => {
    elSsid.textContent = data.ssid || '—';
    elIp.textContent = data.ip || '—';
    elRssi.textContent = data.rssi ? data.rssi + ' dBm' : '—';
  })
  .catch(err => console.error('Erro ao buscar info Wi-Fi:', err));

// Botão esquecer rede
document.getElementById('btnForget').onclick = async () => {
  if(!confirm('Tem certeza que deseja esquecer esta rede?\n\nVocê precisará reconfigurar o Wi-Fi após reiniciar o dispositivo.')) {
    return;
  }
  
  try {
    const r = await fetch('/forget_wifi', {method: 'POST'});
    const msg = await r.text();
    alert(msg);
    
    if(r.ok) {
      document.body.innerHTML = '<div style="text-align:center;padding:40px"><h2>✅ Credenciais apagadas</h2><p>Reinicie o dispositivo para reconfigurar o Wi-Fi.</p></div>';
    }
  } catch(err) {
    alert('Erro ao esquecer rede: ' + err.message);
  }
};
</script>
)RAW";

const char HTML_PAINEL_FIM[] PROGMEM = R"RAW(
</body></html>
)RAW";

#endif // PAGINAS_HTML_H
