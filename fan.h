#ifndef FAN_H
#define FAN_H

#include "Arduino.h"
#include "attiny_i2c.h"

class fanCooler : public attiny_i2c {
  public:
    fanCooler(uint8_t address,uint8_t pin);

    int enabled();
    int enableAutoFan();
    int disableAutoFan();
    void setLimits(float high, float low);
    void control(float t);
    float highLimit();
    float lowLimit();
    void highLimit(float v);
    void lowLimit(float v);
  protected:
    int _use_autofan;
    float _high_limit;
    float _low_limit;
};

#endif
