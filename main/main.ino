// https://github.com/adafruit/Adafruit_SH110x/blob/master/examples/SH1107_128x128/SH1107_128x128.ino
#include <U8g2lib.h>



// NOTE To Disable Input randomization, this define can be commented
#define EXPERIMENTAL


// Pin connections:
const int BTN_RIGHT = 2;
const int BTN_UP = 3;
const int BTN_DOWN = 4;
const int BTN_LEFT = 5;

const int BTN_START = 6;
const int BTN_DIFFICULTY = 7;

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
* Keep as small as possible 
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
* Lets think about the game area and the snake size:
* Snake head should be bigger than rest of the body
* Snake head can move between "grid" slots filling the entire grid
* Snake bodypart should have atleast 1 pixel from grid border
* Each part should be contained within their own grid slot
* 
* Head can be inside 5x5 sized slot with circle radius of 2.
* So the head size is the grid slot defining size.
*
* GRID_SLOT_SIDE defines only the square size that the snake part can be within.
* So the game area has 1 pixel thick border all around which reduces the game area by 1 pixel on each side so 2 pixels horizontal and vertical
*/
// NOTE(temppu) Check also the INIT_POS_MULT and FRUIT TABLE
const uint8_t GRID_SLOT_SIDE = 5;

// Defines the maximum size of the snake, after reaching this, the snake size can not grow anymore.
const uint8_t MAX_SNAKE_LEN = 100;

// Multiplier to define the starting position NOTE(temppu) might cause collisions with positions defined in FRUIT_TABLE / ROCK_TABLE 
const uint8_t INIT_POS_MULT = 6;
const uint8_t SNAKE_HEAD_RADIUS = 2;
const uint8_t SNAKE_BODY_RADIUS = 1;

const uint8_t FRUIT_RADIUS = 1;
const uint8_t ROCK_RADIUS = 4;

// NOTE(temppu) Coordinates in these tables should have the divisibility of GRID_SLOT_SIDE
const uint8_t FRUIT_TABLE[] = {
  90, 40, 120, 90, 105, // X values 5->120
  75, 10,  80, 20,  90, // X values 5->120
   5, 75, 115, 80,  20, // X values 5->120
  55, 35,  45, 95, 100, // X values 5->120
  
  85, 10, 60, 75,  90,  // Y values 5->90   
  70, 15, 65, 35,  25,  // Y values 5->90   
  60, 30, 20, 50,  90,  // Y values 5->90
  45, 65, 35, 25,   5   // Y values 5->90 
};

const uint8_t ROCK_TABLE[] = {
  45, 110, 85, 20,  15, // X values 5->120 
  115, 35, 45, 95, 100, // X values 5->120
  40, 120, 90, 105,  5, // X values 5->120
  75,  10, 80, 20,  90, // X values 5->120
  
  45,  65, 35, 25,   5, // Y values 5->90  
  85,  10, 60, 75,  90, // Y values 5->90   
  60,  30, 20, 50,  90, // Y values 5->90
  70,  15, 65, 35,  25  // Y values 5->90
};



/*
* So we have 128 pixels in each direction on the display. Width is reduced from 128 px to 126px for the game grid and with 5x5 px tile, we should use 125 as the limit spot.
* pixels start from 0 so max pixel position is 127. With the previous specification, this is reduced for grid range [1, 126] which should give 126 pixels to work with.
* This in actuality will be reduced to 125 px so right side will have 2 pixels instead of 1 on the border
* Now, we can define the GAME_AREA to have width of [0, 126] which gives us 127px to use 2 for borders and rest for actual game grid.
* and for height [0, 96] which gives 97 pixels of which 2px for borders and rest for game grid.
* So the game grid has the size of 125 x 95 px
*/
const struct DrawArea GAME_AREA   = { .x_offset = 1, .y_offset = 1  , .w = 125, .h = 95 };
const struct DrawArea SCORE_AREA  = { .x_offset = 0, .y_offset = 102, .w = 126, .h = 26 };

const struct SnakeDirection KEYMAP[] = {
  { 0, -1}, // UP
  { 0,  1}, // DOWN
  {-1,  0}, // LEFT
  { 1,  0}  // RIGHT
};

volatile struct SnakeDirection dir = { 0, 0 };

volatile struct GameObject snake[MAX_SNAKE_LEN];
volatile struct GameObject fruit  = { .x_pos = 0, .y_pos = 0, .radius = 0 };
volatile struct GameObject rock   = { .x_pos = 0, .y_pos = 0, .radius = 0 };

volatile bool is_game_over = 0;
// NOTE(temppu) This variable has double meaning, it is used as a basis for snake current length and having it as uint8_t the max value is limited to 255 
volatile uint8_t score = 0;

// NOTE a bit of a challenge 
volatile uint8_t  input_button_offset = 0;
volatile bool     is_difficulty_hard = 0;

// Function declarations:
void read_input();
void set_dir(uint8_t btn_idx);
void move_snake();
void init_game();
void add_fruit_rock();
void add_snake_part();
void set_difficulty();
void game_over_screen();
void draw_screen();


void setup() {
  /* Button input_pullups normal HIGH */
  pinMode(BTN_UP    , INPUT_PULLUP);
  pinMode(BTN_DOWN  , INPUT_PULLUP);
  pinMode(BTN_LEFT  , INPUT_PULLUP);
  pinMode(BTN_RIGHT , INPUT_PULLUP);

  pinMode(BTN_START     , INPUT_PULLUP);
  pinMode(BTN_DIFFICULTY, INPUT_PULLUP);

  // OLED initialization
  u8g2.begin();
  u8g2.setContrast(255);

  // Game Set
  init_game();
}

void loop() {
  if (is_game_over) {
    game_over_screen();   
    read_input();
  } else {
    read_input();
    move_snake();
    draw_screen();
  }
}


/*
* Function to handle user input and set event based on the input.
* Calls function to set the direction or change difficulty and 
* resetting the game state.
*/
void read_input() {
#ifdef EXPERIMENTAL
  if (!digitalRead(BTN_DIFFICULTY)) {
    is_difficulty_hard = !is_difficulty_hard;
    set_difficulty();
  }
  if (!digitalRead(BTN_UP)) {
    set_dir(0);
  } else if (!digitalRead(BTN_DOWN)) {
    set_dir(1);
  }
  if (!digitalRead(BTN_LEFT)) {
    set_dir(2);
  } else if (!digitalRead(BTN_RIGHT)) {
    set_dir(3);
  }
  if (!digitalRead(BTN_START)) {
    init_game();
  }
#else
  // Original version without input randomization or "difficulty setting"
  if (!digitalRead(BTN_UP) && dir.y == 0) {
    set_dir(0);
  } else if (!digitalRead(BTN_DOWN) && dir.y == 0) {
    set_dir(1);
  }
  if (!digitalRead(BTN_LEFT) && dir.x == 0) {
    set_dir(2);
  } else if (!digitalRead(BTN_RIGHT) && dir.x == 0) {
    set_dir(3);
  }
  if (!digitalRead(BTN_START)) {
    init_game();
  }
#endif
}

/*
* Function that sets the direction of the snake based on the global KEYMAP[].
* Checks also against the reversal of the snake.
*/
void set_dir(uint8_t btn_idx) {
  // Get the offset to find from keymap, four inputs -> should be divisible by 4
  uint8_t key_idx = (btn_idx + input_button_offset) % 4;

  // Check if trying to reverse where came from
  if (KEYMAP[key_idx].x == -dir.x && KEYMAP[key_idx].y == -dir.y) {
    return;
  }

  // Set the new direction
  dir.x = KEYMAP[key_idx].x;
  dir.y = KEYMAP[key_idx].y;
}

/*
* Simply changes the state of global variable based on the difficulty setting.
*/
void set_difficulty() {
  // Randomizes the inputs
  #ifdef EXPERIMENTAL
  if (is_difficulty_hard) {
    input_button_offset = random(0,3);
  } else {
    input_button_offset = 0;
  }
  #else
   input_button_offset = 0;
  #endif
}

/*
* Function to handle setting and resetting the game state to the beginning.
*/
void init_game() {
  // Game set
  score = 0;
  is_game_over = 0;

  add_fruit_rock();

  set_difficulty();

  // Start from stopped mode:
  dir.x = 0;
  dir.y = 0;

  // Head
  // This seeds the actual positioning of each snake part on the grid
  // The .x .y should be the CENTER of the grid when using CIRCLE as the bodypart
  // So, if we have 5x5 grid and the first slot starts from (1,1) and ends (6,6)
  // so with GRID_SLOT_SIDE = 5 and INIT_POS_MULT 3 => (15, 15) as start point 
  snake[0].x_pos  = GRID_SLOT_SIDE * INIT_POS_MULT;
  snake[0].y_pos  = GRID_SLOT_SIDE * INIT_POS_MULT;
  snake[0].radius = SNAKE_HEAD_RADIUS;
  
  // Reset rest of the body:
  for (uint8_t i = 1; i < MAX_SNAKE_LEN; i++) {
    snake[i].x_pos  = 0;
    snake[i].y_pos  = 0;
    snake[i].radius = 0;
  }
}

/*
* Sets the next body part to be drawn to the screen.
* Positions the snake outside of view, this is just to prevent accidental collisions with head 
*   (which could happen depending on the order of calling functions)
* NOTE move_snake() function should handle setting the parts real position at the tail.
*/
void add_snake_part() {
  snake[score + 1].x_pos  = -10;
  snake[score + 1].y_pos  = -10;
  snake[score + 1].radius = SNAKE_BODY_RADIUS;
}

/*
* Function to add fruit and rock to the game area based on fixed positions.
* This is not ideal and could made simpler, but at this time it works.
*/
void add_fruit_rock() {
  // Sizes for each table (this makes it easier to alter const tables)
  uint8_t fruit_table_size  = (sizeof(FRUIT_TABLE)  / sizeof(FRUIT_TABLE[0]));
  uint8_t rock_table_size   = (sizeof(ROCK_TABLE)   / sizeof(ROCK_TABLE[0]));
    
  // Translate to x,y coordinate indexes that fit in the table sizes
  // First half is for X values:
  uint8_t x_idx_fruit       = score % (uint8_t) (fruit_table_size / 2);
  // Second half is for Y values:
  uint8_t y_idx_fruit       = fruit_table_size - (1 + x_idx_fruit);

  // Same applies for the rock table:
  uint8_t x_idx_rock        = score % (uint8_t) (rock_table_size / 2);
  uint8_t y_idx_rock        = rock_table_size - (1 + x_idx_rock);

  // Using the constant table, we can easily get desired output that is easier to test:
  fruit.x_pos   = FRUIT_TABLE[x_idx_fruit];
  fruit.y_pos   = FRUIT_TABLE[y_idx_fruit];
  fruit.radius  = FRUIT_RADIUS;
  
  rock.x_pos    = ROCK_TABLE[x_idx_rock];
  rock.y_pos    = ROCK_TABLE[y_idx_rock];
  rock.radius   = ROCK_RADIUS;
}

/*
* Function to handle moving the snake (x,y) coordinates
*	and check game over conditions.
* Some might say that this function does more than it should but for the scope of this project, I think it is fine.
*/
void move_snake() {
  // Snake head new position
  int8_t n_x = snake[0].x_pos + dir.x * GRID_SLOT_SIDE;
  int8_t n_y = snake[0].y_pos + dir.y * GRID_SLOT_SIDE;

  // Checks whether the snake goes out of GAME_AREA bounds:
  if (GAME_AREA.x_offset >= n_x || GAME_AREA.w <= n_x || GAME_AREA.y_offset >= n_y || GAME_AREA.h <= n_y) {
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
    // Score is used when iterating through the snake and should be increased by 1 
    // having a separate variable would be better but this works.
    score++;
    
    // End the game before value overflows assuming uint8_t  
    if (score == 255) {
      is_game_over = 1;
      return;
    }

    // Add new bodypart only if the snake max length has not been achieved
    if (MAX_SNAKE_LEN > score) {
      add_snake_part();
    }
    // We can add fruits and rocks indefinitely
    add_fruit_rock();
  }

  /* We need to iterate through the snake to check if head collides with the body.
   * Score keeps track of the snakes length. 
   * This will propably result to noticeable slowing down of the game as the score increases
  */
  // Note that lower bound for iterating through the snake is accounted.
  for (int8_t i = (score < MAX_SNAKE_LEN ? score : MAX_SNAKE_LEN); i > 0; i--) {
    snake[i].x_pos = snake[i - 1].x_pos;
    snake[i].y_pos = snake[i - 1].y_pos;
    snake[i].radius = SNAKE_BODY_RADIUS;

    // Check the self collision
    if (snake[i].x_pos == n_x && snake[i].y_pos == n_y) {
      is_game_over = 1;
      return;
    }
  }

  // Update the head to new slot:
  snake[0].x_pos = n_x;
  snake[0].y_pos = n_y;
}

/*
* Function for drawing gameover screen.
*/
void game_over_screen() {
  u8g2.firstPage();
  do {
    // Here are some "magic" numbers that affect the position of the text at this time
    u8g2.setFont(u8g2_font_t0_11_mf);
    u8g2.drawStr(40, 20, "Game Over!");
    u8g2.drawStr(40, 40, "Score:");
    u8g2.setCursor(90, 40);
    u8g2.print(score);
  } while (u8g2.nextPage());
}

/*
* Function that handles drawing to screen (essentially renderer).
* Updates the score view only when needed and displays the snake based on score.
*/
void draw_screen(void) {
  u8g2.firstPage();
  do {
    // Draw score frame:
    u8g2.drawFrame(SCORE_AREA.x_offset, SCORE_AREA.y_offset, SCORE_AREA.w, SCORE_AREA.h);

    
    // Here are some "magic" numbers that affect the position of the text:
    const uint8_t Y_OFFSET = 16;
    
    u8g2.setFont(u8g2_font_t0_11_mf);
    u8g2.drawStr(   SCORE_AREA.x_offset + 4         , SCORE_AREA.y_offset + Y_OFFSET, "Score:");
    u8g2.setCursor( SCORE_AREA.x_offset + 3 + 8 * 5 , SCORE_AREA.y_offset + Y_OFFSET);
    
    u8g2.print(score);

    // Show to the user the difficulty in the score section
    const uint8_t DIFF_X_OFFSET = 8 + 8 * 9 + 12;
    if (is_difficulty_hard) {
      u8g2.drawStr(SCORE_AREA.x_offset + DIFF_X_OFFSET, SCORE_AREA.y_offset + Y_OFFSET, "HARD");
    } else {
      u8g2.drawStr(SCORE_AREA.x_offset + DIFF_X_OFFSET, SCORE_AREA.y_offset + Y_OFFSET, "EASY");
    }
    // Draw game frame:
    u8g2.drawFrame(GAME_AREA.x_offset, GAME_AREA.y_offset, GAME_AREA.w, GAME_AREA.h);

    // Draw the actual snake, as the snake grows, it will slow down the game loop:
    for (uint8_t i = 0; i <= (score < MAX_SNAKE_LEN ? score : MAX_SNAKE_LEN -1); i++) {
      // Draws a circle, where the x and y given are the center position, so should be center of the grid slot
      u8g2.drawCircle(snake[i].x_pos, snake[i].y_pos, snake[i].radius);
    }
    
    // Draws a circle for fruit and rock, where the x and y given are the center position, so should be center of the grid slot
    u8g2.drawCircle(fruit.x_pos , fruit.y_pos , fruit.radius);
    u8g2.drawCircle(rock.x_pos  , rock.y_pos  , rock.radius);

  } while (u8g2.nextPage() && !is_game_over);
}

