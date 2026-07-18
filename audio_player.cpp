#include "audio_player.h"
#include "stations.h"
#include <Audio.h>
#include "config.h"

// --------------------------------------------------
// OBJETOS E VARIÁVEIS INTERNAS
// --------------------------------------------------

static Audio audio;

static TaskHandle_t audioTaskHandle = nullptr;

static portMUX_TYPE metadataMux = portMUX_INITIALIZER_UNLOCKED;

static AudioMetadata currentMetadata = { "", "Aguardando informacoes...", "---", "-- ---" };

static volatile bool metadataChanged = true;

static uint8_t currentVolume = AUDIO_VOLUME;

static volatile bool stationChangeRequested = false;
static volatile size_t requestedStationIndex = 0;

static size_t currentStationIndex = 0;

static volatile bool playing = true;
static volatile bool toggleRequested = false;

// --------------------------------------------------
// FUNÇÕES INTERNAS
// --------------------------------------------------

static void audioTask(void* parameter) {
  bool connected = audio.connecttohost(
    stations[currentStationIndex].url);

  playing = connected;

  for (;;) {
    // -----------------------------------------------
    // PLAY / STOP
    // -----------------------------------------------
    if (toggleRequested) {
      toggleRequested = false;

      if (playing) {
        Serial.println("[Audio] STOP");

        audio.stopSong();
        playing = false;
      } else {
        Serial.println("[Audio] PLAY");

        bool reconnected = audio.connecttohost(
          stations[currentStationIndex].url);

        playing = reconnected;

        Serial.printf(
          "[Audio] Reconexao: %s\n",
          reconnected ? "OK" : "FALHOU");
      }
    }

    // -----------------------------------------------
    // TROCA DE ESTAÇÃO
    // -----------------------------------------------
    if (stationChangeRequested) {
      size_t newIndex;

      portENTER_CRITICAL(&metadataMux);

      newIndex = requestedStationIndex;
      stationChangeRequested = false;

      portEXIT_CRITICAL(&metadataMux);

      if (newIndex < STATION_COUNT) {
        audio.stopSong();

        currentStationIndex = newIndex;

        portENTER_CRITICAL(&metadataMux);

        strlcpy(
          currentMetadata.artist,
          stations[currentStationIndex].name,
          sizeof(currentMetadata.artist));

        strlcpy(
          currentMetadata.title,
          "Aguardando informacoes...",
          sizeof(currentMetadata.title));

        currentMetadata.codec[0] = '\0';
        currentMetadata.bitrate[0] = '\0';

        metadataChanged = true;

        portEXIT_CRITICAL(&metadataMux);

        bool stationConnected = audio.connecttohost(
          stations[currentStationIndex].url);

        playing = stationConnected;

        Serial.printf(
          "[Audio] Estacao selecionada: %s - %s\n",
          stations[currentStationIndex].name,
          stationConnected ? "OK" : "FALHOU");
      }
    }

    if (playing) {
      audio.loop();
    }

    vTaskDelay(1);
  }
}

static void setCodec(const char* codec) {
  if (codec == nullptr) {
    return;
  }

  portENTER_CRITICAL(&metadataMux);

  strlcpy(currentMetadata.codec, codec, sizeof(currentMetadata.codec));

  metadataChanged = true;

  portEXIT_CRITICAL(&metadataMux);
}

// --------------------------------------------------
// INICIALIZAÇÃO
// --------------------------------------------------

void audioPlayerBegin() {
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  audio.setVolume(currentVolume);
}

void audioPlayerStart() {
  if (audioTaskHandle != nullptr) {
    return;
  }

  xTaskCreatePinnedToCore(audioTask, "AudioTask", 8192, nullptr, 0, &audioTaskHandle, 0);
}

// --------------------------------------------------
// VOLUME
// --------------------------------------------------

void audioPlayerSetVolume(uint8_t volume) {
  if (volume > 21) {
    volume = 21;
  }

  currentVolume = volume;
  audio.setVolume(currentVolume);
}

uint8_t audioPlayerGetVolume() {
  return currentVolume;
}

// --------------------------------------------------
// METADATA
// --------------------------------------------------

bool audioPlayerReadMetadata(AudioMetadata& metadata) {
  portENTER_CRITICAL(&metadataMux);

  memcpy(&metadata, &currentMetadata, sizeof(AudioMetadata));

  portEXIT_CRITICAL(&metadataMux);

  return true;
}

bool audioPlayerHasUpdate() {
  bool updated = false;

  portENTER_CRITICAL(&metadataMux);

  if (metadataChanged) {
    metadataChanged = false;
    updated = true;
  }

  portEXIT_CRITICAL(&metadataMux);

  return updated;
}

// --------------------------------------------------
// CALLBACK: ARTISTA E MÚSICA
// --------------------------------------------------

void audio_showstreamtitle(const char* info) {
  if (info == nullptr) {
    return;
  }

  String raw = String(info);

  raw.replace("StreamTitle='", "");
  raw.replace("';", "");
  raw.trim();

  String artist;
  String title;

  int separator = raw.indexOf(" - ");

  if (separator > 0) {
    artist = raw.substring(0, separator);
    title = raw.substring(separator + 3);
  } else {
    artist = stations[currentStationIndex].name;
    title = raw;
  }

  artist.trim();
  title.trim();

  if (artist.length() == 0) {
    artist = stations[currentStationIndex].name;
  }

  if (title.length() == 0) {
    title = "Aguardando informacoes...";
  }

  portENTER_CRITICAL(&metadataMux);

  strlcpy(currentMetadata.artist, artist.c_str(), sizeof(currentMetadata.artist));
  strlcpy(currentMetadata.title, title.c_str(), sizeof(currentMetadata.title));

  metadataChanged = true;

  portEXIT_CRITICAL(&metadataMux);
}

// --------------------------------------------------
// CALLBACK: BITRATE
// --------------------------------------------------

void audio_bitrate(const char* info) {
  if (info == nullptr) {
    return;
  }

  String value = String(info);
  value.trim();

  uint32_t bitrate = 0;

  for (size_t i = 0; i < value.length(); i++) {
    if (isDigit(value[i])) {
      bitrate *= 10;
      bitrate += value[i] - '0';
    }
  }

  // Algumas versões retornam bits por segundo
  if (bitrate > 1000) {
    bitrate /= 1000;
  }

  char bitrateText[20];

  if (bitrate > 0) {
    snprintf(bitrateText, sizeof(bitrateText), "%lu kbps", static_cast<unsigned long>(bitrate));
  } else {
    strlcpy(bitrateText, "--- kbps", sizeof(bitrateText));
  }

  portENTER_CRITICAL(&metadataMux);

  strlcpy(currentMetadata.bitrate, bitrateText, sizeof(currentMetadata.bitrate));

  metadataChanged = true;

  portEXIT_CRITICAL(&metadataMux);
}

// --------------------------------------------------
// CALLBACK: INFORMAÇÕES DO ÁUDIO
// --------------------------------------------------

void audio_info(const char* info) {
  if (info == nullptr) {
    return;
  }

  String message = String(info);
  String upperMessage = message;

  upperMessage.toUpperCase();

  if (upperMessage.indexOf("AAC") >= 0) {
    setCodec("AAC");

  } else if (upperMessage.indexOf("MP3") >= 0) {
    setCodec("MP3");

  } else if (upperMessage.indexOf("FLAC") >= 0) {
    setCodec("FLAC");

  } else if (upperMessage.indexOf("OPUS") >= 0) {
    setCodec("OPUS");

  } else if (
    upperMessage.indexOf("VORBIS") >= 0 || upperMessage.indexOf("OGG") >= 0) {
    setCodec("OGG");

  } else if (upperMessage.indexOf("WAV") >= 0) {
    setCodec("WAV");

  } else if (upperMessage.indexOf("M4A") >= 0) {
    setCodec("M4A");
  }

  Serial.printf("[Audio] %s\n", info);
}

void audioPlayerSelectStation(size_t index) {
  if (index >= STATION_COUNT) {
    return;
  }

  portENTER_CRITICAL(&metadataMux);

  requestedStationIndex = index;
  stationChangeRequested = true;

  portEXIT_CRITICAL(&metadataMux);
}

size_t audioPlayerGetStationIndex() {
  return currentStationIndex;
}

const char* audioPlayerGetStationName() {
  return stations[currentStationIndex].name;
}

void audioPlayerToggle() {
  toggleRequested = true;
}

bool audioPlayerIsPlaying() {
  return playing;
}