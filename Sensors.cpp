#include "Sensors.h"

Sensors::Sensors() {
  ds = new OneWire(PIN_1WIRE);
  ds18b20 = new DallasTemperature(ds);
  i2cping = new hcsr04_i2c(I2C_PING_ADDRESS);
#ifdef USE_BME280
  bme280  = new bme280_i2c(BME280_ADDRESS);
#endif
}

void Sensors::begin() {
  ds18b20->begin();
#ifdef USE_BME280
  bme280->begin();
#endif
}

void Sensors::readData() {

  ds18b20->requestTemperatures();
  waterTemp = ds18b20->getTempCByIndex(0);
  waterLevel = i2cping->distance();
#ifdef USE_BME280
  bme280->read_data();
  airTemp = bme280->temperature();
  pressure = bme280->pressure();
  humidity = bme280->humidity();
#endif
}

void Sensors::logData() {
  waterTempLog[log_wd] = waterTemp;
#ifdef USE_BME280
  airTempLog[log_wd] = airTemp;
  pressureLog[log_wd] = pressure;
  humidityLog[log_wd] = humidity;
#endif
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

void Sensors::waterLevelLimits(int w, int e) {
  _waterLevelLimitWarn = w;
  _waterLevelLimitEmerge = e;
  i2cping->set_levels(_waterLevelLimitWarn, _waterLevelLimitEmerge);
//  _setWaterLevel = true;
}

int Sensors::waterLevelLimitWarn() {
  return _waterLevelLimitWarn;
}

void Sensors::waterLevelLimitWarn(int v) {
  _waterLevelLimitWarn = v;
  i2cping->set_levels(_waterLevelLimitWarn, _waterLevelLimitEmerge);
  //_setWaterLevel = true;
}

int Sensors::waterLevelLimitEmerge() {
  return _waterLevelLimitEmerge;
}

void Sensors::waterLevelLimitEmerge(int v) {
  _waterLevelLimitEmerge = v;
  i2cping->set_levels(_waterLevelLimitWarn, _waterLevelLimitEmerge);
  //_setWaterLevel = true;
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

String Sensors::siteName() {
  return _sitename;
}
void Sensors::siteName(String s) {
  _sitename = s;
}

