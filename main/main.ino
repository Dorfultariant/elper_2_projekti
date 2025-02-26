// https://github.com/adafruit/Adafruit_SH110x/blob/master/examples/SH1107_128x128/SH1107_128x128.ino
#include <U8g2lib.h>
#include <Wire.h>

// Pin connections:
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

// Constants for setting the game
const int8_t MAX_SNAKE_LEN = 20;
const char GAME_OVER_TEXT = "GAME OVER!";

// Constructor for our OLED (There are many different ones, I found it by trial and error)
U8G2_SH1107_SEEED_128X128_2_HW_I2C u8g2(U8G2_R0, OLED_SCL, OLED_SDA);

struct DrawArea {
  int8_t x_offset;
  int8_t y_offset;
  int8_t w;
  int8_t h;
};

struct SnakeDirection {
  int8_t x;
  int8_t y;
};

// keep as small as possible (currently 24 bits or 3 bytes)
struct SnakePart {
  int8_t x_pos;
  int8_t y_pos;
  int8_t radius;
};


/*
 Lets think about the game area:
 We have 127 pixel wide and 100 pixel tall section that we can play on
 Snake head should be bigger than rest of the body
 Snake head can move between "grid" slots filling the entire grid
 Snake bodypart should have 1 pixel atleast from grid border
 Each part should be contained within their own grid slot
 
 Head can be 6x6 sized
 Body can be 3x3 sized
 So the head size is the grid slot defining size.
 With 100 height we get 100 / 6 = 16,666... so we can have 96 pixel height for game area and thus 16 grid slots for height and
 127 / 6 = 21.166... so rougly 21 grid slots for width and 126 pixel width

  With these values we can make the actual game area to defined size which allows us to move the snake incrementally by 6x6 slot amount
*/
const uint8_t GRID_SLOT_SIDE = 6;

const struct DrawArea GAME_AREA = {0, 0, 126, 96};
const struct DrawArea SCORE_AREA = {0, 102, 126, 26};

volatile struct SnakeDirection dir = {0, 0};
volatile struct SnakePart head = {GRID_SLOT_SIDE, GRID_SLOT_SIDE, 3};
volatile bool is_game_over = 0;

/*
* Function that returns different integer value based on which button is pressed
* This is based on coordinate system commonly found in game development
* where upper left corner is (0,0) (x,y) 
* 
* # NOTE THIS IS SUBJECT TO CHANGE RADICALLY
* # NOTE INTERRUPTS ?
*/

void read_input() {
  if (!digitalRead(BTN_UP)) {
    dir.x = 0;
    dir.y = -1;
  }
  if (!digitalRead(BTN_LEFT)) {
    dir.x = -1;
    dir.y = 0;
  }
  if (!digitalRead(BTN_DOWN)) {
    dir.x = 0;
    dir.y = 1;
  } 
  if (!digitalRead(BTN_RIGHT)) {
    dir.x = 1;
    dir.y = 0;
  }
}

void game_over() { 
  is_game_over = true;
  //u8g2.drawStr(SCORE_AREA.x_offset + 1, SCORE_AREA.y_offset + 1, &GAME_OVER_TEXT);
  Serial.println("Game is over!");
}

/*
 Function to handle only moving the snake (x,y) coordinates
*/
void move_snake() {
  head.x_pos += (dir.x * GRID_SLOT_SIDE);
  /*// Check X axle limits:
  if (GAME_AREA.w < head.x_pos || (GAME_AREA.x_offset) > head.x_pos ) {
    game_over();
  }
*/
  head.y_pos += (dir.y * GRID_SLOT_SIDE);
/*  // Check Y axle limits:
  if (GAME_AREA.h < head.y_pos || (GAME_AREA.y_offset) > head.y_pos ) {
    game_over();
  }*/
}

/*
 Function that handles drawing to screen (essentially renderer)
 All drawable elements should be included here and nowhere else.
*/
void draw_screen(void) {
  u8g2.firstPage();
  const char s = { "Score:" };
  do {
    // Draw score frame:
    u8g2.drawFrame(SCORE_AREA.x_offset, SCORE_AREA.y_offset, SCORE_AREA.w, SCORE_AREA.h);

    // Here are some "magic" numbers that affect the position of the text at this time
    u8g2.setFont(u8g2_font_t0_11_mf);
    u8g2.drawStr(SCORE_AREA.x_offset + 8, SCORE_AREA.y_offset + 14, "Score:");
    u8g2.setCursor(SCORE_AREA.x_offset + 10 + 8*5 + 4, SCORE_AREA.y_offset + 14);
    u8g2.print(dir.x);
  
    // Draw game frame:
    u8g2.drawFrame(GAME_AREA.x_offset, GAME_AREA.y_offset, GAME_AREA.w, GAME_AREA.h);
    
    // Draw the actual snake (should be a loop as the snake grows)
    u8g2.drawCircle(head.x_pos, head.y_pos, head.radius);

  } while(u8g2.nextPage() && !is_game_over);
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

  // put your setup code here, to run once:
  u8g2.begin();
  u8g2.setContrast(255);

}

void loop() {
  delay(50);
  read_input();
  move_snake();
  // check gameover
  // check if snake over fruit
  // create fruit at interval
  draw_screen();
}
