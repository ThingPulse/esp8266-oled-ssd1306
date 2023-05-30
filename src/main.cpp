#include <SSD1306I2C.h>
#include <OLEDDisplay.h> // common fonts and text align constants
#include "driver/gpio.h"

static const uint8_t SSD1306_ADDRESS = 0x3C;
static const gpio_num_t SDA = GPIO_NUM_21;
static const gpio_num_t SCL = GPIO_NUM_22;

SSD1306I2C display(SSD1306_ADDRESS, SDA, SCL);

#ifdef __cplusplus
extern "C" {
#endif

void app_main()
{
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  char buffer[12];
  display.drawStringf(0, 24, buffer, "ESP-IDF %s", IDF_VER);
  display.display();
}

#ifdef __cplusplus
}
#endif