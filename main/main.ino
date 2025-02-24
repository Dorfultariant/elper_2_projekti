// https://github.com/adafruit/Adafruit_SH110x/blob/master/examples/SH1107_128x128/SH1107_128x128.ino
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 128
#define OLED_RESET -1

Adafruit_SH1107 display = Adafruit_SH1107(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET, 1000000, 100000);

// Constants:
const int BTN_RIGHT = 2;
const int BTN_UP = 3;
const int BTN_DOWN = 4;
const int BTN_LEFT = 5;

const int BTN_START = 6;
const int BTN_RESET = 7;

/* I2C
* SDA Serial Data
* SCL Serial Clock
*/
const int OLED_SDA = A4;
const int OLED_SCL = A5;

/*
* Function that returns different integer value based on which button is pressed
* This is based on coordinate system commonly found in game development
* where upper left corner is (0,0) (x,y) 
* 
* # NOTE THIS IS SUBJECT TO CHANGE RADICALLY
*/
int read_input() {
  if (digitalRead(BTN_UP)) {
    return -1;
  }
  if (digitalRead(BTN_LEFT)) {
    return -2;
  }
  if (digitalRead(BTN_DOWN)) {
    return 1;
  } 
  if (digitalRead(BTN_RIGHT)) {
    return 2;
  } 
  return 0;
}

void draw_to_screen() {


}

// Straight copy-paste from:
// https://github.com/adafruit/Adafruit_SH110x/blob/master/examples/SH1107_128x128/SH1107_128x128.ino
void testdrawrect(void) {
  display.clearDisplay();

  for(int16_t i=0; i<display.height()/2; i+=2) {
    display.drawRect(i, i, display.width()-2*i, display.height()-2*i, SH110X_WHITE);
    display.display(); // Update screen with each newly-drawn rectangle
    delay(1);
  }

  delay(2000);
}


void setup() {
  Serial.begin(9600);
  /* Button input_pullups normal HIGH */
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);

  pinMode(BTN_RESET, INPUT_PULLUP);
  pinMode(BTN_START, INPUT_PULLUP);


  Serial.println("Start");
  // Delay for OLED Init
  delay(250);

  // Default address
  display.begin(0x3C, 0);

  Serial.println("Begun");
  display.display();
  delay(2000);

  display.clearDisplay();

  Serial.println("Pixel");
  display.drawPixel(10, 12, SH110X_WHITE);

  // Must be called AFTER when writing to screen to display
  display.display();
 

  delay(2000);

  testdrawrect();
}

void loop() {

  Serial.println("Complete");


}