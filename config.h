#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "ST7796U.h"

// --------------------------------------------------
// WI-FI
// --------------------------------------------------

constexpr const char* WIFI_SSID = "ALHN-31DA";
constexpr const char* WIFI_PASSWORD = "4242450201";

// --------------------------------------------------
// ESTAÇÃO
// --------------------------------------------------

constexpr const char* RADIO_URL =
  "http://27613.live.streamtheworld.com/RADIOCIDADEAAC.aac";

constexpr const char* RADIO_NAME = "Rádio Cidade";

// --------------------------------------------------
// ÁUDIO I2S
// --------------------------------------------------

constexpr uint8_t I2S_DOUT = 37;
constexpr uint8_t I2S_BCLK = 36;
constexpr uint8_t I2S_LRC  = 35;

constexpr uint8_t AUDIO_VOLUME = 12;

// --------------------------------------------------
// DISPLAY
// --------------------------------------------------

constexpr uint8_t DISPLAY_ROTATION = 3;
constexpr uint8_t DISPLAY_BRIGHTNESS = 180;

// --------------------------------------------------
// CORES
// --------------------------------------------------

#define COLOR_BACKGROUND TFT_BLUE
#define COLOR_TEXT       TFT_WHITE
#define COLOR_SECONDARY  TFT_CYAN
#define COLOR_RSSI_ON    TFT_GREEN
#define COLOR_RSSI_OFF    0x420800

// --------------------------------------------------
// HORÁRIO
// Brasília
// --------------------------------------------------

constexpr const char* TIMEZONE_INFO = "BRT3";

#endif