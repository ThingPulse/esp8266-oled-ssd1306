 /**
  * The MIT License (MIT)
  *
  * Copyright (c) 2020 by NoobTracker
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
 #include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
 // or #include "SH1106Wire.h", legacy include: `#include "SH1106.h"`
 // For a connection via I2C using brzo_i2c (must be installed) include
 // #include <brzo_i2c.h> // Only needed for Arduino 1.6.5 and earlier
 // #include "SSD1306Brzo.h"
 // #include "SH1106Brzo.h"
 // For a connection via SPI include
 // #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
 // #include "SSD1306Spi.h"
 // #include "SH1106SPi.h"

 // Use the corresponding display class:

 // Initialize the OLED display using SPI
 // D5 -> CLK
 // D7 -> MOSI (DOUT)
 // D0 -> RES
 // D2 -> DC
 // D8 -> CS
 // SSD1306Spi        display(D0, D2, D8);
 // or
 // SH1106Spi         display(D0, D2);

 // Initialize the OLED display using brzo_i2c
 // D3 -> SDA
 // D5 -> SCL
 // SSD1306Brzo display(0x3c, SDA, SCL);
 // or
 // SH1106Brzo  display(0x3c, SDA, SCL);

 // Initialize the OLED display using Wire library
 SSD1306Wire  display(0x3c, SDA, SCL);
 // SH1106 display(0x3c, SDA, SCL);

void ring(byte quad, byte pos){
  float angle = (pos * PI) / 256;
  display.fillRing(display.getWidth() / 2, display.getHeight() / 2, sin(angle) * 30, cos(angle) * 20);
}

void setup() {
  display.init();
  display.flipScreenVertically();
  display.setColor(WHITE);
}
uint16_t pos = 0;
void loop() { 
  display.clear();
  ring(pos >> 8, pos & 0xFF);
  pos++;
  display.display();
  delay(30);
}
