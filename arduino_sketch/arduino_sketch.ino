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

#define DEFAULT_COLOUR  ORANGE

#define speed_y 0
#define speed_h 35
#define temp_y 45
#define temp_h 17


#define IKE_SPEED 0x18
#define IKE_TEMPERATURE 0x19
#define MLF_VOL 0x32
#define MLF_BUTTONS 0x3B
#define MLF_BUTTON_RT 0x40
#define MLF_BUTTONS2 0x01

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
IbusTrx ibusTrx;

static const byte speed_x[3] = {62, 32, 9};
static const byte speed_w[3] = {25, 25, 18};
static const byte temp_x[4] = {55, 39, 25};
static const byte temp_w[4] = {18, 18, 12};

static const byte vol_up_speed[3] = {35, 50, 65};
static const byte vol_down_speed[3] = {30, 45, 60};
unsigned int prev_speed = 0;
unsigned int prev_speed_colour = DEFAULT_COLOUR;
unsigned int prev_temp_colour = DEFAULT_COLOUR; 
char current_char, prev_char;
String def_speed_str = "   ", def_temp_str = "   ";
String prev_speed_str = "***", prev_temp_str = "***";

uint8_t volumeUp[5] = {
  M_MFL, // sender ID (steering wheel)
  0x04,  // length of the message payload (including destination ID and checksum)
  M_RAD, // destination ID (radio)
  MLF_VOL, // the type of message
  11, // Up
  // don't worry about the checksum, the library automatically calculates it for you
};

uint8_t volumeDown[5] = {
  M_MFL, // sender ID (steering wheel)
  0x04,  // length of the message payload (including destination ID and checksum)
  M_RAD, // destination ID (radio)
  MLF_VOL, // the type of message
  10,  // Down  
  // don't worry about the checksum, the library automatically calculates it for you
};

void setup() {
  display.begin();
  display.fillScreen(DEFAULT_COLOUR);
  delay(100);
  display.fillScreen(BLACK);
  display.setTextColor(DEFAULT_COLOUR);

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
      if (payloadFirstByte == IKE_SPEED && length > 3) {
        // data is kph/2
        unsigned int payloadFirstByte = message.b(1);
        int current_speed = mph(payloadFirstByte*2);
        displaySpeed(current_speed);
        setSpeedVolume(current_speed);
        prev_speed = current_speed;

      } else if (payloadFirstByte == IKE_TEMPERATURE && length > 3) {
        int current_temp = message.b(1);
        displayTemperature(current_temp);
      }
    } else if (sourceID == M_MFL) {
      if (payloadFirstByte == MLF_BUTTONS && length > 3) {
        // first pos. code for RT buttom
        if (message.b(1) == MLF_BUTTON_RT) {
          ibusTrx.write(volumeUp);
        }
      } else if (payloadFirstByte == MLF_BUTTONS2) {
        //second code for RT button
        ibusTrx.write(volumeDown);
      }
    }
  }
}

unsigned int mph(unsigned int kph){
  // mph is kph*5/8
  // The Mini's displayed speed is over by a few mph, so add a 4% "correction"
  // Multiply by 100 and add 50 then div 100 to round up when answer would be >= x.5
  // Use unsigned long in the maths to avoid overflow with the multiplication
  // even though we only want an int as output
  return ((kph * 104UL * 5 / 8) + 50)/100;
}

void setSpeedVolume(unsigned int current_speed) {
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

void displaySpeed(unsigned int current_speed) {
  unsigned int colour = DEFAULT_COLOUR;
  if (current_speed > 85) {
    colour = YELLOW;
  }
  displaySpeed(String(current_speed), colour);
}

void displaySpeed(String current_speed_str) {
  displaySpeed(current_speed_str, DEFAULT_COLOUR);
}

void displaySpeed(String current_speed_str, unsigned int colour) {
  if (colour != prev_speed_colour) {
    prev_speed_str=def_speed_str;
    prev_speed_colour = colour;
  }
  prev_speed_str = displayDelta(current_speed_str, prev_speed_str, 3, speed_x, speed_y, speed_w, speed_h, &FreeSansBold24pt7b, colour);
}

void displayTemperature(unsigned int current_temp) {
  unsigned int colour = DEFAULT_COLOUR;
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
  displayTemperature(current_temp_str, DEFAULT_COLOUR);
}

void displayTemperature(String current_temp_str, unsigned int colour) {
  if (colour != prev_temp_colour) {
    prev_temp_str=def_temp_str;
    prev_temp_colour = colour;
  }
  prev_temp_str = displayDelta(current_temp_str, prev_temp_str, 3, temp_x, temp_y, temp_w, temp_h, &FreeSansOblique12pt7b, colour);
}

String displayDelta(String data_str, String prev_str, int num_char, byte x_arr[], byte y, byte w_arr[], byte h, const GFXfont* font, unsigned int colour) {
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
