#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1



typedef struct ClaySettings {
  //functions
  bool ShowHourNumbers;

} __attribute__((__packed__)) ClaySettings;
