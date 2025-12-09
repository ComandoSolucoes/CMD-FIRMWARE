#ifndef CMD_PAGE_DEVICE_DEFAULT_H
#define CMD_PAGE_DEVICE_DEFAULT_H

#include <pgmspace.h>

// ==================== PÁGINA PADRÃO /DEVICE ====================
// Exibida quando nenhum módulo customizado registra a rota /device

const char HTML_DEVICE_DEFAULT[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dispositivo - CMD-C</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,-apple-system,Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:20px;color:#333;display:flex;align-items:center;justify-content:center}
.container{max-width:500px;background:white;border-radius:16px;padding:40px;text-align:center;box-shadow:0 8px 32px rgba(0,0,0,0.2)}
.icon{font-size:80px;margin-bottom:20px;opacity:0.3}
h1{color:#667eea;margin-bottom:15px;font-size:24px}
p{color:#666;line-height:1.6;margin-bottom:25px}
.info-box{background:#f5f5f5;border-radius:8px;padding:20px;margin:20px 0;text-align:left}
.info-box strong{color:#667eea;display:block;margin-bottom:10px}
.info-box ul{list-style:none;padding:0}
.info-box li{padding:8px 0;color:#666;border-bottom:1px solid #e0e0e0}
.info-box li:last-child{border-bottom:none}
.info-box li:before{content:"•";color:#667eea;font-weight:bold;margin-right:10px}
.btn{display:inline-block;padding:14px 30px;background:#667eea;color:white;text-decoration:none;border-radius:8px;font-weight:600;margin-top:10px;transition:all 0.3s}
.btn:hover{background:#5568d3;transform:translateY(-2px)}
</style>
</head><body>
<div class="container">
  <div class="icon">🎛️</div>
  <h1>Dispositivo Genérico</h1>
  <p>Este é um dispositivo CMD-C sem módulo específico configurado.</p>
  
  <div class="info-box">
    <strong>Módulos Disponíveis:</strong>
    <ul>
      <li>4 Canais + Ethernet (Relés)</li>
      <li>LoRa Sensor (Ultrassônico)</li>
      <li>Controle IR Universal</li>
      <li>Outros módulos customizados</li>
    </ul>
  </div>
  
  <p style="font-size:13px;color:#999;margin-top:20px">
    Para adicionar funcionalidades específicas ao dispositivo,<br>
    registre uma rota customizada para <code>/device</code>
  </p>
  
  <a href="/" class="btn">← Voltar ao Menu</a>
</div>
</body></html>
)RAW";

#endif // CMD_PAGE_DEVICE_DEFAULT_H