#ifndef CMD_PAGE_DASHBOARD_H
#define CMD_PAGE_DASHBOARD_H

#include <pgmspace.h>

// ==================== DASHBOARD PRINCIPAL (MODO STA) ====================

const char HTML_DASHBOARD_INICIO[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>CMD-C Dashboard</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px;color:#333}
.container{max-width:600px;margin:0 auto}
h1{color:white;text-align:center;margin-bottom:10px;font-size:32px;text-shadow:0 2px 4px rgba(0,0,0,0.3)}
.subtitle{color:rgba(255,255,255,0.9);text-align:center;margin-bottom:30px;font-size:16px}
.menu-btn{background:white;border:none;padding:20px;margin:10px 0;border-radius:12px;width:100%;text-align:left;cursor:pointer;box-shadow:0 4px 16px rgba(0,0,0,0.1);transition:all 0.3s;display:flex;align-items:center;gap:15px;font-size:16px;color:#333}
.menu-btn:hover{transform:translateY(-4px);box-shadow:0 8px 24px rgba(0,0,0,0.15)}
.menu-btn:active{transform:translateY(0)}
.menu-btn .icon{font-size:28px;width:40px;text-align:center}
.menu-btn .text{flex:1}
.menu-btn .text strong{display:block;font-size:18px;margin-bottom:4px;color:#667eea}
.menu-btn .text small{color:#888;font-size:13px}
.menu-btn .arrow{font-size:20px;color:#ccc}
.btn-primary{background:linear-gradient(135deg,#667eea,#764ba2);border:2px solid rgba(255,255,255,0.3)}
.btn-primary strong{color:white!important}
.btn-primary small{color:rgba(255,255,255,0.9)!important}
.btn-primary .arrow{color:rgba(255,255,255,0.8)}
.btn-danger{background:#ff5252}
.btn-danger:hover{background:#ff1744}
.btn-danger strong{color:white!important}
.btn-danger .arrow{color:rgba(255,255,255,0.7)}
</style>
</head><body>
<div class="container">
<h1>🌐 CMD-C</h1>
<div class="subtitle" id="deviceName">Dispositivo</div>
)RAW";

const char HTML_DASHBOARD_BUTTONS[] PROGMEM = R"RAW(
<button class="menu-btn btn-primary" onclick="location.href='/device'">
  <span class="icon">🎛️</span>
  <span class="text">
    <strong>Dispositivo</strong>
    <small>Controle e configurações específicas</small>
  </span>
  <span class="arrow">›</span>
</button>

<button class="menu-btn" onclick="location.href='/info'">
  <span class="icon">ℹ️</span>
  <span class="text">
    <strong>Informações do Sistema</strong>
    <small>Hardware, IP, MAC, status</small>
  </span>
  <span class="arrow">›</span>
</button>

<button class="menu-btn" onclick="location.href='/network'">
  <span class="icon">📡</span>
  <span class="text">
    <strong>Configuração de Rede</strong>
    <small>WiFi, IP fixo, trocar rede</small>
  </span>
  <span class="arrow">›</span>
</button>

<button class="menu-btn" onclick="location.href='/mqtt'">
  <span class="icon">🔌</span>
  <span class="text">
    <strong>MQTT</strong>
    <small>Broker, publicar, subscrever</small>
  </span>
  <span class="arrow">›</span>
</button>

<button class="menu-btn" onclick="location.href='/security'">
  <span class="icon">🔐</span>
  <span class="text">
    <strong>Segurança</strong>
    <small>Configurar senha de acesso</small>
  </span>
  <span class="arrow">›</span>
</button>

<button class="menu-btn btn-danger" onclick="if(confirm('Reiniciar o dispositivo?'))fetch('/api/reboot',{method:'POST'})">
  <span class="icon">🔄</span>
  <span class="text">
    <strong>Reiniciar Dispositivo</strong>
    <small>Reboot do ESP32</small>
  </span>
  <span class="arrow">›</span>
</button>

<script>
fetch('/api/info')
  .then(r=>r.json())
  .then(d=>{
    document.getElementById('deviceName').textContent=d.device||'CMD-Device';
  });
</script>
)RAW";

const char HTML_DASHBOARD_FIM[] PROGMEM = R"RAW(
</div></body></html>
)RAW";

#endif // CMD_PAGE_DASHBOARD_H