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

#ifndef SSD1306Wire_h
#define SSD1306Wire_h

#include "OLEDDisplay.h"
#include "driver/i2c.h"

class SSD1306I2CEsp : public OLEDDisplay {
  private:
      i2c_port_t          _port;
      uint8_t             _address;
      gpio_num_t          _sda;
      gpio_num_t          _scl;
      bool                _doI2cAutoInit = false;

  public:
    SSD1306I2CEsp(i2c_port_t _port, uint8_t _address, gpio_num_t _sda, gpio_num_t _scl, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64) {
      setGeometry(g);

      this->_port = _port;
      this->_address = _address;
      this->_sda = _sda;
      this->_scl = _scl;
    }

    bool connect() {
      esp_err_t ret;
      i2c_config_t conf;

      conf.mode = I2C_MODE_MASTER;
      conf.sda_io_num = this->_sda;
      conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
      conf.scl_io_num = this->_scl;
      conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
      conf.master.clk_speed = 700000;

      ret = i2c_param_config(this->_port, &conf);
      if (ret != ESP_OK) printf("SSD1306I2CEsp: i2c_param_config() returned %d\n", (int)ret);
      ret = i2c_driver_install(this->_port, conf.mode, 0, 0, 0);
      if (ret != ESP_OK) printf("SSD1306I2CEsp: i2c_driver_install() returned %d\n", (int)ret);

      return (ret == ESP_OK);
    }

    void display(void) {
      initI2cIfNeccesary();
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
        sendCommand(x_offset + minBoundX);
        sendCommand(x_offset + maxBoundX);

        sendCommand(PAGEADDR);
        sendCommand(minBoundY);
        sendCommand(maxBoundY);

        uint8_t k = 0;
        i2c_cmd_handle_t cmd;

        for (y = minBoundY; y <= maxBoundY; y++) {
          for (x = minBoundX; x <= maxBoundX; x++) {
            if (k == 0) {
              cmd = i2c_cmd_link_create();
              i2c_master_start(cmd);
              i2c_master_write_byte(cmd, (this->_address << 1) | I2C_MASTER_WRITE, true);
              i2c_master_write_byte(cmd, 0x40, true);
            }

            i2c_master_write_byte(cmd, buffer[x + y * this->width()], true);
            k++;
            if (k == 16)  {
              i2c_master_stop(cmd);
              i2c_master_cmd_begin(this->_port, cmd, (1000 + (portTICK_RATE_MS >> 1)) / portTICK_RATE_MS);
              i2c_cmd_link_delete(cmd);
              k = 0;
            }
          }
          yield();
        }

        if (k != 0) {
          i2c_master_stop(cmd);
          i2c_master_cmd_begin(this->_port, cmd, (1000 + (portTICK_RATE_MS >> 1)) / portTICK_RATE_MS);
          i2c_cmd_link_delete(cmd);
        }
      #else

        sendCommand(COLUMNADDR);
        sendCommand(x_offset);
        sendCommand(x_offset + (this->width() - 1));

        sendCommand(PAGEADDR);
        sendCommand(0x0);

        if (geometry == GEOMETRY_128_64) {
          sendCommand(0x7);
        } else if (geometry == GEOMETRY_128_32) {
          sendCommand(0x3);
        }

        for (uint16_t i = 0; i < displayBufferSize; i += 16) {
          i2c_cmd_handle_t cmd = i2c_cmd_link_create();
          i2c_master_start(cmd);
          i2c_master_write_byte(cmd, (this->_address << 1) | I2C_MASTER_WRITE, true);
          i2c_master_write_byte(cmd, 0x40, true);
          i2c_master_write(cmd, buffer + i, 16, true);
          i2c_master_stop(cmd);
          i2c_master_cmd_begin(this->_port, cmd, (1000 + (portTICK_RATE_MS >> 1)) / portTICK_RATE_MS);
          i2c_cmd_link_delete(cmd);
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

    void sendCommand(uint8_t command) {
      initI2cIfNeccesary();
      i2c_cmd_handle_t cmd = i2c_cmd_link_create();
      i2c_master_start(cmd);
      i2c_master_write_byte(cmd, (this->_address << 1) | I2C_MASTER_WRITE, true);
      i2c_master_write_byte(cmd, 0x80, true);
      i2c_master_write_byte(cmd, command, true);
      i2c_master_stop(cmd);
      esp_err_t ret = i2c_master_cmd_begin(this->_port, cmd, (1000 + (portTICK_RATE_MS >> 1)) / portTICK_RATE_MS);
      if (ret != ESP_OK) printf("SSD1306I2CEsp: i2c_master_cmd_begin() returned %d\n", (int)ret);
      i2c_cmd_link_delete(cmd);
    }

    inline void initI2cIfNeccesary() __attribute__((always_inline)) {
      if (_doI2cAutoInit) {
        connect();
      }
    }

};

#endif
