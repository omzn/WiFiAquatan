#include "OLEDScreen.h"

OLEDScreen::OLEDScreen(Sensors *s, ledLight *l, fanCooler *f) {
  _page = 0;
  _changed = true;
  _sensors = s;
  _light = l;
  _fan = f;
}

void OLEDScreen::setContrast(uint8_t c) {
  ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306_command(c);
}

void OLEDScreen::onDisplay() {
  ssd1306_command(0xAF);
}

void OLEDScreen::offDisplay() {
  ssd1306_command(0xAE);
}

void OLEDScreen::incPage() {
  _page++;
  _page %= NUM_PAGES;
  _changed = true;
}

bool OLEDScreen::changed() {
  return _changed;
}
void OLEDScreen::changed(bool v) {
  _changed = v;
}

void OLEDScreen::drawPage() {

  void (OLEDScreen::*page[])() = {
#ifdef USE_BME280
    &OLEDScreen::drawWaterTemp,
//    &OLEDScreen::drawWaterTempGraph,
    &OLEDScreen::drawAirTemp,
//    &OLEDScreen::drawAirTempGraph,
    &OLEDScreen::drawPressure,
    &OLEDScreen::drawHumidity,
    &OLEDScreen::drawWaterLevel,
    &OLEDScreen::drawLedFan,
    &OLEDScreen::drawServerInfo
#else
    &OLEDScreen::drawWaterTemp,
//    &OLEDScreen::drawWaterTempGraph,
    &OLEDScreen::drawWaterLevel,
    &OLEDScreen::drawLedFan,
    &OLEDScreen::drawServerInfo
#endif
  };

  if (changed()) {
    clearDisplay();
    changed(false);
  }
  (this->*page[_page])();
}

void OLEDScreen::drawLogo(int x, int y) {
  drawBitmap(x, y, aquatan_logo, 32, 32, WHITE);
}

void OLEDScreen::drawHeader(String msg) {
  //setFont();
  setFont(&FreeSansBold9pt7b);
  fillRect(0, 0, 127, 15, WHITE);
  setCursor(0, 13);
  setTextColor(BLACK, WHITE);
  print(msg);
  setTextColor(WHITE, BLACK);
  setFont();
}

void OLEDScreen::drawClock() {
  char buf[25] ;
  const static char daysOfTheWeek[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  DateTime now;
  now = rtc.now();
  sprintf(buf, "%4d/%02d/%02d %s %02d:%02d", now.year(), now.month(), now.day(), daysOfTheWeek[now.dayOfTheWeek()], now.hour(), now.minute());
  setFont();
  setCursor(6, 57); // Set cursor
  setTextColor(WHITE, BLACK);
  print(buf);
}

void OLEDScreen::drawWaterTemp() {
  drawHeader("Water Temp.");
  drawLogo(96, 16);
  setFont(&FreeSansBold12pt7b);
  fillRect(0, 16, 95, 40, BLACK);
  setCursor(0, 40);
  setTextColor(WHITE, BLACK);
  print(_sensors->getWaterTemp(), 1);
  setFont(&FreeSansBold9pt7b);
  println(" C");
}

void OLEDScreen::drawAirTemp() {
  drawHeader("Air Temp.");
  drawLogo(96, 16);
  setFont(&FreeSansBold12pt7b);
  setCursor(0, 40);
  setTextColor(WHITE, BLACK);
  fillRect(0, 16, 95, 40, BLACK);
  print(_sensors->getAirTemp(), 1);
  setFont(&FreeSansBold9pt7b);
  println("C");
}

void OLEDScreen::drawPressure() {
  drawHeader("Pressure");
  drawLogo(0, 16);
  setFont(&FreeSansBold12pt7b);
  fillRect(32, 16, 127, 40, BLACK);
  setCursor(32, 40);
  setTextColor(WHITE, BLACK);
  print(_sensors->getPressure(),0);
  setFont(&FreeSansBold9pt7b);
  println("hPa");
}

void OLEDScreen::drawHumidity() {
  drawHeader("Humidity");
  drawLogo(0, 16);
  setFont(&FreeSansBold12pt7b);
  fillRect(32, 16, 127, 40, BLACK);
  setCursor(32, 40);
  setTextColor(WHITE, BLACK);
  print(_sensors->getHumidity(), 1);
  setFont(&FreeSansBold9pt7b);
  println("%");
}

void OLEDScreen::drawLedFan() {
  drawHeader("LED & Fan");
  drawLedStatus();
  drawFanStatus();
}

void OLEDScreen::drawWaterLevel() {
  char str[10];
  drawLogo(96, 16);
  drawHeader("Water Lv.");
  setFont(&FreeSansBold12pt7b);
  fillRect(0, 16, 95, 40, BLACK);
  setCursor(0, 40);
  setTextColor(WHITE, BLACK);
  sprintf(str, "%3d", _sensors->getWaterLevel());
  print(str);
  setFont(&FreeSansBold9pt7b);
  println(" cm");
}

void OLEDScreen::drawMeasure() {
  char str[10];
  setCursor(0, 16);
#ifndef USE_BME280
  println();
#endif
  print("W.Temp:");
  print(_sensors->getWaterTemp(), 1);
  println(" 'C");
#ifndef USE_BME280
  println();
#endif
#ifdef USE_BME280
  print("A.Temp:");
  print(_sensors->getAirTemp(), 1);
  println(" 'C");
  print("Press: ");
  print(_sensors->getPressure(), 0);
  println(" hPa");
  print("Humid: ");
  print(_sensors->getHumidity(), 1);
  println(" %");
#endif
  print("W.Lv:  ");
  sprintf(str, "%3d cm", _sensors->getWaterLevel());
  print(str);

}

void OLEDScreen::drawLedStatus() {
  setFont(&FreeSansBold9pt7b);
  setCursor(0, 31);
  print("LED");
  setFont();
  if (_light->value() > 0) {
    fillCircle(48, 28, 12, WHITE);
    setCursor(43, 25);
    setTextColor(BLACK, WHITE); // 'inverted' text
    print("ON");
    setTextColor(WHITE, BLACK);
  } else {
    drawCircle(48, 28, 11, WHITE);
    drawCircle(48, 28, 12, WHITE);
    setCursor(40, 25);
    setTextColor(WHITE);
    print("OFF");
    setTextColor(WHITE, BLACK);
  }
  setCursor(0, 40);
  println("Schedule");
  if (!_light->enabled()) {
    print("   none   ");
  } else {
    char buf[10];
    sprintf(buf, "%02d%02d-%02d%02d", _light->on_h(), _light->on_m(), _light->off_h(), _light->off_m());
    print(buf);
  }
}

void OLEDScreen::drawFanStatus() {
  setFont(&FreeSansBold9pt7b);
  setCursor(64, 31);
  print("FAN");
  setFont();
  if (_fan->value() > 0) {
    fillCircle(112, 28, 12, WHITE);
    setCursor(107, 25);
    setTextColor(BLACK, WHITE); // 'inverted' text
    print("ON");
    setTextColor(WHITE, BLACK);
  } else {
    drawCircle(112, 28, 11, WHITE);
    drawCircle(112, 28, 12, WHITE);
    setCursor(104, 25);
    setTextColor(WHITE);
    print("OFF");
    setTextColor(WHITE, BLACK);
  }
  setCursor(64, 40);
  println("Temp range");
  setCursor(64, 48);
  if (!_fan->enabled()) {
    print("none      ");
  } else {
    print(_fan->lowLimit(), 1);
    print("-");
    print(_fan->highLimit(), 1);
  }
}


void OLEDScreen::drawServerInfo() {
  drawHeader("Network");
  setFont();
  setCursor(0, 16);
  println("URL");
  setFont();
  setCursor(8, 24);
  print(_sensors->siteName());
  println(".local");
  setCursor(8, 32);
  println(WiFi.localIP());
}


void OLEDScreen::drawWaterTempGraph() {
  drawHeader("Water Temp.");
  setFont();
  setCursor(0, 16);
  print("W.Temp");
  drawLine(14, 25, 14, 55, WHITE);
  drawLine(14, 55, 112, 55, WHITE);
  drawPixel(13, 51, WHITE);
  drawPixel(13, 41, WHITE);
  drawPixel(13, 31, WHITE);
  setCursor(113, 47);
  print("1d");
  setCursor(0, 47);
  print("10");
  setCursor(0, 37);
  print("20");
  setCursor(0, 27);
  print("30");
  for (int i = 0; i < 96; i++) {
    int temp = (int)_sensors->getWaterTempLog(i);
    if (temp > 10 && temp < 40) {
      drawPixel(16 + (95 - i), 51 + (10 - temp), WHITE);
    }
  }
}

void OLEDScreen::drawAirTempGraph() {
  drawHeader("Air Temp.");
  setFont();
  setCursor(0, 16);
  print("A.Temp");
  drawLine(14, 25, 14, 55, WHITE);
  drawLine(14, 55, 112, 55, WHITE);
  drawPixel(13, 51, WHITE);
  drawPixel(13, 41, WHITE);
  drawPixel(13, 31, WHITE);
  setCursor(113, 47);
  print("1d");
  setCursor(0, 47);
  print("10");
  setCursor(0, 37);
  print("20");
  setCursor(0, 27);
  print("30");
  for (int i = 0; i < 96; i++) {
    int temp = (int)_sensors->getAirTempLog(i);
    if (temp > 10 && temp < 40) {
      drawPixel(16 + (95 - i), 51 + (10 - temp), WHITE);
    }
  }
}


