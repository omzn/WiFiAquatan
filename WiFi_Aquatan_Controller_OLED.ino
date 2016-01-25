#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <skRTClib.h>
#include <ArduinoJson.h>
#include <pgmspace.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "OLED_pattern.h"
#include "Webpages.h"

#include "hcsr04_i2c.h"
#include "bme280_i2c.h"
#include "attiny_i2c.h"

#define PIN_1WIRE 2
#define PIN_SDA 4
#define PIN_SCL 14
#define PIN_FAN 12
#define PIN_LED 13
#define PIN_BTN 0
#define PIN_RTC_INT 5

#define WEB_LED_ON  LOW
#define WEB_LED_OFF HIGH

#define NUM_PAGES 5

#define I2C_PING_ADDRESS       0x26
#define ATTINY85_LED_ADDRESS   0x27

#define ATTINY85_PIN_LED       0x00 // 
#define ATTINY85_PIN_DIM_LED   0x80 //
#define ATTINY85_PIN_FAN       0x01 // 

#define BME280_ADDRESS   0x76

#define EEPROM_SSID_ADDR    0
#define EEPROM_PASS_ADDR    32
#define EEPROM_MDNS_ADDR    96
// schedule:  0=flag 1=on_h 2=on_m 3=off_h 4=off_m
#define EEPROM_SCHEDULE_ADDR  128
// autofan: 0=flag 1=hi_l(LB) 2=hi_l(HB) 3=lo_l(LB) 4=lo_l(HB)
#define EEPROM_AUTOFAN_ADDR 133
#define EEPROM_TWITTER_TOKEN_ADDR 138
#define EEPROM_TWITTER_CONFIG_ADDR 170
#define EEPROM_WATER_LEVEL_ADDR  186
#define EEPROM_LAST_ADDR    188

double airtemperature_val = 0.0;
double temperature_val = 0.0;
double pressure_val = 0.0;
double humidity_val = 0.0;
int   level_val = 0;
int   led_val = 0;
int   fan_val = 0;

int use_schedule = 0;
int current_on_h;
int current_on_m;
int current_off_h;
int current_off_m;

int use_autofan = 0;
float current_hi_l;
float current_lo_l;

int current_lv_wa = 0;
int current_lv_em = 0;

int use_twitter = 0;
String stewgate_host = "stewgate-u.appspot.com";
String stewgate_token = "";

const char* apSSID   = "AQUAMON1";
boolean settingMode;
String ssidList;
String sitename = "aquamon";

volatile boolean rtcint;
int timer_count = 0;
int average_count = 0;

int log_wd = 0;
float wtemp_log[100];
float atemp_log[100];
float press_log[100];
float humid_log[100];

volatile uint8_t oled_page = 0;
volatile boolean oled_changed = false;

const IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
MDNSResponder mdns;
Adafruit_SSD1306 oled;
OneWire ds(PIN_1WIRE);
DallasTemperature ds18b20(&ds);

hcsr04_i2c i2cping(I2C_PING_ADDRESS);
bme280_i2c bme280(BME280_ADDRESS);
attiny_i2c led(ATTINY85_LED_ADDRESS, ATTINY85_PIN_DIM_LED);
attiny_i2c fan(ATTINY85_LED_ADDRESS, ATTINY85_PIN_FAN);

/***************************************************************
 * interrupt handlers
 ***************************************************************/

void RTCHandler() {
  rtcint = 1;
}

void BTNHandler() {
  detachInterrupt(PIN_BTN);
  delayMicroseconds(10000);
  oled_changed = true;
  oled_page++;
  oled_page %= NUM_PAGES;
  attachInterrupt(PIN_BTN, BTNHandler, FALLING);
}

/***************************************************************
 * Setup and loop
 ***************************************************************/

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, WEB_LED_OFF);
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);
  RTC.begin(16, 1, 1, 0, 0, 0, 0);

  ds18b20.begin();
  bme280.begin();

  rtcint = 0;
  pinMode(PIN_RTC_INT, INPUT_PULLUP);
  digitalWrite(PIN_RTC_INT, HIGH);
  attachInterrupt(PIN_RTC_INT, RTCHandler, FALLING);
  RTC.setTimer(RTC_TIMER_BASE_1S, 1); // Timer 1 Hz

  pinMode(PIN_BTN, INPUT_PULLUP);
  digitalWrite(PIN_BTN, HIGH);
  attachInterrupt(PIN_BTN, BTNHandler, FALLING);

  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);         // Initialize the OLED
  //  oled.clearDisplay();      // Clear the display's internal memory
  oled.display();       // Display what's in the buffer (splashscreen)
  oled.setTextSize(1);  // Set text size 1
  oled.setTextColor(WHITE, BLACK);
  delay(10);
  if (restoreConfig()) {
    if (checkConnection()) {
      if (mdns.begin(sitename.c_str(), WiFi.localIP())) {
        Serial.println("MDNS responder started.");
      }
      settingMode = false;

      ds18b20.requestTemperatures();
      temperature_val = ds18b20.getTempCByIndex(0);
      bme280.read_data();
      airtemperature_val = bme280.temperature();
      pressure_val = bme280.pressure();
      humidity_val = bme280.humidity();
      level_val = i2cping.distance();
      startWebServer_normal();
      return;
    }
  }
  settingMode = true;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID);
  dnsServer.start(53, "*", apIP);
  startWebServer_setting();
  Serial.print("Starting Access Point at \"");
  Serial.print(apSSID);
  Serial.println("\"");
}

void loop() {
  byte tm[7] ;
  char buf[25] ;

  if (settingMode) {
    dnsServer.processNextRequest();
  }
  webServer.handleClient();

  if (oled_changed == true) {
    if (!settingMode) {
      //    Serial.println(oled_changed);
      OLEDdrawPage();
      oled_changed = false;
      oled.display();       // Refresh the display
    }
  }

  if (rtcint == 1) {
    //    Serial.println(oled_changed);
    RTC.rTime(tm) ;
    RTC.cTime(tm, (byte *)buf) ;
    //Serial.println(RTC.getIntType()) ;
    oled.setCursor(6, 57); // Set cursor
    oled.setTextColor(WHITE, BLACK);
    oled.print(buf);

    timer_count++;
    //    if (!settingMode && timer_count >= 30) {
    if (!settingMode) {
      ds18b20.requestTemperatures();
      temperature_val = ds18b20.getTempCByIndex(0);
      bme280.read_data();
      airtemperature_val = bme280.temperature();
      pressure_val = bme280.pressure();
      humidity_val = bme280.humidity();
      level_val    = i2cping.distance();
      OLEDdrawPage();
    }
    oled.display();       // Refresh the display
    timer_count %= 900;

    if (timer_count == 1) {
      wtemp_log[log_wd] = temperature_val;
      atemp_log[log_wd] = airtemperature_val;
      press_log[log_wd] = pressure_val;
      humid_log[log_wd] = humidity_val;
      log_wd++;
      log_wd %= 96;
    }

    int hh = ((tm[2] & 0xF0) >> 4) * 10 + (tm[2] & 0x0F);
    int mm = ((tm[1] & 0xF0) >> 4) * 10 + (tm[1] & 0x0F);

    if (use_schedule) {
      if ((current_on_h < current_off_h) || (current_on_h == current_off_h && current_on_m <= current_off_m )) {
        if ((hh >= current_on_h && mm >= current_on_m) && (hh < current_off_h || hh == current_off_h && mm < current_off_m)) {
          if (led_val == 0) {
            Serial.println("Light turned on.");
            led_val = 255;
            led.value(led_val);
            if (use_twitter && !settingMode) {
              //post_tweet("%5BWiFi-aquatan%5D+%E7%85%A7%E6%98%8E%E3%82%92%E7%82%B9%E7%81%AF%E3%81%97%E3%81%BE%E3%81%97%E3%81%9F%EF%BC%8E");
            }
          }
        } else {
          if (led_val > 0) {
            Serial.println("Light turned off.");
            led_val = 0;
            led.value(led_val);
          }
        }
      } else if ((current_on_h > current_off_h) || (current_on_h == current_off_h && current_on_m >= current_off_m )) {
        if ((hh >= current_off_h && mm >= current_off_m) && (hh < current_on_h || hh == current_on_h && mm < current_on_m)) {
          if (led_val > 0) {
            Serial.println("Light turned off.");
            led_val = 0;
            led.value(led_val);
          }
        } else {
          if (led_val == 0) {
            Serial.println("Light turned on.");
            led_val = 255;
            led.value(led_val);
          }
        }
      }
    }

    if (use_autofan) {
      if (temperature_val >= current_hi_l && fan_val == 0) {
        fan_val = 255;
        fan.value(fan_val);
        Serial.println("Fan turned on.");
      } else if (temperature_val <= current_lo_l && fan_val > 0) {
        fan_val = 0;
        fan.value(fan_val);
        Serial.println("Fan turned off.");
      }
    }

    delay(10);
    rtcint = 0;
  }
  ESP.wdtFeed();
}

/***************************************************************
 * OLED functions
 ***************************************************************/

void OLEDdrawPage() {
  if (oled_page == 0) {
    if (oled_changed == true) {
      oled.clearDisplay();     // Clear the page
      oled.drawBitmap(96, 16, aquatan_logo, 32, 32, WHITE);
      oled.fillRect(0, 0, 127, 15, WHITE);
      oled.setCursor(26, 3);
      oled.setTextColor(BLACK, WHITE);
      oled.print("CURRENT STATUS");
      oled.setTextColor(WHITE, BLACK);
    }
    OLEDshowMeasure();
  } else if (oled_page == 1) {
    if (oled_changed == true) {
      oled.clearDisplay();     // Clear the page
      oled.fillRect(0, 0, 127, 15, WHITE);
      oled.setCursor(26, 3);
      oled.setTextColor(BLACK, WHITE);
      oled.print("WATER TEMP LOG");
      oled.setTextColor(WHITE, BLACK);
    }
    OLEDshowWTGraph();
  } else if (oled_page == 2) {
    if (oled_changed == true) {
      oled.clearDisplay();     // Clear the page
      oled.fillRect(0, 0, 127, 15, WHITE);
      oled.setCursor(32, 3);
      oled.setTextColor(BLACK, WHITE);
      oled.print("AIR TEMP LOG");
      oled.setTextColor(WHITE, BLACK);
    }
    OLEDshowATGraph();
  } else if (oled_page == 3) {
    if (oled_changed == true) {
      oled.clearDisplay();     // Clear the page
      oled.fillRect(0, 0, 127, 15, WHITE);
      oled.setCursor(16, 3);
      oled.setTextColor(BLACK, WHITE);
      oled.print("LED & FAN STATUS");
      oled.setTextColor(WHITE, BLACK);
      oled.setCursor(0, 0); // Set cursor to top-left
    }
    OLEDshowLedStatus();
    OLEDshowFanStatus();
  } else if (oled_page == 4) {
    if (oled_changed == true) {
      oled.clearDisplay();     // Clear the page
      oled.fillRect(0, 0, 127, 15, WHITE);
      oled.setCursor(30, 3);
      oled.setTextColor(BLACK, WHITE);
      oled.print("SERVER INFO");
      oled.setTextColor(WHITE, BLACK);
      oled.setCursor(0, 0); // Set cursor to top-left
    }
    OLEDshowServerInfo();
  }
}
/*    14          112
 * 25 |
 *    |
 * 55 |____________
 */
void OLEDshowWTGraph() {
  static int prev_log_wd = 0;
  if (prev_log_wd != log_wd) {
    oled.fillRect(14, 55, 112, 55, BLACK);
    prev_log_wd = log_wd;
  }
  oled.setCursor(0, 16);
  oled.print("W.Temp");
  oled.drawLine(14, 25, 14, 55, WHITE);
  oled.drawLine(14, 55, 112, 55, WHITE);
  oled.drawPixel(13, 51, WHITE);
  oled.drawPixel(13, 41, WHITE);
  oled.drawPixel(13, 31, WHITE);
  oled.setCursor(113, 47);
  oled.print("1d");
  oled.setCursor(0, 47);
  oled.print("10");
  oled.setCursor(0, 37);
  oled.print("20");
  oled.setCursor(0, 27);
  oled.print("30");
  for (int i = 0; i < 96; i++) {
    int temp = (int)wtemp_log[(i + log_wd) % 96];
    if (temp > 10 && temp < 40) {
      oled.drawPixel(16 + (95 - i), 51 + (10 - temp), WHITE);
    }
  }
}

void OLEDshowATGraph() {
  static int prev_log_wd = 0;
  if (prev_log_wd != log_wd) {
    oled.fillRect(14, 55, 112, 55, BLACK);
    prev_log_wd = log_wd;
  }
  oled.setCursor(0, 16);
  oled.print("A.Temp");
  oled.drawLine(14, 25, 14, 55, WHITE);
  oled.drawLine(14, 55, 112, 55, WHITE);
  oled.drawPixel(13, 51, WHITE);
  oled.drawPixel(13, 41, WHITE);
  oled.drawPixel(13, 31, WHITE);
  oled.setCursor(113, 47);
  oled.print("1d");
  oled.setCursor(0, 47);
  oled.print("10");
  oled.setCursor(0, 37);
  oled.print("20");
  oled.setCursor(0, 27);
  oled.print("30");
  for (int i = 0; i < 96; i++) {
    int temp = (int)atemp_log[(i + log_wd) % 96];
    if (temp > 10 && temp < 40) {
      oled.drawPixel(16 + (95 - i), 51 + (10 - temp), WHITE);
    }
  }
}
void OLEDshowMeasure() {
  oled.setCursor(0, 16);
  oled.print("A.Temp:");
  oled.print(airtemperature_val, 1);
  oled.println(" 'C");
  oled.print("W.Temp:");
  oled.print(temperature_val, 1);
  oled.println(" 'C");
  oled.print("Press: ");
  oled.print(pressure_val, 0);
  oled.println(" hPa");
  oled.print("Humid: ");
  oled.print(humidity_val, 1);
  oled.println(" %");
  oled.print("W.Lv:  ");
  char str[10];
  sprintf(str, "%3d cm", level_val);
  oled.print(str);
}

void OLEDshowLedStatus() {
  oled.setCursor(0, 16);
  oled.print("LED");
  if (led_val > 0) {
    oled.fillCircle(44, 28, 12, WHITE);
    oled.setCursor(39, 25);
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
    oled.print("ON");
    oled.setTextColor(WHITE, BLACK);
  } else {
    oled.drawCircle(44, 28, 11, WHITE);
    oled.drawCircle(44, 28, 12, WHITE);
    oled.setCursor(36, 25);
    oled.setTextColor(WHITE);
    oled.print("OFF");
    oled.setTextColor(WHITE, BLACK);
  }
  oled.setCursor(0, 40);
  oled.println("Schedule");
  if (!use_schedule) {
    oled.print("   none   ");
  } else {
    char buf[10];
    sprintf(buf, "%02d%02d-%02d%02d", current_on_h, current_on_m, current_off_h, current_off_m);
    oled.print(buf);
  }
}

void OLEDshowFanStatus() {
  oled.setCursor(64, 16);
  oled.print("FAN");
  if (fan_val > 0) {
    oled.fillCircle(108, 28, 12, WHITE);
    oled.setCursor(103, 25);
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
    oled.print("ON");
    oled.setTextColor(WHITE, BLACK);
  } else {
    oled.drawCircle(108, 28, 11, WHITE);
    oled.drawCircle(108, 28, 12, WHITE);
    oled.setCursor(100, 25);
    oled.setTextColor(WHITE);
    oled.print("OFF");
    oled.setTextColor(WHITE, BLACK);
  }
  oled.setCursor(64, 40);
  oled.println("Temp range");
  oled.setCursor(64, 48);
  if (!use_autofan) {
    oled.print("none      ");
  } else {
    oled.print(current_hi_l, 1);
    oled.print("-");
    oled.print(current_lo_l, 1);
  }
}

void OLEDshowServerInfo() {
  oled.setCursor(0, 16);
  oled.println("Site name:");
  oled.print(sitename);
  oled.println(".local");
  oled.print("IP: ");
  oled.println(WiFi.localIP());
}

/***************************************************************
 * EEPROM restoring functions
 ***************************************************************/

boolean restoreConfig() {
  Serial.println("Reading EEPROM...");
  String ssid = "";
  String pass = "";

  use_schedule  = EEPROM.read(EEPROM_SCHEDULE_ADDR) == 1 ? 1 : 0;
  current_on_h  = EEPROM.read(EEPROM_SCHEDULE_ADDR + 1);
  current_on_m  = EEPROM.read(EEPROM_SCHEDULE_ADDR + 2);
  current_off_h = EEPROM.read(EEPROM_SCHEDULE_ADDR + 3);
  current_off_m = EEPROM.read(EEPROM_SCHEDULE_ADDR + 4);

  use_autofan = EEPROM.read(EEPROM_AUTOFAN_ADDR) == 1 ? 1 : 0;
  uint32_t b1 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 1);
  uint32_t b2 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 2);
  current_hi_l  = (float)(b1 | b2 << 8) / 10.0;
  Serial.print("hi_l: ");
  Serial.println(current_hi_l);
  b1 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 3);
  b2 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 4);
  current_lo_l  = (float)(b1 | b2 << 8) / 10.0;
  Serial.print("lo_l: ");
  Serial.println(current_lo_l);

  current_lv_wa = EEPROM.read(EEPROM_WATER_LEVEL_ADDR);
  current_lv_em = EEPROM.read(EEPROM_WATER_LEVEL_ADDR + 1);

  use_twitter  = EEPROM.read(EEPROM_TWITTER_CONFIG_ADDR) == 1 ? 1 : 0;
  if (EEPROM.read(EEPROM_TWITTER_TOKEN_ADDR) != 0) {
    stewgate_token = "";
    for (int i = 0; i < 32; ++i) {
      byte c = EEPROM.read(EEPROM_TWITTER_TOKEN_ADDR + i);
      stewgate_token += char(c);
    }
    Serial.println("restored token");
  }

  if (EEPROM.read(EEPROM_SSID_ADDR) != 0) {
    for (int i = EEPROM_SSID_ADDR; i < EEPROM_SSID_ADDR + 32; ++i) {
      ssid += char(EEPROM.read(i));
    }
    Serial.print("SSID: ");
    Serial.println(ssid);
    for (int i = EEPROM_PASS_ADDR; i < EEPROM_PASS_ADDR + 64; ++i) {
      pass += char(EEPROM.read(i));
    }
    Serial.print("Password: ");
    Serial.println(pass);
    WiFi.begin(ssid.c_str(), pass.c_str());
    if (EEPROM.read(EEPROM_MDNS_ADDR) != 0) {
      sitename = "";
      for (int i = 0; i < 32; ++i) {
        byte c = EEPROM.read(EEPROM_MDNS_ADDR + i);
        if (c == 0) {
          break;
        }
        sitename += char(c);
      }
      Serial.println("restored sitename");
    }
    Serial.print("Sitename: ");
    Serial.println(sitename);

    return true;
  }
  else {
    Serial.println("Config not found.");
    return false;
  }
}

/***************************************************************
 * Network functions
 ***************************************************************/

boolean checkConnection() {
  int count = 0;
  Serial.print("Waiting for Wi-Fi connection");
  while ( count < 40 ) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println();
      Serial.println("Connected!");
      return (true);
    }
    delay(500);
    Serial.print(".");
    count++;
  }
  Serial.println("Timed out.");
  return false;
}

/***********************************************************
 * WiFi Client functions
 ***********************************************************/

bool post_tweet(String msg) {
  Serial.print("connecting to ");
  Serial.println(stewgate_host);

  WiFiClient client;
  if (!client.connect(stewgate_host.c_str(), 80)) {
    Serial.println("connection failed");
    return false;
  }

  client.println("POST /api/post/ HTTP/1.0");

  client.print("Host: ");
  client.println(stewgate_host);

  int msgLength = 40;
  msgLength += msg.length();
  client.print("Content-length:");
  client.println(msgLength);
  client.println("");

  client.print("_t=");
  client.print(stewgate_token);
  client.print("&msg=");
  client.println(msg);

  delay(10);

  while (client.available()) {
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");

  return true;
}

/***************************************************************
 * Web server functions
 ***************************************************************/

void startWebServer_setting() {
  oled.clearDisplay();     // Clear the page
  oled.fillRect(0, 0, 127, 15, WHITE);
  oled.setCursor(30, 4); // Set cursor
  oled.setTextColor(BLACK, WHITE);
  //  oled.setTextSize(2);
  oled.print("SETTING MODE");
  //  oled.setTextSize(1);
  oled.setTextColor(WHITE, BLACK);
  oled.setCursor(0, 20); // Set cursor
  oled.print("Connect SSID:");
  oled.println(apSSID);
  oled.drawBitmap(0, 32, aquatan_logo, 32, 32, WHITE);
  oled.display();       // Refresh the display
  delay(10);

  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.softAPIP());
  webServer.on("/setap", []() {
    // 時刻を設定
    RTC.sTime(webServer.arg("year").toInt() - 100,
              webServer.arg("mon").toInt(),
              webServer.arg("day").toInt(),
              webServer.arg("wday").toInt(),
              webServer.arg("hour").toInt(),
              webServer.arg("min").toInt(),
              webServer.arg("sec").toInt()
             );
    for (int i = 0; i < EEPROM_MDNS_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    String ssid = urlDecode(webServer.arg("ssid"));
    Serial.print("SSID: ");
    Serial.println(ssid);
    String pass = urlDecode(webServer.arg("pass"));
    Serial.print("Password: ");
    Serial.println(pass);
    String site = urlDecode(webServer.arg("site"));
    Serial.print("Sitename: ");
    Serial.println(site);
    Serial.println("Writing SSID to EEPROM...");
    for (int i = 0; i < ssid.length(); ++i) {
      EEPROM.write(EEPROM_SSID_ADDR + i, ssid[i]);
    }
    Serial.println("Writing Password to EEPROM...");
    for (int i = 0; i < pass.length(); ++i) {
      EEPROM.write(EEPROM_PASS_ADDR + i, pass[i]);
    }
    if (site != "") {
      for (int i = EEPROM_MDNS_ADDR; i < EEPROM_MDNS_ADDR + 32; ++i) {
        EEPROM.write(i, 0);
      }
      Serial.println("Writing Sitename to EEPROM...");
      for (int i = 0; i < site.length(); ++i) {
        EEPROM.write(EEPROM_MDNS_ADDR + i, site[i]);
      }
    }
    EEPROM.commit();
    Serial.println("Write EEPROM done!");
    String s = "<h2>Setup complete</h2> <p>Device will be connected to \"";
    s += ssid;
    s += "\" after the restart.</p><p>Your computer also need to re-connect to \"";
    s += ssid;
    s += "\".</p><p><button class=\"pure-button\" onclick=\"return quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return false;};setTimeout(\"quitBox()\",10000);</script>";
    webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
    //    delay(15000);
    //    ESP.restart();
  });
  webServer.on("/pure.css", handleCss);
  webServer.onNotFound([]() {
    digitalWrite(PIN_LED, WEB_LED_ON);
    int n = WiFi.scanNetworks();
    delay(100);
    ssidList = "";
    for (int i = 0; i < n; ++i) {
      ssidList += "<option value=\"";
      ssidList += WiFi.SSID(i);
      ssidList += "\">";
      ssidList += WiFi.SSID(i);
      ssidList += "</option>";
    }
    String s = R"=====(
<div class="l-content">
<div class="l-box">
<h3 class="if-head">WiFi Setting</h3>
<p>Please enter your password by selecting the SSID.<br />
You can specify site name for accessing a name like http://aquamonitor.local/</p>
<form class="pure-form pure-form-stacked" method="get" action="setap" name="tm"><label for="ssid">SSID: </label>
<select id="ssid" name="ssid">
)=====";
    s += ssidList;
    s += R"=====(
</select>
<label for="pass">Password: </label><input id="pass" name="pass" length=64 type="password">
<label for="site" >Site name: </label><input id="site" name="site" length=32 type="text" placeholder="Site name">
<input size="4" name="year" type="hidden"><input size="2" name="mon" type="hidden">
<input size="2" name="day" type="hidden"><input size="2" name="wday" type="hidden">
<input size="2" name="hour" type="hidden"><input size="2" name="min" type="hidden">
<input size="2" name="sec" type="hidden">
<button class="pure-button pure-button-primary" type="submit">Submit</button></form>
<script language="JavaScript"><!--
function rt(){ dt = new Date(); document.tm.year.value = dt.getYear();document.tm.mon.value = dt.getMonth() + 1; document.tm.day.value = dt.getDate(); document.tm.wday.value = dt.getDay(); document.tm.hour.value = dt.getHours(); document.tm.min.value = dt.getMinutes(); document.tm.sec.value = dt.getSeconds();}
// -->
</script><script language="JavaScript"><!-- 
setInterval("rt()",1000);
// -->
</script>
</div>
</div>
)=====";
  webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
  digitalWrite(PIN_LED, WEB_LED_OFF);
  });
  webServer.begin();
}


/*
 * Web server for normal operation
 */
void startWebServer_normal() {
  oled.clearDisplay();
  oled.drawBitmap(96,16,aquatan_logo,32,32,WHITE);
  oled.fillRect(0, 0, 127, 15, WHITE);
  oled.setCursor(26, 4);
  oled.setTextColor(BLACK,WHITE);
  oled.print("CURRENT STATUS");
  oled.setTextColor(WHITE,BLACK);
  OLEDdrawPage();
  oled.display();       // Refresh the display
  
  delay(10);
  Serial.print("Starting Web Server at ");
  Serial.println(WiFi.localIP());
  webServer.on("/reset", []() {
    for (int i = 0; i < EEPROM_LAST_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    String s = "<h3 class=\"if-head\">Reset ALL</h3><p>Cleared all settings. Please reset device.</p>";
    s += "<p><button class=\"pure-button\" onclick=\"return quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return false;};</script>";
    webServer.send(200, "text/html", makePage("Reset ALL Settings", s));
//    delay(10000);
//    ESP.restart();
  });
  webServer.on("/wifireset", []() {
    for (int i = 0; i < EEPROM_MDNS_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();
    String s = "<h3 class=\"if-head\">Reset WiFi</h3><p>Cleared WiFi settings. Please reset device.</p>";
    s += "<p><button class=\"pure-button\" onclick=\"return quitBox();\">Close</button></p>";
    s += "<script>function quitBox() { open(location, '_self').close();return false;}</script>";
    webServer.send(200, "text/html", makePage("Reset WiFi Settings", s));
//    delay(10000);
//    ESP.restart();
  });
  webServer.on("/", handleRoot);
  webServer.on("/pure.css", handleCss);
  webServer.on("/measure", handleMeasure);
  webServer.on("/config", handleConfig);
  webServer.on("/action",  handleAction);
  webServer.on("/schedule", handleSchedule);
  webServer.on("/autofan", handleAutofan);
  webServer.on("/wlevel", handleWaterLevel);
  webServer.on("/twitconf", handleTwitconf);
//  webServer.on("/j.js",    handleJS);
  webServer.begin();
}

void handleRoot() {
  Serial.println("got request for /");
  digitalWrite(PIN_LED, WEB_LED_ON);
  webServer.send_P(200, "text/html", page_p);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleCss() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  webServer.send_P(200, "text/css", css_p);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleMeasure() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["atemp"] = airtemperature_val;
  json["temp"] =  temperature_val;
  json["pressure"] = pressure_val;
  json["humidity"] = humidity_val;
  json["water_level"] = level_val;
  json["led"] = led_val;
  json["fan"] = fan_val;
  Serial.println("got request for measure.");
  json.printTo(message);
  webServer.send(200, "application/json", message);
    digitalWrite(PIN_LED, WEB_LED_OFF);

}

void handleConfig() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["use_schedule"] = use_schedule;
  json["on_h"] = current_on_h;
  json["on_m"] = current_on_m;
  json["off_h"] = current_off_h;
  json["off_m"] = current_off_m;
  json["use_autofan"] = use_autofan;
  json["hi_l"] = current_hi_l;
  json["lo_l"] = current_lo_l;
  json["lv_wa"] = current_lv_wa;
  json["lv_em"] = current_lv_em;
  json["use_twitter"] = use_twitter;
  json["stew_token"] = stewgate_token;

  Serial.println("got request for config.");
  json.printTo(message);
  webServer.send(200, "application/json", message);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleSchedule() {
    digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  String use_s = webServer.arg("use_schedule") ;
  int on_h  = webServer.arg("on_h").toInt();
  int on_m  = webServer.arg("on_m").toInt();
  int off_h = webServer.arg("off_h").toInt();
  int off_m = webServer.arg("off_m").toInt();

  Serial.print("use_s:");
  Serial.println(use_s);
  Serial.print("use_schedule:");
  Serial.println(use_schedule);
  Serial.print("on_h:");
  Serial.println(on_h);
  Serial.print("current_on_h:");
  Serial.println(current_on_h);
  
  int use_s_i;
  use_s_i = (use_s == "true" ? 1: 0);
  
  if (use_s_i != use_schedule) {
    use_schedule = use_s_i;    
    EEPROM.write(EEPROM_SCHEDULE_ADDR, char(use_schedule));
    EEPROM.commit();
  }

  if (on_h >= 0 && on_h <= 24 && on_m >= 0 && on_m <= 59 &&
      (on_h != current_on_h || on_m != current_on_m)) {
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 1, char(on_h));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 2, char(on_m));
    current_on_h = on_h;
    current_on_m = on_m;
    EEPROM.commit();
  }
  if (off_h >= 0 && off_h <= 24 && off_m >= 0 && off_m <= 59 &&
      (off_h != current_off_h || off_m != current_off_m) ) {
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 3, char(off_h));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 4, char(off_m));
    current_off_h = off_h;
    current_off_m = off_m;
    EEPROM.commit();
  }
  json["use_schedule"] = use_schedule;
  json["on_h"] = current_on_h;
  json["on_m"] = current_on_m;
  json["off_h"] = current_off_h;
  json["off_m"] = current_off_m;
  json.printTo(message);
  webServer.send(200, "application/json", message);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleAutofan() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  String arg_autofan = webServer.arg("use_autofan") ;
  float hi_l  = webServer.arg("hi_l").toFloat();
  float lo_l  = webServer.arg("lo_l").toFloat();

  Serial.print("arg_autofan:");
  Serial.println(arg_autofan);
  Serial.print("use_autofan:");
  Serial.println(use_autofan);
  Serial.print("hi_l:");
  Serial.println(hi_l);
  Serial.print("current_hi_l:");
  Serial.println(current_hi_l);  

  int use_t_i;
  use_t_i = (arg_autofan == "true" ? 1: 0);
  int16_t hi_l_i = (int)(hi_l * 10);
  int16_t lo_l_i = (int)(lo_l * 10);
  
  if (use_t_i != use_autofan) {
    use_autofan = use_t_i;    
    EEPROM.write(EEPROM_AUTOFAN_ADDR, char(use_autofan));
    EEPROM.commit();
  }

  if (hi_l != current_hi_l || lo_l != current_lo_l) {
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 1, (hi_l_i       & 0xFF));
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 2, (hi_l_i >> 8  & 0xFF));
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 3, (lo_l_i       & 0xFF));
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 4, (lo_l_i >> 8  & 0xFF));
    current_hi_l = hi_l;
    current_lo_l = lo_l;
    EEPROM.commit();
  }
  json["use_autofan"] = use_autofan;
  json["hi_l"] = current_hi_l;
  json["lo_l"] = current_lo_l;
  json.printTo(message);
  webServer.send(200, "application/json", message);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleWaterLevel() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  int8_t lv_wa  = webServer.arg("lv_wa").toInt();
  int8_t lv_em  = webServer.arg("lv_em").toInt();

  Serial.print("lv_wa:");
  Serial.println(lv_wa);
  Serial.print("lv_em:");
  Serial.println(lv_em);  

  if (lv_wa != current_lv_wa || lv_em != current_lv_em) {
    i2cping.set_levels(lv_wa,lv_em);
    EEPROM.write(EEPROM_WATER_LEVEL_ADDR + 0, lv_wa);
    EEPROM.write(EEPROM_WATER_LEVEL_ADDR + 1, lv_em);
    EEPROM.commit();

    current_lv_wa = lv_wa;
    current_lv_em = lv_em;
  }
  json["lv_wa"] = lv_wa;
  json["lv_em"] = lv_em;
  json.printTo(message);
  webServer.send(200, "application/json", message);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleTwitconf() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  String use_tw_s = webServer.arg("use_twitter") ;
  String token  = webServer.arg("stew_token");

  Serial.print("use_twitter:");
  Serial.println(use_tw_s);
  Serial.print("stew_token:");
  Serial.println(token);

  int use_tw_i;
  use_tw_i = (use_tw_s == "true" ? 1: 0);
  
  if (use_tw_i != use_twitter) {
    use_twitter = use_tw_i;    
    EEPROM.write(EEPROM_TWITTER_CONFIG_ADDR, char(use_twitter));
    EEPROM.commit();
  }

  if (token != stewgate_token) {
    stewgate_token=token;
    for (int i = 0; i < stewgate_token.length(); ++i) {
      EEPROM.write(EEPROM_TWITTER_TOKEN_ADDR + i, stewgate_token[i]);
    }
    EEPROM.commit();
  }
  json["use_twitter"] = use_twitter;
  json["stew_token"] = stewgate_token;
  json.printTo(message);
  webServer.send(200, "application/json", message);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

void handleAction() {
  digitalWrite(PIN_LED, WEB_LED_ON);
  String message;
  DynamicJsonBuffer jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  String ledstr = webServer.arg("led");
  String fanstr = webServer.arg("fan");

  if (ledstr != "") {
    if (ledstr == "on") {
      led_val = 255;
    } else if (ledstr == "off") {
      led_val = 0;
    } else if (ledstr.toInt() >= 0 && ledstr.toInt() <= 255) {
      led_val = ledstr.toInt();
    }
    led.value(led_val);
  }
  if (fanstr != "") {
    if (fanstr == "on") {
      fan_val = 255;
    } else if (fanstr == "off") {
      fan_val = 0;
    } else if (fanstr.toInt() >= 0 && fanstr.toInt() <= 255) {
      fan_val = fanstr.toInt();
    }
    fan.value(fan_val);
  }
  json["led"] = led_val;
  json["fan"] = fan_val;

  json.printTo(message);
  webServer.send(200, "application/json", message);
  digitalWrite(PIN_LED, WEB_LED_OFF);
}

String makePage(String title, String contents) {
  String s = R"=====(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<link rel="stylesheet" href="/pure.css">
)=====";  
  s += "<title>";
  s += title;
  s += "</title></head><body>";
  s += contents;
  s += R"=====(
<div class="footer l-box">
<p>WiFi Aquatan Monitor by @omzn 2015 / All rights researved</p>
</div>
)=====";
  s += "</body></html>";
  return s;
}

String urlDecode(String input) {
  String s = input;
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%22", "\"");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%25", "%");
  s.replace("%26", "&");
  s.replace("%27", "\'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.replace("%30", "*");
  s.replace("%31", "+");
  s.replace("%2C", ",");
  s.replace("%2E", ".");
  s.replace("%2F", "/");
  s.replace("%2C", ",");
  s.replace("%3A", ":");
  s.replace("%3A", ";");
  s.replace("%3C", "<");
  s.replace("%3D", "=");
  s.replace("%3E", ">");
  s.replace("%3F", "?");
  s.replace("%40", "@");
  s.replace("%5B", "[");
  s.replace("%5C", "\\");
  s.replace("%5D", "]");
  s.replace("%5E", "^");
  s.replace("%5F", "-");
  s.replace("%60", "`");
  return s;
}

