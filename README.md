
WiFiAquatan
=======================
ESP-WROOM-02 (ESP8266) based aquarium tank manager

* Water teperature sensor (DS18B20)
* Temperature/Pressure/Humidity sensor (BME280)
* Water level sensor (HC-SR04)
* RTC clock (DS1307)
   * Clock is automatically calibrated by NTP.
* 12V LED lights
* 12V cooling fan
* ESP-WROOM-02 and two ATtiny85



How to assemble
-----------


Prepare IDE
-----------
* http://ehbtj.com/electronics/esp8266-as-arduino
  * Now you just add `http://arduino.esp8266.com/stable/package_esp8266com_index.json` to `Preferences > Additional Boards Manager URLs`.
* https://www.mgo-tec.com/spiffs-filesystem-uploader01-html
* set Flash size as 1M (256K SPIFFS)



Install following libraries in "Library Manager" (`Sketch > Include Library > Manage libraries...`).

* Adafruit GFX Library
* Adafruit SSD1306
* ArduinoJson
* RTClib
* OneWire
* DallasTemperature



How to use
-----------
1. Modify the #define in WiFiAquatan.h.
   If you do not use BME280, comment out `#define USE_BME280`.
   If you do not use feeder and pump function, comment out `#define "feeder.h"` and `#define "pump.h"`.
2. Open `Adafruit_SSD1306.h` (in `Arduino/libraries/Adafruit_SSD1306` directory). Comment out `#define SSD1306_128_32` and uncomment `#define SSD1306_128_64`.
3. Make your WiFi aquatan writable. Hold the right SW down and then press the left SW(reset switch).
4. Compile and upload to your WiFi aquatan. 
5. Upload html contents to WiFi aquatan using "ESP8266 Sketch data upload"
6. Connect your computer to the WiFi access point "WIFI_AQUATAN".
7. A dialog will come up, and select and configure your WiFi access point.
8. Reboot device. (Left SW)
9. Current status of the tank will come up.
10. Right SW toggles screens. You will find URL for WiFi aquatan in the last screen.
11. Access the URL with your browser.
12. Click show settings.
13. Set lighting schedule, automatic fan control, and water level of warnings.
14. (Sometimes wifi aquatan freezes after setting. Then press reset SW (left SW).)

