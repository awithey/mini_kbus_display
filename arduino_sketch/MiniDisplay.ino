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

// Color definitions RGB565
#define BLACK           0x0000
#define BLUE            0x001F
#define RED             0xF800
#define GREEN           0x07E0
#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
#define ORANGE          0xF8E0
#define DRKYELLOW       0xEFE0
#define DRKCYAN         0x07EE

#define default_colour  ORANGE

#define speed_y 0
#define speed_h 35
#define temp_y 45
#define temp_h 17


Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
IbusTrx ibusTrx;

int16_t speed_x[3] = {62, 32, 9};
int16_t speed_w[3] = {25, 25, 18};
int16_t temp_x[4] = {55, 39, 25};
int16_t temp_w[4] = {18, 18, 12};
int vol_up_speed[3] = {35, 50, 65};
int vol_down_speed[3] = {30, 45, 60};
int prev_speed = 0;
int prev_speed_colour = default_colour;
int prev_temp_colour = default_colour; 
char current_char, prev_char;
String def_speed_str = "   ", def_temp_str = "   ";
String prev_speed_str = "***", prev_temp_str = "***";

uint8_t volumeUp[5] = {
  M_MFL, // sender ID (steering wheel)
  0x04,  // length of the message payload (including destination ID and checksum)
  M_RAD, // destination ID (radio)
  0x32, // the type of message
  11, // Up
  // don't worry about the checksum, the library automatically calculates it for you
};

uint8_t volumeDown[5] = {
  M_MFL, // sender ID (steering wheel)
  0x04,  // length of the message payload (including destination ID and checksum)
  M_RAD, // destination ID (radio)
  0x32, // the type of message
  10,  // Down  
  // don't worry about the checksum, the library automatically calculates it for you
};

void setup() {
  display.begin();
  display.fillScreen(default_colour);
  delay(100);
  display.fillScreen(BLACK);
  display.setTextColor(default_colour);

  display.setFont(&FreeSansOblique12pt7b);
  // setfont FreeSans print has 0 at BOTTOM of char
  display.setCursor(73, temp_y + temp_h);
  display.print("C");
  displaySpeed(def_speed_str);
  displayTemperature(def_temp_str);

  ibusTrx.begin(Serial); // begin listening on the first hardware serial port
}

void loop() {
  if (ibusTrx.available()) { // if there's a message waiting, do something with it
    IbusMessage message = ibusTrx.readMessage(); // grab the message
    unsigned int sourceID = message.source(); // read the source id
    unsigned int destinationID = message.destination(); // read the destination id
    unsigned int length = message.length(); // read the length of the payload
    unsigned int payloadFirstByte = message.b(0); // read the first byte of the payload
    // do something with this message
    if (sourceID == M_IKE) {
      // this message was sent by the instrument cluster
      if (payloadFirstByte == 0x18 && length > 3) {
        // data is kph/2, mph is k*5/8
        int current_speed = mph(message.b(1)*2);
        displaySpeed(current_speed);
        setSpeedVolume(current_speed);
        prev_speed = current_speed;
      } else if (payloadFirstByte == 0x19 && length > 3) {
        int current_temp = message.b(1);
        displayTemperature(current_temp);
      }
    }
  }
}

int mph(int kph){
  return kph * 5 / 8;
}

void setSpeedVolume(int current_speed) {
  if (current_speed != prev_speed) {
    for (int i=0; i<3; i++){
      if (current_speed >= vol_up_speed[i] && prev_speed < vol_up_speed[i]) {
        ibusTrx.write(volumeUp);
      } else if (current_speed <= vol_down_speed[i] && prev_speed > vol_down_speed[i]) {
        ibusTrx.write(volumeDown);
      }
    }
  }
}

void displaySpeed(int current_speed) {
  int colour = default_colour;
  if (current_speed > 85) {
    colour = YELLOW;
  }
  displaySpeed(String(current_speed), colour);
}

void displaySpeed(String current_speed_str) {
  displaySpeed(current_speed_str, default_colour);
}

void displaySpeed(String current_speed_str, int colour) {
  if (colour != prev_speed_colour) {
    prev_speed_str=def_speed_str;
    prev_speed_colour = colour;
  }
  prev_speed_str = displayDelta(current_speed_str, prev_speed_str, 3, speed_x, speed_y, speed_w, speed_h, &FreeSansBold24pt7b, colour);
}

void displayTemperature(int current_temp) {
  int colour = default_colour;
  if (current_temp < -5) {
    colour = BLUE;
  } else if (current_temp < 2) {
    colour = DRKCYAN;
  } else if (current_temp > 30) {
    colour = YELLOW;
  }
  displayTemperature(String(current_temp), colour);
}

void displayTemperature(String current_temp_str) {
  displayTemperature(current_temp_str, default_colour);
}

void displayTemperature(String current_temp_str, int colour) {
  if (colour != prev_temp_colour) {
    prev_temp_str=def_temp_str;
    prev_temp_colour = colour;
  }
  prev_temp_str = displayDelta(current_temp_str, prev_temp_str, 3, temp_x, temp_y, temp_w, temp_h, &FreeSansOblique12pt7b, colour);
}

String displayDelta(String data_str, String prev_str, int num_char, int16_t x_arr[], int16_t y, int16_t w_arr[], int16_t h, const GFXfont* font, int colour) {
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
