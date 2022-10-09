#include <Wire.h>

const int sensor_pin = 2; // Pino ao qual o sensor está conectado
int isSensorActive; // Status do sensor

void setup() {
  Wire.begin(7); // Se junta ao barramente i2c com endereço #7
  Serial.begin(9600);
  pinMode(sensor_pin, INPUT_PULLUP);
  Wire.onRequest(requestEvent); // Registra o envento
}

void loop() {
  delay(100);
}

// Função executada sempre quando requisitada pelo primário, registrada no setup()
void requestEvent() {
  isSensorActive = digitalRead(sensor_pin);
  if(isSensorActive){
    // A resposta consiste em dois números, o primeiro é o status do sensor, o segundo o tipo do sensor
    // 1 para MC-38, 2 para PIR e 3 para MQ-2
    Wire.write(11); // Resposta do secundário ao primário caso o sensor tenha sido ativado
  } else {
    Wire.write(01); // Resposta do secundário ao primário caso o sensor não tenha sido ativado
  }
}
