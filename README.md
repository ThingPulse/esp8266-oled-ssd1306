# esp8266-oled-ssd1306

This is a driver for the SSD1306 based 128x64 pixel OLED display running on the Arduino/ESP8266 platform.

You can either download this library as a zip file and unpack it to your Arduino/libraries folder or (once it has been added) choose it from the Arduino library manager.

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

Fonts are defined in a proprietary but open format. I wrote a  program that converts any TrueType font into this format. Once the code is useful enough I will publish it or make it available as Webapplication (SaaS), where you can make any font you like available to the library.

## Demo

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
