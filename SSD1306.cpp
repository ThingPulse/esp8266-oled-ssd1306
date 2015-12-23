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

Credits for parts of this code go to Mike Rankin. Thank you so much for sharing!
*/

#include "SSD1306.h"
#include <Wire.h>


SSD1306::SSD1306(int i2cAddress, int sda, int sdc) {
  myI2cAddress = i2cAddress;
  mySda = sda;
  mySdc = sdc;
}

void SSD1306::init() {
  Wire.begin(mySda, mySdc);
  Wire.setClock(400000);
  sendInitCommands();
  resetDisplay();
}

void SSD1306::resetDisplay(void) {
  displayOff();
  clear();
  display();
  displayOn();
}

void SSD1306::reconnect() {
  Wire.begin(mySda, mySdc);
}

void SSD1306::displayOn(void) {
  sendCommand(DISPLAYON);
}

void SSD1306::displayOff(void) {
  sendCommand(DISPLAYOFF);
}

void SSD1306::invertDisplay(void) {
  sendCommand(INVERTDISPLAY);
}

void SSD1306::normalDisplay(void) {
  sendCommand(NORMALDISPLAY);
}


void SSD1306::setContrast(char contrast) {
  sendCommand(SETCONTRAST);
  sendCommand(contrast);
}

void SSD1306::flipScreenVertically() {
  sendCommand(SEGREMAP | 0x1);      //SEGREMAP   //Rotate screen 180 deg
  sendCommand(COMSCANDEC);            //COMSCANDEC  Rotate screen 180 Deg
}

void SSD1306::clear(void) {
    memset(buffer, 0, DISPLAY_BUFFER_SIZE);
}

void SSD1306::display(void) {
    sendCommand(COLUMNADDR);
    sendCommand(0x0);
    sendCommand(0x7F);

    sendCommand(PAGEADDR);
    sendCommand(0x0);
    sendCommand(0x7);

    for (uint16_t i=0; i<DISPLAY_BUFFER_SIZE; i++) {
      // send a bunch of data in one xmission
      Wire.beginTransmission(myI2cAddress);
      Wire.write(0x40);
      for (uint8_t x=0; x<16; x++) {
        Wire.write(buffer[i]);
        i++;
      }
      i--;
      yield();
      Wire.endTransmission();
    }


}

void SSD1306::setPixel(int x, int y) {
  if (x >= 0 && x < DISPLAY_WIDTH && y >= 0 && y < DISPLAY_HEIGHT) {
     switch (myColor) {
      case WHITE:   buffer[x + (y/8)*DISPLAY_WIDTH] |=  (1 << (y&7)); break;
      case BLACK:   buffer[x + (y/8)*DISPLAY_WIDTH] &= ~(1 << (y&7)); break;
      case INVERSE: buffer[x + (y/8)*DISPLAY_WIDTH] ^=  (1 << (y&7)); break;
    }
  }
}

void SSD1306::setChar(int x, int y, unsigned char data) {
  for (int i = 0; i < 8; i++) {
    if (bitRead(data, i)) {
     setPixel(x,y + i);
    }
  }
}

// Code form http://playground.arduino.cc/Main/Utf8ascii
byte SSD1306::utf8ascii(byte ascii) {
    if ( ascii<128 ) {   // Standard ASCII-set 0..0x7F handling
       lastChar=0;
       return( ascii );
    }

    // get previous input
    byte last = lastChar;   // get last char
    lastChar=ascii;         // remember actual character

    switch (last)     // conversion depnding on first UTF8-character
    {   case 0xC2: return  (ascii);  break;
        case 0xC3: return  (ascii | 0xC0);  break;
        case 0x82: if(ascii==0xAC) return(0x80);       // special case Euro-symbol
    }

    return  (0);                                     // otherwise: return zero, if character has to be ignored
}

// Code form http://playground.arduino.cc/Main/Utf8ascii
String SSD1306::utf8ascii(String s) {
        String r= "";
        char c;
        for (int i=0; i<s.length(); i++)
        {
                c = utf8ascii(s.charAt(i));
                if (c!=0) r+=c;
        }
        return r;
}

void SSD1306::drawString(int xMove, int yMove, String text) {

  text = utf8ascii(text);

  uint16_t textHeight       = pgm_read_byte(myFontData + HEIGHT_POS);
  uint16_t sizeOfJumpTable  = pgm_read_byte(myFontData + CHAR_NUM_POS)  * JUMPTABLE_BYTES;
  uint16_t firstChar        = pgm_read_byte(myFontData + FIRST_CHAR_POS);

  uint16_t textWidth        = getStringWidth(text);

  uint16_t cursorX          = 0;

  switch (myTextAlignment) {
    case TEXT_ALIGN_CENTER_BOTH:
      yMove -= textHeight >> 1;
    // Fallthrough
    case TEXT_ALIGN_CENTER:
      xMove -= textWidth >> 1; // divide by 2
      break;
    case TEXT_ALIGN_RIGHT:
      xMove -= textWidth;
      break;
  }


  // Don't draw anything if it is not on the screen.
  if (xMove + textWidth  < 0 || xMove > DISPLAY_WIDTH )  return;
  if (yMove + textHeight < 0 || yMove > DISPLAY_HEIGHT)  return;

  uint16_t length = text.length();

  for (uint16_t j = 0; j < length; j++) {

    byte charCode = text.charAt(j) - firstChar;

    // 4 Bytes per char code
    byte msbJumpToChar    = pgm_read_byte( myFontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES );                  // MSB  \ JumpAddress
    byte lsbJumpToChar    = pgm_read_byte( myFontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_LSB);   // LSB /
    byte charByteSize     = pgm_read_byte( myFontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_SIZE);  // Size
    byte currentCharWidth = pgm_read_byte( myFontData + JUMPTABLE_START + charCode * JUMPTABLE_BYTES + JUMPTABLE_WIDTH); // Width

    // Test if the char is drawable
    if (msbJumpToChar != 255 && lsbJumpToChar != 255) {
      // Get the position of the char data
      uint16_t charDataPosition = JUMPTABLE_START + sizeOfJumpTable + ((msbJumpToChar << 8) + lsbJumpToChar);
      drawInternal(xMove + cursorX, yMove, currentCharWidth, textHeight, myFontData, charDataPosition, charByteSize);
    }

    cursorX += currentCharWidth;
  }
}

void SSD1306::drawStringMaxWidth(int x, int y, int maxLineWidth, String text) {
  int currentLineWidth = 0;
  int startsAt = 0;
  int endsAt = 0;
  int lineNumber = 0;
  char currentChar = ' ';
  int lineHeight = pgm_read_byte(myFontData + HEIGHT_POS);
  String currentLine = "";
  for (int i = 0; i < text.length(); i++) {
    currentChar = text.charAt(i);
    if (currentChar == ' ' || currentChar == '-') {
      String lineCandidate = text.substring(startsAt, i);
      if (getStringWidth(lineCandidate) <= maxLineWidth) {
        endsAt = i;
      } else {

        drawString(x, y + lineNumber * lineHeight, text.substring(startsAt, endsAt));
        lineNumber++;
        startsAt = endsAt + 1;
      }
    }

  }
  drawString(x, y + lineNumber * lineHeight, text.substring(startsAt));
}

int SSD1306::getStringWidth(String text) {
  text = utf8ascii(text);
  int stringWidth      = 0;
  int firstChar        = pgm_read_byte(myFontData + FIRST_CHAR_POS);
  int length           = text.length();
  for (int j = 0; j < length; j++) {
    stringWidth += pgm_read_byte(myFontData + JUMPTABLE_START +  (text.charAt(j) - firstChar) * JUMPTABLE_BYTES + JUMPTABLE_WIDTH);
  }
  return stringWidth;
}

void SSD1306::setTextAlignment(int textAlignment) {
  myTextAlignment = textAlignment;
}

void SSD1306::setFont(const char *fontData) {
  myFontData = fontData;
}

void SSD1306::drawBitmap(int x, int y, int width, int height, const char *bitmap) {
  for (int i = 0; i < width * height / 8; i++ ){
    unsigned char charColumn = 255 - pgm_read_byte(bitmap + i);
    for (int j = 0; j < 8; j++) {
      int targetX = i % width + x;
      int targetY = (i / (width)) * 8 + j + y;
      if (bitRead(charColumn, j)) {
        setPixel(targetX, targetY);
      }
    }
  }
}

void SSD1306::setColor(int color) {
  myColor = color;
}

void SSD1306::drawRect(int x, int y, int width, int height) {
  for (int i = x; i < x + width; i++) {
    setPixel(i, y);
    setPixel(i, y + height);
  }
  for (int i = y; i < y + height; i++) {
    setPixel(x, i);
    setPixel(x + width, i);
  }
}

void SSD1306::fillRect(int x, int y, int width, int height) {
  for (int i = x; i < x + width; i++) {
    for (int j = y; j < y + height; j++) {
      setPixel(i, j);
    }
  }
}

void SSD1306::drawFastImage(int xMove, int yMove, int width, int height, const char *image) {
  drawInternal(xMove, yMove, width, height, image, 0, 0);
}

void SSD1306::drawInternal(int xMove, int yMove, int width, int height, const char *data, uint16_t offset, uint16_t bytesInData) {
  if (width < 0 || height < 0) return;
  if (yMove + height < 0 || yMove > DISPLAY_HEIGHT)  return;
  if (xMove + width  < 0 || xMove > DISPLAY_WIDTH)   return;


  uint8_t  rasterHeight = 1 + ((height - 1) >> 3); // fast ceil(height / 8.0)
  int      yOffset      = yMove & 7;

  bytesInData = bytesInData == 0 ? width * rasterHeight : bytesInData;

  int initYMove   = yMove;
  int initYOffset = yOffset;


  for (int i = 0; i < bytesInData; i++) {

    // Reset if next horizontal drawing phase is started.
    if ( i % rasterHeight == 0) {
      yMove   = initYMove;
      yOffset = initYOffset;
    }

    byte currentByte = pgm_read_byte(data + offset + i);

    int xPos = xMove + (i / rasterHeight);
    int yPos = ((yMove >> 3) + (i % rasterHeight)) * DISPLAY_WIDTH;

    int yScreenPos = yMove + yOffset;
    int dataPos    = xPos  + yPos;

    if (dataPos >=  0  && dataPos < DISPLAY_BUFFER_SIZE &&
        xPos    >=  0  && xPos    < DISPLAY_WIDTH ) {

      if (yOffset >= 0) {
        switch (myColor) {
          case WHITE:   buffer[dataPos] |= currentByte << yOffset; break;
          case BLACK:   buffer[dataPos] &= currentByte << yOffset; break;
          case INVERSE: buffer[dataPos] ^= currentByte << yOffset; break;
        }
        if (dataPos < (DISPLAY_BUFFER_SIZE - DISPLAY_WIDTH)) {
          switch (myColor) { // Double code to not cause performance hit
            case WHITE:   buffer[dataPos + DISPLAY_WIDTH] |= currentByte >> (8 - yOffset); break;
            case BLACK:   buffer[dataPos + DISPLAY_WIDTH] &= currentByte >> (8 - yOffset); break;
            case INVERSE: buffer[dataPos + DISPLAY_WIDTH] ^= currentByte >> (8 - yOffset); break;
          }
        }
      } else {
        // Make new offset position
        yOffset = -yOffset;

        switch (myColor) {
          case WHITE:   buffer[dataPos] |= currentByte >> yOffset; break;
          case BLACK:   buffer[dataPos] &= currentByte >> yOffset; break;
          case INVERSE: buffer[dataPos] ^= currentByte >> yOffset; break;
        }

        // Prepare for next iteration by moving one block up
        yMove -= 8;

        // and setting the new yOffset
        yOffset = 8 - yOffset;
      }

      yield();
    }
  }
}

void SSD1306::drawXbm(int x, int y, int width, int height, const char *xbm) {
  if (width % 8 != 0) {
    width =  ((width / 8) + 1) * 8;
  }
  for (int i = 0; i < width * height / 8; i++ ){
    unsigned char charColumn = pgm_read_byte(xbm + i);
    for (int j = 0; j < 8; j++) {
      int targetX = (i * 8 + j) % width + x;
      int targetY = (8 * i / (width)) + y;
      if (bitRead(charColumn, j)) {
        setPixel(targetX, targetY);
      }
    }
  }
}

void SSD1306::sendCommand(unsigned char com) {
  Wire.beginTransmission(myI2cAddress);     //begin transmitting
  Wire.write(0x80);                          //command mode
  Wire.write(com);
  Wire.endTransmission();                    // stop transmitting
}

void SSD1306::sendInitCommands(void) {
  sendCommand(DISPLAYOFF);
  sendCommand(SETDISPLAYCLOCKDIV);
  sendCommand(0x80);
  sendCommand(SETMULTIPLEX);
  sendCommand(0x3F);
  sendCommand(SETDISPLAYOFFSET);
  sendCommand(0x00);
  sendCommand(SETSTARTLINE);
  sendCommand(CHARGEPUMP);
  sendCommand(0x14);
  sendCommand(MEMORYMODE);
  sendCommand(0x00);
  sendCommand(SEGREMAP);
  sendCommand(COMSCANINC);
  sendCommand(SETCOMPINS);
  sendCommand(0x12);
  sendCommand(SETCONTRAST);
  sendCommand(0xCF);
  sendCommand(SETPRECHARGE);
  sendCommand(0xF1);
  sendCommand(DISPLAYALLON_RESUME);
  sendCommand(NORMALDISPLAY);
  sendCommand(0x2e);            // stop scroll
  sendCommand(DISPLAYON);
}
