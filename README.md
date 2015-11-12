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
