// Código Arduino Atualizado para Wokwi: HC-SR04, DHT22, LDR e LCD I2C (Exibição Alternada)
// Autor: Manus AI
// Data: 03/06/2025

// --- Inclusão de Bibliotecas ---
#include <DHT.h>             // Biblioteca para o sensor DHT
#include <Wire.h>            // Biblioteca para comunicação I2C
#include <LiquidCrystal_I2C.h> // Biblioteca para o LCD I2C

// --- Definição dos Pinos ---

// Sensor Ultrassônico HC-SR04 (4 Pinos)
const int PINO_TRIG = 9;
const int PINO_ECHO = 10;

// Sensor de Temperatura e Umidade DHT22
const int PINO_DHT = 7;
#define DHTTYPE DHT22

// Sensor de Luminosidade (LDR)
const int PINO_LDR = A0;

// LEDs de Status (Nível da Água)
const int PINO_LED_VERDE = 2;
const int PINO_LED_AMARELO = 3;
const int PINO_LED_VERMELHO = 4;

// Atuador Sonoro
const int PINO_BUZZER = 5;

// --- Configurações do LCD ---
LiquidCrystal_I2C lcd(0x27, 16, 2); // Endereço 0x27, 16x2

// --- Constantes e Variáveis Globais ---
const float LIMIAR_ALERTA = 15.0;
const float LIMIAR_ATENCAO = 30.0;

long duracaoEcho;
float distanciaCm;
float temperaturaC;
float umidade;
int valorLDR;
String statusNivel = "Normal";

// Variáveis para controle de tempo da exibição no LCD
unsigned long tempoAnteriorLCD = 0;
const long intervaloLCD = 1500; // Tempo que cada tela fica visível (1.5 segundos)
int telaAtualLCD = 0; // 0: Nível, 1: Temp/Umid, 2: Luz

// --- Inicialização dos Sensores ---
DHT dht(PINO_DHT, DHTTYPE);

// --- Função setup() ---
void setup() {
  pinMode(PINO_LED_VERDE, OUTPUT);
  pinMode(PINO_LED_AMARELO, OUTPUT);
  pinMode(PINO_LED_VERMELHO, OUTPUT);
  pinMode(PINO_BUZZER, OUTPUT);
  pinMode(PINO_TRIG, OUTPUT);
  pinMode(PINO_ECHO, INPUT);

  Serial.begin(9600);
  Serial.println("Sistema Monit. Wokwi v5 (Alternado) Iniciado");

  dht.begin();

  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Iniciando...");

  digitalWrite(PINO_LED_VERDE, LOW);
  digitalWrite(PINO_LED_AMARELO, LOW);
  digitalWrite(PINO_LED_VERMELHO, LOW);
  noTone(PINO_BUZZER);
  delay(1000);
  lcd.clear(); // Limpa após "Iniciando..."
}

// --- Função loop() ---
void loop() {
  // --- Leituras dos Sensores ---
  // 1. Medição da Distância (HC-SR04)
  digitalWrite(PINO_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PINO_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PINO_TRIG, LOW);
  duracaoEcho = pulseIn(PINO_ECHO, HIGH);
  distanciaCm = duracaoEcho * 0.0343 / 2.0;

  // 2. Leitura do Sensor DHT22
  // Leituras do DHT22 podem levar até 2 segundos, fazemos com menos frequência
  // Usaremos os valores da última leitura válida se a atual falhar
  float tempUmidade = dht.readHumidity();
  float tempTemperatura = dht.readTemperature();
  if (!isnan(tempUmidade)) {
      umidade = tempUmidade;
  }
   if (!isnan(tempTemperatura)) {
      temperaturaC = tempTemperatura;
  }

  // 3. Leitura do Sensor de Luminosidade (LDR)
  valorLDR = analogRead(PINO_LDR);

  // --- Impressão no Monitor Serial (Debug) ---
  Serial.println("--- Nova Leitura Ciclo ---");
  if (isnan(umidade) || isnan(temperaturaC)) { Serial.println("Falha DHT"); } else { Serial.print("U:"); Serial.print(umidade); Serial.print("% T:"); Serial.print(temperaturaC); Serial.println("*C"); }
  Serial.print("Distancia: "); Serial.print(distanciaCm); Serial.println(" cm");
  Serial.print("Luz (LDR): "); Serial.println(valorLDR);

  // --- Lógica de Decisão (Nível da Água) e Controle dos Atuadores ---
  if (distanciaCm <= LIMIAR_ALERTA && distanciaCm > 0) {
    statusNivel = "ALERTA";
    digitalWrite(PINO_LED_VERDE, LOW); digitalWrite(PINO_LED_AMARELO, LOW); digitalWrite(PINO_LED_VERMELHO, HIGH);
    tone(PINO_BUZZER, 1000);
  } else if (distanciaCm <= LIMIAR_ATENCAO && distanciaCm > LIMIAR_ALERTA) {
    statusNivel = "Atencao";
    digitalWrite(PINO_LED_VERDE, LOW); digitalWrite(PINO_LED_AMARELO, HIGH); digitalWrite(PINO_LED_VERMELHO, LOW);
    noTone(PINO_BUZZER);
  } else if (distanciaCm > LIMIAR_ATENCAO) {
    statusNivel = "Normal";
    digitalWrite(PINO_LED_VERDE, HIGH); digitalWrite(PINO_LED_AMARELO, LOW); digitalWrite(PINO_LED_VERMELHO, LOW);
    noTone(PINO_BUZZER);
  } else {
    statusNivel = "Erro US";
    digitalWrite(PINO_LED_VERDE, LOW); digitalWrite(PINO_LED_AMARELO, LOW); digitalWrite(PINO_LED_VERMELHO, LOW);
    noTone(PINO_BUZZER);
  }
  Serial.print("Status Nivel: "); Serial.println(statusNivel);

  // --- Atualização do Display LCD (Alternando Telas) ---
  unsigned long tempoAtual = millis();

  // Verifica se é hora de mudar a tela
  if (tempoAtual - tempoAnteriorLCD >= intervaloLCD) {
    tempoAnteriorLCD = tempoAtual; // Atualiza o tempo da última mudança
    telaAtualLCD++; // Avança para a próxima tela
    if (telaAtualLCD > 2) { // Volta para a primeira tela após a última
      telaAtualLCD = 0;
    }

    lcd.clear(); // Limpa o LCD para a nova tela

    // Exibe a informação da tela atual
    switch (telaAtualLCD) {
      case 0: // Tela Nível da Água
        lcd.setCursor(0, 0);
        lcd.print("Nivel: ");
        lcd.print(statusNivel);
        lcd.setCursor(0, 1);
        if (statusNivel != "Erro US") {
          lcd.print("Dist: ");
          lcd.print(distanciaCm, 0);
          lcd.print(" cm");
        } else {
          lcd.print("Dist: --- cm");
        }
        break;

      case 1: // Tela Temperatura e Umidade
        lcd.setCursor(0, 0);
        lcd.print("Temp: ");
        if (isnan(temperaturaC)) { lcd.print("ERRO"); } else { lcd.print(temperaturaC, 1); lcd.print((char)223); lcd.print("C"); }
        lcd.setCursor(0, 1);
        lcd.print("Umid: ");
        if (isnan(umidade)) { lcd.print("ERRO"); } else { lcd.print(umidade, 0); lcd.print(" %"); }
        break;

      case 2: // Tela Luminosidade
        lcd.setCursor(0, 0);
        lcd.print("Luminosidade");
        lcd.setCursor(0, 1);
        lcd.print("LDR: ");
        lcd.print(valorLDR);
        // Poderia adicionar uma interpretação (Ex: Claro/Escuro)
        if (valorLDR < 300) { lcd.print(" (Alta)"); } // Exemplo, ajustar limiar
        else if (valorLDR > 700) { lcd.print(" (Baixa)"); } // Exemplo, ajustar limiar
        break;
    }
  }

  // O delay principal do loop pode ser menor agora, já que o LCD alterna com millis()
  // Mas o DHT22 recomenda leituras a cada 2s. Vamos manter um delay razoável.
  delay(500); // Reduzido para permitir a alternância mais fluida do LCD

}

