#include <SPI.h>
#include <TFT_eSPI.h> // Hardware-specific library
#include <XPT2046_Touchscreen.h>

#include "NotoSans_Bold.h"
#include "OpenFontRender.h"
#include "meters.h"

extern OpenFontRender ofr;
extern TFT_eSPI tft;            // Invoke custom library with default width and height
extern TFT_eSprite spr;

void ringMeter(int x, int y, int r, int val, int minVal, int maxVal, bool refreshMeter, uint32_t fgColour, uint32_t bgColour, const char *units)
{
  int startAngle = 30;
  int endAngle = 330;

  tft.drawSmoothCircle(x, y, r, fgColour, bgColour);

  if (refreshMeter) {
    tft.fillCircle(x, y, r, bgColour);
    uint16_t tmp = r - 3;
    tft.drawArc(x, y, tmp, tmp - tmp / 5, startAngle, endAngle, bgColour, bgColour);
  }

  r -= 3;

  int angle = map(val, 0, maxVal, startAngle, endAngle);

  ofr.setDrawer(spr); // Link renderer to sprite (font will be rendered in sprite spr)

  renderMeterLabel(x, y, r, val, bgColour, units);

  // Allocate a value to the arc thickness dependant of radius
  uint8_t thickness = r / 5;
  if ( r < 25 ) thickness = r / 3;

  tft.drawArc(x, y, r, r - thickness, startAngle, angle, fgColour, bgColour);
  tft.drawArc(x, y, r, r - thickness, angle, endAngle, bgColour, bgColour);
}

void arcMeter(int x, int y, int r, int val, int minVal, int maxVal, bool refreshMeter, uint32_t fgColour, uint32_t bgColour, const char *units, bool rightSide)
{
  int startAngle = 30;
  int endAngle = 150;
  if (rightSide) {
    startAngle = 210;
    endAngle = 330;
  }

  // Allocate a value to the arc thickness dependant of radius
  uint8_t thickness = r / 5;
  if ( r < 25 ) thickness = r / 3;

  if (refreshMeter) {
    tft.drawArc(x, y, r, r - 3 - thickness, startAngle, endAngle, bgColour, bgColour);
  }

  int angleRange = abs(endAngle - startAngle);
  int angleToConsume = map(val, minVal, maxVal, 0, angleRange);
  angleToConsume = max(angleToConsume, 2); // Always show a tiny bar
  angleToConsume = min(angleToConsume, angleRange);


  ofr.setDrawer(spr); // Link renderer to sprite (font will be rendered in sprite spr)
  renderMeterLabel(x, y, r, val, bgColour, units);

  tft.drawArc(x, y, r, r - 1, startAngle, endAngle, fgColour, bgColour);

  r -= 3;

// Oil Pressure is left side
  int angle = startAngle + angleToConsume;
  if (rightSide) {
    angle = endAngle - angleToConsume;
    tft.drawArc(x, y, r, r - thickness, angle, endAngle, fgColour, bgColour);
    tft.drawArc(x, y, r, r - thickness, startAngle, angle, bgColour, bgColour);
  } else {
    tft.drawArc(x, y, r, r - thickness, startAngle, angle, fgColour, bgColour);
    tft.drawArc(x, y, r, r - thickness, angle, endAngle, bgColour, bgColour);
  }
}

void renderMeterLabel(int x, int y, int r, int val, uint32_t bgColour, const char *units) {
    // Add value in centre if radius is a reasonable size
    if ( r >= 25 ) {
      // This code gets the font dimensions in pixels to determine the required the sprite size
      ofr.setFontSize((6 * r) / 4);
      ofr.setFontColor(TFT_WHITE, bgColour);

      uint8_t w = ofr.getTextWidth("444");
      uint8_t h = ofr.getTextHeight("4") + 4;
      spr.createSprite(w, h + 2);
      spr.fillSprite(bgColour); // Blank out the sprite
      char str_buf[8];
      itoa (val, str_buf, 10);
      uint8_t ptr = 0;         // Pointer to a digit character
      uint8_t dx = 0;          // x offset for cursor position
      if (val < 100) dx = ofr.getTextWidth("4") / 2; // Adjust cursor x for 2 digits
      if (val < 10) dx = ofr.getTextWidth("4");      // Adjust cursor x for 1 digit
      while ((uint8_t)str_buf[ptr] != 0) ptr++;      // Count the characters
      while (ptr) {
        ptr--;                                       // Decrement character pointer
        ofr.setCursor(w - dx - (w / 20), -h / 2.5);    // Offset cursor position in sprite
        ofr.rprintf(str_buf + ptr);              // Draw a character
        str_buf[ptr] = 0;                        // Replace character with a null
        dx += w / 3;                             // Adjust cursor for next character
      }
      spr.pushSprite(x - w / 2, y - h / 2); // Push sprite containing the val number
      spr.deleteSprite();                   // Recover used memory
    
      // Make the TFT the print destination, print the units label direct to the TFT
      ofr.setDrawer(tft);
      ofr.setFontColor(TFT_GOLD, TFT_BLACK);
      ofr.setFontSize(r / 2.0);
      ofr.setCursor(x, y + (r * 0.4));
      ofr.cprintf(units);
    }
}