# GEOMETRY_64_48

At this time the 64x48 geometry setting works only with the `Wire.h` library.

I've tested it successfully with a WEMOS D1 mini Lite and a WEMOS OLED shield

Initialization code:
```
#include <Wire.h>
#include <SSD1306Wire.h>
SSD1306Wire display(0x3c, D2, D1, GEOMETRY_64_48 ); // WEMOS OLED shield
```
