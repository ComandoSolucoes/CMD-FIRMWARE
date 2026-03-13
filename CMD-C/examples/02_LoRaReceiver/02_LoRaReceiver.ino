/*
  Receptor LoRa - CMD-C Core Library
  
  Este exemplo mostra como usar a biblioteca CMD-C com um receptor LoRa.
  A biblioteca cuida do WiFi e web server, você só precisa se preocupar
  com a lógica específica do LoRa.
  
  Hardware:
  - ESP32
  - Módulo LoRa (SX1276/RFM95)
  - NeoPixel (opcional, para feedback visual)
  
  Pinos LoRa:
  - SCK:  18
  - MISO: 19
  - MOSI: 23
  - SS:   5
  - RST:  14
  - DIO0: 26
*/

#include <CMDCore.h>
#include <LoRa.h>
#include <SPI.h>
#include <Adafruit_NeoPixel.h>

// ========== CONFIGURAÇÕES ==========
#define LORA_FREQUENCY 915E6
#define LORA_SS_PIN    5
#define LORA_RST_PIN   14
#define LORA_DIO0_PIN  26

#define PIN_LED       2
#define NUMPIXELS     1
#define LED_BRIGHTNESS 60

// ========== OBJETOS ==========
CMDCore core("ESPLora");
Adafruit_NeoPixel pixels(NUMPIXELS, PIN_LED, NEO_GRB + NEO_KHZ800);

// ========== VARIÁVEIS ==========
uint32_t packetCount = 0;
String lastPacket = "";
int lastRSSI = 0;
float lastSNR = 0;

// ========== SETUP ==========
void setup() {
  // 1. Inicializa CMD-C (WiFi + WebServer)
  core.begin();
  
  // 2. Inicializa LED
  pixels.begin();
  pixels.setBrightness(LED_BRIGHTNESS);
  pixels.clear();
  pixels.show();
  
  // 3. Inicializa LoRa
  SPI.begin(18, 19, 23, LORA_SS_PIN);
  LoRa.setPins(LORA_SS_PIN, LORA_RST_PIN, LORA_DIO0_PIN);
  
  Serial.println();
  Serial.print("Inicializando LoRa (");
  Serial.print(LORA_FREQUENCY / 1e6);
  Serial.println(" MHz)...");
  
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("❌ Falha ao iniciar LoRa!");
    while (true) {
      blinkRed();
      delay(1000);
    }
  }
  
  Serial.println("✅ LoRa inicializado!");
  Serial.println("Aguardando pacotes...");
  
  // 4. Adiciona rota customizada para status LoRa
  core.addRoute("/lora", handleLoRaPage);
  
  // 5. LED inicial baseado no WiFi
  updateLED();
}

// ========== LOOP ==========
void loop() {
  // Processa CMD-C (WiFi + WebServer)
  core.handle();
  
  // Processa recepção LoRa
  handleLoRa();
  
  // Atualiza LED
  updateLED();
}

// ========== FUNÇÕES LORA ==========
void handleLoRa() {
  int packetSize = LoRa.parsePacket();
  if (!packetSize) return;
  
  // Lê pacote
  lastPacket = "";
  while (LoRa.available()) {
    lastPacket += (char)LoRa.read();
  }
  
  // Captura métricas
  lastRSSI = LoRa.packetRssi();
  lastSNR = LoRa.packetSnr();
  packetCount++;
  
  // Log
  Serial.println();
  Serial.println("📡 Pacote recebido:");
  Serial.print("  Data: ");
  Serial.println(lastPacket);
  Serial.print("  RSSI: ");
  Serial.print(lastRSSI);
  Serial.println(" dBm");
  Serial.print("  SNR: ");
  Serial.print(lastSNR);
  Serial.println(" dB");
  Serial.print("  Total: ");
  Serial.println(packetCount);
  
  // Feedback visual
  blinkBlue(2);
}

// ========== FUNÇÕES LED ==========
void updateLED() {
  static uint32_t lastUpdate = 0;
  if (millis() - lastUpdate < 100) return;
  lastUpdate = millis();
  
  if (core.isConnected()) {
    // Verde: conectado
    pixels.setPixelColor(0, pixels.Color(0, 50, 0));
  } else {
    // Vermelho pulsante: AP ativo
    float brightness = (sin(millis() / 500.0) + 1.0) / 2.0;
    uint8_t r = 50 * brightness;
    pixels.setPixelColor(0, pixels.Color(r, 0, 0));
  }
  pixels.show();
}

void blinkBlue(uint8_t times) {
  for (uint8_t i = 0; i < times; i++) {
    pixels.setPixelColor(0, pixels.Color(0, 0, 100));
    pixels.show();
    delay(100);
    updateLED();
    delay(100);
  }
}

void blinkRed() {
  pixels.setPixelColor(0, pixels.Color(100, 0, 0));
  pixels.show();
  delay(200);
  pixels.clear();
  pixels.show();
}

// ========== PÁGINA WEB CUSTOMIZADA ==========
void handleLoRaPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>Receptor LoRa</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f5f5f5}";
  html += ".card{background:white;padding:20px;border-radius:8px;margin-bottom:20px;box-shadow:0 2px 4px rgba(0,0,0,0.1)}";
  html += "h1{color:#667eea}";
  html += ".stat{display:flex;justify-content:space-between;padding:10px;border-bottom:1px solid #eee}";
  html += ".value{font-weight:bold;color:#667eea}";
  html += "</style>";
  html += "</head><body>";
  
  html += "<h1>📡 Receptor LoRa CMD-C</h1>";
  
  html += "<div class='card'>";
  html += "<h2>Status WiFi</h2>";
  html += "<div class='stat'><span>Estado:</span><span class='value'>";
  html += core.isConnected() ? "Conectado" : "AP Ativo";
  html += "</span></div>";
  html += "<div class='stat'><span>SSID:</span><span class='value'>";
  html += core.isConnected() ? core.getSSID() : core.getMacAddress();
  html += "</span></div>";
  html += "<div class='stat'><span>IP:</span><span class='value'>";
  html += core.getIP();
  html += "</span></div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h2>Estatísticas LoRa</h2>";
  html += "<div class='stat'><span>Pacotes Recebidos:</span><span class='value'>";
  html += String(packetCount);
  html += "</span></div>";
  html += "<div class='stat'><span>Último Pacote:</span><span class='value'>";
  html += lastPacket.length() > 0 ? lastPacket : "—";
  html += "</span></div>";
  html += "<div class='stat'><span>RSSI:</span><span class='value'>";
  html += lastRSSI != 0 ? String(lastRSSI) + " dBm" : "—";
  html += "</span></div>";
  html += "<div class='stat'><span>SNR:</span><span class='value'>";
  html += lastSNR != 0 ? String(lastSNR) + " dB" : "—";
  html += "</span></div>";
  html += "</div>";
  
  html += "<div class='card'>";
  html += "<h2>Sistema</h2>";
  html += "<div class='stat'><span>Uptime:</span><span class='value'>";
  html += String(core.getUptime()) + "s";
  html += "</span></div>";
  html += "<div class='stat'><span>Versão:</span><span class='value'>";
  html += core.getVersion();
  html += "</span></div>";
  html += "</div>";
  
  html += "</body></html>";
  
}
