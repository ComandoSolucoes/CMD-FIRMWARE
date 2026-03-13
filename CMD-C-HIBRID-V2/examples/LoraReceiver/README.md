# LoRa Receiver - CMD-C

Receptor LoRa que recebe mensagens e publica automaticamente no MQTT.

## 📋 Características

- ✅ Recebe mensagens LoRa (915 MHz)
- ✅ Publica automaticamente no MQTT quando recebe
- ✅ Armazena últimas 5 mensagens na memória
- ✅ Interface web `/device` com log de mensagens
- ✅ Configuração de parâmetros LoRa via web
- ✅ Estatísticas (RSSI, SNR, total recebido)
- ✅ LED feedback (pisca azul ao receber)

## 🔧 Hardware

- ESP32
- Módulo LoRa SX1276/SX1278
- NeoPixel (opcional, para feedback visual)

## 📡 Pinagem LoRa

```
LoRa Module    ESP32
-----------    -----
SCK       →    GPIO 18
MISO      →    GPIO 19
MOSI      →    GPIO 23
NSS       →    GPIO 5
RST       →    GPIO 14
DIO0      →    GPIO 26
```

## 🚀 Como Usar

1. **Upload do código**
2. **Conecte no WiFi do dispositivo** (CMD-C-XXXXXX)
3. **Configure WiFi e MQTT** via portal web
4. **Acesse `/device`** para ver log de mensagens

## 🌐 Interface Web

### `/device` - Log de Mensagens

Mostra as últimas 5 mensagens recebidas com:
- Payload completo
- RSSI (intensidade do sinal)
- SNR (relação sinal/ruído)
- Timestamp de recebimento

### Configurações

- **LoRa**: Spreading Factor, Bandwidth
- **MQTT**: Auto-publicação ON/OFF

## 📊 MQTT

Publica mensagens em: `cmd-c/{MAC}/lora/received`

Formato JSON:
```json
{
  "distance_cm": 123.45,
  "temperature": 25.5,
  "sensor_timestamp": 12345,
  "rssi": -45,
  "snr": 9.5,
  "received_at": 67890,
  "receiver": "LORA-RECEIVER"
}
```

## ⚙️ Configuração

Parâmetros LoRa devem ser **iguais** ao transmissor:
- Frequência: 915 MHz
- Spreading Factor: SF7 (padrão)
- Bandwidth: 125 kHz (padrão)
- Coding Rate: 4/5
- Sync Word: 0x12

## 📁 Estrutura

```
lora-receiver/
├── lora-receiver.ino
├── Config.h
├── LoRaReceiver.h
├── LoRaReceiver.cpp
├── MqttPublisher.h
├── MqttPublisher.cpp
├── WebInterface.h
├── WebInterface.cpp
└── platformio.ini
```

## 🔗 Dependências

- CMD-C Core (biblioteca)
- LoRa (sandeepmistry)
- Adafruit NeoPixel
- ArduinoJson 7.x

## 📝 Logs

Serial Monitor (115200 baud):
```
[LORA-RX] Inicializando LoRa Receiver...
[LORA-RX] ✅ LoRa inicializado com sucesso!
[LORA-RX] Frequência: 915.0 MHz
[LORA-RX] Spreading Factor: SF7
[LORA-RX] 📥 LoRa RX: dist_cm=123.45 (RSSI: -45 dBm, SNR: 9.5 dB)
[LORA-RX] 📡 MQTT: Publicado em lora/received
```
