/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 by ThingPulse, Daniel Eichhorn
 * Copyright (c) 2018 by Fabrice Weinberg
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

#ifndef SH1106Wire_h
#define SH1106Wire_h

#include "OLEDDisplay.h"
#include <Wire.h>

#if defined(ARDUINO_ARCH_ESP32)
#define I2C_OLED_TRANSFER_BYTE 64 /** ESP32 can Transfer Max 128 bytes */
#else
#define I2C_OLED_TRANSFER_BYTE 16
#endif

#define SH1106_SET_PUMP_VOLTAGE 0X30
#define SH1106_SET_PUMP_MODE 0XAD
#define SH1106_PUMP_ON 0X8B
#define SH1106_PUMP_OFF 0X8A
//--------------------------------------

class SH1106Wire : public OLEDDisplay {
  private:
      uint8_t             _address;
      int             _sda;
      int             _scl;
      bool                _doI2cAutoInit = false;
      TwoWire*            _wire = NULL;
      long                _frequency;

  public:
    /**
     * Create and initialize the Display using Wire library
     *
     * Beware for retro-compatibility default values are provided for all parameters see below.
     * Please note that if you don't wan't SD1306Wire to initialize and change frequency speed ot need to
     * ensure -1 value are specified for all 3 parameters. This can be usefull to control TwoWire with multiple
     * device on the same bus.
     *
     * @param address I2C Display address
     * @param sda I2C SDA pin number, default to -1 to skip Wire begin call
     * @param scl I2C SCL pin number, default to -1 (only SDA = -1 is considered to skip Wire begin call)
     * @param g display geometry dafault to generic GEOMETRY_128_64, see OLEDDISPLAY_GEOMETRY definition for other options
     * @param i2cBus on ESP32 with 2 I2C HW buses, I2C_ONE for 1st Bus, I2C_TWO fot 2nd bus, default I2C_ONE
     * @param frequency for Frequency by default Let's use ~700khz if ESP8266 is in 160Mhz mode, this will be limited to ~400khz if the ESP8266 in 80Mhz mode
     */
    SH1106Wire(uint8_t address, int sda = -1, int scl = -1, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64, HW_I2C i2cBus = I2C_ONE, long frequency = 700000) {
      setGeometry(g);

      this->_address = address;
      this->_sda = sda;
      this->_scl = scl;
#if !defined(ARDUINO_ARCH_ESP32) || defined(CONFIG_IDF_TARGET_ESP32C3) || defined(CONFIG_IDF_TARGET_ESP32C6)
      this->_wire = &Wire;
#else
      this->_wire = (i2cBus==I2C_ONE) ? &Wire : &Wire1;
#endif
      this->_frequency = frequency;
    }

    bool connect() {
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
      _wire->begin();
#else
      // On ESP32 arduino, -1 means 'don't change pins', someone else has called begin for us.
      if(this->_sda != -1)
        _wire->begin(this->_sda, this->_scl);
#endif
      // Let's use ~700khz if ESP8266 is in 160Mhz mode
      // this will be limited to ~400khz if the ESP8266 in 80Mhz mode.
      if(this->_frequency != -1)
        _wire->setClock(this->_frequency);
      return true;
    }

    void display(void) {
      initI2cIfNeccesary();
      #if defined(OLEDDISPLAY_DOUBLE_BUFFER)
        uint8_t minBoundY = UINT8_MAX;
        uint8_t maxBoundY = 0;

        uint8_t minBoundX = UINT8_MAX;
        uint8_t maxBoundX = 0;

        uint8_t x, y;

        // Calculate the bounding box of changes. Do not update buffer_back yet:
        // if an I2C transfer fails, the same dirty region must be retried.
        for (y = 0; y < (displayHeight / 8); y++) {
          for (x = 0; x < displayWidth; x++) {
           uint16_t pos = x + y * displayWidth;
           if (buffer[pos] != buffer_back[pos]) {
             minBoundY = _min(minBoundY, y);
             maxBoundY = _max(maxBoundY, y);
             minBoundX = _min(minBoundX, x);
             maxBoundX = _max(maxBoundX, x);
           }
         }
         yield();
        }

        // If the minBoundY wasn't updated
        // we can savely assume that buffer_back[pos] == buffer[pos]
        // holdes true for all values of pos
        if (minBoundY == UINT8_MAX) return;

        // Calculate the colum offset
        uint8_t minBoundXp2H = (minBoundX + 2) & 0x0F;
        uint8_t minBoundXp2L = 0x10 | ((minBoundX + 2) >> 4 );

        bool txOk = true;
        for (y = minBoundY; y <= maxBoundY; y++) {
          // Both sendCommandChecked calls always execute (left operands of &&), ensuring both address
          // commands are sent to the display. pageOk becomes false if either command fails.
          bool pageOk = sendCommandChecked(0xB0 + y);
          pageOk = sendCommandChecked(minBoundXp2H) && pageOk;
          pageOk = sendCommandChecked(minBoundXp2L) && pageOk;
          if (!pageOk) {
            txOk = false;
            yield();
            continue;
          }
          for (x = minBoundX; x <= maxBoundX;) {
            uint8_t chunkSize = _min((uint16_t)I2C_OLED_TRANSFER_BYTE, (uint16_t)(maxBoundX - x + 1));
            if (!sendDataChunk(&buffer[x + y * displayWidth], chunkSize)) {
              txOk = false;
              break;
            }
            x += chunkSize;
          }
          yield();
        }

        if (txOk) {
          uint16_t changedWidth = maxBoundX - minBoundX + 1;
          for (y = minBoundY; y <= maxBoundY; y++) {
            uint16_t pos = minBoundX + y * displayWidth;
            memcpy(&buffer_back[pos], &buffer[pos], changedWidth);
          }
        }
      #else
        uint8_t * p = &buffer[0];
        bool txOk = true;
        for (uint8_t y=0; y<(displayHeight / 8); y++) {
          bool pageOk = sendCommandChecked(0xB0 + y);
          pageOk = sendCommandChecked(0x02) && pageOk;
          pageOk = sendCommandChecked(0x10) && pageOk;
          if (!pageOk) txOk = false;
          for(uint16_t x=0; x<(displayWidth / I2C_OLED_TRANSFER_BYTE); x++) {
            if (pageOk && !sendDataChunk(p, I2C_OLED_TRANSFER_BYTE)) {
              txOk = false;
              pageOk = false;
            }
            p += I2C_OLED_TRANSFER_BYTE;
          }
          yield();
        }
      #endif
    }

    void setI2cAutoInit(bool doI2cAutoInit) {
      _doI2cAutoInit = doI2cAutoInit;
    }

  private:
	int getBufferOffset(void) {
		return 0;
	}
    inline void sendCommand(uint8_t command) __attribute__((always_inline)){
      sendCommandChecked(command);
    }

    bool sendCommandChecked(uint8_t command) {
      for (uint8_t attempt = 0; attempt < 2; attempt++) {
        _wire->beginTransmission(_address);
        _wire->write(0x80);
        _wire->write(command);
        if (_wire->endTransmission() == 0) return true;
        yield();
      }
      return false;
    }

    // Note: sendDataChunk does not retry on I2C failure (unlike sendCommandChecked which retries twice).
    // Rationale: Command retries are low-cost/high-value (reset display address state); data retries would
    // require recalculating page/column address, making them impractical. The caller (display()) tracks
    // txOk to skip buffer_back sync if any chunk fails, allowing the dirty region to be retried on the next call.
    bool sendDataChunk(const uint8_t *data, uint8_t length) {
      _wire->beginTransmission(_address);
      _wire->write(0x40);
      _wire->write(data, length);
      return _wire->endTransmission() == 0;
    }

    void initI2cIfNeccesary() {
      if (_doI2cAutoInit) {
#if !defined(ARDUINO_ARCH_ESP32) && !defined(ARDUINO_ARCH_ESP8266)
        _wire->begin();
#else
        _wire->begin(this->_sda, this->_scl);
#endif
      }
    }

};

#endif
