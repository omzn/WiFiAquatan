#include "OLEDScreen.h"

OLEDScreen::OLEDScreen(Sensors *s, ledLight *l, fanCooler *f) {
  _page = 0;
  _changed = true;
  _sensors = s;
  _light = l;
  _fan = f;
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
  if (changed()) {
    clearDisplay();
  }
  
  if (_page == 0) {
    drawHeader("CURRENT STATUS");
    drawLogo(96, 16);
    drawMeasure();
  } else if (_page == 1) {
    drawHeader("WATER TEMP LOG");
    drawWaterTempGraph();
  } else if (_page == 2) {
    drawHeader("AIR TEMP LOG");
#ifdef USE_BME280
    drawAirTempGraph();
#endif
  } else if (_page == 3) {
    drawHeader("LED & FAN STATUS");
    drawLedStatus();
    drawFanStatus();
  } else if (_page == 4) {
    drawHeader("NETWORK STATUS");
    drawServerInfo();
  }
  _changed = false;
}

void OLEDScreen::drawLogo(int x, int y) {
  drawBitmap(x, y, aquatan_logo, 32, 32, WHITE);
}

void OLEDScreen::drawHeader(String msg) {
  if (_changed == true) {
    fillRect(0, 0, 127, 15, WHITE);
    setCursor(64 - (msg.length() * 3), 3);
    setTextColor(BLACK, WHITE);
    print(msg);
    setTextColor(WHITE, BLACK);
  }
}

void OLEDScreen::drawClock() {
  char buf[25] ;
  const static char daysOfTheWeek[7][4] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  DateTime now;
  now = rtc.now();
  sprintf(buf,"%4d/%02d/%02d %s %02d:%02d",now.year(),now.month(),now.day(),daysOfTheWeek[now.dayOfTheWeek()],now.hour(),now.minute());
  setCursor(6, 57); // Set cursor
  setTextColor(WHITE, BLACK);
  print(buf);
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
  setCursor(0, 16);
  print("LED");
  if (_light->value() > 0) {
    fillCircle(44, 28, 12, WHITE);
    setCursor(39, 25);
    setTextColor(BLACK, WHITE); // 'inverted' text
    print("ON");
    setTextColor(WHITE, BLACK);
  } else {
    drawCircle(44, 28, 11, WHITE);
    drawCircle(44, 28, 12, WHITE);
    setCursor(36, 25);
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
  setCursor(64, 16);
  print("FAN");
  if (_fan->value() > 0) {
    fillCircle(108, 28, 12, WHITE);
    setCursor(103, 25);
    setTextColor(BLACK, WHITE); // 'inverted' text
    print("ON");
    setTextColor(WHITE, BLACK);
  } else {
    drawCircle(108, 28, 11, WHITE);
    drawCircle(108, 28, 12, WHITE);
    setCursor(100, 25);
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
  setCursor(0, 20);
  println("URL");
  print("http://");
  print(_sensors->siteName());
  println(".local/");
}


void OLEDScreen::drawWaterTempGraph() {
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



