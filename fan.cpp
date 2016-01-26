#include "fan.h"

fanCooler::fanCooler(uint8_t address, uint8_t pin) : attiny_i2c(address , pin ) {
  //  Wire.begin();
}

int fanCooler::enabled() {
  return _use_autofan;
}

int fanCooler::enableAutoFan() {
  _use_autofan = 1;
}

int fanCooler::disableAutoFan() {
  _use_autofan = 0;
}

void fanCooler::setLimits(float high, float low) {
  _high_limit = high;
  _low_limit = low;
}

void fanCooler::highLimit(float v) {
  _high_limit = v;
}
void fanCooler::lowLimit(float v) {
  _low_limit = v;
}
float fanCooler::highLimit() {
  return _high_limit;
}
float fanCooler::lowLimit() {
  return _low_limit;
}

void fanCooler::control(float t) {
  if (_use_autofan) {
    if (t >= highLimit() && value() == 0) {
      value(255);
    } else if (t <= lowLimit() && value() > 0) {
      value(0);
    }
  }
}

