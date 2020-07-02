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
 * KS0108 shift register driver by Bruce Ratoff, ko4xl@yahoo.com, July 2020
 *
 * based on SSD1306Spi driver from original thingpulse library and I2C_graphical_LCD_display
 * by Nick Gammon and Bruce Ratoff
 *
 * The KS0108 display has a native parallel interface. This library uses an interface
 * consisting of a pair of 74HCT595 latched shift registers, developed by Bruce Ratoff,
 * to provide a 2-wire (clock+data) high speed serial interface.
 * See below for circuit information. A PCB is available from OSHPark 
 * at https://oshpark.com/shared_projects/ggrOvewK
 *
 */

/*

======== CONNECTIONS FOR 74HCT595 2-wire INTERFACE ========

LCD pins 1,2,3,18,19,20 are connected same as above
LCD pin 5 (R/~W) is connected to GND
LCD pin 17 (~RST) is tied to +5v via a 10K resistor

---- Pins on 74HC595 (uses 2) ----
IC1
  1	  (QB)			   CS2	   LCD pin 16
  2   (QC)			   CS1	   LCD pin 15
  3   (QD)			   D7		   LCD pin 14
  4   (QE)         D6      LCD pin 13
  5   (QF)         D5      LCD pin 12
  6   (QG)         D4      LCD pin 11
  7   (QH)         D3      LCD pin 10
  8   (VSS)        GND     Ground for IC1
  9   (QH*)        Carry   IC2 pin 14 (SER) IC1 shift register out to IC2 shift register in
 10   (SCL)        +5V     IC1 clear input pulled high (always disabled)
 11   (SCK)        CLK     Arduino output pin for CLK (NOTE: Same for IC1 and IC2)
 12   (RCK)        LATCH   IC2 pin 12 and anode of D1
 13   (G)          GND     IC1 output enable pulled low (always enabled)
 14   (SER)        DATA    Free side of R1 (see below) and Arduino output pin for DATA
 15   (QA)                 No connection
 16   (VDD)        +5V     Power for IC1
 
IC2
  1	  (QB)			   D1	   	 LCD pin 8
  2   (QC)			   D0	   	 LCD pin 7
  3   (QD)			   DI	   	 LCD pin 4
  4   (QE)         Enable	 LCD pin 6
  5   (QF)               	 No connection
  6   (QG)                 No connection
  7   (QH)                 No connection
  8   (VSS)        GND     Ground for IC2
  9   (QH*)        Control Cathode of D1
 10   (SCL)        +5V     IC2 clear input pulled high (always disabled)
 11   (SCK)        CLK     Arduino output pin for CLK (NOTE: Same for IC1 and IC2)
 12   (RCK)        LATCH   IC1 pin 12 and anode of D1 (NOTE: Same for IC1 and IC2)
 13   (G)          GND     IC2 output enable pulled low (always enabled - Same for IC1 and IC2)
 14   (SER)        Carry   IC1 pin 9 (QH*) IC2 shift register in from IC1 shift register out
 15   (QA)         D2      LCD pin 9
 16   (VDD)        +5V     Power for IC2
 
2-wire Control circuit:
																	o Control (from IC2 pin 9)
																	|
																	D1 (cathode)
																	D1 (anode)
																	|
	DATA (from Arduino) o----R1-----o----o LATCH (to IC1 and IC2 pin 12)
													(1K)

LCD Power On Reset circuit:
								   +5V
									|
									R3 (1K)
									|
									o----o LCD_RST (LCD pin 17)
									|
									C3 (1uf)
									|
									V
								   GND

For reliable operation, place a 0.1 ufd bypass capacitor across power pins (8 and 16) of each 74HC595

*/


#ifndef KS0108SR_h
#define KS0108SR_h

#include "OLEDDisplay.h"

//#define DEBUG_KS0108SR

#ifdef DEBUG_KS0108SR
#define PCD_DEBUG(s) Serial.println(s);
#else
#define PCD_DEBUG(s) ;
#endif

// Display pins
#define LCD_CS1    0b00000100   // chip select 1  (pin 23)                            0x04
#define LCD_CS2    0b00001000   // chip select 2  (pin 24)                            0x08
#define LCD_RESET  0b00010000   // reset (pin 25)                                     0x10
#define LCD_DATA   0b00100000   // 1xxxxxxx = data; 0xxxxxxx = instruction  (pin 26)  0x20
#define LCD_READ   0b01000000   // x1xxxxxx = read; x0xxxxxx = write  (pin 27)        0x40
#define LCD_ENABLE 0b10000000   // enable by toggling high/low  (pin 28)              0x80

// Display commands
#define LCD_ON          0x3F
#define LCD_OFF         0x3E
#define LCD_SET_ADD     0x40   // plus X address (0 to 63) 
#define LCD_SET_PAGE    0xB8   // plus Y address (0 to 7)
#define LCD_DISP_START  0xC0   // plus X address (0 to 63) - for scrolling

#define LCD_BUSY_DELAY 50

#define LCD_COMMAND	0
#define LCD_DATA		1

// Architecture-specific port manipulation macros for 2-wire interface
#if defined(__AVR__) || defined(ARDUINO_ARCH_SAMD)
#define clkpulse() {*_clkPort |= _clkMask; *_clkPort ^= _clkMask;}
#define setdata(v) {if(v) *_dataPort |= _dataMask; else *_dataPort &= ~_dataMask;}

#elif defined(digitalWriteFast)
#define clkpulse() {digitalWriteFast(_clkPin, HIGH); digitalWriteFast(_clkPin, LOW);}
#define setdata(v) {digitalWriteFast(_dataPin, v);}

#elif defined(ARDUINO_ARCH_ESP8266)
#define clkpulse() {GPOS = _clkMask; GPOC = _clkMask;}
#define setdata(v) {if(v) GPOS = _dataMask; else GPOC = _dataMask;}

#else
#define clkpulse() {digitalWrite(_clkPin, HIGH); digitalWrite(_clkPin, LOW);}
#define setdata(v) {digitalWrite(_dataPin, v);}
#endif

#define sendbit(v) {setdata(v); clkpulse();}


class KS0108SR : public OLEDDisplay {
  private:
			byte _chipSelect;  // currently-selected chip (LCD_CS1 or LCD_CS2)
			byte _lcdx;        // current x position (0 - 127)
			byte _lcdy;        // current y position (0 - 63)
			bool _invmode;		 // true if inverse mode

			byte _clkPin;		// pin for 2-wire CLK
			byte _dataPin;	// pin for 2-wire DATA

	// Data for architecture-specific bit I/O functions
		#if defined(__AVR__)
			volatile byte * _clkPort;	// CLK port
			byte _clkMask;	// CLK bitmask
			volatile byte * _dataPort;	// DATA port
			byte _dataMask;	// DATA bitmask
		#elif defined(ARDUINO_ARCH_SAMD)
			volatile uint32_t * _clkPort;
			uint32_t _clkMask;
			volatile uint32_t * _dataPort;
			uint32_t _dataMask;
		#elif defined(ARDUINO_ARCH_ESP8266)
			uint16_t _clkMask;
			uint16_t _dataMask;
		#endif

		// Send command or data on 2-wire interface
		void do2wireSend (const byte rs, const byte data, const byte enable) {
			sendbit(1);		// Leading 1 to eventually enable latch
			sendbit(0);		// fillers
			sendbit(0);
			sendbit(enable);	// LCD enable
			sendbit(rs);	// rs is 0 for command, 1 for data
			byte t = data;
			for(char i = 0; i < 8; ++i)	 // send data byte one bit at a time, LSB first
			{
				sendbit(t & 0x01);
				t >>= 1;
			}
			sendbit((_chipSelect & LCD_CS1) != 0);
			sendbit((_chipSelect & LCD_CS2) != 0);
			sendbit(1);			// Latch new data
			sendbit(0);
		//	for(int i = 0; i < 15; ++i) UNROLLED FOR SPEED
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
			clkpulse();		// Clear the shifter
		}

		void cmd(const byte data) {
			do2wireSend(0, data, 1);		// rs is 0 meaning instruction, raise enable
			do2wireSend(0, data, 0);		// now drop enable
		}

		void gotoxy (byte x, byte y) {
			if (x > 127) 
				x = 0;                
			if (y > 63)  
				y = 0;
			
			// work out which chip
			if (x >= 64)
				{
				x -= 64;  
				_chipSelect = LCD_CS2;
				}
			else
				_chipSelect = LCD_CS1;
			
			// remember for incrementing later
			_lcdx = x;
			_lcdy = y;
			
			// command LCD to the correct page and address
			cmd (LCD_SET_PAGE | (y >> 3) );  // 8 pixels to a page
			cmd (LCD_SET_ADD  | x );          
			
		}  // end of gotoxy 

		// write a byte to the LCD display at the selected x,y position
		// if inv true, invert the data
		// writing advances the cursor 1 pixel to the right
		// it wraps to the next "line" if necessary (a line is 8 pixels deep)
		void writeData (byte data, const boolean inv = false) {
			// invert data to be written if wanted
			if (inv)
				data ^= 0xFF;
			
			do2wireSend(1, data, 1);		// rs is 1 to indicate data, raise enable
			do2wireSend(1, data, 0);		// now drop enable
			
			// we have now moved right one pixel (in the LCD hardware)
			_lcdx++;
			
			// see if we moved from chip 1 to chip 2, or wrapped at end of line
			if (_lcdx >= 64)
				{
				if (_chipSelect == LCD_CS1)  // on chip 1, move to chip 2
					gotoxy (64, _lcdy);
				else
					gotoxy (0, _lcdy + 8);  // go back to chip 1, down one line
				}
		}  // end of writeData

		int getBufferOffset(void) {
			return 0;
		}


  public:
    KS0108SR(uint8_t clk, uint8_t data, OLEDDISPLAY_GEOMETRY g = GEOMETRY_128_64) {
      setGeometry(g, 128, 64);
      _clkPin  = clk;
      _dataPin  = data;
			_invmode = 0;
    }


    bool connect(){
					PCD_DEBUG("connect() called, scl="+String(_clkPin)+", sda="+String(_dataPin));

			#if defined(__AVR__) || defined(ARDUINO_ARCH_SAMD)
			this->_clkPort = portOutputRegister(digitalPinToPort(_clkPin));
			this->_dataPort = portOutputRegister(digitalPinToPort(_dataPin));
			this->_clkMask = digitalPinToBitMask(_clkPin);
			this->_dataMask = digitalPinToBitMask(_dataPin);
			#endif
			#if defined(ARDUINO_ARCH_ESP8266)
			this->_clkMask = 1<<_clkPin;
			this->_dataMask = 1<<_dataPin;
			#endif

      pinMode(_clkPin, OUTPUT);		// Init I/O pins
      pinMode(_dataPin, OUTPUT);
			digitalWrite(_clkPin, LOW);
			_chipSelect = 0;
			do2wireSend(0, 0, 0);		// Clear the 2 wire interface

			delay(2);		// time for KS0108 to finish resetting
			
      return true;
    }	// connect()
		

    void sendInitCommands(void) {
			PCD_DEBUG("sendInitCommands() called");

			_chipSelect = LCD_CS1;		// select left bank
			cmd(LCD_ON);	// turn it on
			cmd(LCD_DISP_START);	// set scroll position to 0

			_chipSelect = LCD_CS2;		// select right bank
			cmd(LCD_ON);	// turn it on too
			cmd(LCD_DISP_START);	// set scroll position to 0
			
			gotoxy(0, 0);

		}


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
					delay(0);
				 }

				 // If the minBoundY wasn't updated
				 // we can savely assume that buffer_back[pos] == buffer[pos]
				 // holdes true for all values of pos
				 if (minBoundY == UINT8_MAX) return;
				 
				 PCD_DEBUG("display() : minBoundX="+String(minBoundX)+", maxBoundX="+String(maxBoundX)+", minBoundY="+String(minBoundY)+", maxBoundY="+String(maxBoundY));

				 for (y = minBoundY; y <= maxBoundY; y++) {
						gotoxy(minBoundX, y * 8);
						for (x = minBoundX; x <= maxBoundX; x++) {
							writeData(buffer[x + y * displayWidth], _invmode);
							delay(0);
						}
				 }

			#else
				// No double buffering
				gotoxy(0, 0);

				for (uint16_t i=0; i<displayBufferSize; i++) {
					writeData(buffer[i], _invmode);
					delay(0);
				}
			#endif
		}

		// Turn the display on
		void displayOn(void) {
			PCD_DEBUG("displayOn() called");
/*
			// turn LCD chip 1 on
			_chipSelect = LCD_CS1;
			cmd (LCD_ON);
			// turn LCD chip 2 on
			_chipSelect = LCD_CS2;
			cmd (LCD_ON);
			// and put the cursor in the top-left corner
			gotoxy (0, 0);
			// ensure scroll is set to zero
			scroll (0);   
*/
		}

		// Turn the display off
		void displayOff(void) {
			PCD_DEBUG("displayOff() called");
/*
			// turn LCD chip 1 off
			_chipSelect = LCD_CS1;
			cmd (LCD_OFF);
			// turn LCD chip 2 off
			_chipSelect = LCD_CS2;
			cmd (LCD_OFF);
			// and put the cursor in the top-left corner
			gotoxy (0, 0);
			// ensure scroll is set to zero
			scroll (0);   
*/
		}

		// Inverted display mode
		void invertDisplay(void) {
			PCD_DEBUG("invertDisplay() called");
			_invmode = true;
		}

		// Normal display mode
		void normalDisplay(void) {
			PCD_DEBUG("normalDisplay() called");
			_invmode = false;
		}


// *** Unsupported functions for this display ***

		// Set display contrast
		// really low brightness & contrast: contrast = 10, precharge = 5, comdetect = 0
		// normal brightness & contrast:  contrast = 100
		void setContrast(uint8_t contrast = 0, uint8_t tempCoef = 0, uint8_t bias = 0) {
			PCD_DEBUG("setContrast() called contrast="+String(contrast)+", tempCoef="+String(tempCoef)+", bias="+String(bias));
		}

		// Convenience method to access 
		void setBrightness(uint8_t) {return;}

		// Reset display rotation or mirroring
		void resetOrientation() {return;}

		// Turn the display upside down
		void flipScreenVertically() {return;}

		// Mirror the display (to be used in a mirror or as a projector)
		void mirrorScreen() {return;}

};

#endif	// KS0108SR_h
