#ifndef SENSORS_H
#define SENSORS_H

#include "WiFiAquatan.h"

#include <OneWire.h>
#include <DallasTemperature.h>

#include "hcsr04_i2c.h"
#ifdef USE_BME280
#include "bme280_i2c.h"
#endif

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
    int   getWaterLevel();
    void  logData();
    float getWaterTempLog(int id);
    float getAirTempLog(int id);
    float getPressureLog(int id);
    float getHumidityLog(int id);
    void waterLevelLimits(int w, int e);
    int waterLevelLimitWarn();
    int waterLevelLimitEmerge();
    void waterLevelLimitWarn(int v);
    void waterLevelLimitEmerge(int v);
    String siteName();
    void siteName(String s);

  private:
    OneWire *ds;
    DallasTemperature *ds18b20;
    hcsr04_i2c *i2cping;
#ifdef USE_BME280
    bme280_i2c *bme280;
#endif
    float   waterTemp = -273.0;
    float   airTemp = -273.0;
    float   pressure = 0.0;
    float   humidity = -1.0;
    int16_t waterLevel = -1;

    uint16_t _waterLevelLimitWarn = 0;
    uint16_t _waterLevelLimitEmerge = 0;

    uint8_t log_wd = 0;
    float   waterTempLog[100];
    float   airTempLog[100];
    float   pressureLog[100];
    float   humidityLog[100];

    String _sitename;
};

#endif
