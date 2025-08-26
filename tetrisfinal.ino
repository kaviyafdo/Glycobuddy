#include <SPI.h>
#include <MCUFRIEND_kbv.h>
#include <Adafruit_GFX.h>
#include <TouchScreen.h>

#define TFT_WIDTH  320
#define TFT_HEIGHT 480
MCUFRIEND_kbv tft;

// --- COLORS ---
#define WHITE       0xFFFF
#define BLACK       0x0000
#define YELLOW      0xFFE0
#define DARKGREY    0x7BEF
#define BLUE        0x001F
#define RED         0xF800
#define GREEN       0x07E0
#define LIGHTGREY   0xC618
#define HS_BG_COLOR 0x9E3D
#define BG_COLOR    0x21B0

// --- TOUCH ---
const int TS_LEFT = 186, TS_RIGHT = 974, TS_TOP = 963, TS_BOTTOM = 205;
#define YP A3
#define XM A2
#define YM 9
#define XP 8
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// --- Tetris ---
#define GRID_COLS 8
#define GRID_ROWS 7
#define BLOCK_SIZE 36
#define GRID_WIDTH  (GRID_COLS * BLOCK_SIZE)
#define GRID_HEIGHT (GRID_ROWS * BLOCK_SIZE)
#define GRID_OFFSET_X ((TFT_WIDTH - GRID_WIDTH) / 2)
#define GRID_OFFSET_Y 60  // moved up

#define BTN_AREA_Y (GRID_OFFSET_Y + GRID_HEIGHT + 10)
#define BTN_WIDTH  60
#define BTN_HEIGHT 60
#define BTN_GAP    20
#define ARROW_BG_Y BTN_AREA_Y
#define ARROW_BG_H BTN_HEIGHT + 10

uint16_t blockColors[] = {BLUE, GREEN, RED, YELLOW, WHITE, LIGHTGREY};
#define NUM_COLORS (sizeof(blockColors)/sizeof(blockColors[0]))
byte grid[GRID_ROWS][GRID_COLS] = {0};

int tetrisScore = 0;
bool isGameOver = false;
bool isPaused = false;
int blockX, blockY;
uint8_t blockColorIdx = 0;

// Buttons
#define BTN_PAUSE_X (TFT_WIDTH - 50)
#define BTN_PAUSE_Y 10
#define BTN_PAUSE_W 28
#define BTN_PAUSE_H 28

#define BTN_HS_X 60
#define BTN_HS_Y (TFT_HEIGHT - 60)
#define BTN_HS_W 200
#define BTN_HS_H 40

#define BTN_BACK_X 60
#define BTN_BACK_Y (TFT_HEIGHT - 60)
#define BTN_BACK_W 200
#define BTN_BACK_H 40

bool onHighscores = false;
int numSampleScores = 30;
int sampleScores[30] = {120, 300, 450, 90, 60, 250, 180, 500, 340, 410,
                        220, 310, 470, 380, 270, 130, 390, 440, 210, 330,
                        150, 360, 420, 280, 350, 390, 410, 200, 320, 450};

// --- Utility ---
bool touchInRect(int x, int y, int w, int h, int tx, int ty) { 
    return tx > x && tx < x+w && ty > y && ty < y+h; 
}

// --- Drawing ---
void drawPauseButton(bool paused) {
    if (!paused) {
        tft.fillRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, LIGHTGREY);
        tft.fillRect(BTN_PAUSE_X+6, BTN_PAUSE_Y+6, 6, BTN_PAUSE_H-12, BLUE);
        tft.fillRect(BTN_PAUSE_X+16, BTN_PAUSE_Y+6, 6, BTN_PAUSE_H-12, BLUE);
    } else {
        tft.fillRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, LIGHTGREY);
        tft.fillTriangle(BTN_PAUSE_X+7, BTN_PAUSE_Y+6,
                         BTN_PAUSE_X+7, BTN_PAUSE_Y+BTN_PAUSE_H-6,
                         BTN_PAUSE_X+BTN_PAUSE_W-7, BTN_PAUSE_Y+BTN_PAUSE_H/2,
                         BLUE);
    }
    tft.drawRect(BTN_PAUSE_X, BTN_PAUSE_Y, BTN_PAUSE_W, BTN_PAUSE_H, DARKGREY);
}

void drawTetrisButtons() {
    tft.fillRect(0, ARROW_BG_Y, TFT_WIDTH, ARROW_BG_H, LIGHTGREY);
    int ly = ARROW_BG_Y + (BTN_HEIGHT/2) + 5;
    tft.fillTriangle(BTN_GAP, ly, BTN_GAP + BTN_WIDTH, ly - BTN_HEIGHT/2, BTN_GAP + BTN_WIDTH, ly + BTN_HEIGHT/2, BLUE);
    int rx = TFT_WIDTH - BTN_GAP;
    tft.fillTriangle(rx, ly, rx - BTN_WIDTH, ly - BTN_HEIGHT/2, rx - BTN_WIDTH, ly + BTN_HEIGHT/2, RED);
    int down_cx = TFT_WIDTH / 2;
    tft.fillTriangle(down_cx, ly + BTN_HEIGHT/2, down_cx - BTN_WIDTH/2, ly - BTN_HEIGHT/2, down_cx + BTN_WIDTH/2, ly - BTN_HEIGHT/2, GREEN);
}

void drawTetrisGrid() {
    tft.fillScreen(BG_COLOR);
    tft.drawRect(GRID_OFFSET_X - 1, GRID_OFFSET_Y - 1, GRID_WIDTH + 2, GRID_HEIGHT + 2, WHITE);
    for (int i = 0; i < GRID_ROWS; i++)
        for (int j = 0; j < GRID_COLS; j++) {
            int c = grid[i][j] ? blockColors[grid[i][j]-1] : DARKGREY;
            tft.fillRect(GRID_OFFSET_X + j*BLOCK_SIZE + 1, GRID_OFFSET_Y + i*BLOCK_SIZE + 1, BLOCK_SIZE-2, BLOCK_SIZE-2, c);
        }
}

void drawTetrisScore() {
    tft.fillRect(120, 10, 90, 28, BG_COLOR);
    tft.setCursor(130, 15);
    tft.setTextColor(WHITE, BG_COLOR);
    tft.setTextSize(2);
    tft.print("Score:");
    tft.setCursor(210, 15);
    tft.print(tetrisScore);
}

void drawHighscoresButton() {
    tft.fillRect(BTN_HS_X, BTN_HS_Y, BTN_HS_W, BTN_HS_H, HS_BG_COLOR);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    tft.setCursor(BTN_HS_X + 40, BTN_HS_Y + 10);
    tft.print("Highscores");
}

void drawTetrisScreen() {
    drawTetrisGrid();
    drawTetrisButtons();
    drawPauseButton(false);
    drawTetrisScore();
    drawHighscoresButton();
    if (isGameOver) {
        tft.fillScreen(RED);
        tft.setTextColor(WHITE);
        tft.setTextSize(4);
        tft.setCursor(40, 200);
        tft.print("GAME OVER");
    }
}

// --- Gameplay ---
void drawBlock(int row, int col, uint8_t colorIdx) {
    tft.fillRect(GRID_OFFSET_X + col*BLOCK_SIZE + 1, GRID_OFFSET_Y + row*BLOCK_SIZE + 1, BLOCK_SIZE-2, BLOCK_SIZE-2, blockColors[colorIdx]);
}

void eraseCell(int row, int col) {
    tft.fillRect(GRID_OFFSET_X + col*BLOCK_SIZE + 1, GRID_OFFSET_Y + row*BLOCK_SIZE + 1, BLOCK_SIZE-2, BLOCK_SIZE-2, DARKGREY);
    grid[row][col] = 0;
}

// Gravity: drop blocks to fill empty space
void applyGravity() {
    for (int col = 0; col < GRID_COLS; col++) {
        for (int row = GRID_ROWS-2; row >= 0; row--) {
            if (grid[row][col] && grid[row+1][col]==0) {
                grid[row+1][col] = grid[row][col];
                grid[row][col] = 0;
                drawBlock(row+1, col, grid[row+1][col]-1);
                eraseCell(row, col);
            }
        }
    }
}

// Clear same-color neighbors
void checkAndClear(int row, int col) {
    int colorIdx = grid[row][col];
    if (!colorIdx) return;
    bool cleared = false;
    if (col > 0 && grid[row][col-1] == colorIdx) { eraseCell(row,col-1); eraseCell(row,col); cleared=true; }
    if (col < GRID_COLS-1 && grid[row][col+1] == colorIdx) { eraseCell(row,col+1); eraseCell(row,col); cleared=true; }
    if (row > 0 && grid[row-1][col] == colorIdx) { eraseCell(row-1,col); eraseCell(row,col); cleared=true; }
    if (row < GRID_ROWS-1 && grid[row+1][col] == colorIdx) { eraseCell(row+1,col); eraseCell(row,col); cleared=true; }
    if (cleared) applyGravity();
}

void spawnBlock() {
    blockX = GRID_COLS/2;
    blockY = 0;
    blockColorIdx = random(NUM_COLORS);
    if (grid[blockY][blockX]) { isGameOver=true; drawTetrisScreen(); return; }
    drawBlock(blockY, blockX, blockColorIdx);
}

void moveBlock(int dx) {
    int newX = blockX+dx;
    if (newX>=0 && newX<GRID_COLS && !grid[blockY][newX]) { eraseCell(blockY, blockX); blockX=newX; drawBlock(blockY,blockX,blockColorIdx); }
}

void dropBlock() {
    if (blockY+1<GRID_ROWS && !grid[blockY+1][blockX]) { eraseCell(blockY, blockX); blockY++; drawBlock(blockY, blockX, blockColorIdx); }
    else { grid[blockY][blockX] = blockColorIdx+1; tetrisScore+=1; drawTetrisScore(); checkAndClear(blockY,blockX); spawnBlock(); }
}

void startTetrisGame() {
    tft.fillScreen(BG_COLOR);
    memset(grid,0,sizeof(grid));
    tetrisScore=0;
    isGameOver=false;
    isPaused=false;
    drawTetrisScreen();
    spawnBlock();
    drawTetrisScore();
}

// --- Highscores Page ---
int scrollOffset = 0;
void drawHighscoresPage() {
    tft.fillScreen(HS_BG_COLOR);
    tft.setTextColor(WHITE);
    tft.setTextSize(2);
    int yBase = 20 - scrollOffset;
    for (int i=0;i<numSampleScores;i++) {
        int y = yBase + i*25;
        if (y>=0 && y<TFT_HEIGHT-50) {
            tft.setCursor(50,y);
            tft.print(i+1); tft.print(". "); tft.print(sampleScores[i]);
        }
    }
    tft.fillRect(BTN_BACK_X, BTN_BACK_Y, BTN_BACK_W, BTN_BACK_H, LIGHTGREY);
    tft.setTextColor(BLACK);
    tft.setCursor(BTN_BACK_X+70, BTN_BACK_Y+10);
    tft.print("Back");
}

// --- Setup/Loop ---
void setup() {
    Serial.begin(9600);
    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9481;
    tft.begin(ID);
    tft.setRotation(0);
    delay(200);
    startTetrisGame();
}

void loop() {
    TSPoint p = ts.getPoint();
    pinMode(XM, OUTPUT); pinMode(YP, OUTPUT);
    int tx=-1, ty=-1;
    if (p.z>ts.pressureThreshhold) { tx=map(p.x,TS_LEFT,TS_RIGHT,0,tft.width()); ty=map(p.y,TS_TOP,TS_BOTTOM,0,tft.height()); }

    if (onHighscores) {
        static int lastY=-1;
        if (tx>=0) {
            if (touchInRect(BTN_BACK_X,BTN_BACK_Y,BTN_BACK_W,BTN_BACK_H,tx,ty)) { onHighscores=false; drawTetrisScreen(); spawnBlock(); delay(300); lastY=-1; return; }
            if (lastY>=0) scrollOffset += lastY - ty;
            lastY = ty;
            if (scrollOffset<0) scrollOffset=0;
            if (scrollOffset>numSampleScores*25 - (TFT_HEIGHT-50)) scrollOffset = numSampleScores*25 - (TFT_HEIGHT-50);
            drawHighscoresPage();
        } else lastY=-1;
        return;
    }

    if (isGameOver) return;
    if (tx>=0 && touchInRect(BTN_PAUSE_X,BTN_PAUSE_Y,BTN_PAUSE_W,BTN_PAUSE_H,tx,ty)) { isPaused=!isPaused; drawPauseButton(isPaused); delay(300); return; }
    if (tx>=0 && touchInRect(BTN_HS_X,BTN_HS_Y,BTN_HS_W,BTN_HS_H,tx,ty)) { onHighscores=true; drawHighscoresPage(); delay(300); return; }
    if (isPaused) return;
    if (ty>ARROW_BG_Y && ty<ARROW_BG_Y+ARROW_BG_H) { int third=TFT_WIDTH/3; if(tx<third) moveBlock(-1); else if(tx>2*third) moveBlock(1); else dropBlock(); delay(160); }

    static unsigned long lastDrop=0;
    if (millis()-lastDrop>320) { lastDrop=millis(); dropBlock(); }
}
