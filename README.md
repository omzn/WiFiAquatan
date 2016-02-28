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




How to use
-----------
1. Modify the #define in WiFiAquatan.h.
If you do not use BME280, comment out #define USE_BME280.
2. Compile and upload to your WiFi aquatan.
3. Connect your computer to the WiFi access point "WIFI_AQUATAN".
4. A dialog will come up, and select and configure your WiFi access point.
5. Reboot device. (Left SW)
6. Current status of the tank will come up.
7. Right SW toggles screens. You will find URL for WiFi aquatan in the last screen.
8. Access the URL with your browser.
9. Click show settings.
10. Set lighting schedule, automatic fan control, and water level of warnings.
11. (Sometimes wifi aquatan freezes after setting. Then press reset SW (left SW).)

