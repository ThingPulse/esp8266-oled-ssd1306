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
 * Credits for parts of this code go to Mike Rankin. Thank you so much for sharing!
 */

#pragma once

#include <Arduino.h>
#include <Wire.h>

#include "SSD1306Fonts.h"

//#define DEBUG_SSD1306(...) Serial.printf( __VA_ARGS__ )

#ifndef DEBUG_SSD1306
#define DEBUG_SSD1306(...)
#endif

// Use DOUBLE BUFFERING by default
#ifndef SSD1306_REDUCE_MEMORY
#define SSD1306_DOUBLE_BUFFER
#endif


// Display settings
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64
#define DISPLAY_BUFFER_SIZE 1024

// Header Values
#define JUMPTABLE_BYTES 4

#define JUMPTABLE_LSB   1
#define JUMPTABLE_SIZE  2
#define JUMPTABLE_WIDTH 3
#define JUMPTABLE_START 4

#define WIDTH_POS 0
#define HEIGHT_POS 1
#define FIRST_CHAR_POS 2
#define CHAR_NUM_POS 3


// Display commands
#define CHARGEPUMP 0x8D
#define COLUMNADDR 0x21
#define COMSCANDEC 0xC8
#define COMSCANINC 0xC0
#define DISPLAYALLON 0xA5
#define DISPLAYALLON_RESUME 0xA4
#define DISPLAYOFF 0xAE
#define DISPLAYON 0xAF
#define EXTERNALVCC 0x1
#define INVERTDISPLAY 0xA7
#define MEMORYMODE 0x20
#define NORMALDISPLAY 0xA6
#define PAGEADDR 0x22
#define SEGREMAP 0xA0
#define SETCOMPINS 0xDA
#define SETCONTRAST 0x81
#define SETDISPLAYCLOCKDIV 0xD5
#define SETDISPLAYOFFSET 0xD3
#define SETHIGHCOLUMN 0x10
#define SETLOWCOLUMN 0x00
#define SETMULTIPLEX 0xA8
#define SETPRECHARGE 0xD9
#define SETSEGMENTREMAP 0xA1
#define SETSTARTLINE 0x40
#define SETVCOMDETECT 0xDB
#define SWITCHCAPVCC 0x2

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif

enum SSD1306_COLOR {
  BLACK = 0,
  WHITE = 1,
  INVERSE = 2
};

enum SSD1306_TEXT_ALIGNMENT {
  TEXT_ALIGN_LEFT = 0,
  TEXT_ALIGN_RIGHT = 1,
  TEXT_ALIGN_CENTER = 2,
  TEXT_ALIGN_CENTER_BOTH = 3
};

class SSD1306 {
  private:

    uint8_t             i2cAddress;
    uint8_t             sda;
    uint8_t             sdc;

    uint8_t            *buffer;

    #ifdef SSD1306_DOUBLE_BUFFER
    uint8_t            *buffer_back;
    #endif

    SSD1306_TEXT_ALIGNMENT   textAlignment = TEXT_ALIGN_LEFT;
    SSD1306_COLOR            color         = WHITE;

    const char          *fontData      = ArialMT_Plain_10;

    // Send a command to the display (low level function)
    void sendCommand(unsigned char com);

    // Send all the init commands
    void sendInitCommands(void);

    // converts utf8 characters to extended ascii
    byte utf8ascii(byte ascii);
    char* utf8ascii(String s);

    void drawInternal(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const char *data, uint16_t offset, uint16_t bytesInData) __attribute__((always_inline));

    void drawStringInternal(int16_t xMove, int16_t yMove, char* text, uint16_t textLength, uint16_t textWidth);
  public:

    // Create the display object connected to pin sda and sdc
    SSD1306(uint8_t i2cAddress, uint8_t sda, uint8_t sdc);

    // Initialize the display
    bool init();

    // Free the memory used by the display
    void end();

    // Cycle through the initialization
    void resetDisplay(void);

    // Connect again to the display through I2C
    void reconnect(void);

    /* Drawing functions */

    // Sets the color of all pixel operations
    void setColor(SSD1306_COLOR color);

    // Draw a pixel at given position
    void setPixel(int16_t x, int16_t y);
    
    // Draw a line from position 0 to position 1
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1);

    // Draw the border of a rectangle at the given location
    void drawRect(int16_t x, int16_t y, int16_t width, int16_t height);

    // Fill the rectangle
    void fillRect(int16_t x, int16_t y, int16_t width, int16_t height);

    // Draw the border of a circle
    void drawCircle(int16_t x, int16_t y, int16_t radius);

    // Fill circle
    void fillCircle(int16_t x, int16_t y, int16_t radius);

    // Draw a line horizontally
    void drawHorizontalLine(int16_t x, int16_t y, int16_t length);

    // Draw a lin vertically
    void drawVerticalLine(int16_t x, int16_t y, int16_t length);

    /**
     * Draws a rounded progress bar with the outer dimensions given by width and height.
     */
    void drawProgressBar(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t progress);

    // Draw a bitmap in the internal image format
    void drawFastImage(int16_t x, int16_t y, int16_t width, int16_t height, const char *image);

    // Draw a XBM
    void drawXbm(int16_t x, int16_t y, int16_t width, int16_t height, const char *xbm);

    /* Text functions */

    // Draws a string at the given location
    void drawString(int16_t x, int16_t y, String text);

    // Draws a String with a maximum width at the given location.
    // If the given String is wider than the specified width
    // The text will be wrapped to the next line at a space or dash
    void drawStringMaxWidth(int16_t x, int16_t y, uint16_t maxLineWidth, String text);

    // Returns the width of the const char* with the current
    // font settings
    uint16_t getStringWidth(const char* text, uint16_t length);

    // Convencience method for the const char version
    uint16_t getStringWidth(String text);

    // Specifies relative to which anchor point
    // the text is rendered. Available constants:
    // TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT, TEXT_ALIGN_CENTER_BOTH
    void setTextAlignment(SSD1306_TEXT_ALIGNMENT textAlignment);

    // Sets the current font. Available default fonts
    // ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
    void setFont(const char *fontData);

    /* Display functions */

    // Turn the display on
    void displayOn(void);

    // Turn the display offs
    void displayOff(void);

    // Inverted display mode
    void invertDisplay(void);

    // Normal display mode
    void normalDisplay(void);

    // Set display contrast
    void setContrast(char contrast);

    // Turn the display upside down
    void flipScreenVertically();

    // Write the buffer to the display memory
    void display(void);

    // Clear the local pixel buffer
    void clear(void);

};
