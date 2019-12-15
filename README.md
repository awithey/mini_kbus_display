# mini_kbus_display

This is code for a simple digital speedometer and temperature guage for a BMW Mini (it shoudl also work with some "E46" BMWs as well).

It is designed for an Arduino Nano and 7 pin SPI colour oled display (e.g. https://www.banggood.com/0_95-Inch-7pin-Full-Color-65K-Color-SSD1331-SPI-OLED-Display-For-Arduino-p-1068167.html).

If you use a different type of Arduino or display, you will need to update the SPI pin definitions (lines 10-14).

It expects a K-Bus (iBus) interface on the serial pins. I used a TH3112 (http://www.datasheetdir.com/TH3122+LIN)

You need to add the following Arduino Libraries to your IDE:
* https://www.arduinolibraries.info/libraries/ibus-trx
* https://www.arduinolibraries.info/libraries/adafruit-gfx-library
* https://www.arduinolibraries.info/libraries/adafruit-ssd1331-oled-driver-library-for-arduino

Notes:
The speed is in MPH. If you would like KPH, you will need to edit the code: change line 131 to `int current_speed = kph2*2;`
The temperature is in Celcius.

Testing the display.
If you set `bool testing = true;` (line 61), the display will continually cycle through the speed and temperature ranges. This allows you to test on a breadboard without having to connect it to a canbus.