#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiMulti.h>

#include "custom_sd.h"
#include "meters.h"

WiFiMulti WiFiMulti;
TaskHandle_t wifiTask;


void setup(void) {
  Serial.begin(baudRate);
  delay(10);

  sdSpi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);

  if (!SD.begin(SD_CS, sdSpi, 55000000)) {
    sdCardMounted = false;
    Serial.println("Card Mount Failed");
  } else {
    loadSettings();
    if(loadCounter()) {
      bootCount++;
      saveCounter();
    } else {
      sdCardMounted = false;
    }
    writeToSd();
  }

  tsSoftSpi = new SoftSPI (XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK);
  touchscreen.begin(tsSoftSpi);
  touchscreen.setRotation(0);

  tft.begin();
  tft.setRotation(0);
  if (colourIssues) {
    tft.invertDisplay(true);
  }

  tft.fillScreen(globalBgColour);

  if (ofr.loadFont(TTF_FONT, sizeof(TTF_FONT))) {
    Serial.println("Render initialize error");
    return;
  }
  
  WiFiMulti.addAP(ssid.c_str(), password.c_str());

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
  
  if (sdCardMounted && nowMs - lastSdWrite > sdWritePeriod) {
    writeToSd();
    lastSdWrite = nowMs;
  }
}

void getDataFromWifi(void * pvParameters) {
  while (WiFiMulti.run() != WL_CONNECTED) {
    Serial.println("Waiting to connect to wifi");
    delay(250);
  }

  NetworkClient client;
  for(;;) {
    if (!client.connect(host.c_str(), port)) {
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
    vTaskDelay(pdMS_TO_TICKS(wifiUpdateInterval));
  }
  vTaskDelete(NULL);
}
