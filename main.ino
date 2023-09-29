// INCLUSÃO DA BIBLIOTECA NECESSÁRIA
#include <Arduino.h>
// Biblioteca Servo Motor
#include <Servo.h>
// Biblioteca LiquidCrystal(LCD)
#include <LiquidCrystal.h>
// Biblioteca Sensor Ultrassonico
#include <Ultrasonic.h>
// Bibliotecas Shild
#include <SPI.h>
#include <Ethernet.h>

Servo s; // Variável Servo Motor 9G
Servo m; // Variável Servo Motor 15KG

// Pinagem do LCD
LiquidCrystal lcd(9, 8, 3, 2, 1, 0);

// Constante que representa os pinos onde o Sensor Ultrassonico para Pets será ligado
#define PINO_PET_TRIGGER 13
#define PINO_PET_ECHO 12

// Constante que representa os pinos onde o Sensor Ultrassonico para Latas será ligado
#define PINO_LATA_TRIGGER 11
#define PINO_LATA_ECHO 10

Ultrasonic ultrasonicPet(PINO_PET_TRIGGER, PINO_PET_ECHO);
Ultrasonic ultrasonicLata(PINO_LATA_TRIGGER, PINO_LATA_ECHO);

float distanciaPet;
float distanciaLata;

// Constante que representa o pino onde o positivo do buzzer será ligado.
const int buzzer = 5;

bool inserindo = false;

String mensagem;

String urlInsert = "GET /soft-ui-dashboard-main/php-teste/teste.php?exemplo=";

// Definicoes de IP, mascara de rede e gateway
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress server(10, 0, 0, 3);
IPAddress ip(10, 0, 0, 66);
IPAddress myDns(10, 0, 0, 2);

// Inicializa o servidor web na porta 80
EthernetClient client;

// Variables to measure the speed
unsigned long beginMicros, endMicros;

void setup()
{
  _imprimeLCD("MAQUINA RECICLAVEL", "");

  s.attach(6);
  m.attach(7);
  s.write(90); // Inicia Servo Motor 9 G posição inicial
  m.write(65); // Inicia Servo Motor 15KG posição inicial

  lcd.begin(16, 2); // Inicia o lcd de 16x2

  pinMode(buzzer, OUTPUT); // Define o pino buzzer como de saída.

  // start the Ethernet connection:
  Serial.println("Inicializar Ethernet com DHCP:");
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Falha ao configurar Ethernet usando DHCP");
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("O escudo Ethernet não foi encontrado. Desculpe, não pode ser executado sem hardware. :(");
      while (true)
      {
        delay(1); // do nothing, no point running without Ethernet hardware
      }
    }
    if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("O cabo Ethernet não está conectado.");
    }
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  }
  else
  {
    Serial.print(" DHCP atribuída IP ");
    Serial.println(Ethernet.localIP());
  }

  // give the Ethernet shield a second to initialize:
  delay(1000);
  Serial.print("conectando à ");
  Serial.print(server);
  Serial.println("...");

  // Velocidade de transmissão da porta serial
  Serial.begin(9600);
}

void loop()
{
  bool _isError = _tratamentoError();

  if (!_isError)
  {
    if (!inserindo)
    {
      _imprimeLCD("VERIFICANDO MATERIAL", "");
      Serial.print("Verificando material");
      inserindo = true;

      int luz1 = analogRead(A0); // Lê a porta que o sensor esta instalado
      int luz2 = analogRead(A1); // Lê a porta que o sensor esta instalado

      if ((luz1 <= 490 && luz1 >= 300) || (luz2 <= 190 && luz2 >= 100))
      {
        _imprimeLCD("INSERINDO MATERIAL", "TIPO: PET");
        Serial.print("Material inserido Pet");
        m.write(150);
        delay(1000);
        s.write(140);
        delay(1500);
        s.write(90);
        m.write(65);

        mensagem = urlInsert + "1";
        enviaRequisicao(mensagem);
        Serial.println(mensagem);

        _toqueBuzzer();
        inserindo = false;
        _imprimeLCD("MAQUINA RECICLAVEL", "");
        delay(500);
      }

      if ((luz1 <= 50 && luz1 >= 0) || (luz2 <= 50 && luz2 >= 0))
      {
        _imprimeLCD("INSERINDO MATERIAL", "TIPO: LATA");
        Serial.print("Inserido material Lata");
        s.write(50);
        m.write(150);
        delay(1000);
        s.write(90);
        m.write(65);

        mensagem = urlInsert + "2";
        enviaRequisicao(mensagem);
        Serial.println(mensagem);

        _toqueBuzzer();
        _imprimeLCD("MAQUINA RECICLAVEL", "");
        inserindo = false;
      }

      inserindo = false;
    }
  }
  else
  {
    _imprimeLCD("SISTEMA TEMPORARIAMENTE INDISPONIVEL", "");
  }
}

void _toqueBuzzer()
{
  // Ligando o buzzer com uma frequencia de 350 hz.
  tone(buzzer, 350);
  delay(500);

  // Desligando o buzzer.
  noTone(buzzer);
  delay(500);
}

void _imprimeLCD(String title, String subTitle)
{
  lcd.clear();         // Limpa o display
  lcd.setCursor(1, 0); // 2 colunas para a direita 0 = Primeira linha
  lcd.print(title);

  lcd.setCursor(1, 1); // 2 colunas para a direita 1 = Segunda linha
  lcd.print(subTitle);
}

/// Método responsável por calcular a distancia dos sensores
void _verificaDeposito()
{
  // Le e armazena as informacoes do sensor ultrasonico Pets
  long microsecPet = ultrasonicPet.timing();
  distanciaPet = ultrasonicPet.convert(microsecPet, Ultrasonic::CM);

  // Le e armazena as informacoes do sensor ultrasonico Latas
  long microsecLat = ultrasonicLata.timing();
  distanciaLata = ultrasonicLata.convert(microsecLat, Ultrasonic::CM);

  delay(500);
}

bool _tratamentoError()
{
  if (inserindo = false)
  {
    _imprimeLCD("VERIFICANDO SISTEMA", "");
    Serial.print("Iniciado tratamento de Error \n");

    _verificaDeposito(); // Verifica deposito de Latas e Pets

    int luz1 = analogRead(A0); // Lê a porta que o sensor esta instalado
    int luz2 = analogRead(A1); // Lê a porta que o sensor esta instalado

    // Imprimi o valor lido na tela monitor serial da IDE
    Serial.print("LUZ1: ");
    Serial.println(luz1);
    Serial.print("LUZ2: ");
    Serial.println(luz2);
    Serial.print("Deposito Pet: ");
    Serial.println(distanciaPet);
    Serial.print("Deposito Lata: ");
    Serial.println(distanciaLata);

    if (distanciaPet <= 10)
    { 
      Serial.print("Deposito de garrafas PETs está cheio");

      return true;
    }

    if (distanciaLata <= 10)
    { 
      Serial.print("Deposito de latas está cheio");

      return true;
    }

    if ((luz1 >= 0 && luz1 <= 10) || (luz2 >= 0 && luz2 <= 10))
    {    
      Serial.print("LEDs ou Sensores com problemas");    

      return true;
    }
    
    return false;
  }

  return false;
}

void enviaRequisicao(String mensagem)
{
  if (client.connect(server, 80))
  {
    Serial.print("conectado a ");
    Serial.println(client.remoteIP());
    client.println(mensagem);
  }
  else
  {
    Serial.println("conexão falhou");
  }

  beginMicros = micros();
}