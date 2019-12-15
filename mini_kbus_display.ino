#include <IbusTrx.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <Fonts/FreeSansOblique12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <SPI.h>

// SD1331 pins on nano
#define sclk 13
#define mosi 11
#define cs   10
#define rst  7
#define dc   8

#define DISPLAY_TEST false  // Set to true to cycle through values instead of reading the IBus
#define MPH true            // Set to true to display the current speed in MPH, false for KPH
#define FAHRENHEIT false    // Set to true to display the temperature in Farenheit, false for Celsius
#define ADJUSTVOLUME true   // Set to true to adjust the radio / stereo volume with the speed

// Color definitions RGB565
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
#define ORANGE          0xF860
#define DRKYELLOW       0xEFE0
#define DRKCYAN         0x041F

#define DEFAULT_COLOUR  ORANGE
#define BACK_COLOUR  BLACK

#define speed_y 0
#define speed_h 35
#define temp_y 45
#define temp_h 17

#define IKE_SPEED 0x18
#define IKE_TEMPERATURE 0x19
#define MFL_BUTTONS 0x32
#define MFL_RT 0x01

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
IbusTrx ibusTrx;

// character positions
byte speed_x[3] = {62, 32, 9};
byte speed_w[3] = {25, 25, 18};
byte temp_x[3] = {60, 44, 30};
byte temp_w[4] = {18, 18, 12};

// This is always in kph
int vol_up_speed[4] = {55, 72, 90, 105};
// The volume down speed is less than the volume up speed to avoid too many changes if your speed is close to one of the triggers.
int vol_down_speed[4] = {50, 67, 85, 100};
int current_speed_vol = 0;

int prev_speed_kph = 0;
int prev_speed_kph_colour = DEFAULT_COLOUR;
int prev_temp_colour = DEFAULT_COLOUR; 

char current_char, prev_char;
String prev_speed_kph_str = "***", prev_temp_str = "***";
String reset_speed_str = "###", reset_temp_str = "###";

long loop_timer_now;  //holds the current millis
long previous_millis;  //holds the previous millis
int test_speed = 0;
int test_temp = 0;

uint8_t volumeUp[5] = {
  M_MFL, // sender ID (steering wheel)
  0x04,  // length of the message payload (including destination ID and checksum)
  M_RAD, // destination ID (radio)
  MFL_BUTTONS, // the type of message
  0x11, // Up
  // don't worry about the checksum, the library automatically calculates it for you
};

uint8_t volumeDown[5] = {
  M_MFL, // sender ID (steering wheel)
  0x04,  // length of the message payload (including destination ID and checksum)
  M_RAD, // destination ID (radio)
  MFL_BUTTONS, // the type of message
  0x10,  // Down  
  // don't worry about the checksum, the library automatically calculates it for you
};

void setup() {
  display.begin(80000000); //80Mhz
  display.fillScreen(DEFAULT_COLOUR);
  delay(100);
  display.fillScreen(BLACK);
  display.setTextColor(DEFAULT_COLOUR);

  display.setFont(&FreeSansOblique12pt7b);
  // setfont - FreeSans print has 0 at BOTTOM of char
  display.setCursor(68, temp_y + temp_h);
  if (FAHRENHEIT) {
    display.print("°F");
  } else {
    display.print("°C");
  }
  displayTemperature("   ");

  if (MPH) {
    displaySpeed("mph");
  } else {
    displaySpeed("kph");
  }

  ibusTrx.begin(Serial); // begin listening on the first hardware serial port
  previous_millis = millis();
}

void loop() {
  if (DISPLAY_TEST) {
    loop_timer_now = millis();
    if ((loop_timer_now - previous_millis) > 250) {
      test_speed++;
      if (test_speed > 135) test_speed = 0;
      displaySpeed(test_speed);
      setSpeedVolume(test_speed);
      prev_speed_kph = test_speed;
      test_temp++;
      if (test_temp > 55) test_temp = -35;
      displayTemperature(test_temp);
      previous_millis = loop_timer_now; 
    }
  }
  if (!DISPLAY_TEST && ibusTrx.available()) { // if there's a message waiting, do something with it
    IbusMessage message = ibusTrx.readMessage(); // grab the message
    unsigned int sourceID = message.source(); // read the source id
    unsigned int destinationID = message.destination(); // read the destination id
    unsigned int length = message.length(); // read the length of the payload
    unsigned int payloadFirstByte = message.b(0); // read the first byte of the payload
    // do something with this message
    if (sourceID == M_IKE) {
      // this message was sent by the instrument cluster
      if (payloadFirstByte == IKE_SPEED && length > 3) {
        // data is kph/2
        int current_speed_kph = message.b(1)*2;
        displaySpeed(current_speed_kph);
        if (ADJUSTVOLUME) {
          setSpeedVolume(current_speed_kph);
        }
        prev_speed_kph = current_speed_kph;

      } else if (payloadFirstByte == IKE_TEMPERATURE && length > 3) {
        signed char current_temp_c = message.b(1);
        displayTemperature(current_temp_c);
      }
    } else if (sourceID == M_MFL && destinationID == M_TEL) {
      if (payloadFirstByte == MFL_RT && length > 2) {
        // first pos. code for RT buttom
        // ToDo: Do something if R/T button pushed ...
      }
    }
  }
}

int fahrenheit(int celsius) {
  return (celsius*2) + 30;
}

int mph(int kph){
  // mph is kph*5/8
  // The Mini's displayed speed is over by a few mph, so add a 4% "correction"
  // Multiply by 100 and add 50 then div 100 to round up when answer would be >= x.5
  // Use unsigned long in the maths to avoid overflow with the multiplication
  // even though we only want an int as output
  return ((kph * 104UL * 5 / 8) + 50)/100;
}

void setSpeedVolume(int current_speed) {
  if (current_speed != prev_speed_kph) {
    for (int i=0; i<3; i++){
      if (current_speed >= vol_up_speed[i] && prev_speed_kph < vol_up_speed[i]) {
        // target speed vol is i + 1;
        if ((i+1) > current_speed_vol) {
          ibusTrx.write(volumeUp);
          current_speed_vol++;
        }
      } else if (current_speed <= vol_down_speed[i] && prev_speed_kph > vol_down_speed[i]) {
        // target speed vol is i
        if (i < current_speed_vol
        ) {
          ibusTrx.write(volumeDown);
          current_speed_vol--;
        }
      }
    }
  }
}

void displaySpeed(int current_speed) {
  int colour = DEFAULT_COLOUR;
  if (current_speed > 85) {
    colour = YELLOW;
  }
  if (MPH) {
    current_speed = mph(current_speed);
  }
  displaySpeed(String(current_speed), colour);
}

void displaySpeed(String current_speed_str) {
  displaySpeed(current_speed_str, DEFAULT_COLOUR);
}

void displaySpeed(String current_speed_str, int colour) {
  if (colour != prev_speed_kph_colour) {
    prev_speed_kph_str = reset_speed_str; // force update of entire string when colour changes
    prev_speed_kph_colour = colour;
  }
  prev_speed_kph_str = displayDelta(current_speed_str, prev_speed_kph_str, 3, speed_x, speed_y, speed_w, speed_h, &FreeSansBold24pt7b, colour);
}

void displayTemperature(int current_temp) {
  int colour = DEFAULT_COLOUR;
  if (current_temp < -5) {
    colour = BLUE;
  } else if (current_temp < 2) {
    colour = DRKCYAN;
  } else if (current_temp > 30) {
    colour = YELLOW;
  }
  if (FAHRENHEIT) {
    current_temp = fahrenheit(current_temp);
  }
  displayTemperature(String(current_temp), colour);
}

void displayTemperature(String current_temp_str) {
  displayTemperature(current_temp_str, DEFAULT_COLOUR);
}

void displayTemperature(String current_temp_str, int colour) {
  if (colour != prev_temp_colour) {
    prev_temp_str = reset_temp_str; // force update of entire string when colour changes
    prev_temp_colour = colour;
  }
  prev_temp_str = displayDelta(current_temp_str, prev_temp_str, 3, temp_x, temp_y, temp_w, temp_h, &FreeSansOblique12pt7b, colour);
}

// To reduce flickering, only update a character when it changes
String displayDelta(String data_str, String prev_str, int num_char, byte x_arr[], byte y, byte w_arr[], byte h, const GFXfont* font, int colour) {
  int data_str_len = data_str.length();
  int prev_str_len = prev_str.length();

  for (int i=0; i<num_char; i++) {
    if (i < data_str_len) {
      current_char = data_str.charAt(data_str_len-i-1);
    } else {
      current_char = ' ';
    }
    if (i < prev_str_len) {
      prev_char = prev_str.charAt(prev_str_len-i-1);
    } else {
      prev_char = ' ';
    }
    if (current_char != prev_char) {
      if (prev_char > ' ') {
        //erase old digit
        display.fillRect(x_arr[i], y, w_arr[i], h+2, BLACK);
      }
      if (current_char > ' ') {
        display.setTextColor(colour);
        display.setFont(font);
        // setfont FreeSans print has 0 at BOTTOM of char
        display.setCursor(x_arr[i], y+h);
        display.print(current_char);
      }
    }
  }
  prev_str = data_str;
  return prev_str;
}
