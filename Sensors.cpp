#include "Sensors.h"

Sensors::Sensors() {
  ds = new OneWire(PIN_1WIRE);
  ds18b20 = new DallasTemperature(ds);
  i2cping = new hcsr04_i2c(I2C_PING_ADDRESS);
  bme280  = new bme280_i2c(BME280_ADDRESS);
}

void Sensors::begin() {
  ds18b20->begin();
  bme280->begin();
}

void Sensors::readData() {

  ds18b20->requestTemperatures();
  waterTemp = ds18b20->getTempCByIndex(0);
  bme280->read_data();
  airTemp = bme280->temperature();
  pressure = bme280->pressure();
  humidity = bme280->humidity();
  waterLevel = i2cping->distance();
}

void Sensors::logData() {
  waterTempLog[log_wd] = waterTemp;
  airTempLog[log_wd] = airTemp;
  pressureLog[log_wd] = pressure;
  humidityLog[log_wd] = humidity;
  log_wd++;
  log_wd %= 96;
}

float Sensors::getWaterTemp() {
  return waterTemp;
}

float Sensors::getAirTemp() {
  return airTemp;
}

float Sensors::getPressure() {
  return pressure;
}

float Sensors::getHumidity() {
  return humidity;
}

int Sensors::getWaterLevel() {
  return waterLevel;
}

void Sensors::waterLevelLimits(int w,int e) {
  _waterLevelLimitWarn = w;
  _waterLevelLimitEmerge = e;  
  i2cping->set_levels(w,e);
}

int Sensors::waterLevelLimitWarn() {
  return _waterLevelLimitWarn;
}

void Sensors::waterLevelLimitWarn(int v) {
  _waterLevelLimitWarn = v;
}

int Sensors::waterLevelLimitEmerge() {
  return _waterLevelLimitEmerge;
}

void Sensors::waterLevelLimitEmerge(int v) {
  _waterLevelLimitEmerge = v;
}


// get...Log() method returns most recent log data as index 0
float Sensors::getWaterTempLog(int id) {
  return waterTempLog[(id + log_wd) % 96];
}

float Sensors::getAirTempLog(int id) {
  return airTempLog[(id + log_wd) % 96];
}

float Sensors::getPressureLog(int id) {
  return pressureLog[(id + log_wd) % 96];
}

float Sensors::getHumidityLog(int id) {
  return humidityLog[(id + log_wd) % 96];
}

