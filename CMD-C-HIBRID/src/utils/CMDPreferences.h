#ifndef CMD_PREFERENCES_H
#define CMD_PREFERENCES_H

#include <Arduino.h>
#include <EEPROM.h>

// Emula a API do Preferences do ESP32 usando EEPROM
class CMDPreferences {
public:
    CMDPreferences();
    
    bool begin(const char* name, bool readOnly = false);
    void end();
    void clear();
    
    // String
    bool putString(const char* key, const String& value);
    String getString(const char* key, const String& defaultValue = "");
    
    // Números
    bool putUChar(const char* key, uint8_t value);
    uint8_t getUChar(const char* key, uint8_t defaultValue = 0);
    
    bool putUShort(const char* key, uint16_t value);
    uint16_t getUShort(const char* key, uint16_t defaultValue = 0);
    
    bool putUInt(const char* key, uint32_t value);
    uint32_t getUInt(const char* key, uint32_t defaultValue = 0);
    
    bool putULong(const char* key, unsigned long value);
    unsigned long getULong(const char* key, unsigned long defaultValue = 0);
    
    bool putBool(const char* key, bool value);
    bool getBool(const char* key, bool defaultValue = false);
    
    bool remove(const char* key);
    
private:
    static const int EEPROM_SIZE = 512;  // Tamanho reservado
    String namespace_name;
    bool initialized;
    
    struct Entry {
        char key[16];      // Nome da chave
        char value[48];    // Valor (string ou convertido)
        bool used;
    };
    
    static const int MAX_ENTRIES = 8;  // Máximo 8 chaves por namespace
    Entry entries[MAX_ENTRIES];
    
    int findEntry(const char* key);
    int findFreeEntry();
    void loadFromEEPROM();
    void saveToEEPROM();
    uint16_t calculateAddress();
};

#endif