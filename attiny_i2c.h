#ifndef ATTINY_I2C_H
#define ATTINY_I2C_H

#include "Arduino.h"
#include <Wire.h>

class attiny_i2c {
  public:
    attiny_i2c(uint8_t address, uint8_t pin);
    void    value(uint8_t val);
    void    value(uint8_t val1,uint8_t val2);
    uint8_t value();
    void    heartbeat();
  protected:
    int8_t _address;
    uint8_t _pin;
    uint8_t _value;
};

#endif
