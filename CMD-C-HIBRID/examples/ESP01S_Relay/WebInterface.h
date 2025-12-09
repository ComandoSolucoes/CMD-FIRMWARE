#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

#include <Arduino.h>
#include "Config.h"
#include "RelayManager.h"
#include "MqttHandler.h"

class WebInterface {
public:
    WebInterface(RelayManager* relayManager, MqttHandler* mqttHandler);
    
    // Geração de páginas HTML
    String generateDevicePage();
    
private:
    RelayManager* relay;
    MqttHandler* mqtt;
    
    // Geração de seções HTML
    String generateControlSection();
    String generateConfigSection();
    String generateStatsSection();
    
    // Utilitários
    String getRelayLogicName(RelayLogic logic);
    String getInitialStateName(InitialState state);
    String getOperationModeName(OperationMode mode);
};

#endif // WEB_INTERFACE_H
