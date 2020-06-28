/**The MIT License (MIT)

Copyright (c) 2018 by Daniel Eichhorn - ThingPulse

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

See more at https://thingpulse.com
*/

/*
 * 30 Sep 2019 : Bruce Ratoff : Configure to my environment
 * 01 Oct 2019 : Bruce Ratoff : Add WiFiManager for WiFi config
 * 06 Oct 2019 : Bruce Ratoff : Add conditional build for PCD8544 display
 * 07 Oct 2019 : Bruce Ratoff : Dynamically adjust widths/fonts by display size
 * 11 Nov 2019 : Bruce Ratoff : Add UTC page and make time pages optional
 * 27 Jun 2020 : Bruce Ratoff : Fix time zone issues and add DST setting to WiFiManager parameters
 *                            : Listen for '$' on serial port to reset into WiFi Manager
 * 28 Jun 2020 : Bruce Ratoff : Make time in frame header always local time
*/

//#define PCD8544
//#define SSD1306

#include <ESPWiFi.h>

#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>

#include <ESPHTTPClient.h>
#include <JsonListener.h>
#include <FS.h>

// time
#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()

#ifdef PCD8544
#include "PCD8544Spi.h"
#elif defined(SSD1306)
#include "SSD1306Wire.h"
#include "Wire.h"
#else
#include "SH1106Wire.h"
#include "Wire.h"
#endif

#include "OLEDDisplayUi.h"
#include "OpenWeatherMapCurrent.h"
#include "OpenWeatherMapForecast.h"
#include "WeatherStationFonts.h"
#include "WeatherStationImages.h"

#define HOSTNAME "WxW-"
#define CONFIG "/conf.txt"

/***************************
 * Begin Settings
 **************************/

// Time Zone - in hours BEHIND UTC (i.e., local time = UTC - TZ)
#define TZ 5
// DST flag - if true, enable offset for summer time change
#define DST_NOW true
// If defined show mmm/dd/yyyy, else dd/mm/yyyy
#define US_DATE_ORDER
// If defined use am/pm, else 24 hour time
#define CLOCK_12_HOUR
// Define one or both of these to include local time or UTC
//#define LOCAL_TIME_PAGE
#define UTC_TIME_PAGE

// Setup
const int UPDATE_INTERVAL_SECS = 20 * 60; // Update every 20 minutes
#define DST_SHIFT -3600
int UtcOffset = TZ;
int dst_offset = DST_NOW ? DST_SHIFT : 0;

// Display Settings
#if defined PCD8544
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
const int RST_PIN = D2; // Values for Wemos D1 mini
const int DC_PIN = D1;
const int CE_PIN = D8;
#else
const int RST_PIN = 4;  // These are native gpio pin #s - adjust for specific boards if needed
const int DC_PIN = 5;
const int CE_PIN = 15;
#endif
#else
const int I2C_DISPLAY_ADDRESS = 0x3c;
#if defined(ARDUINO_ESP8266_WEMOS_D1MINI)
const int SDA_PIN = D2; // Values for Wemos D1 mini
const int SDC_PIN = D1; // Could also be D5
#elif defined(ARDUINO_ESP8266_OAK)
const int SDA_PIN = P0;   // Values for Digistump Oak
const int SDC_PIN = P2;
#else
const int SDA_PIN = 2; //SDA
const int SDC_PIN = 0; //SCL
#endif
#endif

// const int RX_PIN = 3;

// OpenWeatherMap Settings
// Sign up here to get an API key:
// https://docs.thingpulse.com/how-tos/openweathermap-key/
String OPEN_WEATHER_MAP_APP_ID = "e89fe50a294daafa3193ba20139f2166";
/*
Go to https://openweathermap.org/find?q= and search for a location. Go through the
result set and select the entry closest to the actual location you want to display 
data for. It'll be a URL like https://openweathermap.org/city/2657896. The number
at the end is what you assign to the constant below.
 */
String OPEN_WEATHER_MAP_LOCATION_ID = "4164632";

// Pick a language code from this list:
// Arabic - ar, Bulgarian - bg, Catalan - ca, Czech - cz, German - de, Greek - el,
// English - en, Persian (Farsi) - fa, Finnish - fi, French - fr, Galician - gl,
// Croatian - hr, Hungarian - hu, Italian - it, Japanese - ja, Korean - kr,
// Latvian - la, Lithuanian - lt, Macedonian - mk, Dutch - nl, Polish - pl,
// Portuguese - pt, Romanian - ro, Russian - ru, Swedish - se, Slovak - sk,
// Slovenian - sl, Spanish - es, Turkish - tr, Ukrainian - ua, Vietnamese - vi,
// Chinese Simplified - zh_cn, Chinese Traditional - zh_tw.
String OPEN_WEATHER_MAP_LANGUAGE = "en";
const uint8_t MAX_FORECASTS = 4;

boolean IS_METRIC = false;

// Adjust according to your language
const String WDAY_NAMES[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
const String MONTH_NAMES[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

/***************************
 * End Settings
 **************************/
 
 // Initialize the display
#ifdef PCD8544
PCD8544Spi  display(RST_PIN, DC_PIN, CE_PIN);
#elif defined(SSD1306)
SSD1306Wire     display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
#else
SH1106Wire  display(I2C_DISPLAY_ADDRESS, SDA_PIN, SDC_PIN);
#endif
OLEDDisplayUi   ui( &display );

OpenWeatherMapCurrentData currentWeather;
OpenWeatherMapCurrent currentWeatherClient;

OpenWeatherMapForecastData forecasts[MAX_FORECASTS];
OpenWeatherMapForecast forecastClient;

time_t now;

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

long timeSinceLastWUpdate = 0;

//declaring prototypes
void drawProgress(OLEDDisplay *display, int percentage, String label);
void updateData(OLEDDisplay *display);
void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex);
void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state);
void setReadyForWeatherUpdate();
#ifdef UTC_TIME_PAGE
void drawUTCDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
#endif
#ifdef LOCAL_TIME_PAGE
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y);
#endif

// Add frames
// this array keeps function pointers to all frames
// frames are the single views that slide from right to left
#if defined(LOCAL_TIME_PAGE) && defined(UTC_TIME_PAGE)
FrameCallback frames[] = { drawUTCDateTime, drawDateTime, drawCurrentWeather, drawForecast };
int numberOfFrames = 4;
#elif defined(LOCAL_TIME_PAGE)
FrameCallback frames[] = { drawDateTime, drawCurrentWeather, drawForecast };
int numberOfFrames = 3;
#elif defined(UTC_TIME_PAGE)
FrameCallback frames[] = { drawUTCDateTime, drawCurrentWeather, drawForecast };
int numberOfFrames = 3;
#else
FrameCallback frames[] = { drawCurrentWeather, drawForecast };
int numberOfFrames = 2;
#endif

OverlayCallback overlays[] = { drawHeaderOverlay };
int numberOfOverlays = 1;

void writeSettings() {
  // Save decoded message to SPIFFS file for playback on power up.
  File f = SPIFFS.open(CONFIG, "w");
  if (!f) {
    Serial.println("File open failed!");
  } else {
    Serial.println("Saving settings now...");
    f.println("UtcOffset=" + String(UtcOffset));
    f.println("dst_offset=" + String(dst_offset));
    f.println("weatherKey=" + OPEN_WEATHER_MAP_APP_ID);
    f.println("CityID=" + OPEN_WEATHER_MAP_LOCATION_ID);
    f.println("isMetric=" + String(IS_METRIC));
    f.println("language=" + OPEN_WEATHER_MAP_LANGUAGE);
  }
  f.close();
  readSettings();
  configTime(UtcOffset*3600, dst_offset, "pool.ntp.org");
}

void readSettings() {
  if (SPIFFS.exists(CONFIG) == false) {
    Serial.println("Settings File does not yet exists.");
    writeSettings();
    return;
  }
  File fr = SPIFFS.open(CONFIG, "r");
  String line;
  while(fr.available()) {
    line = fr.readStringUntil('\n');

    if (line.indexOf("UtcOffset=") >= 0) {
      UtcOffset = line.substring(line.lastIndexOf("UtcOffset=") + 10).toInt();
      Serial.println("UtcOffset=" + String(UtcOffset));
    }
    if (line.indexOf("dst_offset=") >= 0) {
      dst_offset = line.substring(line.lastIndexOf("dst_offset=") + 11).toInt();
      Serial.println("dst_offset=" + String(dst_offset));
    }
    if (line.indexOf("weatherKey=") >= 0) {
      OPEN_WEATHER_MAP_APP_ID = line.substring(line.lastIndexOf("weatherKey=") + 11);
      OPEN_WEATHER_MAP_APP_ID.trim();
      Serial.println("WeatherApiKey=" + OPEN_WEATHER_MAP_APP_ID);
    }
    if (line.indexOf("CityID=") >= 0) {
      OPEN_WEATHER_MAP_LOCATION_ID = line.substring(line.lastIndexOf("CityID=") + 7);
      OPEN_WEATHER_MAP_LOCATION_ID.trim();
      Serial.println("CityID: " + OPEN_WEATHER_MAP_LOCATION_ID);
    }
    if (line.indexOf("isMetric=") >= 0) {
      IS_METRIC = line.substring(line.lastIndexOf("isMetric=") + 9).toInt();
      Serial.println("IS_METRIC=" + String(IS_METRIC));
    }
    if (line.indexOf("language=") >= 0) {
      OPEN_WEATHER_MAP_LANGUAGE = line.substring(line.lastIndexOf("language=") + 9);
      OPEN_WEATHER_MAP_LANGUAGE.trim();
      Serial.println("WeatherLanguage=" + OPEN_WEATHER_MAP_LANGUAGE);
    }
  }
  fr.close();
  configTime(UtcOffset*3600, dst_offset, "pool.ntp.org");
  timeSinceLastWUpdate = 0;
}

void configModeCallback (WiFiManager *myWiFiManager) {
  int16_t w = display.getWidth();
  int16_t h = display.getHeight();

  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_10);
  display.drawString(w/2, 0, "WxW Setup");
  display.drawString(w/2, 10, "Connect to AP");
  display.drawString(w/2, 20, myWiFiManager->getConfigPortalSSID());
  display.drawString(w/2, 30, "192.168.4.1");
  display.drawString(w/2, 40, "To configure");
  display.display();
  
  Serial.println("Weather Widget Setup");
  Serial.println("Please connect to AP");
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println("192.168.4.1");
  Serial.println("To setup WxW Configuration");
}

bool shouldSaveConfig = false;
void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  while(Serial.available()) {
    Serial.read();
  }
  SPIFFS.begin();
  delay(10);
  Serial.println();

  // initialize dispaly
  display.init();
  display.clear();
  display.display();

  int16_t w = display.getWidth();
  int16_t h = display.getHeight();

  //display.flipScreenVertically();
  
  display.setFont(ArialMT_Plain_10);
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setContrast(255);

  if (SPIFFS.exists(CONFIG))
    readSettings();

  // Instantiate WiFi Manager
  WiFiManager wifiManager;

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  String s_UtcOffset(UtcOffset);
  WiFiManagerParameter c_UtcOffset("UtcOffset", "UTC Offset", s_UtcOffset.c_str(), 3);
  wifiManager.addParameter(&c_UtcOffset);
  WiFiManagerParameter c_is_dst("is_dst", "DST (y/n)?", (dst_offset == 0 ? "n" : "y"), 1);
  wifiManager.addParameter(&c_is_dst);
  WiFiManagerParameter c_OPEN_WEATHER_MAP_APP_ID("OPEN_WEATHER_MAP_APP_ID", "APP ID", (const char *)OPEN_WEATHER_MAP_APP_ID.c_str(), 40);
  wifiManager.addParameter(&c_OPEN_WEATHER_MAP_APP_ID);
  WiFiManagerParameter c_OPEN_WEATHER_MAP_LOCATION_ID("OPEN_WEATHER_MAP_LOCATION_ID", "Location ID", (const char *)OPEN_WEATHER_MAP_LOCATION_ID.c_str(), 8);
  wifiManager.addParameter(&c_OPEN_WEATHER_MAP_LOCATION_ID);
  WiFiManagerParameter c_IS_METRIC("IS_METRIC", "Metric units (y/n)?", (IS_METRIC ? "y" : "n"), 1);
  wifiManager.addParameter(&c_IS_METRIC);
  WiFiManagerParameter c_OPEN_WEATHER_MAP_LANGUAGE("OPEN_WEATHER_MAP_LANGUAGE", "Language", (const char *)OPEN_WEATHER_MAP_LANGUAGE.c_str(), 2);
  wifiManager.addParameter(&c_OPEN_WEATHER_MAP_LANGUAGE);
    String hostname(HOSTNAME);
  hostname += String(ESP.getChipId(), HEX);
  if (!wifiManager.autoConnect((const char *)hostname.c_str())) {
    delay(3000);
    WiFi.disconnect(true);
    ESP.reset();
    delay(5000);
  }
  if(shouldSaveConfig) {
    UtcOffset = atoi(c_UtcOffset.getValue());
    Serial.println("Setup: WM got UtcOffset="+String(UtcOffset));
    String d_flag(c_is_dst.getValue());
    dst_offset = (d_flag == "y" | d_flag == "Y") ? DST_SHIFT : 0;
    Serial.println("Setup: WM got dst_offset="+String(dst_offset));
    OPEN_WEATHER_MAP_APP_ID = c_OPEN_WEATHER_MAP_APP_ID.getValue();
    Serial.println("Setup: WM got OPEN_WEATHER_MAP_APP_ID="+OPEN_WEATHER_MAP_APP_ID);
    OPEN_WEATHER_MAP_LOCATION_ID = c_OPEN_WEATHER_MAP_LOCATION_ID.getValue();
    Serial.println("Setup: WM got OPEN_WEATHER_MAP_LOCATION_ID="+OPEN_WEATHER_MAP_LOCATION_ID);
    String m_flag(c_IS_METRIC.getValue());
    IS_METRIC = (m_flag=="Y" | m_flag=="y") ? 1 : 0;
    Serial.println("Setup: WM got IS_METRIC="+String(IS_METRIC));
    OPEN_WEATHER_MAP_LANGUAGE = c_OPEN_WEATHER_MAP_LANGUAGE.getValue();
    Serial.println("Setup: WM got OPEN_WEATHER_MAP_LANGUAGE="+OPEN_WEATHER_MAP_LANGUAGE);

    writeSettings();
    shouldSaveConfig = false;
  }

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    display.clear();
    display.drawString(w/2, 10, "Connecting to WiFi");
    display.drawXbm(w/3+4, 30, 8, 8, counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    display.drawXbm(w/3+18, 30, 8, 8, counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    display.drawXbm(w/3+32, 30, 8, 8, counter % 3 == 2 ? activeSymbole : inactiveSymbole);
    display.display();

    counter++;
  }
  
  // Get time from network time service
  configTime(UtcOffset*3600, dst_offset, "pool.ntp.org");

  ui.setTargetFPS(30);

  ui.setActiveSymbol(activeSymbole);
  ui.setInactiveSymbol(inactiveSymbole);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_TOP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  ui.setFrames(frames, numberOfFrames);

  ui.setOverlays(overlays, numberOfOverlays);

  // Inital UI takes care of initalising the display too.
  ui.init();

  Serial.println("");

  updateData(&display);

//  pinMode(RX_PIN, INPUT);

}

void loop() {
/*
  if(!digitalRead(RX_PIN)) {
    delay(30);
    WiFi.disconnect(true);
    ESP.restart();
    delay(5000);
  }
*/

  if(Serial.available()) {
    if((char)Serial.read() == '$') {
      delay(30);
      WiFi.disconnect(true);
      ESP.restart();
      delay(5000);
    }
  }

  if (millis() - timeSinceLastWUpdate > (1000L*UPDATE_INTERVAL_SECS)) {
    setReadyForWeatherUpdate();
    timeSinceLastWUpdate = millis();
  }

  if (readyForWeatherUpdate && ui.getUiState()->frameState == FIXED) {
    updateData(&display);
  }

  int remainingTimeBudget = ui.update();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    delay(remainingTimeBudget);
  }


}

void drawProgress(OLEDDisplay *display, int percentage, String label) {
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();

  display->clear();
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(w/2, 10, label);
  display->drawProgressBar(2, 28, w-4, 10, percentage);
  display->display();
}

void updateData(OLEDDisplay *display) {
  drawProgress(display, 10, "Update time...");
  drawProgress(display, 30, "Update wx...");
  currentWeatherClient.setMetric(IS_METRIC);
  currentWeatherClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  currentWeatherClient.updateCurrentById(&currentWeather, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID);
  drawProgress(display, 50, "Update fcast...");
  forecastClient.setMetric(IS_METRIC);
  forecastClient.setLanguage(OPEN_WEATHER_MAP_LANGUAGE);
  uint8_t allowedHours[] = {12};
  forecastClient.setAllowedHours(allowedHours, sizeof(allowedHours));
  forecastClient.updateForecastsById(forecasts, OPEN_WEATHER_MAP_APP_ID, OPEN_WEATHER_MAP_LOCATION_ID, MAX_FORECASTS);

  readyForWeatherUpdate = false;
  drawProgress(display, 100, "Done...");
  delay(1000);
}

#ifdef UTC_TIME_PAGE
void drawUTCDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);  // get local time
  struct tm* timeInfo;
  timeInfo = gmtime(&now);  // convert to formatted UTC
  char buff[16];
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = WDAY_NAMES[timeInfo->tm_wday];

#ifdef US_DATE_ORDER
  sprintf_P(buff, PSTR("%s, %02d/%02d/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mon+1, timeInfo->tm_mday, timeInfo->tm_year + 1900);
#else
  sprintf_P(buff, PSTR("%s, %02d/%02d/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
#endif
  display->drawString(w/2 + x, 5 + y, String(buff));
  if(w < 90) {
    display->setFont(ArialMT_Plain_16);
  } else {
    display->setFont(ArialMT_Plain_24);
  }
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(w/2 + x, 15 + y, String(buff));

  display->setFont(ArialMT_Plain_10);
  if(h < 64) {
    display->drawString(w/2 + x, 29 + y, "UTC");
  } else {
    display->drawString(w/2 + x, 38 + y, "UTC");
  }

  display->setTextAlignment(TEXT_ALIGN_LEFT);
}
#endif

#ifdef LOCAL_TIME_PAGE
void drawDateTime(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  String date = WDAY_NAMES[timeInfo->tm_wday];

#ifdef US_DATE_ORDER
  sprintf_P(buff, PSTR("%s, %02d/%02d/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mon+1, timeInfo->tm_mday, timeInfo->tm_year + 1900);
#else
  sprintf_P(buff, PSTR("%s, %02d/%02d/%04d"), WDAY_NAMES[timeInfo->tm_wday].c_str(), timeInfo->tm_mday, timeInfo->tm_mon+1, timeInfo->tm_year + 1900);
#endif
  display->drawString(w/2 + x, 5 + y, String(buff));
  if(w < 90) {
    display->setFont(ArialMT_Plain_16);
  } else {
    display->setFont(ArialMT_Plain_24);
  }
#ifdef CLOCK_12_HOUR
  int adj_hour = timeInfo->tm_hour;
  String ampm("a");
  if(adj_hour > 12) {
    adj_hour -= 12;
    ampm = "p";
  } else if(adj_hour == 0) {
    adj_hour = 12;
  } else if(adj_hour == 12) {
    ampm = "p";
  }
  sprintf_P(buff, PSTR("%2d:%02d:%02d"), adj_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(w/2 + x, 15 + y, String(buff)+ampm);
#else
  sprintf_P(buff, PSTR("%02d:%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
  display->drawString(w/2 + x, 15 + y, String(buff));
#endif
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}
#endif

void drawCurrentWeather(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();

  if(w < 90) {
    display->setFont(Dialog_plain_8);
  } else {
    display->setFont(ArialMT_Plain_10);
  }
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(w/2 + x, h/2 + 6 + y, currentWeather.description);

  if(w < 90) {
    display->setFont(ArialMT_Plain_16);
  } else {
    display->setFont(ArialMT_Plain_24);
  }
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(w/2 - 4 + x, h/8 - 3 + y, temp);

  if(w < 90) {
    display->setFont(Meteocons_Plain_21);
  } else {
    display->setFont(Meteocons_Plain_36);
  }
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(w/4 + x, 0 + y, currentWeather.iconMeteoCon);
}


void drawForecast(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();

  drawForecastDetails(display, x, y, 0);
  drawForecastDetails(display, x + (w+1)/3, y, 1);
  drawForecastDetails(display, x + 2*(w+1)/3, y, 2);
}

void drawForecastDetails(OLEDDisplay *display, int x, int y, int dayIndex) {
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();
  time_t observationTimestamp = forecasts[dayIndex].observationTime;
  struct tm* timeInfo;

  timeInfo = localtime(&observationTimestamp);
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(x + (w-3)/6, y, WDAY_NAMES[timeInfo->tm_wday]);

  display->setFont(Meteocons_Plain_21);
  display->drawString(x + (w-3)/6, y + (h-3)/5, forecasts[dayIndex].iconMeteoCon);
  String temp = String(forecasts[dayIndex].temp, 0) + (IS_METRIC ? "°C" : "°F");
  if(w < 90) {
    display->setFont(Dialog_plain_8);
  } else {
    display->setFont(ArialMT_Plain_10);
  }
  display->drawString(x + (w-3)/6, y + (h-3)/5 + 22, temp);
  display->setTextAlignment(TEXT_ALIGN_LEFT);
}

void drawHeaderOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  int16_t w = display->getWidth();
  int16_t h = display->getHeight();
  now = time(nullptr);
  struct tm* timeInfo;
  timeInfo = localtime(&now);
  char buff[16];

  display->setColor(WHITE);
  if(w < 90) {
    display->setFont(Dialog_plain_8);
  } else {
    display->setFont(ArialMT_Plain_10);
  }
  display->setTextAlignment(TEXT_ALIGN_LEFT);

#if defined(CLOCK_12_HOUR)
  int adj_hour = timeInfo->tm_hour;
  String ampm("A");
  if(adj_hour > 12) {
    adj_hour -= 12;
    ampm = "P";
  } else if(adj_hour == 0) {
    adj_hour = 12;
  } else if(adj_hour == 12) {
    ampm = "p";
  }
  sprintf_P(buff, PSTR("%2d:%02d"), adj_hour, timeInfo->tm_min);
  display->drawString(0, h-10, String(buff)+ampm);
#else
  sprintf_P(buff, PSTR("%02d:%02d"), timeInfo->tm_hour, timeInfo->tm_min);
  display->drawString(0, h-10, String(buff));
#endif
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  String temp = String(currentWeather.temp, 1) + (IS_METRIC ? "°C" : "°F");
  display->drawString(w, h-10, temp);
  if(h > 50) {
    display->drawHorizontalLine(0, h-12, w);
  }
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
}
