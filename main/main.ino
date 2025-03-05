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
// Constants for setting the game
const uint8_t GRID_SLOT_SIDE = 6;

const int8_t MAX_SNAKE_LEN = 20;
const uint8_t SNAKE_HEAD_RADIUS = 3;
const uint8_t SNAKE_BODY_RADIUS = 1;

const uint8_t FRUIT_TABLE[] = {
	12, 54, 24, 90,   6,
	42, 78, 12, 84,  18,
	72, 60,  6, 36,  24,
	54, 12, 18, 48,  72
};

const struct DrawArea GAME_AREA = {0, 0, 126, 96};
const struct DrawArea SCORE_AREA = {0, 102, 126, 26};

volatile struct SnakeDirection dir = {0, 0};

volatile struct GameObject snake[MAX_SNAKE_LEN];
volatile struct GameObject fruit = {.x_pos = 0, .y_pos = 0, .radius = 0};

volatile bool is_game_over = 0;
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
  delay(50);
  if (is_game_over) {
    game_over_screen();
  }
  read_input();
  move_snake();
  draw_screen();
}


/*
* Function that returns different integer value based on which button is pressed
* This is based on coordinate system commonly found in game development
* where upper left corner is (0,0) (x,y) 
* 
* # NOTE THIS IS SUBJECT TO CHANGE RADICALLY
* Do we want INTERRUPTS and thus propably bugs / undefined behaviour?
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

void init_game() {
	// Head
	struct GameObject head = { GRID_SLOT_SIDE, GRID_SLOT_SIDE, 3 };
	snake[0].x_pos = head.x_pos;
	snake[0].y_pos = head.y_pos;
	snake[0].radius = head.radius;
	
	// Game set
	add_fruit();
	score = 0;
  is_game_over = 0;

	// Rest of the body:
	for (uint8_t i = 1; i < MAX_SNAKE_LEN; i++) {
		struct GameObject body = {0, 0, 0};
		snake[i].x_pos = body.x_pos;
		snake[i].y_pos = body.y_pos;
		snake[i].radius = body.radius;
	}
}


void game_over_screen() { 
  u8g2.firstPage();
  do {
    // Here are some "magic" numbers that affect the position of the text at this time
    u8g2.setFont(u8g2_font_t0_11_mf);
    u8g2.drawStr(40, 20, "Game Over!");
    u8g2.drawStr(60, 20, "Score:");
    u8g2.setCursor(90, 20);
    u8g2.print(dir.x);
  } while(u8g2.nextPage());
  
  if (!digitalRead(BTN_START)) {
    init_game();
  }
}

void add_snake_part() {
  for (int8_t i = 1; i < MAX_SNAKE_LEN; i++) {
    if (snake[i].radius == 0) {
      // Add new bodypart outside of the screen
      struct GameObject n_body = {-10, -10, SNAKE_BODY_RADIUS};
      snake[i].x_pos = n_body.x_pos;
      snake[i].y_pos = n_body.y_pos;
      snake[i].radius = n_body.radius;
      break;
    }
  }
}


void add_fruit() {
	int8_t r = random(0, (sizeof(FRUIT_TABLE) / sizeof(uint8_t)));
	
	// Using constant table we can easily get desired output that is easier to test:
	fruit.x_pos = FRUIT_TABLE[ score % (sizeof(FRUIT_TABLE) / sizeof(uint8_t))];
	fruit.y_pos = FRUIT_TABLE[ r %     (sizeof(FRUIT_TABLE) / sizeof(uint8_t))];
}

/*
 	Function to handle moving the snake (x,y) coordinates
	and check game over conditions while we are at it.

	If somebody wants to make a more efficient version such as "circle"-like structure
	go ahead.
*/
void move_snake() {
	static int8_t prev_x, prev_y;
	
	// Snake shouldn't be able to "reverse" where it came from:
	if (!((0 > dir.x * prev_x) || (0 > dir.y * prev_y))) {
		prev_x = dir.x;
		prev_y = dir.y;
	}
	
	int8_t n_x = snake[0].x_pos + prev_x * GRID_SLOT_SIDE;
	int8_t n_y = snake[0].y_pos + prev_y * GRID_SLOT_SIDE;
	
	// Checks whether the snake goes out of GAME_AREA bounds:
	if (GAME_AREA.x_offset > n_x || GAME_AREA.w < n_x || GAME_AREA.y_offset > n_y || GAME_AREA.h < n_y) {
		is_game_over = 1;
		return;
	}
	
  // Check if we can collect fruit:
	if (fruit.x_pos == n_x && fruit.y_pos == n_y) {
		score++;
		add_snake_part();
		add_fruit();
	}

	for (int8_t i = MAX_SNAKE_LEN; i > 0; i--) {
		// As we iterate backwards through the snake, we move only parts that need moving:
		if (snake[i].radius == 0) {
			continue;
		}
		// Does indeed update the head size to body as well:
		// snake[i] = snake[i-1];
		/* This does not */
		snake[i].x_pos = snake[i-1].x_pos;
		snake[i].y_pos = snake[i-1].y_pos;
		
		// While we are at it, check the self collision 
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
    u8g2.setCursor(SCORE_AREA.x_offset + 10 + 8*5 + 4, SCORE_AREA.y_offset + 14);
    u8g2.print(dir.x);
  
    // Draw game frame:
    u8g2.drawFrame(GAME_AREA.x_offset, GAME_AREA.y_offset, GAME_AREA.w, GAME_AREA.h);
    
    // Draw the actual snake:
    for (uint8_t i = 0; i < MAX_SNAKE_LEN; i++) {
    	u8g2.drawCircle(snake[i].x_pos, snake[i].y_pos, snake[i].radius);
    }

  } while(u8g2.nextPage() && !is_game_over);
}