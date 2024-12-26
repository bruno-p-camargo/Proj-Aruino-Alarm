#include <Wire.h>
#include <VL53L0X.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

// Pinos RFID
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);

String conteudo; // Declarar 'conteudo' no escopo global

// Pino do LED
#define ledPin 7

// Inicialização do LCD I2C
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço I2C do display e tamanho (ajuste se necessário)

// Pinos A4 e A5 para leitura da distância
#define distanciaPinA4 A4
#define distanciaPinA5 A5

VL53L0X sensor; // Inicializa o sensor VL53L0X

unsigned long tempoAcesso = 0;
unsigned long tempoAtual = 0;
double distancia;
bool tagPresente = false;
bool acessoNegado = false;
bool acessoConcedido = false;

void setup() {
  Serial.begin(9600);

  pinMode(distanciaPinA4, INPUT);
  pinMode(distanciaPinA5, INPUT);
  pinMode(ledPin, OUTPUT);

  // Inicializa o sensor VL53L0X
  Wire.begin();
  if (!sensor.init()) {
    Serial.println("Falha na inicialização do sensor VL53L0X!");
    while (1);
  }
  sensor.setAddress(0x29); // Endereço I2C padrão
  sensor.setMeasurementTimingBudget(20000); // Ajuste do tempo de medição

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("Leitor pronto");

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Leitor RFID:");
}

void loop() {
  tempoAtual = millis();
  if (acessoConcedido && tempoAtual - tempoAcesso >= 3000) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Leitor RFID:");
    acessoConcedido = false;
  }

  sensor.setTimeout(500); 
  distancia = sensor.readRangeSingleMillimeters();
  
  if (sensor.timeoutOccurred()) {
    Serial.println("Timeout na leitura do sensor!");
    return;
  }

  Serial.print("Distancia: ");
  Serial.println(distancia);
  lcd.setCursor(0, 1);
  lcd.print("Distancia: ");
  lcd.print(distancia);
  lcd.print(" mm   ");

  if (distancia > 100) {
    if (!acessoConcedido) {
      tagPresente = verificarRFID(5000); 
      if (!tagPresente && !acessoNegado) {
        acessoNegado = true;
        acionarLED();
      }
    }
  } else if (acessoNegado) {
    acessoNegado = false;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Leitor RFID:");
  }
}

bool verificarRFID(unsigned long timeout) {
  unsigned long startTime = millis();
  while (millis() - startTime < timeout) {
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
      for (byte i = 0; i < mfrc522.uid.size; i++) {
        conteudo.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
        conteudo.concat(String(mfrc522.uid.uidByte[i], HEX));
      }
      conteudo.toUpperCase();
      if (conteudo.substring(1) == "D3 56 67 20") {
        LIBERADO();
        acessoConcedido = true;
        tempoAcesso = millis();
        return true;
      } else {
        conteudo = "";
        return false;
      }
    }
  }
  return false;
}

void LIBERADO() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Acesso Liberado!");
  digitalWrite(ledPin, HIGH);
  delay(500);
  digitalWrite(ledPin, LOW);
  conteudo = "";
}

void RECUSADO() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Acesso Recusado!");
  conteudo = "";
}

void acionarLED() {
  RECUSADO();
  unsigned long currentTime = millis();
  while (millis() - currentTime < 5000) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(500);
  }
}
