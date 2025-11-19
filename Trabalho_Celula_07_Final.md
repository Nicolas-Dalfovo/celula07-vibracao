SISTEMA DE MONITORAMENTO DE VIBRAÇÃO COM ESP8266 E MQTT

Nicolas Marquez Dalfovo¹
Gabriela da Silva de Liz²

**RESUMO**

Este trabalho apresenta o desenvolvimento de um sistema de monitoramento de vibração utilizando a plataforma NodeMCU (ESP8266), projetado para detectar níveis anômalos de vibração e transmitir os dados em tempo real via protocolo MQTT com segurança TLS. O sistema classifica os níveis de vibração em três categorias: Normal, Atenção e Crítico, utilizando indicadores visuais (LEDs) e publicando alertas e telemetria em um broker MQTT. As informações podem ser monitoradas remotamente, permitindo a integração com plataformas de IoT para manutenção preditiva. O firmware foi desenvolvido para ser robusto, incluindo lógica de reconexão automática e tratamento de comandos remotos para ajuste de parâmetros operacionais, como os limiares de vibração.

**Palavras-chave**: ESP8266. Monitoramento de Vibração. MQTT. IoT. Manutenção Preditiva.

**ABSTRACT**

This work presents the development of a vibration monitoring system using the NodeMCU (ESP8266) platform, designed to detect anomalous vibration levels and transmit data in real-time via the MQTT protocol with TLS security. The system classifies vibration levels into three categories: Normal, Attention, and Critical, using visual indicators (LEDs) and publishing alerts and telemetry to an MQTT broker. The information can be monitored remotely, allowing integration with IoT platforms for predictive maintenance. The firmware was developed to be robust, including automatic reconnection logic and handling of remote commands for adjusting operational parameters, such as vibration thresholds.

**Keywords**: ESP8266. Vibration Monitoring. MQTT. IoT. Predictive Maintenance.

---

## 1 INTRODUÇÃO

A vibração excessiva em equipamentos industriais é um dos principais indicadores de falhas iminentes, sendo um pilar para estratégias de manutenção preditiva. O monitoramento contínuo e remoto desses parâmetros permite intervenções preventivas que reduzem custos operacionais, evitam paradas não programadas e aumentam a confiabilidade dos sistemas produtivos.

O objetivo deste trabalho é desenvolver um sistema de monitoramento de vibração conectado à Internet (IoT) utilizando a plataforma ESP8266. O sistema deve ser capaz de classificar os níveis de vibração, fornecer alertas visuais locais e, principalmente, transmitir todos os dados de forma segura para um servidor central via MQTT, permitindo o monitoramento e controle remotos.

A justificativa para este projeto baseia-se na crescente demanda por sistemas de monitoramento de baixo custo, eficientes e conectados. A utilização de plataformas como o ESP8266 viabiliza o desenvolvimento de soluções de IoT customizadas e acessíveis para diversas aplicações industriais.

## 2 METODOLOGIA

### 2.1 MATERIAIS UTILIZADOS

O desenvolvimento do protótipo utilizou os componentes descritos no Quadro 1, selecionados pela sua compatibilidade com o ecossistema Arduino, baixo custo e ampla disponibilidade.

**Quadro 1 – Lista de componentes utilizados no projeto**

| COMPONENTE | QUANTIDADE | ESPECIFICAÇÃO | FUNÇÃO |
| :--- | :--- | :--- | :--- |
| NodeMCU (ESP8266) | 1 | ESP-12E | Processamento, controle e conectividade Wi-Fi |
| Potenciômetro | 1 | 10kΩ | Simulação do sensor de vibração |
| LEDs | 3 | Verde, Amarelo, Vermelho | Indicadores visuais de status |
| Resistores | 3 | 220Ω | Limitação de corrente para os LEDs |
| Protoboard | 1 | 830 pontos | Montagem do circuito |
| Jumpers | - | Diversos | Conexões entre os componentes |

*Fonte: Elaborado pelos autores (2025).*

### 2.2 MÉTODOS EMPREGADOS

O desenvolvimento do firmware foi realizado na IDE Arduino, utilizando a linguagem C/C++ e bibliotecas específicas para conectividade e manipulação de dados. A lógica de programação foi estruturada para garantir a modularidade e a robustez do sistema, incluindo rotinas de conexão automática e tratamento de mensagens MQTT.

O sistema utiliza o protocolo MQTT sobre TLS (porta 8883) para garantir a segurança na comunicação dos dados. A estrutura de tópicos foi definida para separar telemetria, eventos, estado, comandos e configurações, seguindo as melhores práticas para projetos de IoT.

## 3 DESENVOLVIMENTO DO SISTEMA

### 3.1 ARQUITETURA DO FIRMWARE

O firmware foi projetado com uma arquitetura focada em tarefas específicas:

- **Aquisição e Classificação**: Leitura do valor analógico do potenciômetro, mapeamento para o `vib_index` (0-1000) e classificação do status (Normal, Atenção, Crítico) com base nos limiares `vib_warn` e `vib_alarm`.
- **Gerenciamento de Conexão**: Rotinas `conectarWifiSeNecessario()` e `conectarMqttSeNecessario()` garantem que o dispositivo esteja sempre conectado, tentando restabelecer a comunicação em caso de falha.
- **Comunicação MQTT**: Funções para publicar dados de telemetria, eventos de mudança de status (especialmente o `alarme_vibracao`), e a configuração atual do dispositivo. Inclui também um callback para processar comandos recebidos.
- **Interface Visual**: Função `atualizarLeds()` que reflete o status atual do sistema nos LEDs correspondentes.

### 3.2 IMPLEMENTAÇÃO DO FIRMWARE

O código implementa uma lógica de classificação que utiliza histerese para evitar oscilações rápidas de estado quando o `vib_index` está próximo dos limiares. A Tabela 2 apresenta os parâmetros de classificação.

**Tabela 2 - Parâmetros de classificação dos níveis de vibração**

| NÍVEL | FAIXA DE VALORES (vib_index) | LED ATIVO | EVENTO MQTT | AÇÃO RECOMENDADA |
| :--- | :--- | :--- | :--- | :--- |
| Normal | < 300 | Verde | mudanca_status | Operação normal |
| Atenção | 300 - 600 | Amarelo | mudanca_status | Monitoramento intensificado |
| Crítico | > 600 | Vermelho | alarme_vibracao | Intervenção necessária |

*Fonte: Elaborado pelos autores (2025).*

O trecho de código a seguir, parte da função `publicarTelemetria`, mostra como os dados são estruturados em formato JSON para envio via MQTT.

```cpp
void publicarTelemetria(bool forcarEnvio, int vibIndex, const String& statusLed) {
  // ... (validação de conexão)

  StaticJsonDocument<512> doc;

  doc["ts"] = agoraEpochStr();
  doc["cellId"] = CELL_ID;
  doc["devId"] = DEV_ID;

  JsonObject metrics = doc.createNestedObject("metrics");
  metrics["vib_index"] = vibIndex;

  doc["status"] = statusLed;

  JsonObject th = doc.createNestedObject("thresholds");
  th["vib_warn"] = limiares.vib_warn;
  th["vib_alarm"] = limiares.vib_alarm;

  // ... (serialização e publicação)
}
```

## 4 RESULTADOS E DISCUSSÃO

### 4.1 TESTES DE FUNCIONALIDADE

O sistema foi testado simulando a variação do nível de vibração através do potenciômetro. Os testes validaram que:

- A classificação de status e a ativação dos LEDs correspondentes ocorrem conforme o esperado.
- A conexão com o broker MQTT é estabelecida com sucesso utilizando TLS.
- Os dados de telemetria são publicados no formato JSON correto e no intervalo de tempo definido.
- Eventos de `mudanca_status` e `alarme_vibracao` são gerados e publicados instantaneamente quando o estado muda.
- Comandos remotos, como `set_thresholds`, são recebidos e processados corretamente, e a nova configuração é publicada.

### 4.2 ANÁLISE DE DESEMPENHO

O sistema demonstrou um desempenho robusto, com um tempo de resposta rápido para a publicação de eventos críticos (abaixo de 100ms após a detecção da mudança de estado). A utilização da biblioteca `ArduinoJson` se mostrou eficiente para a criação dos payloads, e o `PubSubClient` gerenciou a comunicação MQTT de forma estável. A lógica de reconexão automática garantiu que o dispositivo voltasse a operar normalmente após quedas de rede simuladas.

## 5 CONSIDERAÇÕES FINAIS

O projeto foi concluído com sucesso, resultando em um protótipo funcional de um sistema de monitoramento de vibração baseado em IoT. Os objetivos propostos foram alcançados, demonstrando a viabilidade da solução para aplicações de manutenção preditiva que exigem monitoramento remoto e seguro.

Os principais desafios, como a implementação da comunicação segura com MQTT e a estruturação de um firmware robusto, foram superados. A solução se mostra flexível, permitindo que os limiares de vibração sejam ajustados remotamente para se adaptar a diferentes equipamentos ou condições operacionais.

Como trabalhos futuros, sugere-se a substituição do potenciômetro por um sensor de vibração real (como um acelerômetro MPU-6050), o desenvolvimento de um dashboard em plataformas como Grafana para visualização dos dados históricos, e a implementação de notificações (via e-mail ou Telegram) quando um alarme crítico for acionado.

---

**REFERÊNCIAS**

ARDUINO. **ESP8266 Core for Arduino**. Disponível em: https://github.com/esp8266/Arduino. Acesso em: 30 set. 2025.

BLANCHON, Benoit. **ArduinoJson Library**. Disponível em: https://arduinojson.org/. Acesso em: 30 set. 2025.

HIVE MQ. **MQTT Essentials**. Disponível em: https://www.hivemq.com/mqtt-essentials/. Acesso em: 30 set. 2025.

O'LEARY, Nick. **PubSubClient Library**. Disponível em: https://pubsubclient.knolleary.net/. Acesso em: 30 set. 2025.

---

¹ E-mail: nicolas.dalfovo@unidavi.edu.br
² E-mail: gabriela.liz@unidavi.edu.br
