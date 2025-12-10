#ifndef CMD_PAGE_SECURITY_H
#define CMD_PAGE_SECURITY_H

#include <pgmspace.h>

// ==================== PÁGINA DE SEGURANÇA ====================

const char HTML_SECURITY_PAGE[] PROGMEM = R"RAW(
<!doctype html><html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Segurança - CMD-C</title>
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
.btn-danger{background:#ff5252;color:white}
.btn-danger:hover{background:#ff1744}
.btn-back{background:#e0e0e0;color:#333}
.btn-back:hover{background:#d0d0d0}
input{width:100%;padding:12px;border:2px solid #e0e0e0;border-radius:8px;margin:8px 0}
input:focus{outline:none;border-color:#667eea}
label{display:block;margin-top:12px;font-weight:600;color:#555}
.status{padding:12px;border-radius:8px;margin:12px 0;text-align:center;font-weight:500}
.status-enabled{background:#e8f5e9;color:#2e7d32}
.status-disabled{background:#fff3e0;color:#e65100}
.info-box{background:#f0f4ff;padding:16px;border-radius:8px;margin:16px 0;font-size:14px;line-height:1.6}
.info-box strong{color:#667eea}
small{color:#888;font-size:13px;display:block;margin-top:4px}
.warning{background:#fff3e0;color:#e65100;padding:12px;border-radius:8px;margin:12px 0;font-size:14px}
</style>
</head><body>
<div class="container">
<h1>🔐 Segurança</h1>

<div class="card">
  <h2>Status da Autenticação</h2>
  <div class="status" id="statusBox">Carregando...</div>
</div>

<div class="card">
  <h2>Configurar Senha de Acesso</h2>
  
  <div class="info-box">
    <strong>ℹ️ Como funciona:</strong><br>
    • Usuário fixo: <strong>admin</strong><br>
    • Senha: definida por você (4-32 caracteres)<br>
    • Protege todas as páginas web e APIs<br>
    • Recuperação: Factory Reset (6 boots rápidos)
  </div>
  
  <label>Nova Senha:</label>
  <input type="password" id="newPassword" placeholder="Digite a nova senha (mínimo 4 caracteres)">
  <small>Entre 4 e 32 caracteres</small>
  
  <label>Confirmar Senha:</label>
  <input type="password" id="confirmPassword" placeholder="Digite a senha novamente">
  
  <button class="btn-primary" onclick="setPassword()">💾 Salvar Senha</button>
</div>

<div class="card">
  <h2>Remover Senha</h2>
  
  <div class="warning">
    ⚠️ <strong>Atenção:</strong> Remover a senha deixará o dispositivo sem proteção. Qualquer pessoa na rede poderá acessar.
  </div>
  
  <button class="btn-danger" onclick="clearPassword()">🗑️ Remover Senha</button>
</div>

<button class="btn-back" onclick="location.href='/'">← Voltar</button>

<script>
async function loadStatus() {
  try {
    const r = await fetch('/api/auth/status');
    const data = await r.json();
    
    const statusBox = document.getElementById('statusBox');
    
    if (data.enabled) {
      statusBox.textContent = '🔒 Autenticação Habilitada';
      statusBox.className = 'status status-enabled';
    } else {
      statusBox.textContent = '🔓 Sem Senha Configurada';
      statusBox.className = 'status status-disabled';
    }
  } catch (e) {
    console.error('Erro ao carregar status:', e);
  }
}

async function setPassword() {
  const newPass = document.getElementById('newPassword').value;
  const confirmPass = document.getElementById('confirmPassword').value;
  
  if (!newPass) {
    alert('⚠️ Digite uma senha!');
    return;
  }
  
  if (newPass.length < 4) {
    alert('⚠️ Senha muito curta! Mínimo 4 caracteres.');
    return;
  }
  
  if (newPass.length > 32) {
    alert('⚠️ Senha muito longa! Máximo 32 caracteres.');
    return;
  }
  
  if (newPass !== confirmPass) {
    alert('⚠️ As senhas não coincidem!');
    return;
  }
  
  try {
    const r = await fetch('/api/auth/password', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({password: newPass})
    });
    
    const data = await r.json();
    
    if (data.success) {
      alert('✅ Senha configurada com sucesso!\n\nUsuário: admin\nSenha: (a que você definiu)\n\nVocê será solicitado a fazer login novamente.');
      
      // Limpa campos
      document.getElementById('newPassword').value = '';
      document.getElementById('confirmPassword').value = '';
      
      // Atualiza status
      await loadStatus();
      
      // Aguarda 2 segundos e recarrega (força novo login)
      setTimeout(() => {
        location.reload();
      }, 2000);
    } else {
      alert('❌ Erro ao configurar senha:\n' + (data.message || 'Erro desconhecido'));
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

async function clearPassword() {
  if (!confirm('⚠️ Remover senha de acesso?\n\nO dispositivo ficará sem proteção!')) {
    return;
  }
  
  try {
    const r = await fetch('/api/auth/password', {method: 'DELETE'});
    const data = await r.json();
    
    if (data.success) {
      alert('✅ Senha removida!\n\nO acesso agora está livre.');
      await loadStatus();
    } else {
      alert('❌ Erro ao remover senha');
    }
  } catch (e) {
    alert('❌ Erro: ' + e.message);
  }
}

// Inicialização
window.onload = () => {
  loadStatus();
};
</script>
</div></body></html>
)RAW";

#endif // CMD_PAGE_SECURITY_H