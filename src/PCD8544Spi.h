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

/*
 * PCD8544Spi driver by Bruce Ratoff, ko4xl@yahoo.com, October 2019
 *
 * based on SSD1306Spi driver from original thingpulse library
 *
 * The net intent of these changes is to complete the virtualization of the display
 * hardware, so that other displays with different protocols and different geometries
 * can be accommodated.  The PCD8544 is somewhat similar in protocol to the original
 * OLED displays in the existing library, and so was a good starting example.
 *
 */


#ifndef PCD8544Spi_h
#define PCD8544Spi_h

#include "OLEDDisplay.h"
#include <SPI.h>

//#define DEBUG_PCD8544

#ifdef DEBUG_PCD8544
#define PCD_DEBUG(s) Serial.println(s);
#else
#define PCD_DEBUG(s) ;
#endif

// Display commands
#define EXTCOMMAND	0x21
#define STDCOMMAND	0x20
#define SETINVERSE	0x0D
#define SETNORMAL		0x0c
#define SETBLANK		0x08
#define SETROW0			0x40
#define SETCOLUMN0	0x80
#define SETVOP0			0x80

#define DFLT_VOP 0xB0
#define DFLT_TEMPCOEF 0x04
#define DFLT_BIAS 0x12

#define LCD_COMMAND	0
#define LCD_DATA		1


class PCD8544Spi : public OLEDDisplay {
  private:
      uint8_t             _rst;
      uint8_t             _dc;
      uint8_t             _cs;
			bool								_inverted;

  public:
    PCD8544Spi(uint8_t _rst, uint8_t _dc, uint8_t _cs, OLEDDISPLAY_GEOMETRY g = GEOMETRY_RAWMODE) {
      setGeometry(GEOMETRY_RAWMODE, 84, 48);
      this->_rst = _rst;
      this->_dc  = _dc;
      this->_cs  = _cs;
			this->_inverted = 0;
    }

    bool connect(){
		PCD_DEBUG("connect() called, dc="+String(_dc)+", cs="+String(_cs)+", rst="+String(_rst));
      pinMode(_dc, OUTPUT);
      pinMode(_cs, OUTPUT);
      pinMode(_rst, OUTPUT);

      SPI.begin ();
      SPI.setClockDivider (SPI_CLOCK_DIV4);

      // Pulse Reset low for 1ms
      digitalWrite(_rst, HIGH);
      delay(1);
      digitalWrite(_rst, LOW);
      delay(1);
      digitalWrite(_rst, HIGH);
			delay(1);
      return true;
    }

    void sendInitCommands(void) {
		PCD_DEBUG("sendInitCommands() called");
			sendCommand(EXTCOMMAND);
			sendCommand(DFLT_VOP);
			sendCommand(DFLT_TEMPCOEF);
			sendCommand(DFLT_BIAS);
			sendCommand(STDCOMMAND);
			sendCommand(SETNORMAL);
		}
/*
		void setGeometry(OLEDDISPLAY_GEOMETRY g) {
			this->geometry = GEOMETRY_RAWMODE;
			this->displayWidth                     = 84;
			this->displayHeight                    = 48;
			this->displayBufferSize                = displayWidth*displayHeight/8;
		}
*/
		void display(void) {

			#ifdef OLEDDISPLAY_DOUBLE_BUFFER
				 uint8_t minBoundY = UINT8_MAX;
				 uint8_t maxBoundY = 0;

				 uint8_t minBoundX = UINT8_MAX;
				 uint8_t maxBoundX = 0;

				 uint8_t x, y;

				 // Calculate the Y bounding box of changes
				 // and copy buffer[pos] to buffer_back[pos];
				 for (y = 0; y < (displayHeight / 8); y++) {
					 for (x = 0; x < displayWidth; x++) {
						uint16_t pos = x + y * displayWidth;
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
				 if (minBoundY == UINT8_MAX) return;
				 
				 PCD_DEBUG("display() : minBoundX="+String(minBoundX)+", maxBoundX="+String(maxBoundX)+", minBoundY="+String(minBoundY)+", maxBoundY="+String(maxBoundY));

				 for (y = minBoundY; y <= maxBoundY; y++) {
					sendCommand(SETCOLUMN0 + minBoundX);
					sendCommand(SETROW0 + y);
					digitalWrite(_dc, HIGH);   // data mode
					digitalWrite(_cs, LOW);
						for (x = minBoundX; x <= maxBoundX; x++) {
							SPI.transfer(buffer[x + y * displayWidth]);
							yield();
						}
				 }
				 digitalWrite(_cs, HIGH);

			 #else
				 // No double buffering
				 sendCommand(SETCOLUMN0);
				 sendCommand(SETROW0);

					digitalWrite(_cs, HIGH);
					digitalWrite(_dc, HIGH);   // data mode
					digitalWrite(_cs, LOW);
					for (uint16_t i=0; i<displayBufferSize; i++) {
						SPI.transfer(buffer[i]);
						yield();
					}
					digitalWrite(_cs, HIGH);
			 #endif
		}

		// Turn the display on
		void displayOn(void) {
			PCD_DEBUG("displayOn() called");
			if(_inverted) {
				sendCommand(SETINVERSE);
			} else {
				sendCommand(SETNORMAL);
			}
		}

		// Turn the display off
		void displayOff(void) {
			PCD_DEBUG("displayOff() called");
			sendCommand(SETBLANK);
		}

		// Inverted display mode
		void invertDisplay(void) {
			PCD_DEBUG("invertDisplay() called");
			sendCommand(SETINVERSE);
			_inverted = true;
		}

		// Normal display mode
		void normalDisplay(void) {
			PCD_DEBUG("normalDisplay() called");
			sendCommand(SETNORMAL);
			_inverted = false;
		}

		// Set display contrast
		// really low brightness & contrast: contrast = 10, precharge = 5, comdetect = 0
		// normal brightness & contrast:  contrast = 100
		void setContrast(uint8_t contrast, uint8_t tempCoef = DFLT_TEMPCOEF, uint8_t bias = DFLT_BIAS) {
			PCD_DEBUG("setContrast() called contrast="+String(contrast)+", tempCoef="+String(tempCoef)+", bias="+String(bias));
			sendCommand(EXTCOMMAND);
			sendCommand(0x80 | (contrast & 0x7f));
			sendCommand(0x04 | (tempCoef & 0x03));
			sendCommand(0x10 | (bias & 0x07));
			sendCommand(STDCOMMAND);
		}


// *** Unsupported functions for this display ***

		// Convenience method to access 
		void setBrightness(uint8_t) {return;}

		// Reset display rotation or mirroring
		void resetOrientation() {return;}

		// Turn the display upside down
		void flipScreenVertically() {return;}

		// Mirror the display (to be used in a mirror or as a projector)
		void mirrorScreen() {return;}
			

  private:

		void inline sendCommand(uint8_t com) {
			writeLcd(LCD_COMMAND, com);
		}
		
		void writeLcd(uint8_t dataOrCommand, uint8_t data) {
			digitalWrite(_cs, HIGH);
			digitalWrite(_dc, dataOrCommand);
			digitalWrite(_cs, LOW);
			SPI.transfer(data);
			digitalWrite(_cs, HIGH);
		}
	

};

#endif
