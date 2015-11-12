#include <ArduinoJson.h>

/**The MIT License (MIT)

Copyright (c) 2015 by Daniel Eichhorn

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

See more at http://blog.squix.ch
*/
#include <Wire.h>
#include "ssd1306_i2c.h"
#include "images.h"

// if you are using a ESP8266 module with NodeMCU
// pin labels, you can use this list to keep
// your code and the lables in-sync
#define NODEMCU_D0 16
#define NODEMCU_D1 5
#define NODEMCU_D2 4
#define NODEMCU_D3 0
#define NODEMCU_D4 2
#define NODEMCU_D5 14
#define NODEMCU_D6 12
#define NODEMCU_D7 13
#define NODEMCU_D8 15
#define NODEMCU_D9 3
#define NODEMCU_D10 1
#define NODEMCU_D12 10



// Initialize the oled display for address 0x3c
// sda-pin=14 and sdc-pin=12
SSD1306 display(0x3c, NODEMCU_D6, NODEMCU_D5);

// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
void (*frameCallbacks[])(int x, int y) = {drawFrame1, drawFrame2, drawFrame3, drawFrame4};

// how many frames are there?
int frameCount = 4;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();
  
  // initialize dispaly
  display.init();
  display.flipScreenVertically();
  // set the drawing functions
  display.setFrameCallbacks(frameCount, frameCallbacks);
  // how many ticks does a slide of a frame take?
  display.setFrameTransitionTicks(10);
  // defines how many ticks the driver waits between frame transitions
  display.setFrameWaitTicks(150);

  display.clear();
  display.display();
  
}

void loop() {
  if (display.getFrameState() == display.FRAME_STATE_FIX) {
    // do something which consumes a lot of time in a moment
    // when there is no transition between frames going on.
    // This will keep transitions smooth;
  }

  // clear the frame
  display.clear();

  // Tell the driver to render the next frame.
  // This enables the frame mode including the transition
  // and the drawing of the frame indicators
  display.nextFrameTick();

  // Even in frame mode you can draw static elements.
  // But they won't be transitioned
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, 54, "20:54");

  // copy the buffer to the display
  display.display();
}

void drawFrame1(int x, int y) {
  // draw an xbm image.
  // Please note that everything that should be transitioned
  // needs to be drawn relative to x and y
  display.drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
 }

 void drawFrame2(int x, int y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0 + x, 0 + y, "Arial 10");

  display.setFont(ArialMT_Plain_16);
  display.drawString(0 + x, 10 + y, "Arial 16");

  display.setFont(ArialMT_Plain_24);
  display.drawString(0 + x, 24 + y, "Arial 24");
}

void drawFrame3(int x, int y) {
  // Text alignment demo
  display.setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0 + x, 0 + y, "Left aligned (0,0)");

  // The coordinates define the center of the text
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.drawString(64 + x, 20, "Center aligned (64,20)");

  // The coordinates define the right end of the text
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128 + x, 40, "Right aligned (128,40)");
}

void drawFrame4(int x, int y) {
  // Demo for drawStringMaxWidth: 
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawStringMaxWidth(0 + x, 0 + y, 128, "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore."); 
}



