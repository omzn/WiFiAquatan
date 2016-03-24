#include "Arduino.h"
#include <Wire.h>
#include "attiny_i2c.h"
#include "WiFiAquatan.h"

attiny_i2c::attiny_i2c(uint8_t address, uint8_t pin) {
  _address = address;
  _pin     = pin;
  //  Wire.begin();
}

uint8_t attiny_i2c::value() {
  uint8_t v;
  Wire.beginTransmission(_address);
  Wire.write(_pin);
  Wire.endTransmission();
  Wire.requestFrom(_address, 1); // request 1 bytes
  while (Wire.available()) {
    v = Wire.read();
#ifdef DEBUG
  Serial.print("i2c_val:");
  Serial.println(v);
#endif    
    _value = v;
    break;
  }
  return _value;
}

void attiny_i2c::value(uint8_t val) {
  uint8_t v;
  Wire.beginTransmission(_address);
  Wire.write(_pin);
  Wire.write(val);
  Wire.endTransmission();
  Wire.requestFrom(_address, 1); // request 1 bytes
  while (Wire.available()) {
    v = Wire.read();
    _value = v;
    break;
  }
}

void attiny_i2c::value(uint8_t val1,uint8_t val2) {
  uint8_t v;
  Wire.beginTransmission(_address);
  Wire.write(_pin);
  Wire.write(val1);
  Wire.write(val2);
  Wire.endTransmission();
  Wire.requestFrom(_address, 1); // request 1 bytes
  while (Wire.available()) {
    v = Wire.read();
    _value = v;
    break;
  }
}

void attiny_i2c::heartbeat() {
  Wire.requestFrom(_address, 1); // request 1 bytes
  while (Wire.available()) {
    Wire.read();
    break;
  }
}
