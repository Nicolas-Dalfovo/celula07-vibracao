# Documento Explicativo do Código: Sistema de Monitoramento de Vibração com ESP8266 e MQTT

## RESUMO

Este documento detalha o desenvolvimento de um sistema de monitoramento de vibração utilizando a plataforma NodeMCU (ESP8266). O sistema foi projetado para detectar e classificar níveis de vibração, enviando os dados para um broker MQTT via conexão segura (TLS). A classificação dos níveis de vibração é dividida em três categorias: **Normal**, **Atenção** e **Crítico**, com indicadores visuais através de LEDs coloridos. As informações são enviadas em tempo real para um servidor MQTT, permitindo o monitoramento remoto e a integração com outras plataformas de IoT.

## 1. INTRODUÇÃO

A vibração excessiva em equipamentos é um indicador precoce de possíveis falhas. O monitoramento contínuo desses parâmetros permite a aplicação de manutenção preditiva, o que pode reduzir custos operacionais e aumentar a vida útil dos equipamentos.

O objetivo deste trabalho é apresentar um sistema de monitoramento de vibração de baixo custo, utilizando a plataforma ESP8266, capaz de classificar os níveis de vibração e enviar alertas e dados de telemetria para um sistema central através do protocolo MQTT.

A justificativa para este projeto reside na crescente necessidade de sistemas de monitoramento acessíveis e eficientes. A utilização de plataformas como o ESP8266 permite o desenvolvimento de soluções conectadas à Internet (IoT) de forma rápida e com bom custo-benefício.

## 2. DESENVOLVIMENTO DO SISTEMA

### 2.1. Arquitetura do Sistema

O sistema possui uma arquitetura modular:

-   **Módulo de Aquisição de Dados**: Utiliza o conversor analógico-digital (ADC) do ESP8266 para ler o sinal de um potenciômetro, que simula um sensor de vibração. O valor lido (0-1023) é mapeado para um índice de vibração (0-1000).

-   **Módulo de Processamento**: Implementa a lógica de classificação dos níveis de vibração com base em limiares pré-definidos. Também gerencia a conexão Wi-Fi e a comunicação com o broker MQTT.

-   **Módulo de Atuação e Interface**: Consiste em LEDs coloridos (verde, amarelo e vermelho) que indicam o estado atual do sistema. A interface principal para monitoramento é a plataforma MQTT, que recebe os dados de telemetria e eventos.

### 2.2. Implementação do Firmware

O firmware foi desenvolvido em C/C++ utilizando a IDE Arduino e as bibliotecas `ESP8266WiFi`, `PubSubClient` e `ArduinoJson`.

#### Parâmetros de Classificação

A Tabela 1 descreve os parâmetros utilizados para a classificação dos níveis de vibração.

**Tabela 1 - Parâmetros de Classificação dos Níveis de Vibração**

| NÍVEL   | FAIXA DE VALORES (vib_index) | LED ATIVO | EVENTO MQTT GERADO     |
| :------ | :--------------------------- | :-------- | :--------------------- |
| Normal  | < 300                        | Verde     | mudanca_status         |
| Atenção | 300 - 600                    | Amarelo   | mudanca_status         |
| Crítico | > 600                        | Vermelho  | **alarme_vibracao**    |

#### Lógica de Classificação

O trecho de código a seguir demonstra a lógica principal para classificar o índice de vibração e determinar o status do sistema. O código também implementa uma histerese para evitar que o estado do sistema oscile rapidamente quando o valor de vibração está próximo de um limiar.

```cpp
String classificarStatusLed(int vibIndex) {
  // Lógica de histerese para evitar oscilações
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

  // Classificação direta
  if (vibIndex < limiares.vib_warn)       return STATUS_NORMAL;
  if (vibIndex <= limiares.vib_alarm)     return STATUS_ATENCAO;
  return STATUS_CRITICO;
}
```

#### Comunicação MQTT

O sistema utiliza o protocolo MQTT para comunicação, com os seguintes tópicos:

-   `.../state`: Publica o estado de conexão do dispositivo ("online").
-   `.../telemetry`: Publica periodicamente os dados de telemetria, como o índice de vibração e o status atual.
-   `.../event`: Publica eventos importantes, como a mudança de status e, principalmente, o `alarme_vibracao` quando o nível crítico é atingido.
-   `.../cmd`: Subscreve a este tópico para receber comandos remotos, como `get_status` e `set_thresholds`.
-   `.../config`: Publica a configuração atual do dispositivo, como os limiares de vibração.

## 3. CONSIDERAÇÕES FINAIS

O projeto resultou em um sistema funcional e de baixo custo para o monitoramento de vibração, utilizando o ESP8266 e o protocolo MQTT. Os objetivos propostos foram alcançados, demonstrando a viabilidade da solução para aplicações de IoT e manutenção preditiva.

A modularidade do código e a utilização de um protocolo padrão como o MQTT facilitam a expansão do sistema, como a integração com bancos de dados, dashboards de visualização (como o Grafana) e outros sistemas de automação.

Como trabalhos futuros, sugere-se a substituição do potenciômetro por um sensor de vibração real (como um acelerômetro) e o desenvolvimento de uma interface web para visualização dos dados e controle do dispositivo.
