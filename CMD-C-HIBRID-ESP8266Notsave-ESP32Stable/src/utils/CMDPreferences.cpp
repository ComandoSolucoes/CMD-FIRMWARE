#include "CMDPreferences.h"

CMDPreferences::CMDPreferences() {
#if defined(ESP8266)
    initialized = false;
    needsCommit = false;
    
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].used = false;
        memset(entries[i].key,   0, sizeof(entries[i].key));
        memset(entries[i].value, 0, sizeof(entries[i].value));
    }
#endif
}

// ======================================================
// ESP32  ➜ Wrapper de Preferences
// ======================================================
#if defined(ESP32)

bool CMDPreferences::begin(const char* name, bool readOnly) {
#ifdef CMD_DEBUG_PREFS
    Serial.print(F("[PREFS-ESP32] Namespace: "));
    Serial.println(name);
#endif
    return prefs.begin(name, readOnly);
}

void CMDPreferences::end() {
    prefs.end();
}

void CMDPreferences::clear() {
    prefs.clear();
}

bool CMDPreferences::putString(const char* key, const String& value) {
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
    return prefs.putString(key, value) > 0;
}

String CMDPreferences::getString(const char* key, const String& defaultValue) {
    String v = prefs.getString(key, defaultValue);
#ifdef CMD_DEBUG_PREFS
    Serial.print(F("[PREFS] Get: "));
    Serial.print(key);
    Serial.print(F(" = "));
    if (strcmp(key, "password") == 0 || strcmp(key, "pass") == 0 || strcmp(key, "web_pass") == 0) {
        Serial.println(F("****"));
    } else {
        Serial.println(v);
    }
#endif
    return v;
}

bool CMDPreferences::putUChar(const char* key, uint8_t value) {
    return prefs.putUChar(key, value) > 0;
}

uint8_t CMDPreferences::getUChar(const char* key, uint8_t defaultValue) {
    return prefs.getUChar(key, defaultValue);
}

bool CMDPreferences::putUShort(const char* key, uint16_t value) {
    return prefs.putUShort(key, value) > 0;
}

uint16_t CMDPreferences::getUShort(const char* key, uint16_t defaultValue) {
    return prefs.getUShort(key, defaultValue);
}

bool CMDPreferences::putUInt(const char* key, uint32_t value) {
    return prefs.putUInt(key, value) > 0;
}

uint32_t CMDPreferences::getUInt(const char* key, uint32_t defaultValue) {
    return prefs.getUInt(key, defaultValue);
}

bool CMDPreferences::putULong(const char* key, unsigned long value) {
    return prefs.putULong(key, value) > 0;
}

unsigned long CMDPreferences::getULong(const char* key, unsigned long defaultValue) {
    return prefs.getULong(key, defaultValue);
}

bool CMDPreferences::putBool(const char* key, bool value) {
    return prefs.putBool(key, value) > 0;
}

bool CMDPreferences::getBool(const char* key, bool defaultValue) {
    return prefs.getBool(key, defaultValue);
}

bool CMDPreferences::putFloat(const char* key, float value) {
    return prefs.putFloat(key, value) > 0;
}

float CMDPreferences::getFloat(const char* key, float defaultValue) {
    return prefs.getFloat(key, defaultValue);
}

bool CMDPreferences::remove(const char* key) {
    return prefs.remove(key);
}

void CMDPreferences::commit() {
    // Preferences no ESP32 grava na hora; mantido só por compatibilidade
}

// ======================================================
// ESP8266  ➜ EEPROM emulada com namespaces
// ======================================================
#elif defined(ESP8266)

bool CMDPreferences::begin(const char* name, bool readOnly) {
    namespace_name = String(name);

    if (!initialized) {
        EEPROM.begin(EEPROM_SIZE);
        initialized = true;
    }

    loadFromEEPROM();

#ifdef CMD_DEBUG_PREFS
    uint16_t addr = getNamespaceAddress();
    int usedBytes = 1 + MAX_ENTRIES * (int)sizeof(Entry);

    Serial.print(F("[PREFS-ESP8266] Namespace: "));
    Serial.print(namespace_name);
    Serial.print(F(" | Endereço: "));
    Serial.print(addr);
    Serial.print(F("-"));
    Serial.print(addr + usedBytes - 1);
    Serial.print(F(" (usa ~"));
    Serial.print(usedBytes);
    Serial.println(F(" bytes de 512)"));
#endif

    (void)readOnly; // ignorado aqui
    return true;
}

void CMDPreferences::end() {
    if (needsCommit) {
        commitNow();
    }

    if (initialized) {
        EEPROM.end();
        initialized = false;
    }
}

void CMDPreferences::clear() {
    for (int i = 0; i < MAX_ENTRIES; i++) {
        entries[i].used = false;
        memset(entries[i].key,   0, sizeof(entries[i].key));
        memset(entries[i].value, 0, sizeof(entries[i].value));
    }
    needsCommit = true;
    commitNow();

#ifdef CMD_DEBUG_PREFS
    Serial.print(F("[PREFS] Namespace '"));
    Serial.print(namespace_name);
    Serial.println(F("' limpo"));
#endif
}

bool CMDPreferences::putString(const char* key, const String& value) {
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
        Serial.println(entries[idx].value);
    }
#endif

    return true;
}

String CMDPreferences::getString(const char* key, const String& defaultValue) {
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
}

bool CMDPreferences::putUChar(const char* key, uint8_t value) {
    return putString(key, String(value));
}

uint8_t CMDPreferences::getUChar(const char* key, uint8_t defaultValue) {
    String v = getString(key, String(defaultValue));
    return (uint8_t)v.toInt();
}

bool CMDPreferences::putUShort(const char* key, uint16_t value) {
    return putString(key, String(value));
}

uint16_t CMDPreferences::getUShort(const char* key, uint16_t defaultValue) {
    String v = getString(key, String(defaultValue));
    return (uint16_t)v.toInt();
}

bool CMDPreferences::putUInt(const char* key, uint32_t value) {
    return putString(key, String(value));
}

uint32_t CMDPreferences::getUInt(const char* key, uint32_t defaultValue) {
    String v = getString(key, String(defaultValue));
    return (uint32_t)v.toInt();
}

bool CMDPreferences::putULong(const char* key, unsigned long value) {
    return putString(key, String(value));
}

unsigned long CMDPreferences::getULong(const char* key, unsigned long defaultValue) {
    String v = getString(key, String(defaultValue));
    return (unsigned long)v.toInt();
}

bool CMDPreferences::putBool(const char* key, bool value) {
    return putString(key, value ? "1" : "0");
}

bool CMDPreferences::getBool(const char* key, bool defaultValue) {
    String v = getString(key, defaultValue ? "1" : "0");
    return v == "1";
}

bool CMDPreferences::putFloat(const char* key, float value) {
    return putString(key, String(value, 6));
}

float CMDPreferences::getFloat(const char* key, float defaultValue) {
    String v = getString(key, String(defaultValue, 6));
    return v.toFloat();
}

bool CMDPreferences::remove(const char* key) {
    int idx = findEntry(key);
    if (idx == -1) return false;

    entries[idx].used = false;
    memset(entries[idx].key,   0, sizeof(entries[idx].key));
    memset(entries[idx].value, 0, sizeof(entries[idx].value));

    needsCommit = true;

#ifdef CMD_DEBUG_PREFS
    Serial.print(F("[PREFS] Remove: "));
    Serial.println(key);
#endif

    return true;
}

void CMDPreferences::commit() {
    if (needsCommit) {
        commitNow();
    }
}

// ---------- privados (ESP8266) ----------

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
    // 4 blocos de 512 bytes
    if (namespace_name == "wifi") {
        return 0 * NAMESPACE_SIZE;
    } else if (namespace_name == "cmd-c" || namespace_name == "system") {
        return 1 * NAMESPACE_SIZE;
    } else if (namespace_name == "auth") {
        return 2 * NAMESPACE_SIZE;
    } else if (namespace_name == "cmd-mqtt" || namespace_name == "mqtt") {
        return 3 * NAMESPACE_SIZE;
    } else {
#ifdef CMD_DEBUG_PREFS
        Serial.print(F("[PREFS] AVISO: Namespace '"));
        Serial.print(namespace_name);
        Serial.println(F("' desconhecido! Usando bloco 0"));
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

    bool ok = EEPROM.commit();

    needsCommit = false;

    ESP.wdtEnable(WDTO_8S);
    delay(50);

#ifdef CMD_DEBUG_PREFS
    if (ok) {
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

#endif // ESP8266
