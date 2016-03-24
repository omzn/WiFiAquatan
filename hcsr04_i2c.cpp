#include "Arduino.h"
#include <Wire.h>
#include "hcsr04_i2c.h"

hcsr04_i2c::hcsr04_i2c(uint8_t address) {
  _address = address;
  //  Wire.begin();
}

int hcsr04_i2c::distance() {
  uint32_t c = 0;
  Wire.requestFrom(_address, 1); // request 1 bytes
  //  while (Wire.available() == 0);
  while (Wire.available()) {
    c = Wire.read();
    break;
  }
  //  Serial.print("Distance:");         // print the integer
  //  Serial.println(c);         // print the integer
  return c;
}

void hcsr04_i2c::set_levels(uint8_t low_th, uint8_t high_th) {
  Wire.beginTransmission(_address);
  Wire.write(low_th);
  Wire.write(high_th);
  Wire.endTransmission();
//  delay(100);
  Wire.requestFrom(_address, 1); // request 1 bytes
  while (Wire.available()) {
    Wire.read();
    break;
  }
  //  Serial.print("Set Levels: low=");         // print the integer
  //  Serial.print(low_th);         // print the integer
  //  Serial.print(" high=");         // print the integer
  //  Serial.println(high_th);         // print the integer
}


