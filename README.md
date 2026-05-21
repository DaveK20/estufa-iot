# 🌱 Estufa IoT

> Sistema embarcado de monitoramento e automação para estufas agrícolas baseado em ESP32, com controle automático de ventilação e nebulização, e envio de dados em tempo real para a nuvem via ThingSpeak.

![Platform](https://img.shields.io/badge/Platform-ESP32-blue?logo=espressif)
![Framework](https://img.shields.io/badge/Framework-Arduino-teal?logo=arduino)
![IDE](https://img.shields.io/badge/IDE-PlatformIO-orange?logo=platformio)
![Language](https://img.shields.io/badge/Language-C%2B%2B-00599C?logo=c%2B%2B)
![Cloud](https://img.shields.io/badge/Cloud-ThingSpeak-brightgreen)
![License](https://img.shields.io/badge/License-MIT-yellow)

---

## 📋 Sobre o Projeto

A **Estufa Inteligente** é um sistema embarcado de baixo custo para monitoramento e controle contínuo de variáveis ambientais em estufas. O dispositivo utiliza dois sensores DHT22 — um interno e outro externo — para coletar temperatura e umidade em tempo real, tomando decisões autônomas de acionamento de ventilador e nebulizador com base em limiares configuráveis. Todos os dados, incluindo o status dos atuadores, são enviados periodicamente para a nuvem via ThingSpeak.

O projeto foi desenvolvido como parte de estudos em sistemas embarcados e IoT, explorando timers de hardware, watchdog via `esp_restart()`, controle de relés, comunicação Wi-Fi e integração com plataformas de análise em nuvem.

---

## ✨ Funcionalidades

- 🌡️ Leitura de **temperatura e umidade** interna e externa via dois sensores DHT22
- 💨 **Acionamento automático do ventilador** quando a temperatura interna supera 35 °C
- 💧 **Acionamento automático do nebulizador** quando a temperatura interna atinge 38 °C, com ciclos de funcionamento configuráveis
- ☁️ Envio automático de dados para a nuvem a cada **10 minutos** via ThingSpeak (6 campos: temperatura, umidade interna/externa e status dos atuadores)
- 📊 Visualização remota por **dashboard online**
- ⏱️ **Watchdog via timer de hardware** com reinicialização automática do ESP32 a cada 15 minutos de inatividade
---

## 🛠️ Hardware Utilizado

| Componente | Quantidade | Descrição |
|---|---|---|
| **ESP32 DevKit** | 1 | Microcontrolador principal com Wi-Fi integrado |
| **Sensor DHT22** | 2 | Temperatura e umidade — um interno, um externo |
| **Módulo Relé** | 1 | Acionamento do ventilador (GPIO 26) e do nebulizador (GPIO 25) |
| **Ventilador** | 1 | Controlado via relé — resfriamento da estufa |
| **Bomba/Nebulizador** | 1 | Controlado via relé — umidificação da estufa |
| Resistor 10kΩ | 2 | Pull-up para os pinos de dados dos DHTs |
| Protoboard + Jumpers | — | Montagem e prototipagem do circuito |
---

## ⚙️ Lógica de Controle

O sistema toma decisões de forma autônoma com base na temperatura interna da estufa:

| Condição | Ventilador | Nebulizador |
|---|---|---|
| `tempEstufa < 35 °C` | ❌ Desligado | ❌ Desligado |
| `tempEstufa > 35 °C` | ✅ Ligado | ❌ Desligado |
| `tempEstufa ≥ 38 °C` | ✅ Ligado | ✅ Ciclo ativo |

### Ciclo do Nebulizador

Quando acionado, o nebulizador executa o seguinte ciclo (todos os intervalos configuráveis via `#define`):

```
Liga por 60s → Desliga por 60s → (repete 4x) → Desliga definitivamente
```

Após o ciclo, o sistema aguarda **10 minutos** antes de permitir nova ativação, evitando excesso de umidade.

---

## 📡 Arquitetura do Sistema

```
[DHT22 Interno]  ──┐
                   ├──► [ESP32 DevKit] ──(Wi-Fi / HTTP)──► [ThingSpeak Cloud]
[DHT22 Externo]  ──┘          │                                     │
                              │                               [Dashboard]
                    ┌─────────┴─────────┐
               [Relé Ventilador]  [Relé Nebulizador]
                    │                   │
               [Ventilador]        [Bomba/Nebulizador]
```

---

## 📦 Dependências

Gerenciadas automaticamente pelo **PlatformIO**:

```ini
lib_deps =
    mathworks/ThingSpeak@^2.0.0
    adafruit/Adafruit Unified Sensor@^1.1.11
    adafruit/DHT sensor library@^1.4.4
```

---

## 🚀 Como Usar

### Pré-requisitos

- [PlatformIO](https://platformio.org/) instalado (VS Code + extensão PlatformIO IDE)
- Conta criada no [ThingSpeak](https://thingspeak.com/) com canal de 6 campos configurado
- ESP32, 2x DHT22 e módulo relé de 2 canais

### Configuração

**1. Clone o repositório:**
```bash
git clone https://github.com/DaveK20/estufa-inteligente.git
cd estufa-inteligente
```

**2. Configure suas credenciais** em `src/main.cpp`:
```cpp
#define SSID_REDE "SUA_REDE_WIFI"
#define SENHA_REDE "SUA_SENHA"

int channelNumber = SEU_CHANNEL_ID;
const char *writeAPIKey = "SUA_WRITE_API_KEY";
```

**3. Ajuste os limiares de temperatura** conforme necessário:
```cpp
int tempAtivacaoVentilador = 35;  // °C — acima disso, ventilador liga
int tempAtivacaoNebulizador = 38; // °C — acima disso, nebulizador é acionado
```

**4. Compile e faça o upload:**
```bash
pio run --target upload
```

**5. Monitore a saída serial (115200 baud):**
```bash
pio device monitor
```

### Mapeamento de Pinos

```
ESP32          Periférico
──────         ──────────
GPIO 4  ──►   DHT22 Interno (DATA)
GPIO 5  ──►   DHT22 Externo (DATA)
GPIO 26 ──►   Relé Canal 1 → Ventilador
GPIO 25 ──►   Relé Canal 2 → Nebulizador
3.3V    ──►   VCC (sensores)
GND     ──►   GND
```

> ⚠️ Os relés operam em lógica **invertida** (LOW = ligado, HIGH = desligado), típico de módulos de relé com optoacoplador.

---

## 📊 Campos no ThingSpeak

| Field | Dado |
|---|---|
| Field 1 | Temperatura interna (°C) |
| Field 2 | Umidade interna (%) |
| Field 3 | Temperatura externa (°C) |
| Field 4 | Umidade externa (%) |
| Field 5 | Status do ventilador (0/1) |
| Field 6 | Status do nebulizador (0/1) |

---

## 📁 Estrutura do Projeto

```
estufa-inteligente/
├── src/
│   └── main.cpp          # Código principal
├── platformio.ini         # Configuração do projeto PlatformIO
└── README.md
```

## 📄 Licença

Este projeto está sob a licença MIT. Veja o arquivo [LICENSE](LICENSE) para mais detalhes.
