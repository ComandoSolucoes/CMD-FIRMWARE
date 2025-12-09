#include "CMDPreferences.h"

CMDPreferences::CMDPreferences() : initialized(false) {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].used = false;
        memset(entries[i].key, 0, sizeof(entries[i].key));
        memset(entries[i].value, 0, sizeof(entries[i].value));
    }
}

bool CMDPreferences::begin(const char* name, bool readOnly) {
    namespace_name = String(name);
    
    if (!initialized) {
        EEPROM.begin(EEPROM_SIZE);
        initialized = true;
    }
    
    loadFromEEPROM();
    return true;
}

void CMDPreferences::end() {
    if (initialized) {
        EEPROM.end();
        initialized = false;
    }
}

void CMDPreferences::clear() {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].used = false;
        memset(entries[i].key, 0, sizeof(entries[i].key));
        memset(entries[i].value, 0, sizeof(entries[i].value));
    }
    saveToEEPROM();
}

// String
bool CMDPreferences::putString(const char* key, const String& value) {
    int idx = findEntry(key);
    if (idx == -1) {
        idx = findFreeEntry();
        if (idx == -1) return false;
    }
    
    entries[idx].used = true;
    strncpy(entries[idx].key, key, sizeof(entries[idx].key) - 1);
    strncpy(entries[idx].value, value.c_str(), sizeof(entries[idx].value) - 1);
    
    saveToEEPROM();
    return true;
}

String CMDPreferences::getString(const char* key, const String& defaultValue) {
    int idx = findEntry(key);
    if (idx == -1) return defaultValue;
    return String(entries[idx].value);
}

// UChar
bool CMDPreferences::putUChar(const char* key, uint8_t value) {
    return putString(key, String(value));
}

uint8_t CMDPreferences::getUChar(const char* key, uint8_t defaultValue) {
    String val = getString(key, String(defaultValue));
    return (uint8_t)val.toInt();
}

// UShort
bool CMDPreferences::putUShort(const char* key, uint16_t value) {
    return putString(key, String(value));
}

uint16_t CMDPreferences::getUShort(const char* key, uint16_t defaultValue) {
    String val = getString(key, String(defaultValue));
    return (uint16_t)val.toInt();
}

// UInt
bool CMDPreferences::putUInt(const char* key, uint32_t value) {
    return putString(key, String(value));
}

uint32_t CMDPreferences::getUInt(const char* key, uint32_t defaultValue) {
    String val = getString(key, String(defaultValue));
    return (uint32_t)val.toInt();
}

// ULong
bool CMDPreferences::putULong(const char* key, unsigned long value) {
    return putString(key, String(value));
}

unsigned long CMDPreferences::getULong(const char* key, unsigned long defaultValue) {
    String val = getString(key, String(defaultValue));
    return (unsigned long)val.toInt();
}

// Bool
bool CMDPreferences::putBool(const char* key, bool value) {
    return putString(key, value ? "1" : "0");
}

bool CMDPreferences::getBool(const char* key, bool defaultValue) {
    String val = getString(key, defaultValue ? "1" : "0");
    return val == "1";
}

bool CMDPreferences::remove(const char* key) {
    int idx = findEntry(key);
    if (idx == -1) return false;
    
    entries[idx].used = false;
    memset(entries[idx].key, 0, sizeof(entries[idx].key));
    memset(entries[idx].value, 0, sizeof(entries[idx].value));
    
    saveToEEPROM();
    return true;
}

// Privados
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

uint16_t CMDPreferences::calculateAddress() {
    // Hash simples do namespace para endereço base
    uint16_t hash = 0;
    for (size_t i = 0; i < namespace_name.length(); i++) {
        hash += namespace_name[i];
    }
    return (hash % 4) * 128;  // 4 namespaces de 128 bytes
}

void CMDPreferences::loadFromEEPROM() {
    uint16_t addr = calculateAddress();
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        EEPROM.get(addr, entries[i]);
        addr += sizeof(Entry);
    }
}

void CMDPreferences::saveToEEPROM() {
    uint16_t addr = calculateAddress();
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        EEPROM.put(addr, entries[i]);
        addr += sizeof(Entry);
    }
    
    EEPROM.commit();
}