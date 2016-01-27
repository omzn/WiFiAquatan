#ifndef SENSORS_H
#define SENSORS_H

#include <OneWire.h>
#include <DallasTemperature.h>

#include "hcsr04_i2c.h"
#include "bme280_i2c.h"

#define I2C_PING_ADDRESS       0x26
#define BME280_ADDRESS   0x76
#define PIN_1WIRE 2

class Sensors {
  public:
    Sensors();
    void begin();
    void readData();
    float getWaterTemp();
    float getAirTemp();
    float getPressure();
    float getHumidity();
    int    getWaterLevel();
    void   logData();
    float  getWaterTempLog(int id);
    float  getAirTempLog(int id);
    float  getPressureLog(int id);
    float  getHumidityLog(int id);
    void waterLevelLimits(int w,int e);
    int waterLevelLimitWarn();
    int waterLevelLimitEmerge();
    void waterLevelLimitWarn(int v);
    void waterLevelLimitEmerge(int v);

  private:
    OneWire *ds;
    DallasTemperature *ds18b20;
    hcsr04_i2c *i2cping;
    bme280_i2c *bme280;

    float waterTemp = 0.0;
    float airTemp = 0.0;
    float pressure = 0.0;
    float humidity = 0.0;
    int waterLevel = 0;

    int _waterLevelLimitWarn = 0;
    int _waterLevelLimitEmerge = 0;

    int log_wd = 0;
    float waterTempLog[100];
    float airTempLog[100];
    float pressureLog[100];
    float humidityLog[100];
};

#endif
