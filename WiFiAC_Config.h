#ifndef WIFIAC_CONFIG_H
#define WIFIAC_CONFIG_H

#include "Arduino.h"
#include <EEPROM.h>

#define EEPROM_SSID_ADDR    0
#define EEPROM_PASS_ADDR    32
#define EEPROM_MDNS_ADDR    96
// schedule:  0=flag 1=on_h 2=on_m 3=off_h 4=off_m
#define EEPROM_SCHEDULE_ADDR  128
// threshold: 0=flag 1=hi_l(LB) 2=hi_l(HB) 3=lo_l(LB) 4=lo_l(HB)
#define EEPROM_THRESHOLD_ADDR 133
#define EEPROM_TWITTER_TOKEN_ADDR 138
#define EEPROM_TWITTER_CONFIG_ADDR 170
#define EEPROM_WATER_LEVEL_ADDR  186
#define EEPROM_LAST_ADDR    188

class WiFiAC_Config {
  public:
    WiFiAC_Config();
  private:
    int use_schedule;
    int schedule_on_h;
    int schedule_on_m;
    int schedule_off_h;
    int schedule_off_m;

    int use_autofan;
    float fan_hi_l;
    float fan_lo_l;

    int level_wa;
    int level_em;

    int use_stewgate;
    String stewgate_host = "stewgate-u.appspot.com";
    String stewgate_token = "";

    const char* apSSID   = "AQUAMON";
    boolean settingMode;
    String ssidList;
    String sitename = "aquamon";
};

#endif
