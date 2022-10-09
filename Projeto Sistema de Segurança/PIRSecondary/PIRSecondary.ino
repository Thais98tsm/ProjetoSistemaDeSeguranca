#include <Wire.h>

const int pino_pir = 2; // Pino ao qual o sensor está conectado
int isSensorActive; // Status do sensor

void setup() {
  Wire.begin(5); // Se junta ao barramente i2c com endereço #5
  Serial.begin(9600);
  pinMode(pino_pir, INPUT);
  Wire.onRequest(requestEvent); // Registra o envento
}

void loop() {
  delay(100); 
}

// Função executada sempre quando requisitada pelo primário, registrada no setup()
void requestEvent() {
  isSensorActive = digitalRead(pino_pir);
  if(isSensorActive){
    // A resposta consiste em dois números, o primeiro é o status do sensor 0 para não ativado e 1 para ativado
    // O segundo o tipo do sensor, 1 para MC-38, 2 para PIR e 3 para MQ-2
    Wire.write(12); // Resposta do secundário ao primário caso o sensor tenha sido ativado
  } else {
    Wire.write(02); // Resposta do secundário ao primário caso o sensor não tenha sido ativado
  }
}
