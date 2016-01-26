#include "ledlight.h"

ledLight::ledLight(uint8_t address, uint8_t pin) : attiny_i2c(address , pin ) {
  //  Wire.begin();
}

int ledLight::enabled() {
  return _use_schedule;
}

int ledLight::enableSchedule() {
  _use_schedule = 1;
}

int ledLight::disableSchedule() {
  _use_schedule = 0;
}

void ledLight::setSchedule(int on_h,int on_m, int off_h, int off_m) {
  _on_h = on_h;
  _on_m = on_m;
  _off_h = off_h;
  _off_m = off_m;
}

void ledLight::on_h(int v) {
  _on_h = v;
}
void ledLight::on_m(int v) {
  _on_m = v;
}
void ledLight::off_h(int v) {
  _off_h = v;
}
void ledLight::off_m(int v) {
  _off_m = v;
}
int ledLight::on_h() {
  return _on_h;
}
int ledLight::on_m() {
  return _on_m;
}
int ledLight::off_h() {
  return _off_h;
}
int ledLight::off_m() {
  return _off_m;
}

int ledLight::control(int hh, int mm) {
  if (_use_schedule) {
    if ((_on_h < _off_h) || (_on_h == _off_h && _on_m <= _off_m )) {
      if ((hh > _on_h || hh >= _on_h && mm >= _on_m) && (hh < _off_h || hh == _off_h && mm < _off_m)) {
        if (value() == 0) {
          Serial.println("Light turned on.");
          value(255);
        }
      } else {
        if (value() > 0) {
          Serial.println("Light turned off.");
          value(0);
        }
      }
    } else if ((_on_h > _off_h) || (_on_h == _off_h && _on_m >= _off_m )) {
      if ((hh > _off_h || hh >= _off_h && mm >= _off_m) && (hh < _on_h || hh == _on_h && mm < _on_m)) {
        if (value() > 0) {
          Serial.println("Light turned off.");
          value(0);
        }
      } else {
        if (value() == 0) {
          Serial.println("Light turned on.");
          value(255);
        }
      }
    }
  }
}

