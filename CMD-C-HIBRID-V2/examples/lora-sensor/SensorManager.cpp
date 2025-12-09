#include "SensorManager.h"

SensorManager::SensorManager() 
    : pixels(NUMPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800),
      ambientTemp(DEFAULT_AMBIENT_TEMP),
      readInterval(DEFAULT_READ_INTERVAL),
      autoReadEnabled(true),
      ledEnabled(true),
      lastReadTime(0),
      onReading(nullptr),
      totalReadings(0),
      validReadings(0),
      invalidReadings(0) {
    
    lastReading.distance_cm = 0;
    lastReading.temperature = ambientTemp;
    lastReading.valid = false;
    lastReading.timestamp = 0;
}

void SensorManager::begin() {
    LOG_INFO("Inicializando SensorManager...");
    
    // Abre Preferences
    prefs.begin(PREFS_NAMESPACE, false);
    
    // Carrega configurações
    loadConfig();
    
    // Configura pinos do sensor
    pinMode(PIN_TRIG, OUTPUT);
    pinMode(PIN_ECHO, INPUT);
    digitalWrite(PIN_TRIG, LOW);
    
    // Inicializa NeoPixel
    pixels.begin();
    pixels.clear();
    pixels.setBrightness(LED_BRIGHTNESS);
    pixels.show();
    
    // Feedback visual de inicialização
    if (ledEnabled) {
        ledSetColor(0, 80, 60);
        delay(200);
        ledShowIdle();
    }
    
    LOG_INFOF("Temperatura ambiente: %.1f °C", ambientTemp);
    LOG_INFOF("Intervalo de leitura: %d ms", readInterval);
    LOG_INFO("SensorManager inicializado!");
}

void SensorManager::handle() {
    // Leitura automática periódica
    if (autoReadEnabled) {
        unsigned long now = millis();
        if (now - lastReadTime >= readInterval) {
            lastReadTime = now;
            
            // Feedback LED
            if (ledEnabled) ledShowReading();
            
            // Lê sensor (com média)
            SensorReading reading = readAverage();
            lastReading = reading;
            
            // Chama callback
            if (onReading) {
                onReading(reading);
            }
            
            // Volta para cor idle
            if (ledEnabled) ledShowIdle();
        }
    }
}

// ==================== LEITURA DO SENSOR ====================

SensorReading SensorManager::readOnce() {
    SensorReading reading;
    reading.temperature = ambientTemp;
    reading.timestamp = millis();
    
    float distance = readDistanceCm();
    
    if (isnan(distance)) {
        reading.distance_cm = 0;
        reading.valid = false;
        updateStatistics(false);
    } else {
        reading.distance_cm = distance;
        reading.valid = true;
        updateStatistics(true);
    }
    
    return reading;
}

SensorReading SensorManager::readAverage(uint8_t numReadings) {
    if (numReadings < 1) numReadings = 1;
    
    float sum = 0;
    uint8_t validCount = 0;
    
    for (uint8_t i = 0; i < numReadings; i++) {
        float dist = readDistanceCm();
        if (!isnan(dist)) {
            sum += dist;
            validCount++;
        }
        if (i < numReadings - 1) delay(60); // 60ms entre leituras
    }
    
    SensorReading reading;
    reading.temperature = ambientTemp;
    reading.timestamp = millis();
    
    if (validCount > 0) {
        reading.distance_cm = sum / validCount;
        reading.valid = true;
        updateStatistics(true);
    } else {
        reading.distance_cm = 0;
        reading.valid = false;
        updateStatistics(false);
    }
    
    return reading;
}

float SensorManager::readDistanceCm() {
    // Pulso de trigger (mínimo 10µs)
    digitalWrite(PIN_TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(PIN_TRIG, HIGH);
    delayMicroseconds(12);
    digitalWrite(PIN_TRIG, LOW);
    
    // Mede largura do pulso no ECHO
    uint32_t duration = pulseIn(PIN_ECHO, HIGH, SENSOR_TIMEOUT_US);
    
    if (duration == 0) {
        return NAN; // Timeout
    }
    
    // Calcula distância com compensação de temperatura
    float soundSpeed = calculateSoundSpeed(); // m/s
    float seconds = duration / 1e6f;
    float meters = (seconds * soundSpeed) / 2.0f;
    
    return meters * 100.0f; // Converte para cm
}

float SensorManager::calculateSoundSpeed() {
    // Fórmula: c ≈ 331.45 + 0.61 * T (m/s)
    // Onde T é temperatura em °C
    return 331.45f + 0.61f * ambientTemp;
}

SensorReading SensorManager::getLastReading() {
    return lastReading;
}

void SensorManager::triggerReading() {
    lastReadTime = 0; // Força leitura no próximo handle()
}

// ==================== CONFIGURAÇÃO ====================

void SensorManager::setAmbientTemperature(float tempC) {
    ambientTemp = constrain(tempC, MIN_AMBIENT_TEMP, MAX_AMBIENT_TEMP);
    LOG_INFOF("Temperatura ambiente: %.1f °C", ambientTemp);
}

float SensorManager::getAmbientTemperature() {
    return ambientTemp;
}

void SensorManager::setReadInterval(uint32_t ms) {
    readInterval = constrain(ms, MIN_READ_INTERVAL, MAX_READ_INTERVAL);
    LOG_INFOF("Intervalo de leitura: %d ms", readInterval);
}

uint32_t SensorManager::getReadInterval() {
    return readInterval;
}

void SensorManager::setAutoRead(bool enabled) {
    autoReadEnabled = enabled;
    LOG_INFOF("Leitura automática: %s", enabled ? "HABILITADA" : "DESABILITADA");
}

bool SensorManager::isAutoReadEnabled() {
    return autoReadEnabled;
}

// ==================== LED FEEDBACK ====================

void SensorManager::ledSetColor(uint8_t r, uint8_t g, uint8_t b) {
    if (!ledEnabled) return;
    pixels.setPixelColor(0, pixels.Color(r, g, b));
    pixels.show();
}

void SensorManager::ledOff() {
    ledSetColor(LED_COLOR_OFF);
}

void SensorManager::ledShowIdle() {
    ledSetColor(LED_COLOR_IDLE);
}

void SensorManager::ledShowReading() {
    ledSetColor(LED_COLOR_READING);
}

void SensorManager::ledShowTransmit() {
    ledSetColor(LED_COLOR_TRANSMIT);
}

void SensorManager::ledShowError() {
    ledSetColor(LED_COLOR_ERROR);
}

void SensorManager::ledShowWiFiAP() {
    ledSetColor(LED_COLOR_WIFI_AP);
}

void SensorManager::ledBlink(uint8_t r, uint8_t g, uint8_t b, uint8_t times) {
    if (!ledEnabled) return;
    
    for (uint8_t i = 0; i < times; i++) {
        ledSetColor(r, g, b);
        delay(LED_BLINK_ON_MS);
        ledShowIdle();
        delay(LED_BLINK_OFF_MS);
    }
}

void SensorManager::setLedEnabled(bool enabled) {
    ledEnabled = enabled;
    if (!enabled) {
        ledOff();
    }
    LOG_INFOF("LED: %s", enabled ? "HABILITADO" : "DESABILITADO");
}

bool SensorManager::isLedEnabled() {
    return ledEnabled;
}

// ==================== CALLBACK ====================

void SensorManager::setReadingCallback(SensorReadingCallback callback) {
    onReading = callback;
}

// ==================== PERSISTÊNCIA ====================

void SensorManager::saveConfig() {
    prefs.putFloat(PREFS_AMBIENT_TEMP, ambientTemp);
    prefs.putUInt(PREFS_READ_INTERVAL, readInterval);
    prefs.putBool(PREFS_LED_ENABLED, ledEnabled);
    
    LOG_INFO("Configurações do sensor salvas");
}

void SensorManager::loadConfig() {
    ambientTemp = prefs.getFloat(PREFS_AMBIENT_TEMP, DEFAULT_AMBIENT_TEMP);
    readInterval = prefs.getUInt(PREFS_READ_INTERVAL, DEFAULT_READ_INTERVAL);
    ledEnabled = prefs.getBool(PREFS_LED_ENABLED, true);
    
    // Valida limites
    ambientTemp = constrain(ambientTemp, MIN_AMBIENT_TEMP, MAX_AMBIENT_TEMP);
    readInterval = constrain(readInterval, MIN_READ_INTERVAL, MAX_READ_INTERVAL);
    
    LOG_INFO("Configurações do sensor carregadas");
}

// ==================== ESTATÍSTICAS ====================

void SensorManager::updateStatistics(bool valid) {
    totalReadings++;
    if (valid) {
        validReadings++;
    } else {
        invalidReadings++;
    }
}

uint32_t SensorManager::getTotalReadings() {
    return totalReadings;
}

uint32_t SensorManager::getValidReadings() {
    return validReadings;
}

uint32_t SensorManager::getInvalidReadings() {
    return invalidReadings;
}

float SensorManager::getSuccessRate() {
    if (totalReadings == 0) return 0;
    return (100.0f * validReadings) / totalReadings;
}

void SensorManager::resetStatistics() {
    totalReadings = 0;
    validReadings = 0;
    invalidReadings = 0;
    LOG_INFO("Estatísticas resetadas");
}
