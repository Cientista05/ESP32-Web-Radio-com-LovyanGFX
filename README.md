# ESP32 Web Radio com LovyanGFX

Rádio web simples para ESP32 com display touch 480×320, desenvolvida com LovyanGFX e ESP32-audioI2S.

O projeto reproduz estações de rádio pela internet e possui uma interface gráfica controlada por gestos.

## Recursos

* Reprodução de rádio web em MP3 e AAC
* Áudio executado em uma tarefa dedicada no core 0
* Interface gráfica com LovyanGFX
* Medidor de sinal Wi-Fi em barras
* Exibição do RSSI em dBm
* Relógio sincronizado por NTP
* Data com mês abreviado
* Exibição de codec e bitrate
* Exibição de artista e música
* Rolagem horizontal para textos longos
* Lista de estações com rolagem por toque
* Seleção de estação pela tela
* Controle de volume por gesto horizontal
* Interface sem LVGL

## Hardware utilizado

* ESP32 com display touch 480×320
* Controlador de display ST7796U
* Saída de áudio I2S
* DAC ou amplificador I2S compatível

### Pinos I2S

```cpp
#define I2S_DOUT 37
#define I2S_BCLK 36
#define I2S_LRC  35
```

Os pinos podem ser alterados no arquivo `config.h`.

## Bibliotecas

* LovyanGFX 1.2.24
* ESP32 Arduino Core 3.3.4
* ESP32-audioI2S 3.0.12

## Estrutura do projeto

```text
SC01_radioweb/
├── SC01_radioweb.ino
├── config.h
├── ST7796U.h
├── network.h
├── network.cpp
├── audio_player.h
├── audio_player.cpp
├── stations.h
├── stations.cpp
├── ui.h
└── ui.cpp
```

## Responsabilidade dos arquivos

| Arquivo             | Função                                            |
| ------------------- | ------------------------------------------------- |
| `SC01_radioweb.ino` | Inicialização principal do projeto                |
| `config.h`          | Wi-Fi, pinos, volume e configurações gerais       |
| `ST7796U.h`         | Configuração do display com LovyanGFX             |
| `network.cpp`       | Wi-Fi, RSSI, relógio e data                       |
| `audio_player.cpp`  | Reprodução de áudio, callbacks e troca de estação |
| `stations.cpp`      | Lista de estações                                 |
| `ui.cpp`            | Interface gráfica e controle por toque            |

## Configuração do Wi-Fi

Edite o arquivo `config.h`:

```cpp
constexpr const char* WIFI_SSID = "NOME_DA_REDE";
constexpr const char* WIFI_PASSWORD = "SENHA_DA_REDE";
```

Não publique sua senha real no GitHub.

Uma alternativa é criar um arquivo `secrets.h`:

```cpp
#pragma once

#define WIFI_SSID_PRIVATE "NOME_DA_REDE"
#define WIFI_PASSWORD_PRIVATE "SENHA_DA_REDE"
```

Depois inclua esse arquivo em `config.h`.

Adicione `secrets.h` ao `.gitignore`:

```gitignore
secrets.h
```

## Lista de estações

As estações ficam no arquivo `stations.cpp`.

Exemplo:

```cpp
const RadioStation stations[] = {
  {
    1,
    "Radio Cidade",
    "http://24273.live.streamtheworld.com/RADIOCIDADEAAC.aac"
  },
  {
    2,
    "Antena 1",
    "https://antenaone.crossradio.com.br/stream/1/"
  },
  {
    3,
    "CBN",
    "https://playerservices.streamtheworld.com/api/livestream-redirect/CBN_SPAAC.aac"
  }
};

const size_t STATION_COUNT =
  sizeof(stations) / sizeof(stations[0]);
```

Cada estação possui:

```cpp
struct RadioStation {
  uint16_t id;
  const char* name;
  const char* url;
};
```

O ID deve ser único.

## Gestos da interface

### Abrir lista de estações

Arraste o dedo de cima para baixo a partir da parte superior da tela.

### Escolher uma estação

Role a lista verticalmente e toque no nome da estação.

### Ajustar volume

Faça um movimento horizontal:

* para a direita: aumenta o volume;
* para a esquerda: diminui o volume.

O volume utiliza a faixa de `0` a `21` da biblioteca ESP32-audioI2S.

## Metadata

O projeto utiliza o callback:

```cpp
void audio_showstreamtitle(const char* info);
```

A metadata é separada no formato:

```text
Artista - Música
```

Quando o texto é maior que a área disponível, ele entra em rolagem horizontal.

## Compilação

1. Instale o suporte às placas ESP32 no Arduino IDE.
2. Instale LovyanGFX.
3. Instale ESP32-audioI2S 3.0.12.
4. Abra o arquivo `SC01_radioweb.ino`.
5. Configure a placa e a porta serial.
6. Compile e envie para o ESP32.

## Observações

Algumas URLs de rádio podem mudar ou deixar de funcionar.

Utilize sempre a URL direta do áudio, e não o endereço da página da emissora.

Exemplos de formatos comuns:

```text
http://servidor/stream
https://servidor/live
http://servidor/radio.mp3
http://servidor/radio.aac
```

## Segurança

Antes de publicar o projeto, remova:

* nome da sua rede Wi-Fi;
* senha do Wi-Fi;
* informações pessoais;
* endereços privados;
* arquivos temporários do Arduino IDE.

## Licença

Este projeto pode ser distribuído sob a licença MIT.

Consulte o arquivo `LICENSE` para mais informações.

## Autor

Desenvolvido por Anderson Cardoso da Silva.

