#ifndef CMD_CONFIG_H
#define CMD_CONFIG_H

// ============================ Versão ============================
#define CMD_VERSION "1.0.0"

// ============================ Wi-Fi (AP Mode) ============================
#define CMD_AP_PASSWORD "Comando@123"
#define CMD_AP_IP_1 192
#define CMD_AP_IP_2 168
#define CMD_AP_IP_3 4
#define CMD_AP_IP_4 1

// ============================ Factory Reset ============================
#define CMD_BOOT_COUNT_THRESHOLD  6        // Número de boots para factory reset
#define CMD_BOOT_WINDOW_MS        10000    // Janela de tempo (10 segundos)
#define CMD_BOOT_CLEAR_MS         10000    // Tempo para zerar contador após boot normal

// ============================ Preferences ============================
#define CMD_PREFS_NAMESPACE "cmd-c"        // Namespace fixo para WiFi

// ============================ Debug ============================
#define CMD_DEBUG_PREFIX "[CMD-C] "

#endif // CMD_CONFIG_H
