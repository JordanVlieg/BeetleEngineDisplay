#pragma once

#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_TouchscreenSOFTSPI.h>

#include "engine.h"
#include "NotoSans_Bold.h"
#include "OpenFontRender.h"
#include "meters.h"

#define TTF_FONT NotoSans_Bold
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define SD_MISO  19
#define SD_MOSI  23
#define SD_SCK  18
#define SD_CS  5
#define TOUCH_CS 33 // Chip select for touch screen
#define TFT_CS 15 // Chip select for TFT

// Constants
const unsigned long screenFlashPeriod = 750;
const unsigned long uiPeriod =          250;
const int backlightPin =                21;
const bool colourIssues =               true;
const int baudRate =                    115200;
enum displayMode {
  ALL = 0,
  TEMP = 1,
  TACH = 2,
  OFF = 3,
} mode;

// Settings Variables
unsigned long sdWritePeriod = 5000;
unsigned long wifiUpdateInterval = 1000;
String ssid = "combee";
String password = "blackandyellow";
  // WIFI
uint16_t port = 80;
String host = "192.168.4.1";

// Program Variables
SoftSPI *tsSoftSpi;
XPT2046_TouchscreenSOFTSPI touchscreen(TOUCH_CS, XPT2046_IRQ);

SPIClass sdSpi = SPIClass(VSPI);
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
OpenFontRender ofr;

unsigned long lastScreenFlash =   0;
unsigned long lastUiUpdate =      0;
  // Colours
uint32_t lastOilTempColour =      TFT_BLACK;
uint32_t lastOilPressureColour =  TFT_BLACK;
uint32_t lastCyl2Colour =         TFT_BLACK;
uint32_t lastCyl3Colour =         TFT_BLACK;
uint32_t lastTachColour =         TFT_BLACK;
uint32_t globalBgColour =          TFT_BLACK;

// Methods

// Renderers
void renderOilTemp() {
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;
  uint32_t colour = TFT_GREEN;

  if (mode == TACH || mode == OFF) {
    return;
  } else if (mode == TEMP) {
    radius = 80;
    xpos = 80;
    ypos = 240;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 40;
    ypos = 280;
  }

  if (oilTemp < oilLowTemp) {
    colour = TFT_BLUE;
  }
  else if (oilTemp > oilEmergTemp) {
    colour = TFT_RED;
  } 
  else if (oilTemp > oilWarnTemp) {
    colour = TFT_ORANGE;
  }

  arcMeter(xpos, ypos, radius, oilTemp, oilStartTemp, oilEmergTemp, (colour != lastOilTempColour), colour, TFT_BLACK, "Oil-C", true);

  if (colour != lastOilTempColour) {
    lastOilTempColour = colour;
  }
}

void renderOilPressure() {
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;
  uint32_t colour = TFT_GREEN;

  if (mode == TACH || mode == OFF) {
    return;
  } else if (mode == TEMP) {
    return;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 210;
    ypos = 280;
  }

  if (oilPressure > oilEmergPressure) {
    colour = TFT_RED;
  } 
  if (oilPressure > oilWarnPressure && colour != TFT_RED) {
    colour = TFT_ORANGE;
  }

  arcMeter(xpos, ypos, radius, oilPressure, oilStartPressure, oilEmergPressure, (colour != lastOilPressureColour), colour, TFT_BLACK, "Oil-PSI", false);

  if (colour != lastOilPressureColour) {
    lastOilPressureColour = colour;
  }
}

void renderTach() {
  calculateAvgRpm();
  static int16_t xpos = 0;
  static int16_t ypos = 0;
  static uint8_t radius = 0;
  uint32_t colour = TFT_GREEN;

  int displayRpm = (int)rpm / 10;

  if (mode == TEMP || mode == OFF) {
    return;
  } else if (mode == TACH) {
    radius = 120;
  } else if (mode == ALL) {
    radius = 80;
    xpos = 120;
    ypos = 160;
  }

  if (displayRpm > tachEmerg/10) {
    colour = TFT_RED;
  } 
  else if (displayRpm > tachWarn/10) {
    colour = TFT_ORANGE;
  }

  ringMeter(xpos, ypos, radius, displayRpm, tachStart/10, tachEmerg/10, (colour != lastTachColour), colour, TFT_BLACK , "Tach");

  if (colour != lastTachColour) {
    lastTachColour = colour;
  }
}

void renderCylinder2() {
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;

  if (mode == TACH || mode == OFF) {
    return;
  } else if (mode == TEMP) {
    radius = 60;
    xpos = 200;
    ypos = 60;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 210;
    ypos = 40;
  }

  uint32_t colour = TFT_GREEN;
  if (cyl2Temp < cylLowTemp) {
    colour = TFT_BLUE;
  }
  else if (cyl2Temp > cylEmergTemp) {
    colour = TFT_RED;
  } 
  else if (cyl2Temp > cylWarnTemp) {
    colour = TFT_ORANGE;
  } 

  arcMeter(xpos, ypos, radius, cyl2Temp, cylStartTemp, cylEmergTemp, (colour != lastCyl2Colour), colour, TFT_BLACK, "C2", false);
  if (colour != lastCyl2Colour) {
    lastCyl2Colour = colour;
  }
}

void renderCylinder3() {
  static int16_t xpos = 40;
  static int16_t ypos = 40;
  static uint8_t radius = 40;

  if (mode == TACH || mode == OFF) {
    return;
  } else if (mode == TEMP) {
    radius = 60;
    xpos = 60;
    ypos = 60;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 40;
    ypos = 40;
  }

  uint32_t colour = TFT_GREEN;
  if (cyl3Temp < cylLowTemp) {
    colour = TFT_BLUE;
  }
  else if (cyl3Temp > cylEmergTemp) {
    colour = TFT_RED;
  } 
  else if (cyl3Temp > cylWarnTemp) {
    colour = TFT_ORANGE;
  } 

  arcMeter(xpos, ypos, radius, cyl3Temp, cylStartTemp, cylEmergTemp, (colour != lastCyl3Colour), colour, TFT_BLACK, "C3", true);
  if (colour != lastCyl3Colour) {
    lastCyl3Colour = colour;
  }
}

void maybeFlashWarningScreen(unsigned long nowMs) {
  if (oilTemp >= oilEmergTemp 
    || cyl2Temp >= cylEmergTemp 
    || cyl3Temp >= cylEmergTemp 
    || rpm >= tachEmerg 
    || (oilPressure > oilEmergPressure && lastOilTempColour == TFT_GREEN)) // Only warn on high oil pressure if we are up to temperature
  {
    if (nowMs - lastScreenFlash > screenFlashPeriod) {
      lastScreenFlash = nowMs;
      if (globalBgColour == TFT_BLACK) {
        globalBgColour = TFT_RED;
      } else {
        globalBgColour = TFT_BLACK;
      }
      tft.fillScreen(globalBgColour);
      hasCleanRising = false;
    }
  }
  if (oilTemp < oilEmergTemp && cyl2Temp < cylEmergTemp && cyl3Temp < cylEmergTemp && rpm < tachEmerg && (oilPressure < oilEmergPressure && lastOilTempColour == TFT_GREEN) && globalBgColour == TFT_RED) {
    globalBgColour = TFT_BLACK;
    tft.fillScreen(globalBgColour);
    hasCleanRising = false;
  }
}

void checkForTouch() {
  if (touchscreen.tirqTouched() && touchscreen.touched(tsSoftSpi)) {
    TS_Point p = touchscreen.getPoint(tsSoftSpi);
    int i = 1;
    if (p.y < 2048) {
      i = -1;
    }

    mode = (displayMode)((mode + i + 4) % 4);
    if (mode == OFF) {
      digitalWrite(backlightPin, 0);
    } else {
      digitalWrite(backlightPin, 1);
    }
    tft.fillScreen(globalBgColour);
    vTaskDelay(pdMS_TO_TICKS(250));
    hasCleanRising = false;
  }
}


