#include <Arduino.h>

#include "network.h"
#include "audio_player.h"
#include "ui.h"

void setup() {
  Serial.begin(115200);

  // Display
  uiBegin();
  uiShowStatus("Conectando ao Wi-Fi...");

  // Wi-Fi
  networkBegin();

  while (!networkIsConnected()) {
    uiUpdate();
    delay(200);
  }

  uiClearStatus();

  Serial.println();
  Serial.println("Wi-Fi conectado");

  // Relógio NTP
  networkConfigureTime();

  uiShowStatus("Conectado");

  // Áudio
  audioPlayerBegin();
  audioPlayerStart();

  Serial.println();
Serial.printf("Heap livre : %u bytes\n", ESP.getFreeHeap());
Serial.printf("PSRAM livre: %u bytes\n", ESP.getFreePsram());
}

void loop() {
  uiUpdate();

  delay(10);
}