#include "lgfx/v1/misc/enum.hpp"
#include "lgfx/v1/lgfx_fonts.hpp"
#include "ui.h"

#include <LovyanGFX.hpp>

#include "ST7796U.h"
#include "config.h"
#include "network.h"
#include "audio_player.h"
#include "stations.h"

// --------------------------------------------------
// DISPLAY
// --------------------------------------------------

static LGFX lcd;

static LGFX_Sprite artistSprite(&lcd);
static LGFX_Sprite titleSprite(&lcd);
static LGFX_Sprite stationListSprite(&lcd);

// --------------------------------------------------
// PROTÓTIPOS INTERNOS
// --------------------------------------------------

static void drawBase();
static void drawHeader();

static void drawRSSI();
static void drawClock();
static void drawCodecBitrate();

static String limitText(const String& text, uint16_t maximumWidth);

static void drawMetadata();
static void updateMetadataScroll();

static void drawStationList();
static void limitStationListOffset();
static void selectStationFromTouch(int16_t touchY);
static void closeStationList();

static void openVolumePanel();
static void closeVolumePanel();
static void updateVolumeFromMovement(int16_t deltaX);

// --------------------------------------------------
// CONTROLE DE ATUALIZAÇÃO
// --------------------------------------------------

static unsigned long lastRSSIUpdate = 0;
static unsigned long lastClockUpdate = 0;

static bool stationListSpriteReady = false;

static char lastClock[8] = "";
static char lastArtist[128] = "";
static char lastTitle[160] = "";
static char lastCodec[16] = "";
static char lastBitrate[20] = "";
static char lastDate[16] = "";

// --------------------------------------------------
// LISTA DE ESTAÇÕES
// --------------------------------------------------

static bool stationListOpen = false;

static bool touchPressed = false;
static bool touchMoved = false;

static int16_t touchStartX = 0;
static int16_t touchStartY = 0;
static int16_t touchLastY = 0;

static int16_t stationListOffset = 0;

static constexpr int16_t SCREEN_WIDTH = 480;
static constexpr int16_t SCREEN_HEIGHT = 320;

static constexpr int16_t LIST_Y = 60;
static constexpr int16_t LIST_HEIGHT = 260;

static constexpr int16_t STATION_ITEM_HEIGHT = 48;

static constexpr int16_t OPEN_SWIPE_DISTANCE = 45;
static constexpr int16_t TOUCH_MOVE_THRESHOLD = 8;

static constexpr uint16_t COLOR_LIST_BACKGROUND = TFT_BLUE;
static constexpr uint32_t COLOR_LIST_ITEM = TFT_DARKCYAN;
static constexpr uint16_t COLOR_LIST_SELECTED = TFT_MIDNIGHTBLUE;
static constexpr uint16_t COLOR_LIST_BORDER = TFT_CYAN;

// --------------------------------------------------
// ESTADO DA ROLAGEM
// --------------------------------------------------

static unsigned long lastScrollUpdate = 0;

static constexpr uint32_t SCROLL_INTERVAL = 45;
static constexpr int16_t SCROLL_GAP = 80;

static constexpr int16_t TEXT_AREA_X = 15;
static constexpr int16_t TEXT_AREA_WIDTH = 450;

static constexpr int16_t ARTIST_AREA_Y = 100;
static constexpr int16_t ARTIST_AREA_HEIGHT = 55;

static constexpr int16_t TITLE_AREA_Y = 190;
static constexpr int16_t TITLE_AREA_HEIGHT = 45;

static constexpr uint32_t SCROLL_PAUSE_TIME = 1800;

enum class ScrollMode {
  CENTER_PAUSE,
  MOVING,
  RIGHT_PAUSE
};

struct ScrollState {
  int32_t x;
  int32_t textWidth;
  ScrollMode mode;
  unsigned long pauseStarted;
};

static ScrollState artistScrollState = { 0, 0, ScrollMode::CENTER_PAUSE, 0 };
static ScrollState titleScrollState = { 0, 0, ScrollMode::CENTER_PAUSE, 0 };

static String scrollArtist = "";
static String scrollTitle = "";

// --------------------------------------------------
// PAINEL DE VOLUME
// --------------------------------------------------

static bool volumePanelOpen = false;

static LGFX_Sprite volumeSprite(&lcd);
static bool volumeSpriteReady = false;

static constexpr int16_t VOLUME_SPRITE_WIDTH = 320;
static constexpr int16_t VOLUME_SPRITE_HEIGHT = 90;

static constexpr int16_t VOLUME_SPRITE_X =
  (SCREEN_WIDTH - VOLUME_SPRITE_WIDTH) / 2;

static constexpr int16_t VOLUME_SPRITE_Y =
  LIST_Y + 75;

static constexpr int16_t VOLUME_MIN = 0;
static constexpr int16_t VOLUME_MAX = 21;

static constexpr int16_t VOLUME_OPEN_DISTANCE = 45;

static constexpr uint32_t VOLUME_CLOSE_TIME = 1500;

static unsigned long volumeLastInteraction = 0;

static int16_t volumeMovementAccumulator = 0;

static int16_t touchLastX = 0;

// --------------------------------------------------
// INICIALIZAÇÃO
// --------------------------------------------------

void uiBegin() {
  lcd.init();
  lcd.setRotation(DISPLAY_ROTATION);
  lcd.setBrightness(DISPLAY_BRIGHTNESS);

  artistSprite.setColorDepth(8);
  artistSprite.createSprite(
    TEXT_AREA_WIDTH,
    ARTIST_AREA_HEIGHT);

  titleSprite.setColorDepth(8);
  titleSprite.createSprite(
    TEXT_AREA_WIDTH,
    TITLE_AREA_HEIGHT);

  stationListSprite.setColorDepth(8);

  stationListSpriteReady =
    stationListSprite.createSprite(
      SCREEN_WIDTH,
      SCREEN_HEIGHT - LIST_Y - 42)
    != nullptr;

  volumeSprite.setColorDepth(8);

  volumeSpriteReady =
    volumeSprite.createSprite(
      VOLUME_SPRITE_WIDTH,
      VOLUME_SPRITE_HEIGHT)
    != nullptr;

  if (volumeSpriteReady) {
    Serial.println("Sprite de volume criado");
  } else {
    Serial.println("Falha ao criar sprite de volume");
  }

  drawBase();
}

// --------------------------------------------------
// ATUALIZAÇÃO
// --------------------------------------------------

void uiUpdate() {
  unsigned long now = millis();

  uiHandleTouch();

  if (volumePanelOpen) {
    if (
      !touchPressed && millis() - volumeLastInteraction >= VOLUME_CLOSE_TIME) {
      closeVolumePanel();
    }

    return;
  }

  if (stationListOpen) {
    return;
  }


  if (now - lastRSSIUpdate >= 1500) {
    lastRSSIUpdate = now;
    drawRSSI();
  }

  if (now - lastClockUpdate >= 1000) {
    lastClockUpdate = now;
    drawClock();
  }

  if (audioPlayerHasUpdate()) {
    drawCodecBitrate();
    drawMetadata();
  }

  if (now - lastScrollUpdate >= SCROLL_INTERVAL) {
    lastScrollUpdate = now;
    updateMetadataScroll();
  }
}

// --------------------------------------------------
// TELA BASE
// --------------------------------------------------

static void drawBase() {
  lcd.fillScreen(COLOR_BACKGROUND);

  lcd.drawFastHLine(10, 58, 460, COLOR_SECONDARY);

  drawHeader();
  drawMetadata();
}

static void drawHeader() {
  drawRSSI();
  drawClock();
  drawCodecBitrate();
}

// --------------------------------------------------
// STATUS
// --------------------------------------------------

void uiShowStatus(const char* message) {
  // Limpa toda a área central
  lcd.fillRect(0, 70, 480, 180, COLOR_BACKGROUND);

  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  lcd.setTextSize(2);

  lcd.drawString(message, 240, 170);
}

void uiClearStatus() {
  lcd.fillRect(0, 70, 480, 180, COLOR_BACKGROUND);
}

// --------------------------------------------------
// RSSI
// --------------------------------------------------

static void drawRSSI() {
  constexpr int16_t startX = 12;
  constexpr int16_t baseY = 43;
  constexpr int16_t barWidth = 7;
  constexpr int16_t spacing = 4;

  //lcd.fillRect(5, 5, 115, 48, COLOR_BACKGROUND);

  int32_t rssi = networkGetRSSI();
  uint8_t activeBars = 0;

  if (networkIsConnected()) {
    if (rssi >= -55) {
      activeBars = 4;

    } else if (rssi >= -67) {
      activeBars = 3;

    } else if (rssi >= -75) {
      activeBars = 2;

    } else if (rssi >= -85) {
      activeBars = 1;
    }
  }

  for (uint8_t i = 0; i < 4; i++) {
    int16_t height = 8 + (i * 7);
    int16_t x = startX + (i * (barWidth + spacing));
    int16_t y = baseY - height;

    uint16_t color = i < activeBars ? COLOR_RSSI_ON : COLOR_RSSI_OFF;

    lcd.fillRoundRect(x, y, barWidth, height, 2, color);
  }

  lcd.setTextDatum(textdatum_t::middle_left);
  lcd.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  lcd.setTextSize(1);

  if (networkIsConnected()) {
    char rssiText[20];

    snprintf(rssiText, sizeof(rssiText), "%ld dBm", static_cast<long>(rssi));

    lcd.drawString(rssiText, 60, 29);

  } else {
    lcd.drawString("Offline", 60, 29);
  }
}

// --------------------------------------------------
// RELÓGIO
// --------------------------------------------------

static void drawClock() {
  char currentClock[8];

  networkGetTime(currentClock, sizeof(currentClock));

  if (strcmp(currentClock, lastClock) == 0) {
    return;
  }

  strlcpy(lastClock, currentClock, sizeof(lastClock));

  //lcd.fillRect(185, 5, 110, 45, COLOR_BACKGROUND);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  lcd.setTextSize(3);
  lcd.drawString(currentClock, 240, 28);

  char currentDate[16];

  networkGetDate(
    currentDate,
    sizeof(currentDate));

  lcd.setTextSize(1);
  lcd.setTextDatum(textdatum_t::top_center);

  lcd.drawString(
    currentDate,
    240,
    45);
}

// --------------------------------------------------
// CODEC E BITRATE
// --------------------------------------------------

static void drawCodecBitrate() {
  AudioMetadata metadata;
  audioPlayerReadMetadata(metadata);

  bool codecChanged = strcmp(metadata.codec, lastCodec) != 0;
  bool bitrateChanged = strcmp(metadata.bitrate, lastBitrate) != 0;

  if (!codecChanged && !bitrateChanged) {
    return;
  }

  strlcpy(lastCodec, metadata.codec, sizeof(lastCodec));
  strlcpy(lastBitrate, metadata.bitrate, sizeof(lastBitrate));

  lcd.fillRect(330, 5, 145, 45, COLOR_BACKGROUND);

  lcd.setTextDatum(textdatum_t::middle_right);
  lcd.setTextColor(COLOR_TEXT, COLOR_BACKGROUND);
  lcd.setTextSize(2);
  lcd.drawString(metadata.codec, 470, 18);

  lcd.setTextColor(COLOR_SECONDARY, COLOR_BACKGROUND);
  lcd.setTextSize(1);
  lcd.drawString(metadata.bitrate, 470, 39);
}

// --------------------------------------------------
// ARTISTA E MÚSICA
// --------------------------------------------------

static void drawMetadata() {

  AudioMetadata metadata;
  audioPlayerReadMetadata(metadata);

  bool artistChanged = strcmp(metadata.artist, lastArtist) != 0;
  bool titleChanged = strcmp(metadata.title, lastTitle) != 0;

  if (!artistChanged && !titleChanged) {
    return;
  }

  strlcpy(lastArtist, metadata.artist, sizeof(lastArtist));
  strlcpy(lastTitle, metadata.title, sizeof(lastTitle));

  scrollArtist = String(metadata.artist);
  scrollTitle = String(metadata.title);

  // Calcula a largura do artista
  lcd.setTextSize(3);
  artistScrollState.textWidth = lcd.textWidth(scrollArtist);

  // Calcula a largura da música
  lcd.setTextSize(2);
  titleScrollState.textWidth = lcd.textWidth(scrollTitle);

  // Reinicia a rolagem quando chega uma música nova
  artistScrollState.x = 0;
  artistScrollState.mode = ScrollMode::CENTER_PAUSE;
  artistScrollState.pauseStarted = millis();

  titleScrollState.x = 0;
  titleScrollState.mode = ScrollMode::CENTER_PAUSE;
  titleScrollState.pauseStarted = millis();

  // Limpa a área completa da metadata
  lcd.fillRect(TEXT_AREA_X, 75, TEXT_AREA_WIDTH, 210, COLOR_BACKGROUND);

  // Linha entre artista e música
  lcd.drawFastHLine(90, 177, 300, COLOR_SECONDARY);

  updateMetadataScroll();
}

static void drawCircularText(
  LGFX_Sprite& sprite,
  const String& text,
  ScrollState& state,
  int16_t screenX,
  int16_t screenY,
  int16_t areaHeight,
  uint8_t textSize,
  uint16_t textColor) {
  sprite.fillSprite(COLOR_BACKGROUND);

  sprite.setTextSize(textSize);
  sprite.setTextColor(
    textColor,
    COLOR_BACKGROUND);

  if (text.length() == 0) {
    sprite.pushSprite(screenX, screenY);
    return;
  }

  // Texto curto permanece centralizado
  if (state.textWidth <= TEXT_AREA_WIDTH) {
    sprite.setTextDatum(textdatum_t::middle_center);

    sprite.drawString(
      text,
      TEXT_AREA_WIDTH / 2,
      areaHeight / 2);

    sprite.pushSprite(screenX, screenY);
    return;
  }

  const int16_t centerY = areaHeight / 2;

  switch (state.mode) {

    // Mostra o texto centralizado antes de começar
    case ScrollMode::CENTER_PAUSE:
      sprite.setTextDatum(textdatum_t::middle_center);

      sprite.drawString(
        text,
        TEXT_AREA_WIDTH / 2,
        centerY);

      if (
        millis() - state.pauseStarted
        >= SCROLL_PAUSE_TIME) {
        // Começa na posição centralizada
        state.x =
          (TEXT_AREA_WIDTH - state.textWidth) / 2;

        state.mode = ScrollMode::MOVING;
      }
      break;

    // Texto se movimentando para a esquerda
    case ScrollMode::MOVING:
      sprite.setTextDatum(textdatum_t::middle_left);

      sprite.drawString(
        text,
        state.x,
        centerY);

      state.x--;

      // Saiu completamente pela esquerda
      if (state.x + state.textWidth < 0) {
        state.x = TEXT_AREA_WIDTH;
        state.mode = ScrollMode::RIGHT_PAUSE;
        state.pauseStarted = millis();
      }
      break;

    // Texto parado fora da tela, à direita
    case ScrollMode::RIGHT_PAUSE:
      sprite.setTextDatum(textdatum_t::middle_left);

      sprite.drawString(
        text,
        state.x,
        centerY);

      if (
        millis() - state.pauseStarted
        >= SCROLL_PAUSE_TIME) {
        state.mode = ScrollMode::MOVING;
      }
      break;
  }

  sprite.pushSprite(screenX, screenY);
}

static void updateMetadataScroll() {

  drawCircularText(
    artistSprite,
    scrollArtist,
    artistScrollState,
    TEXT_AREA_X,
    ARTIST_AREA_Y,
    ARTIST_AREA_HEIGHT,
    3,
    COLOR_TEXT);

  drawCircularText(
    titleSprite,
    scrollTitle,
    titleScrollState,
    TEXT_AREA_X,
    TITLE_AREA_Y,
    TITLE_AREA_HEIGHT,
    2,
    COLOR_SECONDARY);
}

// --------------------------------------------------
// LIMITADOR DE TEXTO
// --------------------------------------------------

static String limitText(const String& text, uint16_t maximumWidth) {
  if (text.length() == 0) {
    return "";
  }

  if (lcd.textWidth(text) <= maximumWidth) {
    return text;
  }

  String result = text;

  while (result.length() > 3) {
    result.remove(result.length() - 1);

    String testText = result + "...";

    if (lcd.textWidth(testText) <= maximumWidth) {
      return testText;
    }
  }

  return result;
}

static void drawStationList() {
  constexpr int16_t HEADER_HEIGHT = 42;

  const int16_t contentScreenY =
    LIST_Y + HEADER_HEIGHT;

  const int16_t contentHeight =
    SCREEN_HEIGHT - contentScreenY;

  // Fundo geral e título desenhados diretamente
  lcd.fillRect(
    0,
    LIST_Y,
    SCREEN_WIDTH,
    HEADER_HEIGHT,
    COLOR_LIST_BACKGROUND);

  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextSize(2);
  lcd.setTextColor(
    TFT_WHITE,
    COLOR_LIST_BACKGROUND);

  lcd.drawString(
    "ESTACOES",
    SCREEN_WIDTH / 2,
    LIST_Y + 20);

  if (!stationListSpriteReady) {
    Serial.println("Sprite da lista indisponivel");
    return;
  }

  // Limpa o quadro inteiro antes de desenhar
  stationListSprite.fillSprite(
    COLOR_LIST_BACKGROUND);

  size_t selectedIndex =
    audioPlayerGetStationIndex();

  for (size_t i = 0; i < STATION_COUNT; i++) {
    int16_t itemY =
      stationListOffset + static_cast<int16_t>(i * STATION_ITEM_HEIGHT);

    if (
      itemY + STATION_ITEM_HEIGHT <= 0 || itemY >= contentHeight) {
      continue;
    }

    uint16_t itemColor =
      i == selectedIndex
        ? TFT_NAVY
        : TFT_DARKCYAN;

    stationListSprite.fillRoundRect(
      10,
      itemY + 3,
      SCREEN_WIDTH - 20,
      STATION_ITEM_HEIGHT - 6,
      6,
      itemColor);

    char stationText[96];

    snprintf(
      stationText,
      sizeof(stationText),
      "%u - %s",
      stations[i].id,
      stations[i].name);

    stationListSprite.setTextDatum(
      textdatum_t::middle_left);

    stationListSprite.setTextSize(2);

    stationListSprite.setTextColor(
      TFT_WHITE,
      itemColor);

    stationListSprite.drawString(
      stationText,
      25,
      itemY + (STATION_ITEM_HEIGHT / 2));
  }

  stationListSprite.pushSprite(
    0,
    contentScreenY);
}

void uiHandleTouch() {
  uint16_t x = 0;
  uint16_t y = 0;

  bool pressed = lcd.getTouch(&x, &y);

  // --------------------------------------------------
  // INÍCIO DO TOQUE
  // --------------------------------------------------

  if (pressed && !touchPressed) {
    touchPressed = true;
    touchMoved = false;

    touchStartX = static_cast<int16_t>(x);
    touchStartY = static_cast<int16_t>(y);

    touchLastX = static_cast<int16_t>(x);
    touchLastY = static_cast<int16_t>(y);

    return;
  }

  // --------------------------------------------------
  // DEDO CONTINUA PRESSIONADO
  // --------------------------------------------------

  if (pressed && touchPressed) {
    int16_t currentX = static_cast<int16_t>(x);
    int16_t currentY = static_cast<int16_t>(y);

    int16_t deltaX =
      currentX - touchLastX;

    int16_t deltaY =
      currentY - touchLastY;

    int16_t totalMovementX =
      currentX - touchStartX;

    int16_t totalMovementY =
      currentY - touchStartY;

    if (
      abs(totalMovementX) > TOUCH_MOVE_THRESHOLD || abs(totalMovementY) > TOUCH_MOVE_THRESHOLD) {
      touchMoved = true;
    }

    // ------------------------------------------------
    // PAINEL DE VOLUME ABERTO
    // ------------------------------------------------
    if (volumePanelOpen) {
      updateVolumeFromMovement(deltaX);

      touchLastX = currentX;
      touchLastY = currentY;
      return;
    }
    // ------------------------------------------------
    // LISTA DE ESTAÇÕES ABERTA
    // ------------------------------------------------

    if (stationListOpen) {
      if (abs(deltaY) >= 3) {
        stationListOffset += deltaY;
        limitStationListOffset();

        static unsigned long lastListDraw = 0;
        constexpr uint32_t LIST_DRAW_INTERVAL = 20;

        unsigned long now = millis();

        if (
          now - lastListDraw >= LIST_DRAW_INTERVAL) {
          lastListDraw = now;
          drawStationList();
        }
      }

      touchLastX = currentX;
      touchLastY = currentY;

      return;
    }

    // ------------------------------------------------
    // ABRIR PAINEL DE VOLUME
    // Gesto iniciado no lado esquerdo e movido
    // horizontalmente para a direita.
    // ------------------------------------------------

    // Abre o volume com movimento horizontal
    // para a direita ou para a esquerda
    if (
      abs(totalMovementX) > VOLUME_OPEN_DISTANCE && abs(totalMovementX) > abs(totalMovementY)) {
      openVolumePanel();

      touchLastX = currentX;
      touchLastY = currentY;

      return;
    }

    // ------------------------------------------------
    // ABRIR LISTA DE ESTAÇÕES
    // Gesto iniciado no topo e movido para baixo.
    // ------------------------------------------------

    if (
      touchStartY < 70 && totalMovementY > OPEN_SWIPE_DISTANCE && abs(totalMovementY) > abs(totalMovementX)) {
      stationListOpen = true;
      stationListOffset = 0;

      Serial.printf(
        "Heap ao abrir lista: %u bytes\n",
        ESP.getFreeHeap());

      lcd.fillRect(
        0,
        LIST_Y,
        SCREEN_WIDTH,
        SCREEN_HEIGHT - LIST_Y,
        COLOR_LIST_BACKGROUND);

      drawStationList();

      touchLastX = currentX;
      touchLastY = currentY;

      return;
    }

    touchLastX = currentX;
    touchLastY = currentY;

    return;
  }

  // --------------------------------------------------
  // DEDO FOI SOLTO
  // --------------------------------------------------

  if (!pressed && touchPressed) {
    touchPressed = false;

    // Volume permanece visível até o timeout
    if (volumePanelOpen) {
      volumeLastInteraction = millis();
      touchMoved = false;
      return;
    }

    // Finaliza o movimento da lista
    if (stationListOpen) {
      limitStationListOffset();
      drawStationList();

      // Seleciona apenas quando foi realmente um toque
      if (!touchMoved) {
        selectStationFromTouch(touchStartY);
      }

      touchMoved = false;
      return;
    }

    touchMoved = false;
  }
}

static void limitStationListOffset() {
  constexpr int16_t HEADER_HEIGHT = 42;

  const int32_t contentHeight =
    static_cast<int32_t>(STATION_COUNT) * STATION_ITEM_HEIGHT;

  const int16_t visibleHeight =
    SCREEN_HEIGHT - LIST_Y - HEADER_HEIGHT;

  if (contentHeight <= visibleHeight) {
    stationListOffset = 0;
    return;
  }

  if (stationListOffset > 0) {
    stationListOffset = 0;
  }

  const int32_t minimumOffset =
    visibleHeight - contentHeight;

  if (stationListOffset < minimumOffset) {
    stationListOffset = minimumOffset;
  }
}

static void selectStationFromTouch(int16_t touchY) {
  constexpr int16_t HEADER_HEIGHT = 42;

  const int16_t listContentY =
    LIST_Y + HEADER_HEIGHT;

  if (touchY < listContentY) {
    closeStationList();
    return;
  }

  int32_t relativeY =
    touchY - listContentY - stationListOffset;

  if (relativeY < 0) {
    return;
  }

  size_t index =
    static_cast<size_t>(
      relativeY / STATION_ITEM_HEIGHT);

  if (index >= STATION_COUNT) {
    return;
  }

  audioPlayerSelectStation(index);
  closeStationList();
}

static void drawVolumePanel() {
  uint8_t volume = audioPlayerGetVolume();

  if (volume > VOLUME_MAX) {
    volume = VOLUME_MAX;
  }

  // Converte 0...21 para 0...100%
  uint8_t percentage = map(
    volume,
    VOLUME_MIN,
    VOLUME_MAX,
    0,
    100);

  // Fundo da mesma área usada pela lista
  lcd.fillRect(
    0,
    LIST_Y,
    SCREEN_WIDTH,
    SCREEN_HEIGHT - LIST_Y,
    COLOR_LIST_BACKGROUND);

  // Título do painel
  lcd.setFont(&fonts::Font0);
  lcd.setTextDatum(textdatum_t::middle_center);
  lcd.setTextSize(2);
  lcd.setTextColor(
    TFT_WHITE,
    COLOR_LIST_BACKGROUND);

  lcd.drawString(
    volume == 0 ? "VOLUME - MUDO" : "VOLUME",
    SCREEN_WIDTH / 2,
    LIST_Y + 26);

  if (!volumeSpriteReady) {
    Serial.println("Sprite de volume indisponivel");
    return;
  }

  // Monta tudo no sprite antes de enviar ao LCD
  volumeSprite.fillSprite(TFT_BLUE);

  // Moldura interna discreta
  volumeSprite.drawRoundRect(
    0,
    0,
    VOLUME_SPRITE_WIDTH,
    VOLUME_SPRITE_HEIGHT,
    10,
    TFT_CYAN);

  volumeSprite.setFont(&fonts::Font0);

  // Número do volume
  char volumeText[8];

  snprintf(
    volumeText,
    sizeof(volumeText),
    "%02u",
    volume);

  volumeSprite.setTextDatum(textdatum_t::middle_center);
  volumeSprite.setTextSize(3);
  volumeSprite.setTextColor(TFT_WHITE);

  volumeSprite.drawString(
    volumeText,
    VOLUME_SPRITE_WIDTH / 2,
    24);

  // Porcentagem à direita do número
  char percentageText[12];

  snprintf(
    percentageText,
    sizeof(percentageText),
    "%u%%",
    percentage);

  volumeSprite.setTextSize(1);
  volumeSprite.setTextColor(TFT_CYAN);

  volumeSprite.drawString(
    percentageText,
    VOLUME_SPRITE_WIDTH / 2,
    47);

  // ------------------------------------------------
  // BARRA SEGMENTADA
  // ------------------------------------------------

  constexpr int16_t segmentCount = 21;
  constexpr int16_t segmentGap = 3;

  constexpr int16_t barX = 28;
  constexpr int16_t barY = 62;
  constexpr int16_t barWidth =
    VOLUME_SPRITE_WIDTH - 56;

  constexpr int16_t barHeight = 14;

  const int16_t segmentWidth =
    (barWidth - ((segmentCount - 1) * segmentGap)) / segmentCount;

  // Símbolo de menos
  volumeSprite.setTextDatum(textdatum_t::middle_center);
  volumeSprite.setTextSize(2);
  volumeSprite.setTextColor(TFT_WHITE);

  volumeSprite.drawString(
    "-",
    14,
    barY + (barHeight / 2));

  // Símbolo de mais
  volumeSprite.drawString(
    "+",
    VOLUME_SPRITE_WIDTH - 14,
    barY + (barHeight / 2));

  for (int16_t i = 0; i < segmentCount; i++) {
    int16_t segmentX =
      barX + i * (segmentWidth + segmentGap);

    uint16_t segmentColor;

    if (i < volume) {
      // Últimos níveis ganham destaque
      if (i >= 17) {
        segmentColor = TFT_YELLOW;
      } else {
        segmentColor = TFT_CYAN;
      }
    } else {
      segmentColor = TFT_NAVY;
    }

    volumeSprite.fillRoundRect(
      segmentX,
      barY,
      segmentWidth,
      barHeight,
      2,
      segmentColor);
  }

  // Envia o painel pronto
  volumeSprite.pushSprite(
    VOLUME_SPRITE_X,
    VOLUME_SPRITE_Y);
}

static void openVolumePanel() {
  if (stationListOpen) {
    return;
  }

  volumePanelOpen = true;
  volumeMovementAccumulator = 0;
  volumeLastInteraction = millis();

  lcd.fillRect(
    0,
    LIST_Y,
    SCREEN_WIDTH,
    SCREEN_HEIGHT - LIST_Y,
    COLOR_LIST_BACKGROUND);

  drawVolumePanel();
}



static void updateVolumeFromMovement(int16_t deltaX) {
  volumeMovementAccumulator += deltaX;

  constexpr int16_t PIXELS_PER_STEP = 8;

  int16_t volume = audioPlayerGetVolume();
  bool changed = false;

  while (
    volumeMovementAccumulator >= PIXELS_PER_STEP) {
    volumeMovementAccumulator -= PIXELS_PER_STEP;

    if (volume < VOLUME_MAX) {
      volume++;
      changed = true;
    }
  }

  while (
    volumeMovementAccumulator <= -PIXELS_PER_STEP) {
    volumeMovementAccumulator += PIXELS_PER_STEP;

    if (volume > VOLUME_MIN) {
      volume--;
      changed = true;
    }
  }

  if (changed) {
    audioPlayerSetVolume(volume);
    drawVolumePanel();
  }

  volumeLastInteraction = millis();
}

static void closeStationList() {
  stationListOpen = false;
  stationListOffset = 0;

  // Força o redesenho de todos os dados
  lastClock[0] = '\0';
  lastDate[0] = '\0';
  lastArtist[0] = '\0';
  lastTitle[0] = '\0';
  lastCodec[0] = '\0';
  lastBitrate[0] = '\0';

  lastRSSIUpdate = 0;
  lastClockUpdate = 0;
  lastScrollUpdate = millis();

  drawBase();

  drawRSSI();
  drawClock();
  drawCodecBitrate();
  drawMetadata();
  updateMetadataScroll();
}


static void closeVolumePanel() {
  volumePanelOpen = false;
  volumeLastInteraction = 0;

  // Força o redesenho de todos os dados
  lastClock[0] = '\0';
  lastDate[0] = '\0';
  lastArtist[0] = '\0';
  lastTitle[0] = '\0';
  lastCodec[0] = '\0';
  lastBitrate[0] = '\0';

  lastRSSIUpdate = 0;
  lastClockUpdate = 0;
  lastScrollUpdate = millis();

  drawBase();

  drawRSSI();
  drawClock();
  drawCodecBitrate();
  drawMetadata();
  updateMetadataScroll();
}