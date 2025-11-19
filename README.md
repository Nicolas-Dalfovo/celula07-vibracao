# Sistema de Monitoramento de Vibração - Célula 07 (ESP8266)

Este projeto implementa um sistema de monitoramento de vibração utilizando um NodeMCU (ESP8266) que se conecta a um broker MQTT via TLS para enviar dados de telemetria e receber comandos remotos.

## Visão Geral

O sistema utiliza um potenciômetro para simular um sensor de vibração, classificando o nível de vibração em três categorias:

- **Normal**: Nível de vibração seguro.
- **Atenção**: Nível de vibração que requer monitoramento.
- **Crítico**: Nível de vibração perigoso, acionando um alarme.

O status é indicado por LEDs coloridos e as informações são publicadas em um broker MQTT, permitindo o monitoramento remoto e a integração com outras plataformas de IoT.

## Hardware Necessário

- NodeMCU v1.0 (ESP-12E)
- LEDs (Verde, Amarelo, Vermelho)
- Potenciômetro (10kΩ)
- Resistores (3x 220Ω)
- Protoboard e Jumpers

## Software e Dependências

- [Arduino IDE](https://www.arduino.cc/en/software)
- Placas Adicionais para o Gerenciador de Placas: `http://arduino.esp8266.com/stable/package_esp8266com_index.json`
- Bibliotecas (instaláveis via Gerenciador de Bibliotecas da IDE):
  - `PubSubClient` by Nick O'Leary
  - `ArduinoJson` by Benoit Blanchon

## Configuração

1.  **Renomeie** o arquivo `main_celula07.ino` para `main_celula07.ino` ou o nome que preferir para seu sketch.
2.  **Preencha o arquivo `secrets.h`** com suas credenciais de Wi-Fi e do broker MQTT. Substitua os valores entre `{{...}}`.

    ```cpp
    // --- Configuração WiFi ---
    const char* WIFI_SSID = "SEU_SSID_WIFI";
    const char* WIFI_PASS = "SUA_SENHA_WIFI";

    // --- Configuração MQTT ---
    const char* MQTT_HOST = "SEU_MQTT_HOST";
    const int   MQTT_PORT = 8883; // Porta TLS
    const char* MQTT_USER = "SEU_USUARIO_MQTT";
    const char* MQTT_PASS = "SUA_SENHA_MQTT";
    ```

## Como Iniciar

1.  Abra o arquivo `.ino` na Arduino IDE.
2.  Selecione a placa correta em **Ferramentas > Placa > ESP8266 Boards > NodeMCU 1.0 (ESP-12E Module)**.
3.  Selecione a porta COM correta.
4.  Clique em **Carregar** para compilar e enviar o código para o NodeMCU.
5.  Abra o **Monitor Serial** com a taxa de 9600 baud para visualizar os logs de conexão e telemetria.
