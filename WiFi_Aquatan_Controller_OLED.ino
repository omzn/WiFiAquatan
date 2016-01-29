/*
 * WiFi Aquatan Controller (China OLED version) ***
 *   RTC:  RTC-8564NB
 *   Temp: DS18B20
 *   Env:  BME280
 *   OLED: 0.96" OLED panel
 *
 */

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
#include <ArduinoJson.h>
#include <pgmspace.h>
#include <skRTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "Webpages.h"
#include "sensors.h"
#include "ledlight.h"
#include "fan.h"
//#include "networkinfo.h"
#include "OLEDScreen.h"

#define PIN_1WIRE 2
#define PIN_SDA 4
#define PIN_SCL 14
#define PIN_FAN 12
#define PIN_LED 13
#define PIN_BTN 0
#define PIN_RTC_INT 5

#define WEB_LED_ON  LOW
#define WEB_LED_OFF HIGH

#define I2C_PING_ADDRESS       0x26
#define ATTINY85_LED_ADDRESS   0x27

#define ATTINY85_PIN_LED       0x00
#define ATTINY85_PIN_DIM_LED   0x80
#define ATTINY85_PIN_FAN       0x01

#define BME280_ADDRESS   0x76

#define EEPROM_SSID_ADDR    0
#define EEPROM_PASS_ADDR    32
#define EEPROM_MDNS_ADDR    96
#define EEPROM_SCHEDULE_ADDR  128
#define EEPROM_AUTOFAN_ADDR 133
#define EEPROM_TWITTER_TOKEN_ADDR 138
#define EEPROM_TWITTER_CONFIG_ADDR 170
#define EEPROM_WATER_LEVEL_ADDR  186
#define EEPROM_LAST_ADDR    188

#define DEFAULT_SITE_NAME "aquamon"

int use_twitter = 0;
String stewgate_host = "stewgate-u.appspot.com";
String stewgate_token = "";

boolean settingMode;

const char* apSSID   = "AQUAMON1";
String ssidList;

volatile boolean rtcint;
int timer_count = 0;

const IPAddress apIP(192, 168, 1, 1);
DNSServer dnsServer;
ESP8266WebServer webServer(80);
MDNSResponder mdns;

Sensors    sensors;
ledLight   light(ATTINY85_LED_ADDRESS, ATTINY85_PIN_DIM_LED);
fanCooler  fan(ATTINY85_LED_ADDRESS, ATTINY85_PIN_FAN);

OLEDScreen oled(&sensors, &light, &fan);

/*
 * interrupt handlers
 */

void RTCHandler() {
  rtcint = 1;
}

void BTNHandler() {
  detachInterrupt(PIN_BTN);
  delayMicroseconds(10000);
  oled.incPage();
  attachInterrupt(PIN_BTN, BTNHandler, FALLING);
}

/*
 * Setup and loop
 */

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, WEB_LED_OFF);
  pinMode(PIN_FAN, OUTPUT);
  digitalWrite(PIN_FAN, LOW);

  Wire.begin(PIN_SDA, PIN_SCL);

  sensors.begin();
  sensors.siteName(DEFAULT_SITE_NAME);
  
  RTC.begin(16, 1, 1, 0, 0, 0, 0);
  rtcint = 0;
  RTC.setTimer(RTC_TIMER_BASE_1S, 1); // Timer 1 Hz

  pinMode(PIN_RTC_INT, INPUT_PULLUP);
  digitalWrite(PIN_RTC_INT, HIGH);
  attachInterrupt(PIN_RTC_INT, RTCHandler, FALLING);

  pinMode(PIN_BTN, INPUT_PULLUP);
  digitalWrite(PIN_BTN, HIGH);
  attachInterrupt(PIN_BTN, BTNHandler, FALLING);

  oled.begin(SSD1306_SWITCHCAPVCC,0x3C);         // Initialize the OLED
  oled.display();       // Display what's in the buffer (splashscreen)
  oled.setTextSize(1);  // Set text size 1
  oled.setTextColor(WHITE, BLACK);
  delay(10);

  if (restoreConfig()) {
    if (checkConnection()) {
      WiFi.mode(WIFI_STA);
      if (mdns.begin(sensors.siteName().c_str(), WiFi.localIP())) {
        Serial.println("MDNS responder started.");
      }
      settingMode = false;

      sensors.readData();
      startWebServer_normal();
      return;
    } else {
      settingMode = true;
    }
  } else {
    settingMode = true;
  }

  if (settingMode == true) {
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
}

void loop() {
  if (settingMode) {
    dnsServer.processNextRequest();
  }
  webServer.handleClient();

  if (oled.changed()) {
    if (!settingMode) {
      oled.drawPage();
      oled.drawClock();
      oled.display();       // Refresh the display
    }
  }

  if (rtcint == 1) {
    oled.drawClock();
    timer_count++;
    if (!settingMode) {
      sensors.readData();
      oled.drawPage();
    }
    oled.display();       // Refresh the display
    timer_count %= 900;

    if (timer_count == 1) {
      sensors.logData();
    }

    byte tm[7] ;
    RTC.rTime(tm) ;
    int hh = ((tm[2] & 0xF0) >> 4) * 10 + (tm[2] & 0x0F);
    int mm = ((tm[1] & 0xF0) >> 4) * 10 + (tm[1] & 0x0F);

    light.control(hh, mm);
    fan.control(sensors.getWaterTemp());

    delay(10);
    rtcint = 0;
  }
  ESP.wdtFeed();
}

/***************************************************************
 * EEPROM restoring functions
 ***************************************************************/

boolean restoreConfig() {
  Serial.println("Reading EEPROM...");
  String ssid = "";
  String pass = "";

  // Initialize on first boot
  if (EEPROM.read(0) == 255) {
    for (int i = 0; i < EEPROM_LAST_ADDR; ++i) {
      EEPROM.write(i, 0);
    }
    EEPROM.commit();    
  }

  int use_schedule  = EEPROM.read(EEPROM_SCHEDULE_ADDR) == 1 ? 1 : 0;
  if (use_schedule == 1) {
    light.enableSchedule();
  } else {
    light.disableSchedule();
  }
  int e_on_h  = EEPROM.read(EEPROM_SCHEDULE_ADDR + 1);
  int e_on_m  = EEPROM.read(EEPROM_SCHEDULE_ADDR + 2);
  int e_off_h = EEPROM.read(EEPROM_SCHEDULE_ADDR + 3);
  int e_off_m = EEPROM.read(EEPROM_SCHEDULE_ADDR + 4);
  light.setSchedule(e_on_h, e_on_m, e_off_h, e_off_m);

  int use_autofan = EEPROM.read(EEPROM_AUTOFAN_ADDR) == 1 ? 1 : 0;
  if (use_autofan == 1) {
    fan.enableAutoFan();
  } else {
    fan.disableAutoFan();
  }
  uint32_t b1 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 1);
  uint32_t b2 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 2);
  fan.highLimit((float)(b1 | b2 << 8) / 10.0);

  b1 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 3);
  b2 = EEPROM.read(EEPROM_AUTOFAN_ADDR + 4);
  fan.lowLimit((float)(b1 | b2 << 8) / 10.0);

  int lv_wa = EEPROM.read(EEPROM_WATER_LEVEL_ADDR);
  int lv_em = EEPROM.read(EEPROM_WATER_LEVEL_ADDR + 1);
  sensors.waterLevelLimitWarn(lv_wa);
  sensors.waterLevelLimitEmerge(lv_em);

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
      String sitename = "";
      for (int i = 0; i < 32; ++i) {
        byte c = EEPROM.read(EEPROM_MDNS_ADDR + i);
        if (c == 0) {
          break;
        }
        sitename += char(c);
      }
      sensors.siteName(sitename);
      Serial.println("restored sitename");
    }

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
  oled.drawHeader("SETTING MODE");
  oled.setCursor(0, 20); // Set cursor
  oled.print("Connect SSID:");
  oled.println(apSSID);
  oled.drawLogo(0, 32);
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
  oled.drawPage();
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

  json["atemp"] = sensors.getAirTemp();
  json["temp"] =  sensors.getWaterTemp();
  json["pressure"] = sensors.getPressure();
  json["humidity"] = sensors.getHumidity();
  json["water_level"] = sensors.getWaterLevel();
  json["led"] = light.value();
  json["fan"] = fan.value();
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

  json["use_schedule"] = light.enabled();
  json["on_h"] = light.on_h();
  json["on_m"] = light.on_m();
  json["off_h"] = light.off_h();
  json["off_m"] = light.off_m();
  json["use_autofan"] = fan.enabled();
  json["hi_l"] = fan.highLimit();
  json["lo_l"] = fan.lowLimit();
  json["lv_wa"] = sensors.waterLevelLimitWarn();
  json["lv_em"] = sensors.waterLevelLimitEmerge();
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

  int use_s_i;
  use_s_i = (use_s == "true" ? 1: 0);
  
  if (use_s_i != light.enabled()) {
    if (use_s_i) {
      light.enableSchedule();
    } else {
      light.disableSchedule();
    }
    EEPROM.write(EEPROM_SCHEDULE_ADDR, char(use_s_i));
    EEPROM.commit();
  }

  if (on_h >= 0 && on_h <= 24 && on_m >= 0 && on_m <= 59 &&
      (on_h != light.on_h() || on_m != light.on_m())) {
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 1, char(on_h));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 2, char(on_m));
    light.on_h(on_h);
    light.on_m(on_m);
    EEPROM.commit();
  }
  if (off_h >= 0 && off_h <= 24 && off_m >= 0 && off_m <= 59 &&
      (off_h != light.off_h() || off_m != light.off_m()) ) {
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 3, char(off_h));
    EEPROM.write(EEPROM_SCHEDULE_ADDR + 4, char(off_m));
    light.off_h(off_h);
    light.off_m(off_m);
    EEPROM.commit();
  }
  json["use_schedule"] = light.enabled();
  json["on_h"] = light.on_h();
  json["on_m"] = light.on_m();
  json["off_h"] = light.off_h();
  json["off_m"] = light.off_m();
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

  int use_t_i;
  use_t_i = (arg_autofan == "true" ? 1: 0);
  int16_t hi_l_i = (int)(hi_l * 10);
  int16_t lo_l_i = (int)(lo_l * 10);
  
  if (use_t_i != fan.enabled()) {
    if (use_t_i) {
      fan.enableAutoFan();
    } else {
      fan.disableAutoFan();      
    }
    EEPROM.write(EEPROM_AUTOFAN_ADDR, char(use_t_i));
    EEPROM.commit();
  }

  if (hi_l != fan.highLimit() || lo_l != fan.lowLimit()) {
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 1, (hi_l_i       & 0xFF));
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 2, (hi_l_i >> 8  & 0xFF));
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 3, (lo_l_i       & 0xFF));
    EEPROM.write(EEPROM_AUTOFAN_ADDR + 4, (lo_l_i >> 8  & 0xFF));
    fan.highLimit(hi_l);
    fan.lowLimit(lo_l);
    EEPROM.commit();
  }
  json["use_autofan"] = fan.enabled();
  json["hi_l"] = fan.highLimit();
  json["lo_l"] = fan.lowLimit();
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

  if (lv_wa != sensors.waterLevelLimitWarn() || lv_em != sensors.waterLevelLimitEmerge()) {
    sensors.waterLevelLimits(lv_wa,lv_em);
    EEPROM.write(EEPROM_WATER_LEVEL_ADDR + 0, lv_wa);
    EEPROM.write(EEPROM_WATER_LEVEL_ADDR + 1, lv_em);
    EEPROM.commit();
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
      light.value(255);
    } else if (ledstr == "off") {
      light.value(0);
    } else if (ledstr.toInt() >= 0 && ledstr.toInt() <= 255) {
      light.value(ledstr.toInt());
    }
  }
  if (fanstr != "") {
    if (fanstr == "on") {
      fan.value( 255);
    } else if (fanstr == "off") {
      fan.value( 0);
    } else if (fanstr.toInt() >= 0 && fanstr.toInt() <= 255) {
      fan.value(fanstr.toInt());
    }
  }
  json["led"] = light.value();
  json["fan"] = fan.value();

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

