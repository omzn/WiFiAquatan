#ifndef HCSR04_I2C_H
#define HCSR04_I2C_H

#include "Arduino.h"
#include <Wire.h>

class hcsr04_i2c {
  public:
    hcsr04_i2c(uint8_t address);
    void set_levels(uint8_t low_th, uint8_t high_th);
    int  distance();
  private:
    int _address;
};

#endif
