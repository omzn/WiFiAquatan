#ifndef LEDLIGHT_H
#define LEDLIGHT_H

#include "Arduino.h"
#include "attiny_i2c.h"

class ledLight : public attiny_i2c {
  public:
    ledLight(uint8_t address,uint8_t pin);

    int enabled();
    int enableSchedule();
    int disableSchedule();
    void setSchedule(int on_h, int on_m, int off_h, int off_m);
    int control(int hh, int mm);
    void dim_on(int interval);
    void dim_off(int interval);
    uint8_t dim();
    uint8_t duration();
    int on_h();
    int on_m();
    int off_h();
    int off_m();
    void on_h(int v);
    void on_m(int v);
    void off_h(int v);
    void off_m(int v);
    void enableDim();
    void enableDim(int dur);
    void disableDim();
  protected:
    uint8_t _use_schedule;
    uint8_t _state; 
    uint8_t _slow;
    uint8_t _dim;
    uint8_t _duration = 10;
    int _value; 
    int _on_h;
    int _on_m;
    int _off_h;
    int _off_m;
    uint8_t _reset_flag;
};

#endif
