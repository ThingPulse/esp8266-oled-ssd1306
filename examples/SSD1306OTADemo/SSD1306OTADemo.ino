/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 by Daniel Eichhorn
 * Copyright (c) 2016 by Fabrice Weinberg
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

// WiFi includes
 #include <ESP8266WiFi.h>

 // OTA Includes
 #include <ESP8266mDNS.h>
 #include <ArduinoOTA.h>

 const char *ssid         = "[Your SSID]";
 const char *password     = "[Your Password]";


// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
// or #include "SH1106.h" alis for `#include "SH1106Wire.h"`
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
// SSD1306Brzo display(0x3c, D3, D5);
// or
// SH1106Brzo  display(0x3c, D3, D5);

// Initialize the OLED display using Wire library
SSD1306  display(0x3c, D3, D5);
// SH1106 display(0x3c, D3, D5);


void setup() {
  WiFi.begin ( ssid, password );

  // Wait for connection
  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 10 );
  }

  display.init();
  display.flipScreenVertically();
  display.setContrast(255);

  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2 - 10, "OTA Update");
    display.display();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    display.drawProgressBar(4, 32, 120, 8, progress / (total / 100) );
    display.display();
  });

  ArduinoOTA.onEnd([]() {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
    display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Restart");
    display.display();
  });

  // Align text vertical/horizontal center
  display.setTextAlignment(TEXT_ALIGN_CENTER_BOTH);
  display.setFont(ArialMT_Plain_10);
  display.drawString(DISPLAY_WIDTH/2, DISPLAY_HEIGHT/2, "Ready for OTA:\n" + WiFi.localIP().toString());
  display.display();
}

void loop() {
  ArduinoOTA.handle();
}
