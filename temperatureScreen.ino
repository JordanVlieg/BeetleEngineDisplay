#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include "NotoSans_Bold.h"
#include "OpenFontRender.h"
#include "meters.h"

#define TTF_FONT NotoSans_Bold
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

// Constants
const uint16_t port = 80;
const char *host = "192.168.4.1";
const int magicNum = 30000000;
const int arrayLength = 100;
const int tachStart = 0;const int tachLow = 700;const int tachWarn = 4200;const int tachEmerg = 5000;
const int oilStartTemp = 50; const int oilLowTemp = 70;const int oilWarnTemp = 106;const int oilEmergTemp = 115;
const int cylStartTemp = 110;const int cylLowTemp = 130;const int cylWarnTemp = 185;const int cylEmergTemp = 205;
const int oilStartPressure = 0;const int oilWarnPressure = 50;const int oilEmergPressure = 55;
const unsigned long screenFlashPeriod = 750;
const unsigned long uiPeriod = 250;
const unsigned long updateInterval = 1000;
const int coilPin = 22;
const bool colourIssues = true;

// Variables
int oilTemp = 0; int cyl2Temp = 0; int cyl3Temp = 0; int oilPressure = 0;
unsigned long lastScreenFlash = 0;
unsigned long lastUiUpdate = 0;
float rpm = 0;
unsigned long lastRisingEdgeMicros = 0;
float rpmArr[arrayLength];
int rpmPtr = 0;
int leftPtr = 0;
int coilState = LOW;
int lastCoilState = LOW;
bool hasCleanRising = false;
uint32_t lastOilTempColour = TFT_BLACK;uint32_t lastOilPressureColour = TFT_BLACK;uint32_t lastCyl2Colour = TFT_BLACK;uint32_t lastCyl3Colour = TFT_BLACK;uint32_t lastTachColour = TFT_BLACK;uint32_t globalBgColour = TFT_BLACK;
WiFiMulti WiFiMulti;
TaskHandle_t wifiTask;
SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();            // Invoke custom library with default width and height
TFT_eSprite spr = TFT_eSprite(&tft);  // Declare Sprite object "spr" with pointer to "tft" object
OpenFontRender ofr;

enum displayMode {
  ALL = 0,
  TEMP = 1,
  TACH = 2,
} mode;

void setup(void) {
  Serial.begin(115200);
  delay(10);

  mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(mySpi);
  ts.setRotation(3);
  tft.begin();
  tft.setRotation(3);

  if (colourIssues) {
    tft.invertDisplay(true);
  }

  tft.fillScreen(TFT_BLACK);

  if (ofr.loadFont(TTF_FONT, sizeof(TTF_FONT))) {
    Serial.println("Render initialize error");
    return;
  }
 
  WiFiMulti.addAP("combee", "blackandyellow");

  ofr.setDrawer(tft);
  ofr.setFontColor(TFT_WHITE);
  ofr.setFontSize(50);
  ofr.setCursor(10, 10);
  ofr.printf("Connecting to WiFi");

  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.print("Waiting to connect to wifi");
    delay(500);
  }

  pinMode(coilPin, INPUT);

  // create a task that will be executed in the Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                    getDataFromWifi,
                    "WifiTask",
                    50000,       /* Stack size of task */
                    NULL,
                    10,
                    &wifiTask,
                    0); 

  tft.fillScreen(globalBgColour);
}

void loop() {
  rpmTracking();
  checkForTouch();
  
  unsigned long nowMs = millis();
  maybeFlashWarningScreen(nowMs);
  if (nowMs - lastUiUpdate > uiPeriod) {
    lastUiUpdate = nowMs;
    renderCylinder2();
    renderCylinder3();
    renderOilTemp();
    renderOilPressure();
    renderTach();
    hasCleanRising = false;
  }
  // This delay allows the input pin to settle, and prevents ridiculous RPM readings.
  delay(2);
}

void rpmTracking() {
  coilState = digitalRead(coilPin);
  if (lastCoilState != coilState) {
    lastCoilState = coilState;
    if (coilState == HIGH) {
      unsigned long pulseMicros = micros();
      float tentativeRpm = (float)magicNum / (float)(pulseMicros - lastRisingEdgeMicros);
      lastRisingEdgeMicros = pulseMicros;
      if (hasCleanRising) {
        rpmArr[rpmPtr] = tentativeRpm;
        rpmPtr = (rpmPtr + 1) % arrayLength;
      }
      hasCleanRising = true;
    }
  }
}

void calculateAvgRpm() {
  int sparksSinceLast = ((rpmPtr - leftPtr + arrayLength) % arrayLength);
  if (sparksSinceLast > 0) {
    double sum = 0;
    for (int i = 1; i < sparksSinceLast; i++) {
      sum += rpmArr[((leftPtr + i) % arrayLength)];
    }
    leftPtr = rpmPtr;
    rpm = sum / sparksSinceLast;
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
  if (ts.tirqTouched() && ts.touched()) {
    mode = (displayMode)((mode + 1) % 3);
    tft.fillScreen(globalBgColour);
    vTaskDelay(pdMS_TO_TICKS(250));
    hasCleanRising = false;
  }
}

void getDataFromWifi(void * pvParameters) {
  NetworkClient client;
  for(;;) {
    if (!client.connect(host, port)) {
      continue;
    }
    client.print(String("GET ") + "/csv" + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
      if (millis() - timeout > 300) {
        Serial.println(">>> Client Timeout !");
        client.stop();
        break;
      }
    }

    while (client.available()) {
      String line = client.readStringUntil('\r');
      
      int startIndex = 0;
      int endIndex = line.indexOf(",", startIndex);
      int counter = 0;
      String value;

      while (endIndex != -1) {
        endIndex = line.indexOf(",", startIndex);
        if (endIndex == -1) { // No more delimiters found
          value = line.substring(startIndex);
        } else {
          value = line.substring(startIndex, endIndex);
          startIndex = endIndex + 1;
        }
        int t = value.toInt();
        if (counter == 0) {
          oilTemp = t;
        } else if (counter == 1) {
          oilPressure = t;
        } else if (counter == 2) {
          cyl3Temp = t;
        } else if (counter == 3) {
          cyl2Temp = t;
        }
        counter++;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(updateInterval));
  }
  vTaskDelete(NULL);
}

void renderOilTemp() {
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;
  uint32_t colour = TFT_GREEN;

  if (mode == TACH) {
    return;
  } else if (mode == TEMP) {
    radius = 80;
    xpos = 150;
    ypos = 160;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 40;
    ypos = 200;
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

  arcMeter(xpos, ypos, radius, oilTemp, oilStartTemp, oilEmergTemp, (colour != lastOilTempColour), colour, globalBgColour, "Oil-C", true);

  if (colour != lastOilTempColour) {
    lastOilTempColour = colour;
  }
}

void renderOilPressure() {
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;
  uint32_t colour = TFT_GREEN;

  if (mode == TACH) {
    return;
  } else if (mode == TEMP) {
    return;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 290;
    ypos = 200;
  }

  // Only do this calculation if we are in normal running RPM range.
  // if (rpm > 1000 && rpm < 10000) {
  //   float denom = (rpm / (float)1000);
  //   float factor = (float) oilPressure / denom;
  //   if (factor < 9.0 || factor > 17.0) {
  //     colour = TFT_ORANGE;
  //   } 
  //   if (factor < 8.0 || factor > 22.0) {
  //     colour = TFT_RED;
  //   } 
  // }
  if (oilPressure > oilEmergPressure) {
    colour = TFT_RED;
  } 
  if (oilPressure > oilWarnPressure && colour != TFT_RED) {
    colour = TFT_ORANGE;
  }

  arcMeter(xpos, ypos, radius, oilPressure, oilStartPressure, oilEmergPressure, (colour != lastOilPressureColour), colour, globalBgColour, "Oil-PSI", false);

  if (colour != lastOilPressureColour) {
    lastOilPressureColour = colour;
  }
}

void renderTach() {
  calculateAvgRpm();
  Serial.println("RPM:" + String(rpm));
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;
  uint32_t colour = TFT_GREEN;

  int displayRpm = (int)rpm / 10;

  if (mode == TEMP) {
    return;
  } else if (mode == TACH) {
    radius = 120;
  } else if (mode == ALL) {
    radius = 80;
    xpos = 160;
    ypos = 120;
  }

  if (displayRpm > tachEmerg/10) {
    colour = TFT_RED;
  } 
  else if (displayRpm > tachWarn/10) {
    colour = TFT_ORANGE;
  }

  ringMeter(xpos, ypos, radius, displayRpm, tachStart/10, tachEmerg/10, (colour != lastTachColour), colour, globalBgColour , "Tach");

  if (colour != lastTachColour) {
    lastTachColour = colour;
  }
}

void renderCylinder2() {
  static int16_t xpos;
  static int16_t ypos;
  static uint8_t radius;

  if (mode == TACH) {
    return;
  } else if (mode == TEMP) {
    radius = 60;
    xpos = 270;
    ypos = 60;
  } else if (mode == ALL) {
    radius = 40;
    xpos = 290;
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

  arcMeter(xpos, ypos, radius, cyl2Temp, cylStartTemp, cylEmergTemp, (colour != lastCyl2Colour), colour, globalBgColour, "C2", false);
  if (colour != lastCyl2Colour) {
    lastCyl2Colour = colour;
  }
}

void renderCylinder3() {
  static int16_t xpos = 40;
  static int16_t ypos = 40;
  static uint8_t radius = 40;

  if (mode == TACH) {
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

  arcMeter(xpos, ypos, radius, cyl3Temp, cylStartTemp, cylEmergTemp, (colour != lastCyl3Colour), colour, globalBgColour, "C3", true);
  if (colour != lastCyl3Colour) {
    lastCyl3Colour = colour;
  }
}
