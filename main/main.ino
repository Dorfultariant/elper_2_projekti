// https://github.com/adafruit/Adafruit_SH110x/blob/master/examples/SH1107_128x128/SH1107_128x128.ino
#include <U8g2lib.h>

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

// Constructor for our OLED (There are many different ones, I found it by trial and error)
// Src: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
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

/* 
* Keep as small as possible (currently 24 bits or 3 bytes) 
* option to remove radius but then again it is handy if there are multiple 
* different instances of objects.
* And this not likely to be an issue either way...
*/
struct GameObject {
  int8_t x_pos;
  int8_t y_pos;
  int8_t radius;
};


/*
 Lets think about the game area and the snake size:
 Snake head should be bigger than rest of the body
 Snake head can move between "grid" slots filling the entire grid
 Snake bodypart should have atleast 1 pixel from grid border
 Each part should be contained within their own grid slot
 
 Head can be inside 5x5 sized slot with circle radius of 2.
 Body fits inside the grid slot and should be smaller than the head.
 So the head size is the grid slot defining size.

 GRID_SLOT_SIDE defines only the part that the snake part can be within.
 So the game area has 1 pixel thick border all around which reduces the game area by 1 pixel on each side so 2 pixels horizontal and vertical
*/
// NOTE(temppu) Check also the HEAD_COORD_XY and FRUIT TABLE
const uint8_t GRID_SLOT_SIDE = 5;

const uint8_t MAX_SNAKE_LEN = 100;

const uint8_t SNAKE_HEAD_RADIUS = 2;
const uint8_t SNAKE_BODY_RADIUS = 1;

const uint8_t FRUIT_RADIUS = 1;

const uint8_t ROCK_RADIUS = 4;

const uint8_t FRAME_THICKNESS = 1;

// NOTE(temppu) If the GRID_SLOT_SIDE is odd, add 1 to it for this calculation
// With GRID_SLOT_SIZE = 5, FRAME_THICKNESS = 1 the result should be 4, so head start @(4,4)
// #WARNING CHANGE START POINT
const uint8_t HEAD_COORD_XY = (GRID_SLOT_SIDE + 1) / 2;


// NOTE(temppu) Coordinates in this table should have the same divisibility as the HEAD_COORD_XY
const uint8_t FRUIT_TABLE[] = {
  10, 50, 25, 90, 5,
  40, 75, 10, 80, 15,
  70, 60, 65, 35, 25,
  60, 30, 15, 50, 70
};

const uint8_t ROCK_TABLE[] = {
  15, 45, 20, 85, 5,
  55, 65, 15, 70, 10,
  40, 60, 65, 35, 25,
  65, 35, 15, 50, 70
};
/*
* So we have 128 pixels in each direction on the display. Width is reduced from 128 px to 126px for the game grid and with 5x5 px tile, we should use 125 as the limit spot.
* pixels start from 0 so max pixel position is 127. With the previous specification, this is reduced for grid range [1, 126] which should give 126 pixels to work with.
* This in actuality will be reduced to 125 px so right side will have 2 pixels instead of 1 on the border
* Now, we can define the GAME_AREA to have width of [0, 126] which gives us 127px to use 2 for borders and rest for actual game grid.
* and for height [0, 96] which gives 97 pixels of which 2px for borders and rest for game grid.
* So the game grid has the size of 125 x 95 px
*/
const struct DrawArea GAME_AREA = { .x_offset = 0, .y_offset = 0, .w = 127, .h = 97 };
const struct DrawArea SCORE_AREA = { .x_offset = 0, .y_offset = 102, .w = 126, .h = 26 };

volatile struct SnakeDirection dir = { 0, 0 };

volatile struct GameObject snake[MAX_SNAKE_LEN];
volatile struct GameObject fruit = { .x_pos = 0, .y_pos = 0, .radius = 0 };

volatile struct GameObject rock = { .x_pos = 0, .y_pos = 0, .radius = 0 };

volatile bool is_game_over = 0;
// NOTE(temppu) This variable has double meaning, it is used as a basis for snake current length
volatile int8_t score = 0;

// Function declarations:
void read_input();
void move_snake();
void init_game();
void add_fruit();
void add_snake_part();
void game_over_screen();
void draw_screen();


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

  init_game();
}

void loop() {
  // Unfortunately, overhead from u8g2lib and not so efficient snake logic, the game is not that hard
  /*int8_t d = 50 - score * 5;
  if (d >= 5) {
    delay(d);
  }*/

  if (is_game_over) {
    game_over_screen();
  } else {
    read_input();
    move_snake();
    draw_screen();
  }
}


/*
* Function that returns different integer value based on which button is pressed
* This is based on coordinate system commonly found in game development
* where upper left corner is (0,0) (x,y) 
* 
* # NOTE(temppu) 
* Do we want INTERRUPTS and thus propably bugs / undefined behaviour?
*/
void read_input() {
  if (!digitalRead(BTN_UP) && dir.y == 0) {
    dir.x = 0;
    dir.y = -1;
  } else if (!digitalRead(BTN_DOWN) && dir.y == 0) {
    dir.x = 0;
    dir.y = 1;
  }
  if (!digitalRead(BTN_LEFT) && dir.x == 0) {
    dir.x = -1;
    dir.y = 0;
  } else if (!digitalRead(BTN_RIGHT) && dir.x == 0) {
    dir.x = 1;
    dir.y = 0;
  }
  if (!digitalRead(BTN_RESET)) {
    init_game();
  }
}

void init_game() {
  
  // Game set
  score = 0;
  is_game_over = 0;
  add_fruit();

  // Start from stopped mode:
  dir.x = 0;
  dir.y = 0;

  // Head
  // This seeds the actual positioning of each snake part on the grid
  // The .x .y should be the CENTER of the grid when using CIRCLE as the bodypart
  // So, if we have 5x5 grid and the first slot starts from (1,1) and ends (6,6)
  // which would result to .x = 4 .y = 4 as the head start
  snake[0].x_pos = HEAD_COORD_XY;
  snake[0].y_pos = HEAD_COORD_XY;
  snake[0].radius = SNAKE_HEAD_RADIUS;
    
  // Rest of the body:
  for (uint8_t i = 1; i < MAX_SNAKE_LEN; i++) {
    snake[i].x_pos = 0;
    snake[i].y_pos = 0;
    snake[i].radius = 0;
  }
}


void add_snake_part() {
  snake[score + 1].x_pos = -10;
  snake[score + 1].y_pos = -10;
  snake[score + 1].radius = SNAKE_BODY_RADIUS;
}

// Selects fruits spot from specified table "randomly".   
void add_fruit() {
  uint8_t size = (sizeof(FRUIT_TABLE) / sizeof(uint8_t));
  int8_t r = random(0, size);
  uint8_t x_idx = score % size;
  uint8_t y_idx = r % size;

  // Using the constant table FRUIT_TABLE we can easily get desired output that is easier to test:
  fruit.x_pos = HEAD_COORD_XY + FRUIT_TABLE[x_idx];
  fruit.y_pos = HEAD_COORD_XY + FRUIT_TABLE[y_idx];
  fruit.radius = FRUIT_RADIUS;
  
  rock.x_pos = HEAD_COORD_XY + ROCK_TABLE[y_idx];
  rock.y_pos = HEAD_COORD_XY + ROCK_TABLE[r % size];
  rock.radius = ROCK_RADIUS;
}

/*
 	Function to handle moving the snake (x,y) coordinates
	and check game over conditions while we are at it.

	If somebody wants to make a more efficient version such as "circle"-like structure
	go ahead.
*/
void move_snake() {
  // Snake head new position
  int8_t n_x = snake[0].x_pos + dir.x * GRID_SLOT_SIDE;
  int8_t n_y = snake[0].y_pos + dir.y * GRID_SLOT_SIDE;

  // Checks whether the snake goes out of GAME_AREA bounds:
  if (GAME_AREA.x_offset > n_x || GAME_AREA.w < n_x || GAME_AREA.y_offset > n_y || GAME_AREA.h < n_y) {
    is_game_over = 1;
    return;
  }

  // Check if head collides with rock
  if (rock.x_pos == n_x && rock.y_pos == n_y) {
    // Score is used when iterating through the snake
    is_game_over = 1;
    return;
  }

  // Check if we can collect fruit:
  if (fruit.x_pos == n_x && fruit.y_pos == n_y) {
    // Score is used when iterating through the snake
    score++;
    add_snake_part();
    add_fruit();
  }

  // We need to iterate through the snake to check if head collides with the body.
  // Score keeps track of the snakes length, this will propably result to noticeable slowing down of the game
  // as score increases
  for (int8_t i = score; i > 0; i--) {
    snake[i].x_pos = snake[i - 1].x_pos;
    snake[i].y_pos = snake[i - 1].y_pos;
    snake[i].radius = SNAKE_BODY_RADIUS;

    // Check the self collision
    if (snake[i].x_pos == n_x && snake[i].y_pos == n_y) {
      is_game_over = 1;
      break;
    }
  }

  // Update the head to new slot:
  snake[0].x_pos = n_x;
  snake[0].y_pos = n_y;
}

/*
* Drawing the end screen for user.
*/
void game_over_screen() {
  //u8g2.clearDisplay();
  u8g2.firstPage();
  do {
    // Here are some "magic" numbers that affect the position of the text at this time
    u8g2.setFont(u8g2_font_t0_11_mf);
    u8g2.drawStr(40, 20, "Game Over!");
    u8g2.drawStr(40, 40, "Score:");
    u8g2.setCursor(90, 40);
    u8g2.print(score);
  } while (u8g2.nextPage());

  if (!digitalRead(BTN_START)) {
    init_game();
    delay(10);
  }
}

/*
 Function that handles drawing to screen (essentially renderer)
 All drawable elements should be included here and nowhere else.
*/
void draw_screen(void) {
  u8g2.firstPage();
  do {
    // Draw score frame:
    u8g2.drawFrame(SCORE_AREA.x_offset, SCORE_AREA.y_offset, SCORE_AREA.w, SCORE_AREA.h);

    // Here are some "magic" numbers that affect the position of the text at this time
    u8g2.setFont(u8g2_font_t0_11_mf);
    u8g2.drawStr(SCORE_AREA.x_offset + 8, SCORE_AREA.y_offset + 14, "Score:");
    u8g2.setCursor(SCORE_AREA.x_offset + 10 + 8 * 5 + 4, SCORE_AREA.y_offset + 14);
    u8g2.print(score);

    // Draw game frame:
    u8g2.drawFrame(GAME_AREA.x_offset, GAME_AREA.y_offset, GAME_AREA.w, GAME_AREA.h);

    // Draw the actual snake:
    for (uint8_t i = 0; i < MAX_SNAKE_LEN; i++) {
      if (snake[i].radius == 0) break;
      // Draws a circle, where the x and y given are the center position, so should be center of the grid slot
      u8g2.drawCircle(snake[i].x_pos, snake[i].y_pos, snake[i].radius);
    }
    
    // Draws a circle, where the x and y given are the center position, so should be center of the grid slot
    u8g2.drawCircle(fruit.x_pos, fruit.y_pos, fruit.radius);
    u8g2.drawCircle(rock.x_pos, rock.y_pos, rock.radius);

  } while (u8g2.nextPage() && !is_game_over);
}
