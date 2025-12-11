# ARDUINO IDE - Instruções de Instalação

## 📦 Bibliotecas Necessárias

Via **Gerenciador de Bibliotecas** (Sketch → Include Library → Manage Libraries):

1. **LoRa** by Sandeep Mistry (versão 0.8.0 ou superior)
2. **Adafruit NeoPixel** (versão 1.10.5 ou superior)
3. **ArduinoJson** (versão 7.x)

## 📂 Estrutura do Projeto

Crie uma pasta chamada `lora-receiver` e coloque todos os arquivos dentro:

```
lora-receiver/
├── lora-receiver.ino      ← Arduino abre este arquivo
├── Config.h
├── LoRaReceiver.h
├── LoRaReceiver.cpp
├── MqttPublisher.h
├── MqttPublisher.cpp
├── WebInterface.h
└── WebInterface.cpp
```

## 🔧 Configuração da Placa

1. **Placa**: ESP32 Dev Module
2. **Upload Speed**: 115200
3. **Flash Frequency**: 80MHz
4. **Partition Scheme**: Default ou Minimal SPIFFS

## 📚 Biblioteca CMD-C

A biblioteca CMD-C Core deve estar instalada em:
- `Arduino/libraries/cmd-c/`

Ou como você normalmente instala suas bibliotecas customizadas.

## ⚙️ Compilação

1. Abra `lora-receiver.ino` no Arduino IDE
2. Selecione a placa ESP32
3. Conecte o ESP32 via USB
4. Clique em **Upload** (→)

## 🔍 Serial Monitor

Configure para **115200 baud** para ver os logs:
```
[LORA-RX] Inicializando LoRa Receiver...
[LORA-RX] ✅ LoRa inicializado com sucesso!
[LORA-RX] 📥 LoRa RX: dist_cm=123.45 (RSSI: -45 dBm)
```

## ❌ Erros Comuns

### "CMDCore.h: No such file"
→ Biblioteca CMD-C não instalada. Copie para `Arduino/libraries/`

### "LoRa.h: No such file"
→ Instale biblioteca LoRa via Gerenciador de Bibliotecas

### "ArduinoJson.h version mismatch"
→ Certifique-se de usar ArduinoJson 7.x (não 6.x)

## 🎉 Pronto!

Após upload, conecte no WiFi `CMD-C-XXXXXX` e configure via portal web.
