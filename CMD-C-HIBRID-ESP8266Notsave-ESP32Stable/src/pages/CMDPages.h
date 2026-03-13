#ifndef CMD_PAGES_H
#define CMD_PAGES_H

/*
 * CMD-C Pages - Include Master
 * 
 * Este arquivo centraliza todas as páginas HTML do CMD-C.
 * Cada página está em seu próprio arquivo para melhor organização.
 * 
 * Estrutura:
 * - CMDPage_Config.h     : Configuração WiFi (modo AP)
 * - CMDPage_Dashboard.h  : Menu principal (modo STA)
 * - CMDPage_Info.h       : Informações do sistema
 * - CMDPage_Network.h    : Configuração de rede + IP fixo
 * - CMDPage_MQTT.h       : MQTT (preparado para futuro)
 */

// Inclui todas as páginas modulares
#include "pages/CMDPage_Config.h"
#include "pages/CMDPage_Dashboard.h"
#include "pages/CMDPage_Info.h"
#include "pages/CMDPage_Network.h"
#include "pages/CMDPage_MQTT.h"
#include "pages/CMDPage_Security.h"
#include "pages/CMDPage_DeviceDefault.h"

#endif // CMD_PAGES_H