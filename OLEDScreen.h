#ifndef OLEDSCREEN_H
#define OLEDSCREEN_H

#include <Adafruit_SSD1306.h>
#include <skRTClib.h>
#include "OLED_pattern.h"
#include "Sensors.h"

#define NUM_PAGES (5)

class OLEDScreen : public Adafruit_SSD1306 {
  public:
    OLEDScreen(Sensors *s);

    void incPage();
    int  getPage();
    void drawPage(int page_num);
    void drawHeader(String msg);
    void drawClock();
    void drawLogo(int x, int y);
    void drawMeasure();
    
  private:
    Sensors *sensors;
    int page;
    bool changed;
};

#endif
