# esp8266-oled-ssd1306

This is a driver for the SSD1306 based 128x64 pixel OLED display running on the Arduino/ESP8266 platform.

You can either download this library as a zip file and unpack it to your Arduino/libraries folder or (once it has been added) choose it from the Arduino library manager.

## Credits
Many thanks go to Fabrice Weinberg (@FWeinb) for optimizing and refactoring the UI library.
The init sequence for the SSD1306 was inspired by Adafruits library for the same display.

## Usage

The SSD1306Demo is a very comprehensive example demonstrating the most important features of the library.

## Features

* Draw pixels at given coordinates
* Draw or fill a rectangle with given dimensions
* Draw Text at given coordinates:
 * Define Alignment: Left, Right and Center
 * Set the Fontface you want to use (see section Fonts below)
 * Limit the width of the text by an amount of pixels. Before this widths will be reached, the renderer will wrap the text to a new line if possible
* Display content in automatically side scrolling carousel
 * Define transition cycles
 * Define how long one frame will be displayed
 * Draw the different frames in callback methods
 * One indicator per frame will be automatically displayed. The active frame will be displayed from inactive once

## Fonts

Fonts are defined in a proprietary but open format. You can create new font files by choosing from a given list
of open sourced Fonts from this web app: http://oleddisplay.squix.ch
Choose the font family, style and size, check the preview image and if you like what you see click the "Create" button. This will create the font array in a text area form where you can copy and paste it into a new or existing header file.

![FontTool](https://github.com/squix78/esp8266-oled-ssd1306/raw/master/resources/FontTool.png)


## API

### Display Control

```C++
// Create the display object connected to pin sda and sdc
SSD1306(int i2cAddress, int sda, int sdc);

// Initialize the display
void init();

// Cycle through the initialization
void resetDisplay(void);

// Connect again to the display through I2C
void reconnect(void);

// Turn the display on
void displayOn(void);

// Turn the display offs
void displayOff(void);

// Clear the local pixel buffer
void clear(void);

// Write the buffer to the display memory
void display(void);

// Set display contrast
void setContrast(char contrast);

// Turn the display upside down
void flipScreenVertically();

// Send a command to the display (low level function)
void sendCommand(unsigned char com);

// Send all the init commands
void sendInitCommands(void);
```

## Pixel drawing

```C++
// Draw a pixel at given position
void setPixel(int x, int y);

// Draw 8 bits at the given position
void setChar(int x, int y, unsigned char data);

// Draw the border of a rectangle at the given location
void drawRect(int x, int y, int width, int height);

// Fill the rectangle
void fillRect(int x, int y, int width, int height);

// Draw a bitmap with the given dimensions
void drawBitmap(int x, int y, int width, int height, const char *bitmap);

// Draw an XBM image with the given dimensions
void drawXbm(int x, int y, int width, int height, const char *xbm);

// Sets the color of all pixel operations
void setColor(int color);
```

## Text operations

``` C++
// Draws a string at the given location
void drawString(int x, int y, String text);

// Draws a String with a maximum width at the given location.
// If the given String is wider than the specified width
// The text will be wrapped to the next line at a space or dash
void drawStringMaxWidth(int x, int y, int maxLineWidth, String text);

// Returns the width of the String with the current
// font settings
int getStringWidth(String text);

// Specifies relative to which anchor point
// the text is rendered. Available constants:
// TEXT_ALIGN_LEFT, TEXT_ALIGN_CENTER, TEXT_ALIGN_RIGHT
void setTextAlignment(int textAlignment);

// Sets the current font. Available default fonts
// defined in SSD1306Fonts.h:
// ArialMT_Plain_10, ArialMT_Plain_16, ArialMT_Plain_24
// Or create one with the font tool at http://oleddisplay.squix.ch
void setFont(const char *fontData);
```

## Ui Library (SSD1306Ui)

The Ui Library is used to provide a basic set of Ui elements called, `Frames` and `Overlays`. A `Frame` is used to provide
information the default behaviour is to display a `Frame` for a defined time and than move to the next. The library also provides an `Indicator` that will be updated accordingly. An `Overlay` on the other hand is a pieces of information (e.g. a clock) that is displayed always at the same position. 


```C++
  // Initialize the library should be called in `begin()`
  void init();
  
  // Configure the internal used target FPS
  void setTargetFPS(byte fps);

  // Enable automatic transition to next frame,
  // time can be configured with `setTimePerFrame` and `setTimePerTransition`.
  void enableAutoTransition();

  // Disable automatic transition to next frame.
  void disableAutoTransition();

  // Set the direction of the automatic transitioning
  void setAutoTransitionForwards();
  void setAutoTransitionBackwards();

  // Set the approx. time a frame is displayed
  void setTimePerFrame(int time);

  // Set the approx. time a transition will take
  void setTimePerTransition(int time);

  // Set the position of the indicator bar.
  // TOP, RIGHT, BOTTOM, LEFT
  void setIndicatorPosition(IndicatorPosition pos);

  // Set the direction of the indicator bar. Defining the order of frames ASCENDING / DESCENDING
  void setIndicatorDirection(IndicatorDirection dir);

  // Set the symbole to indicate an active frame in the indicator bar.
  void setActiveSymbole(const char* symbole);

  // Set the symbole to indicate an inactive frame in the indicator bar.
  void setInactiveSymbole(const char* symbole);

  // Configure what animation is used to transition from one frame to another
  void setFrameAnimation(AnimationDirection dir);

  // Add frame drawing functions
  void setFrames(FrameCallback* frameFunctions, int frameCount);

  //Add overlays drawing functions that are draw independent of the Frames
  void setOverlays(OverlayCallback* overlayFunctions, int overlayCount);

  // Manuell Controll
  void  nextFrame();
  void  previousFrame();

  // State Info
  SSD1306UiState getUiState();

  // This needs to be called in your main loop 
  // will return the "remaining" time you have to keep the
  // target FPS
  int update();
```

## Example: SSD1306Demo

### Frame 1
![DemoFrame1](https://github.com/squix78/esp8266-oled-ssd1306/raw/master/resources/DemoFrame1.jpg)

This frame shows three things:
 * How to draw an xbm image
 * How to draw a static text which is not moved by the frame transition
 * The active/inactive frame indicators

### Frame 2
![DemoFrame2](https://github.com/squix78/esp8266-oled-ssd1306/raw/master/resources/DemoFrame2.jpg)

Currently there are one fontface with three sizes included in the library: Arial 10, 16 and 24. Once the converter is published you will be able to convert any ttf font into the used format.

### Frame 3

![DemoFrame3](https://github.com/squix78/esp8266-oled-ssd1306/raw/master/resources/DemoFrame3.jpg)

This frame demonstrates the text alignment. The coordinates in the frame show relative to which position the texts have been rendered.

### Frame 4

![DemoFrame4](https://github.com/squix78/esp8266-oled-ssd1306/raw/master/resources/DemoFrame4.jpg)

This shows how to use define a maximum width after which the driver automatically wraps a word to the next line. This comes in very handy if you have longer texts to display.
