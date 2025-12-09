
#include <CMDCore.h>

// Cria o objeto CMD-C com nome do dispositivo
CMDCore core("ESP8266-Teste");

int main() {

  setup();
  loop();
  
  return 1;
}

void setup() {
  // Inicializa tudo: WiFi, WebServer, Factory Reset
  core.begin();
  
  Serial.println();
  Serial.println("=== Setup concluído ===");
  Serial.print("Dispositivo: ");
  Serial.println(core.isAPMode() ? "Modo AP" : "Conectado");
  Serial.print("IP: ");
  Serial.println(core.getIP());

  inicializarFuncaoRelays();
}

void loop() {
  // Processa WiFi + WebServer
  core.handle();
  
  // Seu código aqui
  // ...
}

void inicializarFuncaoRelays() {
  //.............
}
