#ifndef SENSORS_H
#define SENSORS_H

#include <OneWire.h>
#include <DallasTemperature.h>

#include "hcsr04_i2c.h"
#include "bme280_i2c.h"
#include "attiny_i2c.h"

#define I2C_PING_ADDRESS       0x26
#define ATTINY85_LED_ADDRESS   0x27

#define ATTINY85_PIN_LED       0x00
#define ATTINY85_PIN_DIM_LED   0x80
#define ATTINY85_PIN_FAN       0x01

#define BME280_ADDRESS   0x76
#define PIN_1WIRE 2

class Sensors {
  public:
    Sensors();
    void readData();
    double getWaterTemp();
    double getAirTemp();
    double getPressure();
    double getHumidity();
    int    getWaterLevel();
    void   logData();
    
  private:
    OneWire *ds;
    DallasTemperature *ds18b20;

    hcsr04_i2c *i2cping;
    bme280_i2c *bme280;
    attiny_i2c *led;
    attiny_i2c *fan;

    double waterTemp = 0.0;
    double airTemp = 0.0;
    double pressure = 0.0;
    double humidity = 0.0;
    int waterLevel = 0;

    int log_wd = 0;
    float waterTempLog[100];
    float airTempLog[100];
    float pressureLog[100];
    float humidityLog[100];
};

#endif
