#ifndef BME280_I2C_H
#define BME280_I2C_H

#include "Arduino.h"
#include <Wire.h>

class bme280_i2c {
  public:
    bme280_i2c(uint8_t address);
    void begin();
    void read_data();
    double temperature();
    double pressure();
    double humidity();
  private:
    int _address;
    unsigned long int hum_raw, temp_raw, pres_raw;
    double hum_cal, temp_cal, pres_cal;
    signed long int t_fine;

    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;
    int8_t  dig_H1;
    int16_t dig_H2;
    int8_t  dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t  dig_H6;

    void read_trim();
    void write_reg(uint8_t reg_address, uint8_t data);
    double _temperature();
    double _pressure();
    double _humidity();

};

#endif
