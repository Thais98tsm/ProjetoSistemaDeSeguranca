/*Bibliotecas utilizadas*/
#include <Thread.h>
#include <ThreadController.h>
#include <Key.h>
#include <Keypad.h>
#include <Wire.h>
#include <avr/wdt.h>
#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

/*Constantes*/
const byte LINHAS = 4; // Linhas do teclado
const byte COLUNAS = 4; // Colunas do teclado
const int buzzer = 13;

/*Variáveis globais*/
int slave = -1;
int slavesAddresses[127];
int slavesNumber = -1;
int index = 0;
unsigned long millisSinceLastUpdate;
int storedPassword[8];
char dataEntry[12];
bool isSystemActive, confirmPassword = false;
LiquidCrystal_I2C lcd(39,16,2);

const char TECLAS_MATRIZ[LINHAS][COLUNAS] = { // Matriz de caracteres (mapeamento do teclado)
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

const byte PINOS_LINHAS[LINHAS] = {3, 4, 5, 6}; // Pinos de conexao com as linhas do teclado
const byte PINOS_COLUNAS[COLUNAS] = {7, 8, 9, 10}; // Pinos de conexao com as colunas do teclado

Keypad teclado_personalizado = Keypad(makeKeymap(TECLAS_MATRIZ), PINOS_LINHAS, PINOS_COLUNAS, LINHAS, COLUNAS); // Inicia teclado

Thread checkSensors;
Thread checkKeypad;
ThreadController controller;

void setup() {
  Wire.begin();        // join i2c bus (address optional for master)
  Serial.begin(9600);  // start serial for output
  pinMode(buzzer,OUTPUT);
  lcd.init();
  millisSinceLastUpdate = millis();
  wdt_enable(WDTO_4S);
  I2CScanner();
  readAddressesEEPROM();
  writeAddressesEEPROM();
  Serial.println("Iniciando...");
  checkSensors.setInterval(330);
  checkSensors.onRun(scanSensors);
  checkKeypad.onRun(checkKeyPressed);
  controller.add(&checkSensors);
  lcd.setCursor(0,0);
  lcd.backlight();
  readSystemStatusEEPROM();
  activateSystem();
}

void loop() {
  checkKeypad.run();
  if(isSystemActive){
    controller.run();
  }
}

/* Função que verifica se alguma tecla foi pressionada */
void checkKeyPressed() {
  char keyPressed = teclado_personalizado.getKey(); // Atribui a variavel a leitura do teclado
  if (keyPressed == 35) { // Se a tecla "#" foi pressionada
    dataEntry[index] = keyPressed;
    index++;
    lcd.clear();
    lcd.print("#");
    if (index > 9) {
        readPasswordEEPROM();
        if(checkPassword()){
          isSystemActive = !isSystemActive;
          writeSystemStatusEEPROM();
          activateSystem();
        }
    }
  }
  if (keyPressed == 65) { // Se a tecla "A" foi pressionada ativa a rotina de mudança de senha
    dataEntry[index] = keyPressed;
    index++;
    lcd.print("A");
    if (index > 9 && !confirmPassword) {
      readPasswordEEPROM();
        if(checkPassword()){
          confirmPassword = true;
          lcd.clear();
          lcd.print("Nova senha:");
          delay(2000);
          lcd.clear();
        } else {
          confirmPassword = false;
          lcd.clear();
          lcd.print("Senha incorreta.");
          delay(2000);
          activateSystem();
        }
    }
    if (index > 9 && confirmPassword) {
        writePasswordEEPROM();
        confirmPassword = false;
    }
    //Serial.println(keyPressed); // Imprime a tecla pressionada na porta serial
    if(!confirmPassword) {
      lcd.clear();
      lcd.print("Senha atual:");
      delay(2000);
      lcd.clear();
      lcd.print("A");
    }  
  }
  if(keyPressed == 67) {
    confirmPassword = false;
    clearArray();
    activateSystem();
  }
  
  if(keyPressed > 47 && keyPressed < 58) {// Números decimais
    if(dataEntry[0] == 35 || dataEntry[0] == 65){
      dataEntry[index] = keyPressed;
      lcd.print("*");
      index++;
    }
  }
}

/* Função responsável por fazer a varredura dos sensores */
void scanSensors() {
  slave = slave + 1;
  if(slave > (slavesNumber)){
    slave = 0;
    addressesUpdate();
  }
  if(Wire.requestFrom(slavesAddresses[slave], 1) != 0 ){  // request 1 byte from slave device
    while (Wire.available()) { // slave may send less than requested
      int slaveData = Wire.read(); // receive a byte as character
      if(slaveData > 10){
        Serial.println("Ativou sensor");
        wdt_disable();
        while(1){
          tone(buzzer,1500);   
          delay(500);
     
          //Desligando o buzzer.
          noTone(buzzer);
          delay(500);
        }
      }
    }
  } else {
     delay(4000);
  }
  wdt_reset();
  delay(500);
}

/*Função responsável por fazer um broadcast para identificar o endereço dos dispositivos conectados e ativos*/
void I2CScanner() {
  byte error, address;
  slavesNumber = -1;

  Serial.println("Fazendo o scanner");

  for(address = 1; address < 127; address++){
    if(address == 39){ //Endereço ao qual o display LCD está conectado, por isso é ignorado
      return;
    } else {
      Wire.beginTransmission(address);
      error = Wire.endTransmission();

      if(error == 0){
        slavesNumber = slavesNumber + 1;
        slavesAddresses[slavesNumber] = address;
      }
    }
  }
}


/*Função que verifica se houve atualizações nos endereços dos dispositivos a cada x tempo*/
void addressesUpdate() {
  if((millis() - millisSinceLastUpdate) > 60000){
    I2CScanner();
    millisSinceLastUpdate = millis();
  }
}

/*Função que armazena os endereços dos dispositivos conectados na EEPROM(memória não volátil)*/
void writeAddressesEEPROM() {
  int index = 1;
  EEPROM.write(0, slavesNumber);
  for(int i = 0; i <= slavesNumber; i++) {
    EEPROM.write(index, slavesAddresses[i]);
    index++;
  }
}

/*Função que faz a leitura dos endereços dos dispositivos conectados na EEPROM*/
void readAddressesEEPROM() {
  int slavesNumberStored = EEPROM.read(0);
  int slaveAddressStored;
  int index = 0;
  Serial.println("Lendo da EEPROM");
  if(slavesNumberStored > slavesNumber) {
    for(int i = 1; i <= slavesNumberStored; i++) {
      slaveAddressStored = EEPROM.read(i);
      if(slavesAddresses[index] != slaveAddressStored) {
        Serial.print("Foi detectado mal funcionamento em um dos sensores.");
        break;  
      }
      index++;
    }
    return;
  }
  if(slavesNumberStored < slavesNumber) {
    writeAddressesEEPROM();
    clearArray();
    return;
  }
  Serial.println("Todos os dispositivos estão ok!"); 
}

/*Função que armazena a senha na EEPROM(memória não volátil)*/
void writePasswordEEPROM() {
  int index = 1;
  for(int i = 31; i <= 38; i++) {
    EEPROM.write(i, convertCharToInt(dataEntry[index]));
    index++;
  }
  lcd.clear();
  clearArray();
  activateSystem();
}

/*Função que faz a leitura dos endereços dos dispositivos conectados na EEPROM*/
void readPasswordEEPROM() {
  int index = 0;
  Serial.println("Lendo senha da EEPROM");
  for(int i = 31; i <= 38; i++) {
    storedPassword[index] = EEPROM.read(i);
    index++;
  } 
}

/*Função que armazena o telefone na EEPROM(memória não volátil)*/
void writeSystemStatusEEPROM() {
  int index = 30;
  EEPROM.write(index, isSystemActive);
}

/*Função que faz a leitura dos endereços dos dispositivos conectados na EEPROM*/
void readSystemStatusEEPROM() {
  int index = 30;
  Serial.println("Lendo status da EEPROM");
  isSystemActive = EEPROM.read(index);
}

/*Função que verifica se a senha está certa*/
bool checkPassword() {
  int contador = 0;
  for(int i = 0; i < 8 ; i++){
    if((dataEntry[0] == 35 && dataEntry[9] == 35) || (dataEntry[0] == 65 && dataEntry[9] == 65)) {
      if(convertCharToInt(dataEntry[i + 1]) == storedPassword[i]){
        contador++;
      }
    }
  }
  if (contador == 8) {
    clearArray();
    return true;
  } else {
    clearArray();
    return false;
  }
}

/*Função que converte char para int*/
int convertCharToInt(char value){
  return value - '0';
}

/*Função que limpa o vetor de entrada de dados*/
void clearArray() {
  index = 0;
  for(int i = 0; i < 12 ; i++){
    dataEntry[i] = 0;
  }
}

/*Função que ativa ou desativa o sistema*/
void activateSystem() {
  if(isSystemActive){
    lcd.clear();
    lcd.print("Ativado");
    wdt_enable(WDTO_4S);
  }else{
    lcd.clear();
    lcd.print("Desativado");
    wdt_disable();
  }
}
