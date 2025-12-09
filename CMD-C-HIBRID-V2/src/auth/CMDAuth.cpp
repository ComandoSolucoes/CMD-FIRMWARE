#include "CMDAuth.h"

CMDAuth::CMDAuth() : authEnabled(false) {}

void CMDAuth::begin() {
    prefs.begin(CMD_PREFS_NAMESPACE, false);
    
    // Carrega senha salva
    savedPassword = prefs.getString("web_pass", "");
    authEnabled = savedPassword.length() > 0;
    
    if (authEnabled) {
        Serial.println(CMD_DEBUG_PREFIX "🔐 Autenticação web habilitada");
    } else {
        Serial.println(CMD_DEBUG_PREFIX "🔓 Sem autenticação web (configure em /security)");
    }
}

bool CMDAuth::setPassword(const String& password) {
    if (password.length() < 4) {
        Serial.println(CMD_DEBUG_PREFIX "❌ Senha muito curta (mínimo 4 caracteres)");
        return false;
    }
    
    if (password.length() > 32) {
        Serial.println(CMD_DEBUG_PREFIX "❌ Senha muito longa (máximo 32 caracteres)");
        return false;
    }
    
    // Salva senha em texto plano
    prefs.putString("web_pass", password);
    savedPassword = password;
    authEnabled = true;
    
    Serial.println(CMD_DEBUG_PREFIX "✅ Senha web configurada");
    
    return true;
}

bool CMDAuth::hasPassword() {
    return authEnabled && savedPassword.length() > 0;
}

void CMDAuth::clearPassword() {
    prefs.remove("web_pass");
    savedPassword = "";
    authEnabled = false;
    
    Serial.println(CMD_DEBUG_PREFIX "🔓 Senha web removida");
}

String CMDAuth::getPassword() {
    return savedPassword;
}

bool CMDAuth::authenticate(WebServer& server) {
    // Se não tem senha configurada, libera acesso
    if (!hasPassword()) {
        return true;
    }
    
    // Valida HTTP Basic Auth
    // User fixo: "admin"
    // Pass: o que foi configurado
    if (!server.authenticate("admin", savedPassword.c_str())) {
        server.requestAuthentication(BASIC_AUTH, "CMD-C", "Digite usuário e senha");
        return false;
    }
    
    return true;
}