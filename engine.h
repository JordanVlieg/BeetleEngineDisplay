#pragma once

// Tachometer
int tachStart = 0;
int tachLow = 700;
int tachWarn = 4200;
int tachEmerg = 5000;

// Oil temperature
int oilStartTemp = 50;
int oilLowTemp = 60;
int oilWarnTemp = 105;
int oilEmergTemp = 115;

// Cylinder temperature
int cylStartTemp = 110;
int cylLowTemp = 130;
int cylWarnTemp = 205;
int cylEmergTemp = 220;

// Oil Pressure
int oilStartPressure = 0;
int oilWarnPressure = 70;
int oilEmergPressure = 80;

// Misc
const int coilPin = 27;
const int arrayLength = 100;
const int measureDelayMicros = 2000;

// This is a magic number to convert between the period between 2 sparks on a 4 cylinder engine to its RPM
float magicNum = 30000000;

// Variables
float rpm = 0;
unsigned long lastRisingEdgeMicros = 0;
float rpmArr[arrayLength];
int rpmPtr = 0;
int leftPtr = 0;
int coilState = LOW;
int lastCoilState = LOW;
bool hasCleanRising = false;
int oilTemp = 0;
int cyl2Temp = 0;
int cyl3Temp = 0;
int oilPressure = 0;

// Methods

void rpmTracking() {
  // This delay allows the input pin to settle, and prevents ridiculous RPM readings.
  delayMicroseconds(measureDelayMicros);
  coilState = digitalRead(coilPin);
  if (lastCoilState != coilState) {
    unsigned long pulseMicros = micros();
    unsigned long delta = pulseMicros - lastRisingEdgeMicros;
    lastCoilState = coilState;
    if (coilState == HIGH) {
      float tentativeRpm = magicNum / (float)delta;
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
    for (int i = 0; i < sparksSinceLast; i++) {
      sum += rpmArr[((leftPtr + i) % arrayLength)];
    }
    leftPtr = rpmPtr;
    rpm = sum / sparksSinceLast;
  }
}