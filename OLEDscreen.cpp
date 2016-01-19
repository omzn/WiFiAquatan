#include "Arduino.h"
#include "OLEDscreen.h"

OLEDscreen::OLEDscreen()
{
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);         // Initialize the OLED
  //  oled.clearDisplay();      // Clear the display's internal memory
  oled.display();       // Display what's in the buffer (splashscreen)
  oled.setTextSize(1);  // Set text size 1
  oled.setTextColor(WHITE, BLACK);
}

