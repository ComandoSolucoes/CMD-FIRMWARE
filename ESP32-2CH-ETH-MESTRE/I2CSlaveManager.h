#ifndef I2C_SLAVE_MANAGER_H
#define I2C_SLAVE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "Config.h"

// Estado de comunicação de um escravo
struct SlaveStatus {
    bool online;               // Escravo respondendo?
    unsigned long lastContact; // millis() da última comunicação OK
    uint32_t errorCount;       // Contador de erros consecutivos
    uint32_t totalErrors;      // Total acumulado de erros
};

// Dados de um escravo (8 entradas + 8 saídas)
struct SlaveData {
    bool outputs[NUM_SLAVE_CHANNELS]; // do1-do8 (o que enviamos)
    bool inputs[NUM_SLAVE_CHANNELS];  // di1-di8 (o que recebemos)
};

// Callback chamado quando uma entrada de escravo muda
typedef std::function<void(uint8_t slaveIndex, uint8_t inputIndex, bool state)> SlaveInputChangedCallback;

class I2CSlaveManager {
public:
    I2CSlaveManager();

    // Inicializa o barramento I2C e detecta escravos
    void begin();

    // Chamado no loop — executa ciclo de polling para todos os escravos
    void handle();

    // ==================== CONTROLE DE SAÍDAS ====================

    // Define saída de um escravo (slaveIndex: 0 ou 1, outputIndex: 0-7)
    void setOutput(uint8_t slaveIndex, uint8_t outputIndex, bool state);

    // Lê saída atual (estado lógico armazenado)
    bool getOutput(uint8_t slaveIndex, uint8_t outputIndex);

    // ==================== LEITURA DE ENTRADAS ====================

    // Lê entrada de um escravo (slaveIndex: 0 ou 1, inputIndex: 0-7)
    bool getInput(uint8_t slaveIndex, uint8_t inputIndex);

    // ==================== STATUS ====================

    bool isSlaveOnline(uint8_t slaveIndex);
    SlaveStatus getSlaveStatus(uint8_t slaveIndex);

    // ==================== CALLBACK ====================

    void setInputChangedCallback(SlaveInputChangedCallback cb);

private:
    SlaveData   slaveData[NUM_SLAVES];
    SlaveStatus slaveStatus[NUM_SLAVES];
    unsigned long lastPollTime;

    SlaveInputChangedCallback inputChangedCb;

    // Endereços I2C dos escravos
    const uint8_t slaveAddresses[NUM_SLAVES] = { I2C_SLAVE1_ADDR, I2C_SLAVE2_ADDR };

    // Executa um ciclo completo de comunicação com um escravo:
    // 1) Envia outputs em JSON
    // 2) Solicita e lê inputs em JSON
    bool communicateWithSlave(uint8_t slaveIndex);

    // Serializa outputs para JSON e envia via Wire
    bool sendOutputsJson(uint8_t slaveIndex);

    // Solicita e faz parse dos inputs JSON recebidos
    bool receiveInputsJson(uint8_t slaveIndex);

    // Detecta mudanças nos inputs e dispara callback
    void detectInputChanges(uint8_t slaveIndex, bool previousInputs[NUM_SLAVE_CHANNELS]);
};

#endif // I2C_SLAVE_MANAGER_H