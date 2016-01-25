#include "Sensors.h"

Sensors::Sensors() {
  ds = new OneWire(PIN_1WIRE);
  ds18b20 = new DallasTemperature(ds);
  led = new attiny_i2c(ATTINY85_LED_ADDRESS, ATTINY85_PIN_DIM_LED);
  fan = new attiny_i2c(ATTINY85_LED_ADDRESS, ATTINY85_PIN_FAN);
  i2cping = new hcsr04_i2c(I2C_PING_ADDRESS);
  bme280  = new bme280_i2c(BME280_ADDRESS);

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
  waterTem_Log[log_wd] = waterTemp;
  airTempLog[log_wd] = airTemp;
  pressureLog[log_wd] = pressure;
  humidityLog[log_wd] = humidity;
  log_wd++;
  log_wd %= 96;
}

double Sensors::getWaterTemp() {
  return waterTemp;
}

double Sensors::getAirTemp() {
  return airTemp;
}

double Sensors::getPressure() {
  return pressure;
}

double Sensors::getHumidity() {
  return humidity;
}

int Sensors::getWaterLevel() {
  return waterLevel;
}

