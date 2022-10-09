#include <Wire.h>

const int pinoA7 = A7; // Pino ao qual o sensor está conectado
const int leitura_sensor = 300; // Valor até o qual é aceitavel a leitura, acima disso o sensor é ativado
int isSensorActive; // Status do sensor

void setup() {
  Wire.begin(3); // Se junta ao barramente i2c com endereço #7
  pinMode(pinoA7, INPUT);
  Wire.onRequest(requestEvent); // Registra o envento
}

void loop() {
  delay(100);
  
}

// Função executada sempre quando requisitada pelo primário, registrada no setup()
void requestEvent() {
  if(analogRead(pinoA7) > leitura_sensor){
    isSensorActive = HIGH;
  } else {
    isSensorActive = LOW;
  }
  if(isSensorActive){
    // A resposta consiste em dois números, o primeiro é o status do sensor, o segundo o tipo do sensor
    // 1 para MC-38, 2 para PIR e 3 para MQ-2
    Wire.write(13); // Resposta do secundário ao primário caso o sensor tenha sido ativado
  } else {
    Wire.write(03); // Resposta do secundário ao primário caso o sensor não tenha sido ativado
  }
}
