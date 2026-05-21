#include <Arduino.h>    // Biblioteca padrao arduino
#include <WiFi.h>       // Biblioteca responsavel pela conexao WIFI
#include <ThingSpeak.h> // Bibliotecas para enviar dados ao ThingSpeak
#include <DHT.h>        // Biblioteca para comunicar com o sensor de temperatura
#include <nvs_flash.h>

#define SSID_REDE "SEU-SSID"                // coloque aqui o nome da rede que se deseja conectar
#define SENHA_REDE "SUA-SENHA"              // coloque aqui a senha da rede que se deseja conectar
#define THINGSPEAK_DELAY 600000               // intervalo entre envios de dados ao ThingSpeak (em ms) 600000
#define DELAY_COLETA 3000                     // intervalo de coleta de dados (em ms)
#define DELAY_NEBULIZACAO 600000              // delay para inicializar a bomba (nebulizador) (em ms) 600000
#define TEMPO_FUNCIONAMENTO_NEBULIZADOR 60000 // delay para definir o tempo de funcionamento da bomba (em ms)
#define dhtInterno 4                          // dht da estufa
#define dhtExterno 5                          // dht exterior
#define DHTTYPE DHT22                         // definindo o tipo de dht
#define VENTILADOR 26                         // pino de controle do ventilador
#define NEBULIZADOR 25                        // pino de controle do nebulizador

DHT dht(dhtInterno, DHTTYPE);  // dht da estufa
DHT dht2(dhtExterno, DHTTYPE); // dht do exterior

hw_timer_t *contador = NULL; // contador responsavel por guardar o timer

// 5 min = 300000ms
// 15 min = 900000ms

WiFiClient client; // definindo cliente de conexao de servico

bool nebulock = false;
int statusNebulizador = 0;

int channelNumber = 1;                        // endereco de envio dos dados para o canal ThingSpeak
const char *writeAPIKey = "CHAVE-THINGSPEAK"; // Chave de escrita do canal do ThingSpeak
unsigned long ultima_conexao;                 // tempo desde ultima conexao com o ThingSpeak
unsigned long ultima_coleta;                  // tempo desde ultima coleta de dado
unsigned long ultima_nebulizacao;             // tempo desde a ultima ativacao da bomba
int connectionTimeDelay = 10000;              // delay definido para aguardar conexao com a internet
float tempEstufa;                             // armazena a temperatura dentro da estufa
float umidadeEstufa;                          // armazena a humidade dentro da estufa
float tempExterior;                           // armazena a temperatura fora da estufa
float umidadeExterior;                        // armazena a humidade fora da estufa
int envioThingSpeak;

int tempAtivacaoVentilador = 35;  // temperatura de ativacao do ventilador
int tempAtivacaoNebulizador = 38; // temperatura de ativacao do nebulizador

uint8_t conectarWifi();                // funcao para conectar wifi
int enviarThingSpeak();                // envia informacoes ao ThingSpeak
void lerSensores();                    // requisitando temperatura ao sensor DS18B20
void controlarEstufa();                // controlando ventilador e nebulizador
void imprimirDados();                  // imprimindo leituras do sensor
void controlarRelay(uint8_t, uint8_t); // controlando relays
void IRAM_ATTR resetModule();

void setup()
{
  //nvs_flash_erase(); // erase the NVS partition and...
  //nvs_flash_init(); // initialize the NVS partition.
  Serial.begin(115200);
  dht.begin();  // iniciando dht estufa
  dht2.begin(); // iniciando dht exterior
  pinMode(VENTILADOR, OUTPUT);
  pinMode(NEBULIZADOR, OUTPUT);
  controlarRelay(LOW, HIGH);
  delay(5000);
  controlarRelay(HIGH, HIGH);

  // DEFINICAO DO TIMER
  contador = timerBegin(0, 80, true);
  timerAttachInterrupt(contador, &resetModule, true);
  timerAlarmWrite(contador, 900000 * 1000, true);
  timerAlarmEnable(contador);
  // =====================//

  ultima_conexao = 0; // tempo referente a ultima conexao ao ThingSpeak
  ultima_coleta = 0;  // tempo referente a ultima leitura
  envioThingSpeak = enviarThingSpeak();
}

void loop()
{
  if (millis() - ultima_conexao > THINGSPEAK_DELAY) // verifica se chegou o momento para enviar informacoes ao ThingSpeak
  {
    lerSensores();     // requisitando temperatura ao sensor DS18B20
    controlarEstufa(); // controlando relays
    envioThingSpeak = enviarThingSpeak();
    ultima_conexao = millis();
  }

  if ((millis() - ultima_coleta) > DELAY_COLETA) // tempo para realizar leitura das temperaturas
  {
    imprimirDados();
    ultima_coleta = millis(); // definindo tempo da ultima coleta
  }
  timerWrite(contador, 0);
}

/**
 * @brief controlando relays
 *
 * @param  ventilador
 * @param nebulizador
 */
void controlarRelay(uint8_t ventilador, uint8_t nebulizador)
{
  digitalWrite(VENTILADOR, ventilador);
  digitalWrite(NEBULIZADOR, nebulizador);
}

/**
 * @brief controlando acionamento do ventilador e do nebulizador
 *
 */
void controlarEstufa()
{
  statusNebulizador = !digitalRead(NEBULIZADOR);
  if (tempEstufa < tempAtivacaoVentilador) // temperatura abaixo do minimo, desligar o relay
  {
    controlarRelay(HIGH, HIGH);
  }

  if ((tempEstufa > tempAtivacaoVentilador)) // ligando o ventilador
  {

    controlarRelay(LOW, HIGH);
  }

  if (tempEstufa >= tempAtivacaoNebulizador /*&& nebulock == true*/)
  {
    controlarRelay(LOW, LOW);
    delay(TEMPO_FUNCIONAMENTO_NEBULIZADOR);
    statusNebulizador = !digitalRead(NEBULIZADOR);

    for (int i = 0; i < 4; i++)
    {
      controlarRelay(LOW, HIGH);
      delay(TEMPO_FUNCIONAMENTO_NEBULIZADOR);

      controlarRelay(LOW, LOW);
      delay(TEMPO_FUNCIONAMENTO_NEBULIZADOR);
    }

    controlarRelay(LOW, HIGH);
    delay(TEMPO_FUNCIONAMENTO_NEBULIZADOR);

    nebulock = false;
  }

  if (millis() - ultima_nebulizacao > DELAY_NEBULIZACAO)
  {
    nebulock = true;
    ultima_nebulizacao = millis();
  }
}

void IRAM_ATTR resetModule()
{
  esp_restart();
}

/**
 * @brief printando informacoes na tela
 *
 */
void imprimirDados()
{
  Serial.printf("\nTemp estufa: %.2f C \tUmid estufa: %.2f", tempEstufa, umidadeEstufa);
  Serial.printf("\nTemp Exterior: %.2f C \tUmid exterior: %.2f", tempExterior, umidadeExterior);
  Serial.printf("\nnebulock: %d", nebulock);
  Serial.printf("\nWiFi_status: %d\tStatus_ThingSpeak: %i", conectarWifi(), envioThingSpeak);
  Serial.printf("\nventilador: %i\tnebulizador: %i", !digitalRead(VENTILADOR), statusNebulizador);
  Serial.println();
}

/**
 * @brief enviando campos ao thingspeak
 *
 */
int enviarThingSpeak()
{
  conectarWifi();           // Garante que a conexão wi-fi esteja ativa
  ThingSpeak.begin(client); // inicializa o ThingSpeak
  delay(50);
  // escrevendo em um campo especifico
  ThingSpeak.setField(1, tempEstufa);
  ThingSpeak.setField(2, umidadeEstufa);
  ThingSpeak.setField(3, tempExterior);
  ThingSpeak.setField(4, umidadeExterior);
  ThingSpeak.setField(5, !digitalRead(VENTILADOR));
  ThingSpeak.setField(6, statusNebulizador);

  return ThingSpeak.writeFields(channelNumber, writeAPIKey); // enviando campos de dados ao thingspeak
}

/**
 * @brief lendo sensores dht22
 *
 */
void lerSensores()
{
  tempEstufa = dht.readTemperature();    // pegando temperatura do sensor da estufa
  umidadeEstufa = dht.readHumidity();    // pegando humidade do sensor da estufa
  tempExterior = dht2.readTemperature(); // pegando tempera+-+tura do sensor do exterior
  umidadeExterior = dht2.readHumidity(); // pegando humidade do sensor do exterior

  // tempEstufa = random(30, 45);      // pegando temperatura do sensor da estufa
  // umidadeEstufa = random(60, 80);   // pegando humidade do sensor da estufa
  // tempExterior = random(30, 45);    // pegando temperatura do sensor do exterior
  // umidadeExterior = random(60, 80); // pegando humidade do sensor do exterior
}

/**
 * @brief Conectando e verificando conexao com wifi
 *
 */
uint8_t conectarWifi()
{
  WiFi.begin(SSID_REDE, SENHA_REDE); // inicializando wifi com o ssid e senha informado

  return WiFi.waitForConnectResult(10000);
}
