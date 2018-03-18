/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * ThingPulse invests considerable time and money to develop these open source libraries.
 * Please support us by buying our products (and not the clones) from
 * https://thingpulse.com
 *
 */

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "images.h"

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, D3, D5);
SSD1306  display2(0x3c, D1, D2);


#define DEMO_DURATION 3000
typedef void (*Demo)(SSD1306 *display);

int demoMode = 0;
int counter = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();


  // Initialising the UI will init the display too.
  display.init();
  display2.init();

  display.setI2cAutoInit(true);
  display2.setI2cAutoInit(true);


  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

  display.flipScreenVertically();
  display2.setFont(ArialMT_Plain_10);



}

void drawFontFaceDemo(SSD1306 *d) {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    d->setTextAlignment(TEXT_ALIGN_LEFT);
    d->setFont(ArialMT_Plain_10);
    d->drawString(0, 0, "Hello world");
    d->setFont(ArialMT_Plain_16);
    d->drawString(0, 10, "Hello world");
    d->setFont(ArialMT_Plain_24);
    d->drawString(0, 26, "Hello world");
}

void drawTextFlowDemo(SSD1306 *d) {
    d->setFont(ArialMT_Plain_10);
    d->setTextAlignment(TEXT_ALIGN_LEFT);
    d->drawStringMaxWidth(0, 0, 128,
      "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
}

void drawTextAlignmentDemo(SSD1306 *d) {
    // Text alignment demo
  d->setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  d->setTextAlignment(TEXT_ALIGN_LEFT);
  d->drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  d->setTextAlignment(TEXT_ALIGN_CENTER);
  d->drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  d->setTextAlignment(TEXT_ALIGN_RIGHT);
  d->drawString(128, 33, "Right aligned (128,33)");
}

void drawRectDemo(SSD1306 *d) {
      // Draw a pixel at given position
    for (int i = 0; i < 10; i++) {
      d->setPixel(i, i);
      d->setPixel(10 - i, i);
    }
    d->drawRect(12, 12, 20, 20);

    // Fill the rectangle
    d->fillRect(14, 14, 17, 17);

    // Draw a line horizontally
    d->drawHorizontalLine(0, 40, 20);

    // Draw a line horizontally
    d->drawVerticalLine(40, 0, 20);
}

void drawCircleDemo(SSD1306 *d) {
  for (int i=1; i < 8; i++) {
    d->setColor(WHITE);
    d->drawCircle(32, 32, i*3);
    if (i % 2 == 0) {
      d->setColor(BLACK);
    }
    d->fillCircle(96, 32, 32 - i* 3);
  }
}

void drawProgressBarDemo(SSD1306 *d) {
  int progress = (counter / 5) % 100;
  // draw the progress bar
  d->drawProgressBar(0, 32, 120, 10, progress);

  // draw the percentage as String
  d->setTextAlignment(TEXT_ALIGN_CENTER);
  d->drawString(64, 15, String(progress) + "%");
}

void drawImageDemo(SSD1306 *d) {
    // see http://blog.squix.org/2015/05/esp8266-nodemcu-how-to-create-xbm.html
    // on how to create xbm files
    d->drawXbm(34, 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}


Demo demos[] = {drawFontFaceDemo, drawTextFlowDemo, drawTextAlignmentDemo, drawRectDemo, drawCircleDemo, drawProgressBarDemo, drawImageDemo};
int demoLength = (sizeof(demos) / sizeof(Demo));
long timeSinceLastModeSwitch = 0;

void loop() {
  display.clear();
  demos[demoMode](&display);
  display.display();

  display2.clear();
  demos[(demoMode + 1) % demoLength](&display2);
  display2.display();


  if (millis() - timeSinceLastModeSwitch > DEMO_DURATION) {
    demoMode = (demoMode + 1)  % demoLength;
    timeSinceLastModeSwitch = millis();
  }
  counter++;
  delay(10);
}
