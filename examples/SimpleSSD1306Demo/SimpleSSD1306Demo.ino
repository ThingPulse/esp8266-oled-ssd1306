/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
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
 */

#include <Wire.h>
#include "SSD1306.h"
#include "images.h"

#define DEMO_SPEED 500
#define NUMBER_OF_DEMOS 5
typedef void (*Demo)(void);

// Initialize the OLED display on address 0x3c
// D3 and D4 are the pin names on the NodeMCU. Change to int values
// if get compilation errors
SSD1306   display(0x3c, D3, D4);


int demoMode = 0;
int counter = 1;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();


  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);

}

void fontFaceDemo() {
    // Font Demo1
    // create more fonts at http://oleddisplay.squix.ch/
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "Hello world");
    display.setFont(ArialMT_Plain_16);
    display.drawString(0, 10, "Hello world");
    display.setFont(ArialMT_Plain_24);
    display.drawString(0, 26, "Hello world");
}

void textFlowDemo() {
    display.setFont(ArialMT_Plain_10);
    display.drawStringMaxWidth(0, 0, 128, 
      "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore." );
}

void textAlignmentDemo() {
    // Text alignment demo
  display.setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 10, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64, 22, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 33, "Right aligned (128,33)");
}

void drawRectDemo() {
      // Draw a pixel at given position
    for (int i = 0; i < 10; i++) {
      display.setPixel(i, i);
      display.setPixel(10 - i, i);
    }
    display.drawRect(12, 12, 20, 20);

    // Fill the rectangle
    display.fillRect(14, 14, 17, 17);

    // Draw a line horizontally
    display.drawHorizontalLine(0, 40, 20);

    // Draw a line horizontally
    display.drawVerticalLine(40, 0, 20);

    // this demo is too fast otherwise
    delay(10);
} 

void circleDemo() {
  for (int i=1; i < 8; i++) {
    display.setColor(WHITE);
    display.drawCircle(32, 32, i*3);
    if (i % 2 == 0) {
      display.setColor(BLACK);
    }
    display.fillCircle(96, 32, 32 - i* 3);
  }
  delay(25);
}

void progressBarDemo() {
  display.drawProgressBar(0, 32, 120, 16, (counter / 5) % 100);
}

Demo demos[] = {progressBarDemo, /*fontFaceDemo, textFlowDemo, textAlignmentDemo, drawRectDemo, circleDemo*/};
int demoLength = (sizeof(demos) / sizeof(Demo));

void loop() {
  // clear the display
  display.clear();
  // draw the current demo method
  demos[demoMode]();
  // write the buffer to the display
  display.display();

  if (counter % DEMO_SPEED == 0) {
    demoMode = (demoMode + 1)  % demoLength;
    Serial.println(String(counter) + ": " + String(demoMode));
  }
  counter++;
}

