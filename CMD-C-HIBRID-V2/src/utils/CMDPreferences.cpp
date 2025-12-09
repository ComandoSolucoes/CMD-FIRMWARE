#include "CMDPreferences.h"

CMDPreferences::CMDPreferences() {
#if defined(ESP8266)
    initialized = false;
    needsCommit = false;
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].used = false;
        memset(entries[i].key, 0, sizeof(entries[i].key));
        memset(entries[i].value, 0, sizeof(entries[i].value));
    }
#endif
}

// ═══════════════════════════════════════════════════════════════════
// MÉTODOS PÚBLICOS - HÍBRIDOS
// ═══════════════════════════════════════════════════════════════════

bool CMDPreferences::begin(const char* name, bool readOnly) {
#if defined(ESP32)
    // ───────────────────────────────────────────────────────────────
    // ESP32: Usa Preferences nativo (simples e direto!)
    // ───────────────────────────────────────────────────────────────
    #ifdef CMD_DEBUG_PREFS
        Serial.print(F("[PREFS-ESP32] Namespace: "));
        Serial.println(name);
    #endif
    
    return prefs.begin(name, readOnly);
    
#elif defined(ESP8266)
    // ───────────────────────────────────────────────────────────────
    // ESP8266: Usa EEPROM com mapeamento manual otimizado
    // ───────────────────────────────────────────────────────────────
    namespace_name = String(name);
    
    if (!initialized) {
        EEPROM.begin(EEPROM_SIZE);
        initialized = true;
    }
    
    loadFromEEPROM();
    
    #ifdef CMD_DEBUG_PREFS
        uint16_t addr = getNamespaceAddress();
        int blockSize = 0;
        
        if (namespace_name == "wifi") {
            blockSize = 250;
        } else if (namespace_name == "system" || namespace_name == "cmd-c") {
            blockSize = 100;
        } else if (namespace_name == "auth") {
            blockSize = 100;
        } else if (namespace_name == "mqtt") {
            blockSize = 62;
        }
        
        Serial.print(F("[PREFS-ESP8266] Namespace: "));
        Serial.print(namespace_name);
        Serial.print(F(" | Endereço: "));
        Serial.print(addr);
        Serial.print(F("-"));
        Serial.print(addr + blockSize - 1);
        Serial.print(F(" ("));
        Serial.print(blockSize);
        Serial.println(F(" bytes)"));
    #endif
    
    return true;
#endif
}

void CMDPreferences::end() {
#if defined(ESP32)
    prefs.end();
    
#elif defined(ESP8266)
    if (needsCommit) {
        commitNow();
    }
    
    if (initialized) {
        EEPROM.end();
        initialized = false;
    }
#endif
}

void CMDPreferences::clear() {
#if defined(ESP32)
    prefs.clear();
    
#elif defined(ESP8266)
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].used = false;
        memset(entries[i].key, 0, sizeof(entries[i].key));
        memset(entries[i].value, 0, sizeof(entries[i].value));
    }
    needsCommit = true;
    commitNow();
    
    #ifdef CMD_DEBUG_PREFS
        Serial.print(F("[PREFS] Namespace '"));
        Serial.print(namespace_name);
        Serial.println(F("' limpo"));
    #endif
#endif
}

// ═══════════════════════════════════════════════════════════════════
// STRING
// ═══════════════════════════════════════════════════════════════════

bool CMDPreferences::putString(const char* key, const String& value) {
#if defined(ESP32)
    return prefs.putString(key, value) > 0;
    
#elif defined(ESP8266)
    if (strlen(key) >= KEY_SIZE) {
        #ifdef CMD_DEBUG_PREFS
            Serial.print(F("[PREFS] ERRO: Key '"));
            Serial.print(key);
            Serial.println(F("' muito grande!"));
        #endif
        return false;
    }
    
    if (value.length() >= VALUE_SIZE) {
        #ifdef CMD_DEBUG_PREFS
            Serial.print(F("[PREFS] AVISO: Value truncado de "));
            Serial.print(value.length());
            Serial.print(F(" para "));
            Serial.println(VALUE_SIZE - 1);
        #endif
    }
    
    int idx = findEntry(key);
    if (idx == -1) {
        idx = findFreeEntry();
        if (idx == -1) {
            #ifdef CMD_DEBUG_PREFS
                Serial.println(F("[PREFS] ERRO: Sem espaço livre!"));
            #endif
            return false;
        }
    }
    
    entries[idx].used = true;
    strncpy(entries[idx].key, key, KEY_SIZE - 1);
    entries[idx].key[KEY_SIZE - 1] = '\0';
    
    strncpy(entries[idx].value, value.c_str(), VALUE_SIZE - 1);
    entries[idx].value[VALUE_SIZE - 1] = '\0';
    
    needsCommit = true;
    
    #ifdef CMD_DEBUG_PREFS
        Serial.print(F("[PREFS] Put: "));
        Serial.print(key);
        Serial.print(F(" = "));
        if (strcmp(key, "password") == 0 || strcmp(key, "pass") == 0 || strcmp(key, "web_pass") == 0) {
            Serial.println(F("****"));
        } else {
            Serial.println(value);
        }
    #endif
    
    return true;
#endif
}

String CMDPreferences::getString(const char* key, const String& defaultValue) {
#if defined(ESP32)
    return prefs.getString(key, defaultValue);
    
#elif defined(ESP8266)
    int idx = findEntry(key);
    if (idx == -1) {
        #ifdef CMD_DEBUG_PREFS
            Serial.print(F("[PREFS] Get: "));
            Serial.print(key);
            Serial.println(F(" = (default)"));
        #endif
        return defaultValue;
    }
    
    #ifdef CMD_DEBUG_PREFS
        Serial.print(F("[PREFS] Get: "));
        Serial.print(key);
        Serial.print(F(" = "));
        if (strcmp(key, "password") == 0 || strcmp(key, "pass") == 0 || strcmp(key, "web_pass") == 0) {
            Serial.println(F("****"));
        } else {
            Serial.println(entries[idx].value);
        }
    #endif
    
    return String(entries[idx].value);
#endif
}

// ═══════════════════════════════════════════════════════════════════
// TIPOS NUMÉRICOS (Implementação idêntica para ESP32 e ESP8266)
// ═══════════════════════════════════════════════════════════════════

bool CMDPreferences::putUChar(const char* key, uint8_t value) {
#if defined(ESP32)
    return prefs.putUChar(key, value) > 0;
#else
    return putString(key, String(value));
#endif
}

uint8_t CMDPreferences::getUChar(const char* key, uint8_t defaultValue) {
#if defined(ESP32)
    return prefs.getUChar(key, defaultValue);
#else
    String val = getString(key, String(defaultValue));
    return (uint8_t)val.toInt();
#endif
}

bool CMDPreferences::putUShort(const char* key, uint16_t value) {
#if defined(ESP32)
    return prefs.putUShort(key, value) > 0;
#else
    return putString(key, String(value));
#endif
}

uint16_t CMDPreferences::getUShort(const char* key, uint16_t defaultValue) {
#if defined(ESP32)
    return prefs.getUShort(key, defaultValue);
#else
    String val = getString(key, String(defaultValue));
    return (uint16_t)val.toInt();
#endif
}

bool CMDPreferences::putUInt(const char* key, uint32_t value) {
#if defined(ESP32)
    return prefs.putUInt(key, value) > 0;
#else
    return putString(key, String(value));
#endif
}

uint32_t CMDPreferences::getUInt(const char* key, uint32_t defaultValue) {
#if defined(ESP32)
    return prefs.getUInt(key, defaultValue);
#else
    String val = getString(key, String(defaultValue));
    return (uint32_t)val.toInt();
#endif
}

bool CMDPreferences::putULong(const char* key, unsigned long value) {
#if defined(ESP32)
    return prefs.putULong(key, value) > 0;
#else
    return putString(key, String(value));
#endif
}

unsigned long CMDPreferences::getULong(const char* key, unsigned long defaultValue) {
#if defined(ESP32)
    return prefs.getULong(key, defaultValue);
#else
    String val = getString(key, String(defaultValue));
    return (unsigned long)val.toInt();
#endif
}

bool CMDPreferences::putBool(const char* key, bool value) {
#if defined(ESP32)
    return prefs.putBool(key, value) > 0;
#else
    return putString(key, value ? "1" : "0");
#endif
}

bool CMDPreferences::getBool(const char* key, bool defaultValue) {
#if defined(ESP32)
    return prefs.getBool(key, defaultValue);
#else
    String val = getString(key, defaultValue ? "1" : "0");
    return val == "1";
#endif
}

bool CMDPreferences::putFloat(const char* key, float value) {
#if defined(ESP32)
    return prefs.putFloat(key, value) > 0;
#else
    return putString(key, String(value, 6));
#endif
}

float CMDPreferences::getFloat(const char* key, float defaultValue) {
#if defined(ESP32)
    return prefs.getFloat(key, defaultValue);
#else
    String val = getString(key, String(defaultValue, 6));
    return val.toFloat();
#endif
}

bool CMDPreferences::remove(const char* key) {
#if defined(ESP32)
    return prefs.remove(key);
    
#elif defined(ESP8266)
    int idx = findEntry(key);
    if (idx == -1) return false;
    
    entries[idx].used = false;
    memset(entries[idx].key, 0, sizeof(entries[idx].key));
    memset(entries[idx].value, 0, sizeof(entries[idx].value));
    
    needsCommit = true;
    
    #ifdef CMD_DEBUG_PREFS
        Serial.print(F("[PREFS] Remove: "));
        Serial.println(key);
    #endif
    
    return true;
#endif
}

void CMDPreferences::commit() {
#if defined(ESP32)
    // ESP32 Preferences não precisa de commit explícito
    // (mas mantemos por compatibilidade)
    
#elif defined(ESP8266)
    if (needsCommit) {
        commitNow();
    }
#endif
}

// ═══════════════════════════════════════════════════════════════════
// MÉTODOS PRIVADOS - APENAS ESP8266
// ═══════════════════════════════════════════════════════════════════

#if defined(ESP8266)

int CMDPreferences::findEntry(const char* key) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (entries[i].used && strcmp(entries[i].key, key) == 0) {
            return i;
        }
    }
    return -1;
}

int CMDPreferences::findFreeEntry() {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        if (!entries[i].used) {
            return i;
        }
    }
    return -1;
}

uint16_t CMDPreferences::getNamespaceAddress() {
    // Namespaces fixos otimizados
    if (namespace_name == "wifi") {
        return 0;      // 0-249 (250 bytes)
    } else if (namespace_name == "cmd-c" || namespace_name == "system") {
        return 250;    // 250-349 (100 bytes)
    } else if (namespace_name == "auth") {
        return 350;    // 350-449 (100 bytes)
    } else if (namespace_name == "mqtt") {
        return 450;    // 450-511 (62 bytes)
    } else {
        #ifdef CMD_DEBUG_PREFS
            Serial.print(F("[PREFS] AVISO: Namespace '"));
            Serial.print(namespace_name);
            Serial.println(F("' desconhecido!"));
        #endif
        return 0;
    }
}

void CMDPreferences::loadFromEEPROM() {
    uint16_t addr = getNamespaceAddress();
    
    uint8_t magic = EEPROM.read(addr);
    
    if (magic != 0xAA) {
        #ifdef CMD_DEBUG_PREFS
            Serial.print(F("[PREFS] Namespace '"));
            Serial.print(namespace_name);
            Serial.println(F("' vazio, inicializando..."));
        #endif
        
        for (int i = 0; i < MAX_ENTRIES; i++) {
            entries[i].used = false;
        }
        
        EEPROM.write(addr, 0xAA);
        EEPROM.commit();
        return;
    }
    
    addr++;
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        EEPROM.get(addr, entries[i]);
        addr += sizeof(Entry);
    }
    
    #ifdef CMD_DEBUG_PREFS
        int count = 0;
        for (int i = 0; i < MAX_ENTRIES; i++) {
            if (entries[i].used) count++;
        }
        Serial.print(F("[PREFS] Carregado "));
        Serial.print(count);
        Serial.print(F(" de "));
        Serial.print(MAX_ENTRIES);
        Serial.println(F(" entradas"));
    #endif
}

void CMDPreferences::commitNow() {
    if (!needsCommit) return;
    
    #ifdef CMD_DEBUG_PREFS
        Serial.println(F("[PREFS] Commit iniciado..."));
    #endif
    
    ESP.wdtDisable();
    
    uint16_t addr = getNamespaceAddress();
    
    EEPROM.write(addr, 0xAA);
    addr++;
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        EEPROM.put(addr, entries[i]);
        addr += sizeof(Entry);
    }
    
    bool success = EEPROM.commit();
    
    needsCommit = false;
    
    ESP.wdtEnable(WDTO_8S);
    
    delay(50);
    
    #ifdef CMD_DEBUG_PREFS
        if (success) {
            Serial.println(F("[PREFS] ✅ Commit OK"));
            
            int count = 0;
            for (int i = 0; i < MAX_ENTRIES; i++) {
                if (entries[i].used) count++;
            }
            Serial.print(F("[PREFS] Salvo "));
            Serial.print(count);
            Serial.println(F(" entradas"));
        } else {
            Serial.println(F("[PREFS] ❌ Commit FALHOU!"));
        }
    #endif
}

#endif