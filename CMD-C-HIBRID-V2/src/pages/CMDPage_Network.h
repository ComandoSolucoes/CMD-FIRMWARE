#ifndef CMD_PAGE_NETWORK_H
#define CMD_PAGE_NETWORK_H

#include <pgmspace.h>

// ==================== PÁGINA DE REDE ====================

const char HTML_NETWORK_PAGE[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Configuração de Rede - CMD-C</title>
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
.btn-danger{background:#ff5252;color:white}
.btn-danger:hover{background:#ff1744}
.btn-secondary{background:#ff9800;color:white}
.btn-secondary:hover{background:#f57c00}
.btn-back{background:#e0e0e0;color:#333}
.btn-back:hover{background:#d0d0d0}
input{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;margin:8px 0}
input:focus{outline:none;border-color:#667eea}
label{display:block;margin-top:12px;font-weight:600;color:#555}
.info{background:#f0f4ff;padding:12px;border-radius:8px;margin:12px 0;font-size:14px;color:#667eea}
.success{background:#e8f5e9;color:#2e7d32}
.warning{background:#fff3e0;color:#e65100}
.toggle{display:flex;align-items:center;gap:10px;margin:12px 0}
.toggle input[type="checkbox"]{width:auto}
.ssid-list{margin-top:16px;max-height:300px;overflow-y:auto}
.ssid{display:flex;justify-content:space-between;align-items:center;padding:12px;border-bottom:1px solid #f0f0f0;cursor:pointer;transition:background 0.2s}
.ssid:hover{background:#f8f9ff}
.ssid:last-child{border-bottom:none}
.ssid-name{font-weight:500}
.ssid-info{display:flex;gap:8px;align-items:center;font-size:13px;color:#888}
.badge{display:inline-block;padding:4px 8px;background:#e8f5e9;color:#2e7d32;border-radius:4px;font-size:11px;font-weight:600}
.loading{text-align:center;color:#888;padding:20px;font-style:italic}
</style>
</head><body>
<div class="container">
<h1>📡 Configuração de Rede</h1>

<div class="card">
  <h2>Rede Atual</h2>
  <div class="info" id="currentNet">Carregando...</div>
</div>

<div class="card">
  <h2>Trocar de Rede WiFi</h2>
  <button class="btn-secondary" onclick="scanWifi()">📡 Escanear Redes</button>
  <div id="scanArea" style="margin-top:16px"></div>
  
  <div id="connectForm" style="display:none;margin-top:16px">
    <label>Nome da Rede (SSID):</label>
    <input type="text" id="newSsid" placeholder="Selecione uma rede acima">
    <label>Senha:</label>
    <input type="password" id="newPass" placeholder="Digite a senha">
    <button class="btn-primary" onclick="connectNewWifi()">🔗 Conectar</button>
  </div>
  
  <button class="btn-danger" onclick="forgetWifi()" style="margin-top:16px">🗑️ Esquecer Rede e Voltar ao AP</button>
</div>

<div class="card">
  <h2>Configuração de IP</h2>
  <div class="toggle">
    <input type="checkbox" id="useStatic" onchange="toggleStaticIP()">
    <label for="useStatic" style="margin:0">Usar IP Fixo</label>
  </div>
  <div id="staticFields" style="display:none">
    <label>IP Fixo:</label>
    <input type="text" id="staticIp" placeholder="192.168.1.100">
    <label>Gateway:</label>
    <input type="text" id="staticGw" placeholder="192.168.1.1">
    <label>Máscara de Sub-rede:</label>
    <input type="text" id="staticSn" placeholder="255.255.255.0">
    <button class="btn-primary" onclick="saveStaticIP()">💾 Salvar IP Fixo</button>
    <button class="btn-secondary" onclick="clearStaticIP()">🔄 Voltar para DHCP</button>
  </div>
  <div class="info" id="ipStatus">Usando DHCP (automático)</div>
</div>

<button class="btn-back" onclick="location.href='/'">← Voltar</button>

<script>
let hasStatic = false;

Promise.all([
  fetch('/api/wifi').then(r=>r.json()),
  fetch('/api/network/static').then(r=>r.json())
]).then(([wifi, net])=>{
  document.getElementById('currentNet').innerHTML=
    `<strong>SSID:</strong> ${wifi.ssid}<br><strong>IP:</strong> ${wifi.ip}<br><strong>RSSI:</strong> ${wifi.rssi} dBm`;
  
  hasStatic = net.enabled;
  document.getElementById('useStatic').checked = hasStatic;
  
  if(hasStatic){
    document.getElementById('staticIp').value = net.ip;
    document.getElementById('staticGw').value = net.gateway;
    document.getElementById('staticSn').value = net.subnet;
    document.getElementById('staticFields').style.display = 'block';
    document.getElementById('ipStatus').className = 'info success';
    document.getElementById('ipStatus').textContent = `IP Fixo configurado: ${net.ip}`;
  }
});

function toggleStaticIP(){
  const enabled = document.getElementById('useStatic').checked;
  document.getElementById('staticFields').style.display = enabled ? 'block' : 'none';
  
  if(enabled && !hasStatic){
    const currentIP = document.getElementById('currentNet').textContent.match(/IP:\s*([0-9.]+)/);
    if(currentIP){
      const parts = currentIP[1].split('.');
      document.getElementById('staticIp').value = currentIP[1];
      document.getElementById('staticGw').value = parts[0]+'.'+parts[1]+'.'+parts[2]+'.1';
      document.getElementById('staticSn').value = '255.255.255.0';
    }
  }
}

function saveStaticIP(){
  const ip = document.getElementById('staticIp').value;
  const gw = document.getElementById('staticGw').value;
  const sn = document.getElementById('staticSn').value;
  
  if(!ip || !gw || !sn){
    alert('⚠️ Preencha todos os campos!');
    return;
  }
  
  if(!validateIP(ip) || !validateIP(gw) || !validateIP(sn)){
    alert('⚠️ IP inválido! Use o formato: 192.168.1.100');
    return;
  }
  
  fetch('/api/network/static',{
    method:'POST',
    headers:{'Content-Type':'application/json'},
    body:JSON.stringify({ip,gateway:gw,subnet:sn})
  })
  .then(r=>r.json())
  .then(d=>{
    if(d.success){
      alert('✅ IP fixo salvo!\n\nReinicie o dispositivo para aplicar.');
      document.getElementById('ipStatus').className = 'info success';
      document.getElementById('ipStatus').textContent = `IP Fixo configurado: ${ip}`;
    }else{
      alert('❌ Erro: '+(d.message||'Desconhecido'));
    }
  });
}

function clearStaticIP(){
  if(!confirm('⚠️ Voltar para DHCP?\n\nO IP será obtido automaticamente.')){
    return;
  }
  
  fetch('/api/network/static',{method:'DELETE'})
  .then(r=>r.json())
  .then(()=>{
    alert('✅ IP fixo removido!\n\nReinicie para usar DHCP.');
    document.getElementById('useStatic').checked = false;
    document.getElementById('staticFields').style.display = 'none';
    document.getElementById('ipStatus').className = 'info';
    document.getElementById('ipStatus').textContent = 'Usando DHCP (automático)';
  });
}

function scanWifi(){
  const scanArea = document.getElementById('scanArea');
  const connectForm = document.getElementById('connectForm');
  
  scanArea.innerHTML = '<div class="loading">🔍 Escaneando redes WiFi...</div>';
  connectForm.style.display = 'none';
  
  fetch('/scan')
    .then(r=>r.json())
    .then(data=>{
      if(!Array.isArray(data) || data.length === 0){
        scanArea.innerHTML = '<div class="loading">❌ Nenhuma rede encontrada</div>';
        return;
      }
      
      data.sort((a,b)=>b.rssi-a.rssi);
      
      scanArea.innerHTML = '<div class="ssid-list">' + data.map(net=>{
        const strength = net.rssi>-50?'Excelente':net.rssi>-60?'Bom':net.rssi>-70?'Regular':'Fraco';
        return `<div class="ssid" onclick="selectNetwork('${net.ssid}')">
          <div class="ssid-name">${net.ssid}</div>
          <div class="ssid-info">
            <span class="badge">${strength}</span>
            <span>${net.rssi} dBm</span>
            ${net.secure?'<span>🔒</span>':'<span>🔓</span>'}
          </div>
        </div>`;
      }).join('') + '</div>';
      
      connectForm.style.display = 'block';
    })
    .catch(e=>{
      scanArea.innerHTML = '<div class="loading">❌ Erro ao escanear</div>';
    });
}

function selectNetwork(ssid){
  document.getElementById('newSsid').value = ssid;
  document.getElementById('newPass').focus();
}

function connectNewWifi(){
  const ssid = document.getElementById('newSsid').value;
  const pass = document.getElementById('newPass').value;
  
  if(!ssid){
    alert('⚠️ Selecione uma rede WiFi!');
    return;
  }
  
  if(!confirm(`🔗 Conectar na rede "${ssid}"?\n\nO dispositivo irá desconectar da rede atual.`)){
    return;
  }
  
  const scanArea = document.getElementById('scanArea');
  scanArea.innerHTML = '<div class="loading">⏳ Conectando em ' + ssid + '...</div>';
  
  const body = new URLSearchParams({ssid, pass});
  fetch('/connect', {
    method: 'POST',
    headers: {'Content-Type': 'application/x-www-form-urlencoded'},
    body
  })
  .then(r=>r.text())
  .then(text=>{
    if(text.includes('Conectado')){
      scanArea.innerHTML = '<div class="info success">✅ ' + text + '</div>';
      setTimeout(()=>{
        alert('✅ Conectado com sucesso!\n\nAtualizando página...');
        location.reload();
      }, 2000);
    }else{
      scanArea.innerHTML = '<div class="info warning">❌ ' + text + '</div>';
    }
  })
  .catch(e=>{
    scanArea.innerHTML = '<div class="info warning">❌ Erro ao conectar</div>';
  });
}

function forgetWifi(){
  if(!confirm('⚠️ Tem certeza?\n\nO dispositivo perderá a conexão e voltará ao modo AP.')){
    return;
  }
  fetch('/api/wifi/forget',{method:'POST'})
    .then(r=>r.json())
    .then(()=>{
      alert('✅ Rede esquecida!\n\nO dispositivo reiniciará em modo AP.');
      setTimeout(()=>location.reload(),2000);
    });
}

function validateIP(ip){
  const parts = ip.split('.');
  if(parts.length !== 4) return false;
  return parts.every(p=>{
    const n = parseInt(p);
    return n >= 0 && n <= 255;
  });
}
</script>
</div></body></html>
)RAW";

#endif // CMD_PAGE_NETWORK_H