#include <SPI.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>


#define TFT_WIDTH  320
#define TFT_HEIGHT 480
MCUFRIEND_kbv tft;


// --- COLORS ---
#define LIGHTBLUE   0x9E3D
#define ACCENT      0x1A4C
#define TOP_COLOR   0x11B4
#define BOTTOM_COLOR 0x0845
#define WHITE       0xFFFF
#define BLACK       0x0000
#define YELLOW      0xFFE0
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define LIGHTGREY   0xC618


// --- TOUCH ---
const int TS_LEFT = 186, TS_RIGHT = 974, TS_TOP = 963, TS_BOTTOM = 205;
#define YP A3
#define XM A2
#define YM 9
#define XP 8
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);


enum Screen { BOOT, HOME, TETRIS, HIGHSCORES };
Screen currentScreen = BOOT;


int cx, cy;
#define TEARDROP_R  (TFT_WIDTH/3)
#define PENGUIN_SIZE 40


// --- Tabs ---
#define TAB_H 38
#define TAB_W 220
#define TAB_X ((TFT_WIDTH-TAB_W)/2)
#define TAB_Y 5
#define BOTTOM_TAB_H 38
#define BOTTOM_TAB_W 150
#define BOTTOM_TAB_X ((TFT_WIDTH-BOTTOM_TAB_W)/2)
#define BOTTOM_TAB_Y (TFT_HEIGHT-BOTTOM_TAB_H-8)


// --- Penguin Bitmap (must be RGB565, 40x40, 1600 values) ---
extern const uint8_t penguin[]; // use your image2cpp array here


bool penguinVisible = true; // Only one is visible at a time


// --- Tetris ---
#define GRID_COLS 8
#define GRID_ROWS 7
#define BLOCK_SIZE 36
#define GRID_WIDTH  (GRID_COLS * BLOCK_SIZE)
#define GRID_HEIGHT (GRID_ROWS * BLOCK_SIZE)
#define GRID_OFFSET_X ((TFT_WIDTH - GRID_WIDTH) / 2)
#define GRID_OFFSET_Y 62


uint16_t blockColors[] = {BLUE, GREEN, RED, YELLOW, WHITE, LIGHTBLUE, ACCENT};
#define NUM_COLORS (sizeof(blockColors)/sizeof(blockColors[0]))
byte grid[GRID_ROWS][GRID_COLS] = {0};
int tetrisScore = 0;
bool isGameOver = false;
bool isPaused = false;
int blockX, blockY;
uint8_t blockColorIdx = 0;


#define BTN_PAUSE_X (TFT_WIDTH - 70)
#define BTN_PAUSE_Y (GRID_OFFSET_Y - 34)
#define BTN_PAUSE_W 28
#define BTN_PAUSE_H 28
#define BTN_HOME_X 12
#define BTN_HOME_Y (GRID_OFFSET_Y - 34)
#define BTN_HOME_W 28
#define BTN_HOME_H 28


#define BTN_AREA_Y (GRID_OFFSET_Y + GRID_HEIGHT + 24)
#define BTN_WIDTH  60
#define BTN_HEIGHT 60
#define BTN_GAP    20
#define ARROW_BG_Y BTN_AREA_Y
#define ARROW_BG_H BTN_HEIGHT + 10


// --- Highscores ---
#define MAX_SCORES 9
int highScores[MAX_SCORES] = {132, 127, 114, 101, 89, 87, 65, 44, 32};
int numScores = 9;


// --- Utility ---
bool touchInRect(int x, int y, int w, int h, int tx, int ty) { return tx > x && tx < x+w && ty > y && ty < y+h; }
bool touchInCircle(int x, int y, int r, int tx, int ty) { int dx = tx-x, dy = ty-y; return (dx*dx+dy*dy)<(r*r); }


// --- Boot ---
void drawGradientBackground() {
   int w = tft.width(), h = tft.height();
   for (int y = 0; y < h; y++) {
       uint8_t rTop = (TOP_COLOR >> 11) & 0x1F, gTop = (TOP_COLOR >> 5) & 0x3F, bTop = TOP_COLOR & 0x1F;
       uint8_t rBot = (BOTTOM_COLOR >> 11) & 0x1F, gBot = (BOTTOM_COLOR >> 5) & 0x3F, bBot = BOTTOM_COLOR & 0x1F;
       float ratio = (float)y / h;
       uint8_t r = rTop + (rBot - rTop) * ratio, g = gTop + (gBot - gTop) * ratio, b = bTop + (bBot - bTop) * ratio;
       uint16_t color = (r << 11) | (g << 5) | b;
       tft.drawFastHLine(0, y, w, color);
   }
}
void drawTeardropOutline(float scale, int steps, uint16_t color) {
   float yMin = -1.0, yMax = 1.0;
   for (int i = 0; i <= steps; i++) {
       float y = yMin + (yMax - yMin) * i / steps;
       float rhs = (1 - y) * (1 - y) * (1 - y) * (1 + y);
       float x = sqrt(rhs / 4.0);
       float flippedY = -y;
       int py = cy + (int)(flippedY * scale);
       int pxRight = cx + (int)(x * scale);
       int pxLeft  = cx - (int)(x * scale);
       tft.drawPixel(pxRight, py, color);
       tft.drawPixel(pxLeft, py, color);
   }
}
void fillTeardrop(float scale, int steps, uint16_t color) {
   float yMin = -1.0, yMax = 1.0;
   for (int i = 0; i <= steps; i++) {
       float y = yMin + (yMax - yMin) * i / steps;
       float rhs = (1 - y) * (1 - y) * (1 - y) * (1 + y);
       float x = sqrt(rhs / 4.0);
       float flippedY = -y;
       int py = cy + (int)(flippedY * scale);
       int pxRight = cx + (int)(x * scale);
       int pxLeft  = cx - (int)(x * scale);
       if (pxRight >= pxLeft) tft.drawFastHLine(pxLeft, py, pxRight - pxLeft + 1, color);
   }
}
void showBootScreen() {
   drawGradientBackground();
   fillTeardrop(TEARDROP_R, 600, WHITE);
   drawTeardropOutline(TEARDROP_R, 600, WHITE);
   tft.setTextColor(ACCENT);
   tft.setTextSize(2);
   tft.setCursor(cx - 52, cy + 36);
   tft.print("GlycoBuddy");
   delay(2000);
   currentScreen = HOME;
   drawHomeScreen();
}


// --- Tabs ---
void drawTopTab() {
   tft.fillRoundRect(TAB_X, TAB_Y, TAB_W, TAB_H, 17, LIGHTBLUE);
   tft.drawRoundRect(TAB_X, TAB_Y, TAB_W, TAB_H, 17, ACCENT);
   tft.setTextColor(ACCENT);
   tft.setTextSize(2);
   int16_t x1, y1;
   uint16_t w, h;
   tft.getTextBounds("GlycoBuddy", 0, 0, &x1, &y1, &w, &h);
   int mid = TAB_X + (TAB_W-w)/2;
   tft.setCursor(mid, TAB_Y + (TAB_H-h)/2 + 2);
   tft.print("SugarStack");
}


void drawBottomTab(const char* label) {
   tft.fillRoundRect(BOTTOM_TAB_X, BOTTOM_TAB_Y, BOTTOM_TAB_W, BOTTOM_TAB_H, 15, LIGHTBLUE);
   tft.drawRoundRect(BOTTOM_TAB_X, BOTTOM_TAB_Y, BOTTOM_TAB_W, BOTTOM_TAB_H, 15, ACCENT);
   tft.setTextColor(ACCENT);
   tft.setTextSize(2);
   int16_t x1, y1;
   uint16_t w, h;
   tft.getTextBounds(label, 0, 0, &x1, &y1, &w, &h);
   int mid = BOTTOM_TAB_X + (BOTTOM_TAB_W-w)/2;
   tft.setCursor(mid, BOTTOM_TAB_Y + (BOTTOM_TAB_H-h)/2 + 2);
   tft.print(label);
}


void drawPenguinOrReading() {
   tft.fillCircle(cx, cy, PENGUIN_SIZE/2+2, WHITE);
   if (penguinVisible) {
       int x = cx - PENGUIN_SIZE/2;
       int y = cy - PENGUIN_SIZE/2;
       tft.setAddrWindow(x, y, x + PENGUIN_SIZE - 1, y + PENGUIN_SIZE - 1);
       tft.pushColors(penguin, PENGUIN_SIZE*PENGUIN_SIZE, 1);
   } else {
       tft.setTextColor(ACCENT);
       tft.setTextSize(2);
       tft.setCursor(cx - 18, cy + 2);
       tft.print("7.6");
       tft.setTextSize(1);
       tft.setCursor(cx - 26, cy + 22);
       tft.print("mmol/L");
   }
}


void drawHomeScreen() {
   drawGradientBackground();
   fillTeardrop(TEARDROP_R, 600, WHITE);
   drawTeardropOutline(TEARDROP_R, 600, WHITE);
   drawTopTab();
   drawBottomTab("High Scores");
   drawPenguinOrReading();
}


// --- Tetris ---
void drawPauseButton(bool paused) {
   if (!paused) {
       tft.fillRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, LIGHTGREY);
       tft.fillRect(BTN_PAUSE_X+6, BTN_PAUSE_Y+6, 6, BTN_PAUSE_H-12, BLUE);
       tft.fillRect(BTN_PAUSE_X+16, BTN_PAUSE_Y+6, 6, BTN_PAUSE_H-12, BLUE);
       tft.drawRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, DARKGREY);
   } else {
       tft.fillRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, LIGHTGREY);
       tft.fillTriangle(
           BTN_PAUSE_X+7, BTN_PAUSE_Y+6,
           BTN_PAUSE_X+7, BTN_PAUSE_Y+BTN_PAUSE_H-6,
           BTN_PAUSE_X+BTN_PAUSE_W-7, BTN_PAUSE_Y+BTN_PAUSE_H/2,
           BLUE
       );
       tft.drawRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, DARKGREY);
   }
}
void drawHomeButton() {
   tft.fillRect(BTN_HOME_X, BTN_HOME_Y, BTN_HOME_W, BTN_HOME_H, LIGHTGREY);
   tft.fillTriangle(
       BTN_HOME_X+BTN_HOME_W/2, BTN_HOME_Y+4,
       BTN_HOME_X+4, BTN_HOME_Y+BTN_HOME_H/2+2,
       BTN_HOME_X+BTN_HOME_W-4, BTN_HOME_Y+BTN_HOME_H/2+2,
       RED
   );
   tft.fillRect(BTN_HOME_X+8, BTN_HOME_Y+BTN_HOME_H/2, BTN_HOME_W-16, BTN_HOME_H/2-4, BLUE);
   tft.drawRect(BTN_HOME_X, BTN_HOME_Y, BTN_HOME_W, BTN_HOME_H, DARKGREY);
}
void drawTetrisButtons() {
   tft.fillRect(0, ARROW_BG_Y, TFT_WIDTH, ARROW_BG_H, LIGHTGREY);
   int ly = ARROW_BG_Y + (BTN_HEIGHT/2) + 5;
   // Left arrow
   tft.fillTriangle(
       BTN_GAP, ly,
       BTN_GAP + BTN_WIDTH, ly - BTN_HEIGHT/2,
       BTN_GAP + BTN_WIDTH, ly + BTN_HEIGHT/2,
       BLUE
   );
   // Right arrow
   int rx = TFT_WIDTH - BTN_GAP;
   tft.fillTriangle(
       rx, ly,
       rx - BTN_WIDTH, ly - BTN_HEIGHT/2,
       rx - BTN_WIDTH, ly + BTN_HEIGHT/2,
       RED
   );
   // Down arrow
   int down_cx = TFT_WIDTH / 2;
   tft.fillTriangle(
       down_cx, ly + BTN_HEIGHT/2,
       down_cx - BTN_WIDTH/2, ly - BTN_HEIGHT/2,
       down_cx + BTN_WIDTH/2, ly - BTN_HEIGHT/2,
       GREEN
   );
   drawBottomTab("High Scores");
}
void drawTetrisGrid() {
   tft.fillRect(0, 0, TFT_WIDTH, ARROW_BG_Y, BLACK);
   tft.drawRect(GRID_OFFSET_X - 1, GRID_OFFSET_Y - 1, GRID_WIDTH + 2, GRID_HEIGHT + 2, WHITE);
   for (int i = 0; i < GRID_ROWS; i++) {
       for (int j = 0; j < GRID_COLS; j++) {
           int c = grid[i][j] ? blockColors[grid[i][j] - 1] : DARKGREY;
           int x = GRID_OFFSET_X + j * BLOCK_SIZE;
           int y = GRID_OFFSET_Y + i * BLOCK_SIZE;
           tft.fillRect(x+1, y+1, BLOCK_SIZE-2, BLOCK_SIZE-2, c);
       }
   }
}
void drawTetrisScore() {
   tft.fillRect(120, 0, 90, 28, BLACK);
   tft.setCursor(130, 5);
   tft.setTextColor(WHITE, BLACK);
   tft.setTextSize(2);
   tft.print("Score:");
   tft.setCursor(210, 5);
   tft.print(tetrisScore);
}
void drawScore() { drawTetrisScore(); }


void drawTetrisScreen() {
   drawTetrisGrid();
   drawTetrisButtons();
   drawPauseButton(false);
   drawHomeButton();
   drawTetrisScore();
   if (isGameOver) {
       tft.fillScreen(RED);
       tft.setTextColor(WHITE);
       tft.setTextSize(4);
       tft.setCursor(40, 200);
       tft.print("GAME OVER");
       drawBottomTab("Home");
   }
  
}


void startTetrisGame() {
   memset(grid, 0, sizeof(grid));
   tetrisScore = 0;
   isGameOver = false;
   isPaused = false;
   blockX = GRID_COLS / 2;
   blockY = 0;
   blockColorIdx = random(NUM_COLORS);
   drawTetrisScreen();
   drawBlock(blockY, blockX, blockColorIdx);
}


void drawBlock(int row, int col, uint8_t colorIdx) {
   int x = GRID_OFFSET_X + col * BLOCK_SIZE;
   int y = GRID_OFFSET_Y + row * BLOCK_SIZE;
   tft.fillRect(x+1, y+1, BLOCK_SIZE-2, BLOCK_SIZE-2, blockColors[colorIdx]);
}
void eraseCell(int row, int col) {
   int x = GRID_OFFSET_X + col * BLOCK_SIZE;
   int y = GRID_OFFSET_Y + row * BLOCK_SIZE;
   tft.fillRect(x+1, y+1, BLOCK_SIZE-2, BLOCK_SIZE-2, DARKGREY);
}


void spawnBlock() {
   blockX = GRID_COLS / 2;
   blockY = 0;
   blockColorIdx = random(NUM_COLORS);
   if (grid[blockY][blockX]) {
       isGameOver = true;
       drawTetrisScreen();
       return;
   }
   drawBlock(blockY, blockX, blockColorIdx);
}
void moveBlock(int dx) {
   int newX = blockX + dx;
   if (newX >= 0 && newX < GRID_COLS && !grid[blockY][newX]) {
       eraseCell(blockY, blockX);
       blockX = newX;
       drawBlock(blockY, blockX, blockColorIdx);
   }
}
void dropBlock() {
   if (blockY + 1 < GRID_ROWS && !grid[blockY+1][blockX]) {
       eraseCell(blockY, blockX);
       blockY++;
       drawBlock(blockY, blockX, blockColorIdx);
   } else {
       grid[blockY][blockX] = blockColorIdx + 1;
       tetrisScore += 1;
       drawScore();
       checkAndClear(blockY, blockX);
       spawnBlock();
   }
}
void checkAndClear(int row, int col) {
   int colorIdx = grid[row][col];
   // Horizontal check
   if (col > 0 && grid[row][col-1] == colorIdx) {
       eraseCell(row, col);
       eraseCell(row, col-1);
       grid[row][col] = 0;
       grid[row][col-1] = 0;
   }
   if (col < GRID_COLS-1 && grid[row][col+1] == colorIdx) {
       eraseCell(row, col);
       eraseCell(row, col+1);
       grid[row][col] = 0;
       grid[row][col+1] = 0;
   }
   // Vertical check
   if (row > 0 && grid[row-1][col] == colorIdx) {
       eraseCell(row, col);
       eraseCell(row-1, col);
       grid[row][col] = 0;
       grid[row-1][col] = 0;
   }
   if (row < GRID_ROWS-1 && grid[row+1][col] == colorIdx) {
       eraseCell(row, col);
       eraseCell(row+1, col);
       grid[row][col] = 0;
       grid[row+1][col] = 0;
   }
}


// --- Highscores ---
void drawHighScoresScreen() {
   drawGradientBackground();
   drawBottomTab("Home");
   tft.setTextSize(3);
   tft.setTextColor(WHITE);
   tft.setCursor(60, 30);
   tft.print("High Scores");
   int y0 = 100;
   tft.setTextSize(2);
   for (int i = 0; i < numScores; i++) {
       int y = y0 + i*38;
       tft.setCursor(70, y);
       tft.setTextColor(WHITE);
       tft.print(i+1);
       tft.print(". ");
       tft.print(highScores[i]);
   }
}


// --- Setup/Loop ---
void setup() {
   Serial.begin(9600);
   uint16_t ID = tft.readID();
   if (ID == 0xD3D3) ID = 0x9481;
   tft.begin(ID);
   tft.setRotation(0);
   cx = tft.width() / 2;
   cy = tft.height() / 2;
   showBootScreen();
}


void loop() {
   TSPoint p = ts.getPoint();
   pinMode(XM, OUTPUT);
   pinMode(YP, OUTPUT);
   int tx = -1, ty = -1;
   if (p.z > ts.pressureThreshhold) {
       tx = map(p.x, TS_LEFT, TS_RIGHT, 0, tft.width());
       ty = map(p.y, TS_TOP, TS_BOTTOM, 0, tft.height());
   }
   if (currentScreen == HOME) {
       if (tx >= 0 && touchInRect(TAB_X, TAB_Y, TAB_W, TAB_H, tx, ty)) {
           currentScreen = TETRIS;
           startTetrisGame();
           delay(350);
           return;
       }
       if (tx >= 0 && touchInRect(BOTTOM_TAB_X, BOTTOM_TAB_Y, BOTTOM_TAB_W, BOTTOM_TAB_H, tx, ty)) {
           currentScreen = HIGHSCORES;
           drawHighScoresScreen();
           delay(350);
           return;
       }
       if (tx >= 0 && touchInCircle(cx, cy, PENGUIN_SIZE/2+6, tx, ty)) {
           penguinVisible = !penguinVisible;
           drawPenguinOrReading();
           delay(350);
           return;
       }
   }
   else if (currentScreen == TETRIS) {
       if (tx >= 0 && touchInRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, tx, ty)) {
           isPaused = !isPaused;
           drawPauseButton(isPaused);
           if (isPaused) {
               tft.fillRect(40, 120, TFT_WIDTH-80, 100, DARKGREY);
               tft.setTextColor(WHITE, DARKGREY);
               tft.setTextSize(3);
               tft.setCursor(100, 150);
               tft.print("PAUSED");
           } else {
               drawTetrisScreen();
           }
           delay(350);
           return;
       }
       if (tx >= 0 && touchInRect(BTN_HOME_X, BTN_HOME_Y, BTN_HOME_W, BTN_HOME_H, tx, ty)) {
           currentScreen = HOME;
           drawHomeScreen();
           delay(350);
           return;
       }
       if (tx >= 0 && touchInRect(BOTTOM_TAB_X, BOTTOM_TAB_Y, BOTTOM_TAB_W, BOTTOM_TAB_H, tx, ty)) {
           if (isGameOver) {
               currentScreen = HOME;
               drawHomeScreen();
               delay(350);
               return;
           } else {
               currentScreen = HIGHSCORES;
               drawHighScoresScreen();
               delay(350);
               return;
           }
       }
       if (isPaused || isGameOver) {
           // Game over: allow pressing bottom Home tab
           if (isGameOver && tx >= 0 && touchInRect(BOTTOM_TAB_X, BOTTOM_TAB_Y, BOTTOM_TAB_W, BOTTOM_TAB_H, tx, ty)) {
               currentScreen = HOME;
               drawHomeScreen();
               delay(350);
               return;
           }
           return;
       }
       if (ty > ARROW_BG_Y && ty < ARROW_BG_Y + ARROW_BG_H) {
           int third = TFT_WIDTH / 3;
           if (tx < third) moveBlock(-1);
           else if (tx > 2*third) moveBlock(1);
           else dropBlock();
           delay(160);
       }
       static unsigned long lastDrop = 0;
       if (millis() - lastDrop > 320) {
           lastDrop = millis();
           dropBlock();
       }
   }
   else if (currentScreen == HIGHSCORES) {
       if (tx >= 0 && touchInRect(BOTTOM_TAB_X, BOTTOM_TAB_Y, BOTTOM_TAB_W, BOTTOM_TAB_H, tx, ty)) {
           currentScreen = HOME;
           drawHomeScreen();
           delay(350);
           return;
       }
   }
}
