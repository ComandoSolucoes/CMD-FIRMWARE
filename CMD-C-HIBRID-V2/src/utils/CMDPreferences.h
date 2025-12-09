#ifndef CMDPREFERENCES_H
#define CMDPREFERENCES_H

#include <Arduino.h>

#if defined(ESP32)
    #include <Preferences.h>
#elif defined(ESP8266)
    #include <EEPROM.h>
#endif

// Descomente para ativar logs de debug
#define CMD_DEBUG_PREFS

/*
 * CMDPreferences v3.0 - Híbrido ESP32/ESP8266
 * 
 * ╔═══════════════════════════════════════════════════════════════╗
 * ║  IMPLEMENTAÇÃO HÍBRIDA                                        ║
 * ╠═══════════════════════════════════════════════════════════════╣
 * ║  ESP32:  Usa Preferences.h nativo                            ║
 * ║          - Sem limite de namespaces                           ║
 * ║          - Sem limite de entries                              ║
 * ║          - Gerenciamento automático                           ║
 * ║                                                                ║
 * ║  ESP8266: Usa EEPROM.h emulado                               ║
 * ║          - 512 bytes total                                    ║
 * ║          - Namespaces mapeados manualmente                    ║
 * ║          - Otimizado para evitar overflow                     ║
 * ╚═══════════════════════════════════════════════════════════════╝
 * 
 * ARQUITETURA ESP8266 (512 bytes):
 * ================================
 * 
 * Bloco 1: wifi      (0-249)   - 250 bytes → 5 entries
 * Bloco 2: cmd-c     (250-349) - 100 bytes → 2 entries
 * Bloco 3: cmd-auth  (350-449) - 100 bytes → 2 entries
 * Bloco 4: cmd-mqtt  (450-511) - 62 bytes  → 1 entry compacta
 * 
 * DADOS SALVOS:
 * ============
 * 
 * WiFi (5 entries): ssid, password, static_ip, gateway, dns
 * CMD-C (2 entries): boot_count, ignore_next_boot
 * CMD-Auth (2 entries): web_user, web_pass
 * CMD-MQTT (1 entry): config compacta "broker:port:user:pass:topic"
 */

class CMDPreferences {
public:
    CMDPreferences();
    
    bool begin(const char* name, bool readOnly = false);
    void end();
    void clear();
    
    // String
    bool putString(const char* key, const String& value);
    String getString(const char* key, const String& defaultValue = String());
    
    // UChar
    bool putUChar(const char* key, uint8_t value);
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0);
    
    // UShort
    bool putUShort(const char* key, uint16_t value);
    uint16_t getUShort(const char* key, uint16_t defaultValue = 0);
    
    // UInt
    bool putUInt(const char* key, uint32_t value);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
    
    // ULong
    bool putULong(const char* key, unsigned long value);
    unsigned long getULong(const char* key, unsigned long defaultValue = 0);
    
    // Bool
    bool putBool(const char* key, bool value);
    bool getBool(const char* key, bool defaultValue = false);
    
    // Float
    bool putFloat(const char* key, float value);
    float getFloat(const char* key, float defaultValue = 0.0f);
    
    bool remove(const char* key);
    void commit();
    
private:
#if defined(ESP32)
    // ═══════════════════════════════════════════════════════════
    // ESP32 - USA PREFERENCES NATIVO
    // ═══════════════════════════════════════════════════════════
    Preferences prefs;
    
#elif defined(ESP8266)
    // ═══════════════════════════════════════════════════════════
    // ESP8266 - USA EEPROM COM MAPEAMENTO MANUAL
    // ═══════════════════════════════════════════════════════════
    static const int EEPROM_SIZE = 512;
    static const int MAX_ENTRIES = 5;       // 5 entries para namespace wifi
    static const int KEY_SIZE = 16;         // 16 chars para chave
    static const int VALUE_SIZE = 32;       // 32 chars para valor
    
    struct Entry {
        bool used;              // 1 byte
        char key[KEY_SIZE];     // 16 bytes
        char value[VALUE_SIZE]; // 32 bytes
        // TOTAL: 49 bytes por entry
    };
    
    Entry entries[MAX_ENTRIES];
    String namespace_name;
    bool initialized;
    bool needsCommit;
    
    int findEntry(const char* key);
    int findFreeEntry();
    uint16_t getNamespaceAddress();
    void loadFromEEPROM();
    void commitNow();
#endif
};

#endif