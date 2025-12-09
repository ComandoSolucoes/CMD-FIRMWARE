#ifndef CMD_AUTH_H
#define CMD_AUTH_H

#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>
#include "config/CMDConfig.h"

class CMDAuth {
public:
    CMDAuth();
    
    // Inicialização
    void begin();
    
    // Senha
    bool setPassword(const String& password);
    bool hasPassword();
    void clearPassword();
    String getPassword(); // Para debug, remove depois
    
    // Validação
    bool authenticate(WebServer& server);
    
private:
    Preferences prefs;
    String savedPassword;
    bool authEnabled;
};

#endif // CMD_AUTH_H