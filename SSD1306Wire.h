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

#ifndef SSD1306Wire_h
#define SSD1306Wire_h

#include "OLEDDisplay.h"
#include <Wire.h>

class SSD1306Wire : public OLEDDisplay {
  private:
      uint8_t             _address;
      uint8_t             _sda;
      uint8_t             _scl;
      uint8_t             _driver;

  public:
    SSD1306Wire(uint8_t _address, uint8_t _sda, uint8_t _scl, uint8_t _driver=DRIVER_SSD1306) {
      this->_address = _address;
      this->_sda     = _sda;
      this->_scl     = _scl;
      this->_driver  = _driver;
    }

    bool connect() {
      Wire.begin(this->_sda, this->_scl);
      // Let's use ~700khz if ESP8266 is in 160Mhz mode
      // this will be limited to ~400khz if the ESP8266 in 80Mhz mode.
      Wire.setClock(700000);
      return true;
    }

    void display(void) {

      #ifdef OLEDDISPLAY_DOUBLE_BUFFER
        uint8_t minBoundY = ~0;
        uint8_t maxBoundY = 0;

        uint8_t minBoundX = ~0;
        uint8_t maxBoundX = 0;
        uint8_t x, y;

        // Calculate the Y bounding box of changes
        // and copy buffer[pos] to buffer_back[pos];
        for (y = 0; y < (DISPLAY_HEIGHT / 8); y++) {
          for (x = 0; x < DISPLAY_WIDTH; x++) {
           uint16_t pos = x + y * DISPLAY_WIDTH;
           if (buffer[pos] != buffer_back[pos]) {
             minBoundY = _min(minBoundY, y);
             maxBoundY = _max(maxBoundY, y);
             minBoundX = _min(minBoundX, x);
             maxBoundX = _max(maxBoundX, x);
           }
           buffer_back[pos] = buffer[pos];
         }
         yield();
        }

        // If the minBoundY wasn't updated
        // we can savely assume that buffer_back[pos] == buffer[pos]
        // holdes true for all values of pos
        if (minBoundY == ~0) return;

        if (_driver == DRIVER_SSD1306) {
          sendCommand(COLUMNADDR);
          sendCommand(minBoundX);
          sendCommand(maxBoundX);

          sendCommand(PAGEADDR);
          sendCommand(minBoundY);
          sendCommand(maxBoundY);

        } else if (_driver == DRIVER_SH1106) {
          // We should avoid this, and optimize using excellent 
          // bound method but I did not succeded on SH1106
          minBoundX = 0;
          maxBoundX = 127;
        }

        byte k = 0;
        for (y = minBoundY; y <= maxBoundY; y++) {

          if (_driver == DRIVER_SH1106) {
            sendCommand(0xB0+y);//set page address;
            // SH1106 ix 132x64 RAM so we need to shift by 2 to center to 128
            sendCommand(0x00|((minBoundX+2)&0x0F)) ;//set lower column address
            sendCommand(0x10|((minBoundX+2)>>4)) ;  //set higher column address
          }

          for (x = minBoundX; x <= maxBoundX; x++) {
            if (k == 0) {
              Wire.beginTransmission(_address);
              Wire.write(0x40);
            }
            Wire.write(buffer[x + y * DISPLAY_WIDTH]);
            k++;
            if (k == 16)  {
              Wire.endTransmission();
              k = 0;
            }
          }
        }
        
      #else

        if (_driver == DRIVER_SSD1306) {
          sendCommand(COLUMNADDR);
          sendCommand(0x0);
          sendCommand(0x7F);

          sendCommand(PAGEADDR);
          sendCommand(0x0);
          sendCommand(0x7);
        }

        uint8_t * p = &buffer[0];
        for (uint8_t y=0; y<8; y++) {

          if (_driver == DRIVER_SH1106) {
            sendCommand(0xB0+y);//set page addressSSD_Data_Mode;
            // SH1106 ix 132x64 RAM so we need to shift by 2 to center to 128
            sendCommand(0x02) ;//set lower column address 
            sendCommand(0x10) ;//set higher column address
          }

          for( uint8_t x=0; x<8; x++) {
            Wire.beginTransmission(_address);
            Wire.write(0x40);
            for (uint8_t k = 0; k < 16; k++) {
              Wire.write(*p++);
            }
            Wire.endTransmission();
          }
        }
      #endif
    }

  private:
    inline void sendCommand(uint8_t command) __attribute__((always_inline)){
      Wire.beginTransmission(_address);
      Wire.write(0x80);
      Wire.write(command);
      Wire.endTransmission();
    }

    uint8_t driver() {
      return _driver;
    }

};

#endif
