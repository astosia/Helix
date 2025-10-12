#pragma once
#include <pebble.h>

#define SETTINGS_KEY 1



typedef struct ClaySettings {
  //functions
  bool ShowHourNumbers;
  bool JumpHourOn;
  bool InvertScreen;
  bool RemoveZero24h;
  bool AddZero12h;
  bool BTVibeOn;

  GColor DigitsColor;
  GColor DialColorMinutes;
  GColor DialColorHours;
  GColor ColorMinorTick;
  GColor ColorMajorTick;
  

} __attribute__((__packed__)) ClaySettings;
