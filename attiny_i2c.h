#ifndef ATTINY_I2C_H
#define ATTINY_I2C_H

#include "Arduino.h"
#include <Wire.h>

class attiny_i2c {
  public:
    attiny_i2c(uint8_t address,uint8_t pin);
    void    value(uint8_t val);
    uint8_t value();
   private:
    int _address;
    int _pin;
    int _value;
};

#endif
