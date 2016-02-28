#ifndef OLEDSCREEN_H
#define OLEDSCREEN_H

#include <Adafruit_SSD1306.h>
#include <RTClib.h>

#include "WiFiAquatan.h"
#include "OLED_pattern.h"
#include "Sensors.h"
#include "ledlight.h"
#include "fan.h"

#define NUM_PAGES (5)

class OLEDScreen : public Adafruit_SSD1306 {
  public:
    OLEDScreen(Sensors *s, ledLight *l, fanCooler *f);
    void setContrast(uint8_t c);
    void onDisplay();
    void offDisplay();
    void incPage();
    int  getPage();
    void drawPage();
    void drawHeader(String msg);
    void drawClock();
    void drawLogo(int x, int y);
    void drawMeasure();
    void drawLedStatus();
    void drawFanStatus();
    void drawServerInfo();
    void drawWaterTempGraph();
    void drawAirTempGraph();
    bool changed();
    void changed(bool v);

  private:
    Sensors   *_sensors;
    ledLight  *_light;
    fanCooler *_fan;
    int        _page;
    volatile bool       _changed;
#ifdef USE_RTC8564
    RTC_RTC8564 rtc;
#else
    RTC_DS1307 rtc;
#endif

};

#endif
