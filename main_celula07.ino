/*
  Projeto: Célula 07 – Telemetria MQTT (ESP8266 TLS) com sensor de vibração
  Célula: 07
  Placa: NodeMCU/ESP-12E (ESP8266)

  Regras de LEDs (padrão do trabalho):
    - Vibração < 300    -> LED VERDE (normal)
    - Vibração 300–600  -> LED AMARELO (atenção)
    - Vibração > 600    -> LED VERMELHO (crítico)
  Serial: imprime ADC, índice de vibração, e status LED a cada ciclo.

  Tópicos:
    base = iot/<campus>/<curso>/<turma>/cell/<cellId>/device/<devId>/
    - state      (retained)    -> "online" no connect
    - telemetry  (QoS 1)       -> dados periódicos (~3s) e on-change
    - event      (QoS 1)       -> eventos (ex.: alarme_vibracao)
    - cmd        (sub)         -> recebe comandos (get_status, set_thresholds)
    - config     (retained)    -> publica configuração atual (limiares etc.)

  Dependências (Gerenciador de Bibliotecas):
    - PubSubClient
    - ArduinoJson
    - ESP8266WiFi
    - WiFiClientSecure
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ====================== CONFIG WI-FI / MQTT ====================== //
#include "secrets.h"

String topicoBase;
String topicoEstado;
String topicoTelemetria;
String topicoEvento;
String topicoComando;
String topicoConfiguracao;

// ====================== HARDWARE / LIMIARES ====================== //
const uint8_t LED_VERDE    = 4;  // GPIO4 (D2)
const uint8_t LED_AMARELO  = 5;  // GPIO5 (D1)
const uint8_t LED_VERMELHO = 12; // GPIO12 (D6)

const int ADC_MIN = 0;
const int ADC_MAX = 1023;

struct Limiares {
  int vib_warn  = 300;
  int vib_alarm = 600;
} limiares;

const int HISTERESE_VIB = 20;

const char* STATUS_NORMAL = "normal";
const char* STATUS_ATENCAO = "atencao";
const char* STATUS_CRITICO = "critico";

// ====================== ESTADO / CLIENTES / TEMPO ====================== //
WiFiClientSecure clienteWifiSeguro;
PubSubClient clienteMqtt(clienteWifiSeguro);

unsigned long INTERVALO_TELEMETRIA_MS = 3000UL;
unsigned long ultimoEnvioMs = 0;

String statusLedAtual = "desconhecido";
String statusLedAnterior = "desconhecido";

// ====================== PROTÓTIPOS ====================== //
void conectarWifiSeNecessario();
void conectarMqttSeNecessario();
void callbackMqtt(char* topico, byte* payload, unsigned int length);

int lerIndiceVibracao();
String classificarStatusLed(int vibIndex);
void atualizarLeds(const String& statusLed);

void publicarEstadoOnline();
void publicarConfiguracao();
void publicarTelemetria(bool forcarEnvio, int vibIndex, const String& statusLed);
void publicarEventoMudancaStatus(const String& deStatus, const String& paraStatus, int vibIndex);

void processarPayloadComando(const String& payload);
bool aplicarLimiaresDoJson(JsonObject data);

String agoraEpochStr();

// ====================== SETUP ====================== //
void setup() {
  Serial.begin(9600);
  delay(100);

  pinMode(LED_VERDE,    OUTPUT);
  pinMode(LED_AMARELO,  OUTPUT);
  pinMode(LED_VERMELHO, OUTPUT);
  digitalWrite(LED_VERDE,    LOW);
  digitalWrite(LED_AMARELO,  LOW);
  digitalWrite(LED_VERMELHO, LOW);

  topicoBase        = String("iot/") + CAMPUS + "/" + CURSO + "/" + TURMA +
                      "/cell/" + String(CELL_ID) + "/device/" + DEV_ID + "/";
  topicoEstado      = topicoBase + "state";
  topicoTelemetria  = topicoBase + "telemetry";
  topicoEvento      = topicoBase + "event";
  topicoComando     = topicoBase + "cmd";
  topicoConfiguracao = topicoBase + "config";

  Serial.print(F("[TOPICO BASE] ")); Serial.println(topicoBase);

  clienteWifiSeguro.setInsecure(); 

  conectarWifiSeNecessario();

  clienteMqtt.setServer(MQTT_HOST, MQTT_PORT);
  clienteMqtt.setCallback(callbackMqtt);

  conectarMqttSeNecessario();

  int vibIndex = lerIndiceVibracao();
  statusLedAtual = classificarStatusLed(vibIndex);
  atualizarLeds(statusLedAtual);
  publicarTelemetria(true, vibIndex, statusLedAtual);

  Serial.println(F("[OK] Inicialização concluída."));
}

// ====================== LOOP PRINCIPAL ====================== //
void loop() {
  conectarWifiSeNecessario();
  
  if (!clienteMqtt.loop()) {
    conectarMqttSeNecessario();
  }

  int vibIndex = lerIndiceVibracao();
  String novoStatusLed = classificarStatusLed(vibIndex);

  if (novoStatusLed != statusLedAtual) {
    statusLedAnterior = statusLedAtual;
    statusLedAtual    = novoStatusLed;

    atualizarLeds(statusLedAtual);
    publicarEventoMudancaStatus(statusLedAnterior, statusLedAtual, vibIndex);
    publicarTelemetria(true, vibIndex, statusLedAtual);
  }
  
  unsigned long agora = millis();
  if (agora - ultimoEnvioMs >= INTERVALO_TELEMETRIA_MS) {
    publicarTelemetria(false, vibIndex, statusLedAtual);
    ultimoEnvioMs = agora;
  }

  delay(50);
}

// ====================== CONEXÃO ====================== //

void conectarWifiSeNecessario() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print(F("[WiFi] Conectando a "));
  Serial.print(WIFI_SSID);
  Serial.print(F(" ... "));
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print('.');
  }
  Serial.println(F(" conectado!"));
  Serial.print(F("IP: ")); Serial.println(WiFi.localIP());
}

void conectarMqttSeNecessario() {
  if (clienteMqtt.connected()) return;

  Serial.print(F("[MQTT] Tentando conectar em "));
  Serial.print(MQTT_HOST); Serial.print(':'); Serial.print(MQTT_PORT);
  Serial.print(F(" ... "));

  if (clienteMqtt.connect(DEV_ID, MQTT_USER, MQTT_PASS)) {
    Serial.println(F("conectado!"));
    
    clienteMqtt.subscribe(topicoComando.c_str(), 1); 
    Serial.print(F("[MQTT] Subscrito em: ")); Serial.println(topicoComando);

    publicarEstadoOnline();
    publicarConfiguracao();
  } else {
    Serial.print(F("falhou (rc="));
    Serial.print(clienteMqtt.state());
    Serial.println(F(")"));
    delay(1000);
  }
}

// ====================== CALLBACK DE MENSAGENS ====================== //

void callbackMqtt(char* topico, byte* payload, unsigned int length) {
  Serial.print(F("[CMD] Mensagem recebida no tópico: ")); Serial.println(topico);

  if (String(topico) != topicoComando) {
    return;
  }

  char payloadBuffer[length + 1];
  memcpy(payloadBuffer, payload, length);
  payloadBuffer[length] = '\0';

  String payloadStr = String(payloadBuffer);
  Serial.print(F("[CMD] Payload: ")); Serial.println(payloadStr);

  processarPayloadComando(payloadStr);
}

// ====================== LEITURA / CLASSIFICAÇÃO / LED ====================== //

int lerIndiceVibracao() {
  int raw = analogRead(A0);
  raw = constrain(raw, ADC_MIN, ADC_MAX);
  int vibIndex = map(raw, ADC_MIN, ADC_MAX, 0, 1000);
  
  Serial.print(F("[SENSOR] ADC: ")); Serial.print(raw);
  Serial.print(F(" | Índice de vibração: ")); Serial.println(vibIndex);
  
  return vibIndex;
}

String classificarStatusLed(int vibIndex) {
  if (statusLedAtual == STATUS_NORMAL) {
    if (vibIndex < limiares.vib_warn + HISTERESE_VIB) return STATUS_NORMAL;
  }
  if (statusLedAtual == STATUS_ATENCAO) {
    if (vibIndex >= (limiares.vib_warn - HISTERESE_VIB) && vibIndex <= (limiares.vib_alarm + HISTERESE_VIB))
      return STATUS_ATENCAO;
  }
  if (statusLedAtual == STATUS_CRITICO) {
    if (vibIndex > limiares.vib_alarm - HISTERESE_VIB) return STATUS_CRITICO;
  }

  if (vibIndex < limiares.vib_warn)       return STATUS_NORMAL;
  if (vibIndex <= limiares.vib_alarm)     return STATUS_ATENCAO;
  return STATUS_CRITICO;
}

void atualizarLeds(const String& status) {
  uint8_t v = LOW, a = LOW, r = LOW;

  if (status == STATUS_NORMAL)       v = HIGH;
  else if (status == STATUS_ATENCAO) a = HIGH;
  else if (status == STATUS_CRITICO) r = HIGH;

  digitalWrite(LED_VERDE,    v);
  digitalWrite(LED_AMARELO,  a);
  digitalWrite(LED_VERMELHO, r);
}

// ====================== PUBLICAÇÕES MQTT ====================== //

void publicarEstadoOnline() {
  clienteMqtt.publish(topicoEstado.c_str(), "online", true);
}

void publicarConfiguracao() {
  if (!clienteMqtt.connected()) {
    Serial.println(F("[CONFIG] IGNORADO: Cliente MQTT não conectado."));
    return;
  }

  StaticJsonDocument<320> doc;
  doc["cellId"] = CELL_ID;
  doc["devId"]  = DEV_ID;

  JsonObject t = doc.createNestedObject("thresholds");
  t["vib_warn"]  = limiares.vib_warn;
  t["vib_alarm"] = limiares.vib_alarm;

  JsonObject misc = doc.createNestedObject("misc");
  misc["telemetry_interval_ms"] = INTERVALO_TELEMETRIA_MS;

  char buf[360];
  size_t n = serializeJson(doc, buf, sizeof(buf));

  Serial.print(F("[CONFIG] Tamanho payload: ")); Serial.println(n);

  if (n == 0) {
    Serial.println(F("[CONFIG] ERRO: Falha na serialização JSON."));
    return;
  }

  bool publicado = clienteMqtt.publish(topicoConfiguracao.c_str(), (const uint8_t*)buf, n, true);

  if (!publicado) {
    Serial.println(F("[CONFIG] ERRO: Falha ao publicar MQTT."));
  }
  delay(50);
}

void publicarTelemetria(bool forcarEnvio, int vibIndex, const String& statusLed) {
  Serial.println("[TELEMETRY] Iniciando publicação...");
  
  if (!clienteMqtt.connected()) {
    Serial.println(F("[TELEMETRY] IGNORADO: Cliente MQTT não conectado."));
    return;
  }

  StaticJsonDocument<512> doc; 
  
  doc["ts"] = agoraEpochStr();
  doc["cellId"] = CELL_ID;
  doc["devId"]  = DEV_ID;

  JsonObject metrics = doc.createNestedObject("metrics");
  metrics["vib_index"] = vibIndex;

  doc["status"] = statusLed;

  JsonObject th = doc.createNestedObject("thresholds");
  th["vib_warn"]  = limiares.vib_warn;
  th["vib_alarm"] = limiares.vib_alarm;
  
  char buf[550]; 
  size_t n = serializeJson(doc, buf, sizeof(buf));
  
  Serial.print(F("[TELEMETRY] Tamanho payload: ")); Serial.println(n);

  if (n == 0) {
    Serial.println(F("[TELEMETRY] ERRO: Falha na serialização JSON."));
    return;
  }

  bool publicado = clienteMqtt.publish(topicoTelemetria.c_str(), (const uint8_t*)buf, n, false);
  
  if (publicado) {
    Serial.println(F("[TELEMETRY] Publicado com SUCESSO."));
  } else {
    Serial.println(F("[TELEMETRY] ERRO: Falha ao publicar MQTT."));
  }
  delay(50);
}

void publicarEventoMudancaStatus(const String& deStatus, const String& paraStatus, int vibIndex) {
  if (!clienteMqtt.connected()) {
    Serial.println(F("[EVENT] IGNORADO: Cliente MQTT não conectado."));
    return;
  }

  StaticJsonDocument<288> doc;
  doc["ts"] = agoraEpochStr();
  
  if (paraStatus == STATUS_CRITICO) {
    doc["type"] = "alarme_vibracao";
  } else {
    doc["type"] = "mudanca_status";
  }
  
  doc["from"] = deStatus;
  doc["to"]   = paraStatus;
  doc["vib_index"] = vibIndex;

  char buf[300];
  size_t n = serializeJson(doc, buf, sizeof(buf));

  Serial.print(F("[EVENT] Tamanho payload: ")); Serial.println(n);
  
  if (n == 0) {
    Serial.println(F("[EVENT] ERRO: Falha na serialização JSON."));
    return;
  }

  bool publicado = clienteMqtt.publish(topicoEvento.c_str(), (const uint8_t*)buf, n, false);

  if (!publicado) {
    Serial.println(F("[EVENT] ERRO: Falha ao publicar MQTT."));
  }
  delay(50);
}

// ====================== CMD HANDLER ====================== //

void processarPayloadComando(const String& payload) {
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, payload);
  
  if (err) {
    Serial.print(F("[CMD] JSON inválido: ")); Serial.println(err.c_str());
    return;
  }

  const char* acao = doc["action"];
  if (!acao) {
    Serial.println(F("[CMD] action ausente"));
    return;
  }

  String acaoStr = acao;

  Serial.print(F("[CMD] Comando acao: ")); Serial.println(acaoStr);

  if (acaoStr == "get_status") {
    Serial.println(F("[CMD] Executando get_status..."));
    int vibIndex = lerIndiceVibracao();
    String led = classificarStatusLed(vibIndex);
    publicarTelemetria(true, vibIndex, led);
    return;
  }

  if (acaoStr == "set_thresholds") {
    JsonObject dados = doc["data"];
    if (dados.isNull()) {
      Serial.println(F("[CMD] set_thresholds sem data"));
      return;
    }
    if (aplicarLimiaresDoJson(dados)) {
      publicarConfiguracao();
      
      int vibIndex = lerIndiceVibracao();
      statusLedAtual = classificarStatusLed(vibIndex);
      atualizarLeds(statusLedAtual);
      publicarTelemetria(true, vibIndex, statusLedAtual);

      Serial.println(F("[CMD] thresholds atualizados e telemetria enviada."));
    } else {
      Serial.println(F("[CMD] thresholds inválidos (use 0–1000 com warn < alarm)"));
    }
    return;
  }

  Serial.print(F("[CMD] ação não suportada: "));
  Serial.println(acaoStr);
}

bool aplicarLimiaresDoJson(JsonObject data) {
  int novoWarn  = data.containsKey("vib_warn")  ? data["vib_warn"].as<int>()  : limiares.vib_warn;
  int novoAlarm = data.containsKey("vib_alarm") ? data["vib_alarm"].as<int>() : limiares.vib_alarm;

  if (novoWarn < 0 || novoWarn > 1000) return false;
  if (novoAlarm < 0 || novoAlarm > 1000) return false;
  if (novoWarn >= novoAlarm) return false;

  limiares.vib_warn = novoWarn;
  limiares.vib_alarm = novoAlarm;
  return true;
}

// ====================== UTIL ====================== //

String agoraEpochStr() {
  unsigned long s = millis() / 1000UL;
  return String(s);
}
