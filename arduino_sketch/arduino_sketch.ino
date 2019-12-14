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
#define ORANGE          0xF860
#define DRKYELLOW       0xEFE0
#define DRKCYAN         0x041F

#define DEFAULT_COLOUR  ORANGE
#define BACK_COLOUR  BLACK

#define speed_x 90
#define speed_y 30
#define temp_x 69
#define temp_y 62


#define IKE_SPEED 0x18
#define IKE_TEMPERATURE 0x19
#define MFL_BUTTONS 0x32
#define MFL_RT 0x01

Adafruit_SSD1331 display = Adafruit_SSD1331(cs, dc, rst);
IbusTrx ibusTrx;

int vol_up_speed[4] = {35, 45, 55, 65};
int vol_down_speed[4] = {30, 40, 50, 60};
int current_speed_vol = 0;
int prev_speed = 0;
int prev_speed_colour = DEFAULT_COLOUR;
int prev_temp_colour = DEFAULT_COLOUR; 
char current_char, prev_char;
String def_speed_str = "   ", def_temp_str = "   ";
String prev_speed_str = "***", prev_temp_str = "***";

bool testing = true;

long loop_timer_now;          //holds the current millis
long previous_millis;         //holds the previous millis
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
  // setfont FreeSans print has 0 at BOTTOM of char
  display.setCursor(73, temp_y);
  display.print("C");
  displaySpeed(def_speed_str);
  displayTemperature(def_temp_str);

  ibusTrx.begin(Serial); // begin listening on the first hardware serial port
  previous_millis = millis();
}

void loop() {
  if (testing) {
    loop_timer_now = millis();
    if ((loop_timer_now - previous_millis) > 1000) {
      test_speed++;
      if (test_speed > 135) test_speed = 0;
      displaySpeed(test_speed);
      setSpeedVolume(test_speed);
      prev_speed = test_speed;
      test_temp++;
      if (test_temp > 55) test_temp = -35;
      displayTemperature(test_temp);
      previous_millis = loop_timer_now; 
    }
  }
  if (!testing && ibusTrx.available()) { // if there's a message waiting, do something with it
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
        int kph2 = message.b(1);
        int current_speed = mph(kph2*2);
        displaySpeed(current_speed);
        setSpeedVolume(current_speed);
        prev_speed = current_speed;

      } else if (payloadFirstByte == IKE_TEMPERATURE && length > 3) {
        signed char current_temp = (byte) 255; //message.b(1);
        displayTemperature(current_temp);
      }
    } else if (sourceID == M_MFL && destinationID == M_TEL) {
      if (payloadFirstByte == MFL_RT && length > 2) {
        // first pos. code for RT buttom
      }
    }
  }
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
  if (current_speed != prev_speed) {
    for (int i=0; i<3; i++){
      if (current_speed >= vol_up_speed[i] && prev_speed < vol_up_speed[i]) {
        // target speed vol is i + 1;
        if ((i+1) > current_speed_vol) {
          ibusTrx.write(volumeUp);
          current_speed_vol++;
        }
      } else if (current_speed <= vol_down_speed[i] && prev_speed > vol_down_speed[i]) {
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
  displaySpeed(String(current_speed), colour);
}

void displaySpeed(String current_speed_str) {
  displaySpeed(current_speed_str, DEFAULT_COLOUR);
}

void displaySpeed(String current_speed_str, int colour) {
  if (colour != prev_speed_colour) {
    prev_speed_str=def_speed_str;
    prev_speed_colour = colour;
  }
  prev_speed_str = displayDelta(current_speed_str, prev_speed_str, speed_x, speed_y, &FreeSansBold24pt7b, colour);
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
  displayTemperature(String(current_temp), colour);
}

void displayTemperature(String current_temp_str) {
  displayTemperature(current_temp_str, DEFAULT_COLOUR);
}

void displayTemperature(String current_temp_str, int colour) {
  if (colour != prev_temp_colour) {
    prev_temp_str=def_temp_str;
    prev_temp_colour = colour;
  }
  prev_temp_str = displayDelta(current_temp_str, prev_temp_str, temp_x, temp_y, &FreeSansOblique12pt7b, colour);
}

String displayDelta(String data_str, String prev_str, int x, int y, const GFXfont* font, int colour) {
  if (data_str != prev_str) {
    // only update if changed
    
    display.setFont(font);
    int16_t  x1, y1;
    uint16_t ww, hh;
    display.getTextBounds(string2char(prev_str), 1, 1, &x1, &y1, &ww, &hh );
    display.setCursor(x - ww, y);

    // erase prev string
    display.setTextColor(BACK_COLOUR);
    display.print(prev_str);

    display.getTextBounds(string2char(data_str), 1, 1, &x1, &y1, &ww, &hh );
    display.setCursor(x - ww, y);
    display.setTextColor(colour);
    display.print(data_str);
    prev_str = data_str;
  }
  return prev_str;
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}
