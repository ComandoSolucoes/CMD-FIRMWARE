#ifndef CMD_PAGE_CONFIG_H
#define CMD_PAGE_CONFIG_H

#include <pgmspace.h>

// ==================== PÁGINA DE CONFIGURAÇÃO WiFi (MODO AP) ====================

const char HTML_INICIO[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CMD-C WiFi Config</title>
)RAW";

const char HTML_ESTILOS[] PROGMEM = R"RAW(
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px;color:#333}
.container{max-width:500px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:30px;font-size:28px;text-shadow:0 2px 4px rgba(0,0,0,0.2)}
.card{background:white;border-radius:16px;padding:24px;margin-bottom:20px;box-shadow:0 8px 32px rgba(0,0,0,0.1)}
.card h2{font-size:18px;margin-bottom:16px;color:#667eea}
button,input{font-size:16px;padding:8px;margin:4px 0;}
button{width:100%;background:#667eea;color:white;border:none;padding:14px;border-radius:8px;font-size:16px;font-weight:600;cursor:pointer;transition:all 0.3s}
button:hover{background:#5568d3;transform:translateY(-2px);box-shadow:0 4px 12px rgba(102,126,234,0.4)}
button:active{transform:translateY(0)}
input{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;font-size:16px;margin-bottom:16px;transition:border-color 0.3s}
input:focus{outline:none;border-color:#667eea}
label{display:block;font-weight:600;margin-bottom:8px;color:#555}
.ssid-list{margin-top:16px;max-height:300px;overflow-y:auto}
.ssid{display:flex;justify-content:space-between;align-items:center;padding:12px;border-bottom:1px solid #f0f0f0;cursor:pointer;transition:background 0.2s}
.ssid:hover{background:#f8f9ff}
.ssid:last-child{border-bottom:none}
.ssid-name{font-weight:500}
.ssid-info{display:flex;gap:8px;align-items:center;font-size:13px;color:#888}
.lock{font-size:16px}
.status{padding:12px;background:#f0f4ff;border-radius:8px;font-size:14px;color:#667eea;text-align:center;font-weight:500;white-space:pre-line}
.loading{text-align:center;color:#888;padding:20px;font-style:italic}
small{color:#888;font-size:13px}
.badge{display:inline-block;padding:4px 8px;background:#e8f5e9;color:#2e7d32;border-radius:4px;font-size:11px;font-weight:600}
</style>
)RAW";

const char HTML_BODY_ABRE[] PROGMEM = R"RAW(
</head><body>
<div class="container">
<h1>⚙️ CMD-C Setup</h1>
)RAW";

const char HTML_CARD_SCAN[] PROGMEM = R"RAW(
<div class="card">
  <h2>📡 Redes Disponíveis</h2>
  <button id="btnScan">Escanear Redes</button>
  <div id="scanArea"></div>
</div>
)RAW";

const char HTML_CARD_FORM[] PROGMEM = R"RAW(
<div class="card">
  <h2>🔐 Conectar ao WiFi</h2>
  <label>Nome da Rede (SSID)</label>
  <input id="ssid" placeholder="Selecione uma rede acima ou digite">
  <label>Senha</label>
  <input id="pass" type="password" placeholder="Digite a senha do WiFi">
  <button id="btnConnect">Conectar</button>
</div>
)RAW";

const char HTML_CARD_STATUS[] PROGMEM = R"RAW(
<div class="card">
  <h2>📊 Status</h2>
  <div class="status" id="status">Aguardando configuração...</div>
</div>
)RAW";

const char HTML_SCRIPT[] PROGMEM = R"RAW(
<script>
const elScan=document.getElementById('scanArea');
const elSsid=document.getElementById('ssid');
const elPass=document.getElementById('pass');
const elStatus=document.getElementById('status');
const btnScan=document.getElementById('btnScan');
const btnConnect=document.getElementById('btnConnect');

btnScan.onclick=async()=>{
  btnScan.disabled=true;
  btnScan.textContent='Escaneando...';
  elScan.innerHTML='<div class="loading">🔍 Procurando redes WiFi...</div>';
  
  try{
    const r=await fetch('/scan');
    const data=await r.json();
    
    if(!Array.isArray(data)||data.length===0){
      elScan.innerHTML='<div class="loading">❌ Nenhuma rede encontrada</div>';
      return;
    }
    
    data.sort((a,b)=>b.rssi-a.rssi);
    
    elScan.innerHTML='<div class="ssid-list">'+data.map((net,i)=>{
      const strength=net.rssi>-50?'Excelente':net.rssi>-60?'Bom':net.rssi>-70?'Regular':'Fraco';
      return `<div class="ssid" data-ssid="${net.ssid}">
        <div class="ssid-name">${net.ssid}</div>
        <div class="ssid-info">
          <span class="badge">${strength}</span>
          <span>${net.rssi} dBm</span>
          ${net.secure?'<span class="lock">🔒</span>':'<span class="lock">🔓</span>'}
        </div>
      </div>`;
    }).join('')+'</div>';
    
    document.querySelectorAll('.ssid').forEach(el=>{
      el.onclick=()=>elSsid.value=el.dataset.ssid;
    });
  }catch(e){
    elScan.innerHTML='<div class="loading">❌ Erro ao escanear</div>';
  }finally{
    btnScan.disabled=false;
    btnScan.textContent='Escanear Novamente';
  }
};

btnConnect.onclick=async()=>{
  if(!elSsid.value){
    alert('⚠️ Digite o nome da rede!');
    return;
  }
  
  btnConnect.disabled=true;
  btnConnect.textContent='Conectando...';
  elStatus.textContent='⏳ Conectando ao WiFi...\nIsso pode levar até 15 segundos.';
  elStatus.style.background='#f0f4ff';
  elStatus.style.color='#667eea';
  
  try{
    const body=new URLSearchParams({ssid:elSsid.value,pass:elPass.value});
    const r=await fetch('/connect',{
      method:'POST',
      headers:{'Content-Type':'application/x-www-form-urlencoded'},
      body
    });
    const text=await r.text();
    
    // Verifica se conectou com sucesso
    if(text.includes('Conectado') || text.includes('IP:')){
      // ✅ Extrai o IP da resposta
      const ipMatch = text.match(/\d+\.\d+\.\d+\.\d+/);
      const ipAddress = ipMatch ? ipMatch[0] : null;
      
      if(!ipAddress){
        elStatus.textContent='✅ Conectado!\n\nMas não foi possível obter o IP.\nProcure o dispositivo na rede.';
        elStatus.style.background='#fff3cd';
        elStatus.style.color='#856404';
        return;
      }
      
      // ✅ Primeiro contador: Reiniciando em 3, 2, 1...
      let rebootCountdown = 3;
      
      const updateRebootCountdown = () => {
        elStatus.textContent='✅ Conectado com sucesso!\n\n'+
          '📡 SSID: '+elSsid.value+'\n'+
          '🌐 IP: '+ipAddress+'\n\n'+
          '🔄 Reiniciando em '+rebootCountdown+' segundo'+(rebootCountdown !== 1 ? 's' : '')+'...';
        elStatus.style.background='#e8f5e9';
        elStatus.style.color='#2e7d32';
        
        if(rebootCountdown > 0){
          rebootCountdown--;
          setTimeout(updateRebootCountdown, 1000);
        }else{
          // ✅ Chegou a 0, executa o reboot
          executeReboot();
        }
      };
      
      const executeReboot = async () => {
        try{
          await fetch('/api/reboot',{method:'POST'});
        }catch(e){
          // Conexão será perdida, é esperado
        }
        
        // ✅ Segundo contador: Redirecionando em 10, 9, 8...
        let redirectCountdown = 10;
        
        const updateRedirectCountdown = () => {
          elStatus.textContent='✅ Conectado!\n\n'+
            '🌐 Redirecionando para:\nhttp://'+ipAddress+'\n\n'+
            '⏳ Aguarde '+redirectCountdown+' segundo'+(redirectCountdown !== 1 ? 's' : '');
          
          if(redirectCountdown > 0){
            redirectCountdown--;
            setTimeout(updateRedirectCountdown, 1000);
          }else{
            // ✅ Contador chegou a 0, redireciona
            elStatus.textContent='✅ Conectado!\n\n'+
              '🌐 Redirecionando para:\nhttp://'+ipAddress+'\n\n'+
              '🚀 Redirecionando agora...';
            
            setTimeout(()=>{
              window.location.href='http://'+ipAddress;
            },500);
          }
        };
        
        updateRedirectCountdown();
      };
      
      // ✅ Inicia o primeiro contador
      updateRebootCountdown();
      
    }else{
      // ❌ Falha na conexão
      elStatus.textContent='❌ '+text+'\n\n💡 Verifique a senha e tente novamente';
      elStatus.style.background='#ffebee';
      elStatus.style.color='#c62828';
      btnConnect.disabled=false;
      btnConnect.textContent='Tentar Novamente';
    }
  }catch(e){
    elStatus.textContent='❌ Erro: '+e.message;
    elStatus.style.background='#ffebee';
    elStatus.style.color='#c62828';
    btnConnect.disabled=false;
    btnConnect.textContent='Tentar Novamente';
  }
};

window.onload=()=>btnScan.click();
</script>
)RAW";

const char HTML_FIM[] PROGMEM = R"RAW(
</div></body></html>
)RAW";

#endif // CMD_PAGE_CONFIG_H