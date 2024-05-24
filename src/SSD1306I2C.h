/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 by Helmut Tschemernjak - www.radioshuttle.de
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

#ifndef SSD1306I2C_h
#define SSD1306I2C_h


#include "OLEDDisplay.h"

#ifdef __MBED__
#include <mbed.h>

#ifndef UINT8_MAX
 #define UINT8_MAX 0xff
#endif
#elif ESP_PLATFORM
#include <algorithm>
#include "driver/gpio.h"
#include "driver/i2c.h"

#define I2C_MASTER_NUM              0                          /*!< I2C master i2c port number, the number of i2c peripheral interfaces available will depend on the chip */
#define I2C_MASTER_FREQ_HZ          400000                     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                          /*!< I2C master doesn't need buffer */
#define I2C_MASTER_TIMEOUT_MS       1000
#endif

class SSD1306I2C : public OLEDDisplay {
public:
#ifdef __MBED__
    SSD1306I2C(uint8_t address, PinName sda, PinName scl, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64) {
      this->_address = address << 1;  // convert from 7 to 8 bit for mbed.
#elif ESP_PLATFORM
    SSD1306I2C(uint8_t address, gpio_num_t sda, gpio_num_t scl, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64) {
      this->_address = address;
#endif
      setGeometry(g);

      this->_sda = sda;
      this->_scl = scl;

#ifdef __MBED__
	  _i2c = new I2C(sda, scl);
#endif
    }

    bool connect() {
		// mbed supports 100k and 400k some device maybe 1000k
#ifdef TARGET_STM32L4
	  _i2c->frequency(1000000);
#elif __MBED__
	  _i2c->frequency(400000);
#elif ESP_PLATFORM
    return (i2c_master_init(_sda, _scl) == ESP_OK);
#endif
      return true;
    }

    void display(void) {
      const int x_offset = (128 - this->width()) / 2;
#ifdef OLEDDISPLAY_DOUBLE_BUFFER
        uint8_t minBoundY = UINT8_MAX;
        uint8_t maxBoundY = 0;

        uint8_t minBoundX = UINT8_MAX;
        uint8_t maxBoundX = 0;
        uint8_t x, y;

        // Calculate the Y bounding box of changes
        // and copy buffer[pos] to buffer_back[pos];
        for (y = 0; y < (this->height() / 8); y++) {
          for (x = 0; x < this->width(); x++) {
           uint16_t pos = x + y * this->width();
           if (buffer[pos] != buffer_back[pos]) {
             minBoundY = std::min(minBoundY, y);
             maxBoundY = std::max(maxBoundY, y);
             minBoundX = std::min(minBoundX, x);
             maxBoundX = std::max(maxBoundX, x);
           }
           buffer_back[pos] = buffer[pos];
         }
         yield();
        }

        // If the minBoundY wasn't updated
        // we can savely assume that buffer_back[pos] == buffer[pos]
        // holdes true for all values of pos

        if (minBoundY == UINT8_MAX) return;

        sendCommand(COLUMNADDR);
        sendCommand(x_offset + minBoundX);	// column start address (0 = reset)
        sendCommand(x_offset + maxBoundX);	// column end address (127 = reset)

        sendCommand(PAGEADDR);
        sendCommand(minBoundY);				// page start address
        sendCommand(maxBoundY);				// page end address

        for (y = minBoundY; y <= maxBoundY; y++) {
			uint8_t *start = &buffer[(minBoundX + y * this->width())-1];
			uint8_t save = *start;
			
			*start = 0x40; // control
#ifdef __MBED__
			_i2c->write(_address, (char *)start, (maxBoundX-minBoundX) + 1 + 1);
#elif ESP_PLATFORM
      ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, _address, start, (maxBoundX - minBoundX) + 1 + 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
#endif
			*start = save;
		}
#else

        sendCommand(COLUMNADDR);
        sendCommand(x_offset);						// column start address (0 = reset)
        sendCommand(x_offset + (this->width() - 1));// column end address (127 = reset)

        sendCommand(PAGEADDR);
        sendCommand(0x0);							// page start address (0 = reset)

        if (geometry == GEOMETRY_128_64) {
          sendCommand(0x7);
        } else if (geometry == GEOMETRY_128_32) {
          sendCommand(0x3);
        }

		buffer[-1] = 0x40; // control
#ifdef __MBED__
			_i2c->write(_address, (char *)&buffer[-1], displayBufferSize + 1);
#elif ESP_PLATFORM
      ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, _address, &buffer[-1], displayBufferSize + 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
#endif
#endif
    }

private:
	int getBufferOffset(void) {
		return 0;
	}

    inline void sendCommand(uint8_t command) __attribute__((always_inline)) {
		uint8_t _data[2];
	  	_data[0] = 0x80; // control
	  	_data[1] = command;
#ifdef __MBED__
			_i2c->write(_address, _data, sizeof(_data));
#elif ESP_PLATFORM
      ESP_ERROR_CHECK(i2c_master_write_to_device(I2C_MASTER_NUM, _address, _data, sizeof(_data), I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS));
#endif
    }

#ifdef __MBED__
	uint8_t             _address;
	PinName             _sda;
	PinName             _scl;
	I2C *_i2c;
#elif ESP_PLATFORM

  /**
   * @brief i2c master initialization
   */
  static esp_err_t i2c_master_init(gpio_num_t sda, gpio_num_t scl)
  {
    uint8_t i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {};
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = sda;
    conf.scl_io_num = scl;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;

    i2c_param_config(i2c_master_port, &conf);

    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
  }

  uint8_t _address;
  gpio_num_t _sda;
  gpio_num_t _scl;
#endif
};



#endif
