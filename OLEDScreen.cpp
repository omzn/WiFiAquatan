#include "OLEDScreen.h"

OLEDScreen::OLEDScreen(Sensors *s, ledLight *l, fanCooler *f) {
  page = 0;
  changed = true;
  sensors = s;
  light = l;
  fan = f;
}

void OLEDScreen::incPage() {
  page++;
  page %= NUM_PAGES;
  changed = true;
}

void OLEDScreen::drawPage(int page_num) {
  changed = false;
}

void OLEDScreen::drawLogo(int x, int y) {
  drawBitmap(x, y, aquatan_logo, 32, 32, WHITE);
}

void OLEDScreen::drawHeader(String msg) {
  fillRect(0, 0, 127, 15, WHITE);
  setCursor(127 - (msg.length() * 3), 3);
  setTextColor(BLACK, WHITE);
  print(msg);
  setTextColor(WHITE, BLACK);
}

void OLEDScreen::drawClock() {
  byte tm[7] ;
  char buf[25] ;

  RTC.rTime(tm) ;
  RTC.cTime(tm, (byte *)buf) ;
  //Serial.println(RTC.getIntType()) ;
  setCursor(6, 57); // Set cursor
  setTextColor(WHITE, BLACK);
  print(buf);
}

void OLEDScreen::drawMeasure() {
  char str[10];

  setCursor(0, 16);
  print("A.Temp:");
  print(sensors->getAirTemp(), 1);
  println(" 'C");
  print("W.Temp:");
  print(sensors->getWaterTemp(), 1);
  println(" 'C");
  print("Press: ");
  print(sensors->getPressure(), 0);
  println(" hPa");
  print("Humid: ");
  print(sensors->getHumidity(), 1);
  println(" %");
  print("W.Lv:  ");
  sprintf(str, "%3d cm", sensors->getWaterLevel());
  print(str);

}


