# mini_kbus_display

This is code for a simple digital speedometer and temperature guage for a BMW Mini (it will also work with "E46" 3 Series BMWs as well).

It is designed for an Arduino Nano and 7 pin SPI colour oled display (e.g. https://www.banggood.com/0_95-Inch-7pin-Full-Color-65K-Color-SSD1331-SPI-OLED-Display-For-Arduino-p-1068167.html).

If you use a different type of Arduino or display, you will need to update the SPI pin definitions (lines 10-14).

It expects a K-Bus (iBus) interface on the serial pins. I used a TH3112 (http://www.datasheetdir.com/TH3122+LIN)

You need to add the following Arduino Libraries to your IDE:
* https://www.arduinolibraries.info/libraries/ibus-trx
* https://www.arduinolibraries.info/libraries/adafruit-gfx-library
* https://www.arduinolibraries.info/libraries/adafruit-ssd1331-oled-driver-library-for-arduino

Notes:
* The speed is in MPH. If you would like KPH, you will need to edit the code, set: `#define MPH false`
* The temperature is in Celcius. For Fahrenheit, set: `#define FAHRENHEIT true`
* The radio / stereo volume will be adjusted with your speed. (I have an aftermarket head-unit). If you don't want this, set: `#define ADJUSTVOLUME false`

### Testing the display.
If you set `#define DISPLAY_TEST true` (line 15), the display will continually cycle through the speed and temperature ranges. This allows you to test on a breadboard without having to connect it to a canbus.

### 3D Printed case and mount
see: https://www.thingiverse.com/thing:4050115

