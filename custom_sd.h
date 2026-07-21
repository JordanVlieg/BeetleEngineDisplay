#pragma once

#include <FS.h>

#include "ui.h"
#include "engine.h"
#include "SD.h"
#include "FS.h"

bool sdCardMounted = true;
int disableLogging = 0;
unsigned long lastSdWrite = 0;
uint32_t bootCount = 0;

bool loadCounter()
{
  File f = SD.open("/bootcount.bin", FILE_READ);
  if (f && f.read((uint8_t *)&bootCount, sizeof(bootCount)) == sizeof(bootCount))
  {
    // Success
    Serial.println("Loaded bootcount");
  }
  else
  {
    bootCount = 0;
    return false;
    Serial.println("Failed to load bootcount");
  }
  f.close();
  return true;
}

void saveCounter()
{
  File f = SD.open("/bootcount.bin", FILE_WRITE);
  f.seek(0);
  f.write((uint8_t *)&bootCount, sizeof(bootCount));
  f.close();
  Serial.println("Saved bootcount");
}

void writeToSd() {
  if (disableLogging) {
    return;
  }
  char filename[64];
  snprintf(filename, sizeof(filename), "/logs/log_%u.txt",
         (unsigned int)bootCount);
  File file = SD.open(filename, FILE_APPEND);
  if (file)
  {
      file.print(rpm);
      file.print(",");
      file.print(oilTemp);
      file.print(",");
      file.print(cyl2Temp);
      file.print(",");
      file.print(cyl3Temp);
      file.print(",");
      file.print(oilPressure);
      file.print(",");
      file.println(lastUiUpdate);

      file.close();
  } else {
    Serial.println("Failed to write to SD");
  }
}

void loadSettings() {
  File file = SD.open("/settings.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to load settings");
    return;
  }
  Serial.println("Loading settings");

  while (file.available())
  {
    String line = file.readStringUntil('\n');
    line.trim();

    Serial.println(line);

    // Skip blank lines and comments
    if (line.length() == 0 || line.startsWith("#"))
        continue;

    int separator = line.indexOf('=');

    if (separator == -1) {
      Serial.println("Skip entry");
      continue;
    }

    String key = line.substring(0, separator);
    String value = line.substring(separator + 1);
    key.trim();
    value.trim();

    Serial.println(key);
    Serial.println(value);

    if (key == "ssid") {
      ssid = value;
    }
    else if (key == "password") {
      password = value;
    }
    else if (key == "host") {
      host = value;
    }
    else if (key == "port") {
      port = value.toInt();
    }
    else if (key == "wifi_period") {
      wifiUpdateInterval = value.toInt();
    }
    else if (key == "sd_period") {
      sdWritePeriod = value.toInt();
    }
    else if (key == "tach_start") {
      tachStart = value.toInt();
    }
    else if (key == "tach_low") {
      tachLow = value.toInt();
    }
    else if (key == "tach_warn") {
      tachWarn = value.toInt();
    }
    else if (key == "tach_emerg") {
      tachEmerg = value.toInt();
    }
    else if (key == "oil_start_temp") {
      oilStartTemp = value.toInt();
    }
    else if (key == "oil_low_temp") {
      oilLowTemp = value.toInt();
    }
    else if (key == "oil_warn_temp") {
      oilWarnTemp = value.toInt();
    }
    else if (key == "oil_emerg_temp") {
      oilEmergTemp = value.toInt();
    }
    else if (key == "cyl_start_temp") {
      cylStartTemp = value.toInt();
    }
    else if (key == "cyl_low_temp") {
      cylLowTemp = value.toInt();
    }
    else if (key == "cyl_warn_temp") {
      cylWarnTemp = value.toInt();
    }
    else if (key == "cyl_emerg_temp") {
      cylEmergTemp = value.toInt();
    }
    else if (key == "oil_start_pressure") {
      oilStartPressure = value.toInt();
    }
    else if (key == "oil_warn_pressure") {
      oilWarnPressure = value.toInt();
    }
    else if (key == "oil_emerg_pressure") {
      oilEmergPressure = value.toInt();
    }
    else if (key == "disable_logging") {
      disableLogging = value.toInt();
    }
  }
}