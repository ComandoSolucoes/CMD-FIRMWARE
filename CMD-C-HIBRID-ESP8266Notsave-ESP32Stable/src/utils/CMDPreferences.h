#ifndef CMDPREFERENCES_H
#define CMDPREFERENCES_H

#include <Arduino.h>

#if defined(ESP32)
  #include <Preferences.h>
#elif defined(ESP8266)
  #include <EEPROM.h>
#endif

// Ative logs de debug
#define CMD_DEBUG_PREFS

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
    // ESP32: wrapper do Preferences nativo
    Preferences prefs;
    
#elif defined(ESP8266)
    // ESP8266: EEPROM mapeada em 4 namespaces fixos
    static const int EEPROM_SIZE      = 2048;   // 2KB
    static const int MAX_ENTRIES      = 6;      // até 6 chaves por namespace
    static const int KEY_SIZE         = 16;
    static const int VALUE_SIZE       = 32;
    static const int NAMESPACE_COUNT  = 4;
    static const int NAMESPACE_SIZE   = EEPROM_SIZE / NAMESPACE_COUNT; // 512 bytes/bloco
    
    struct Entry {
        bool used;
        char key[KEY_SIZE];
        char value[VALUE_SIZE];
    };
    
    Entry  entries[MAX_ENTRIES];
    String namespace_name;
    bool   initialized;
    bool   needsCommit;
    
    int      findEntry(const char* key);
    int      findFreeEntry();
    uint16_t getNamespaceAddress();
    void     loadFromEEPROM();
    void     commitNow();
#endif
};

#endif
