#include <Arduino.h>
#include "Adafruit_GFX.h"
#include "MCUFRIEND_kbv.h"
#include "TouchScreen.h"

// ----- Color constants -----
const uint16_t RED     = 0xF800;
const uint16_t GREEN   = 0x07E0;
const uint16_t BLUE    = 0x001F;
const uint16_t YELLOW  = 0xFFE0;
const uint16_t WHITE   = 0xFFFF;
const uint16_t GRAY    = 0x8410;
const uint16_t BLACK   = 0x0000;

// ----- Display constants -----
const uint16_t DISPLAY_WIDTH   = 320;
const uint16_t DISPLAY_HEIGHT  = 480;

// ----- Touchscreen constants -----
const uint16_t PRESSURE_LEFT = 10;
const uint16_t PRESSURE_RIGHT = 1200;

const int XP = 8, XM = A2, YP = A3, YM = 9; // TFT shield pins
TouchScreen ts(XP, YP, XM, YM, 300);

// Calibration values from your test
const int XBEGIN = 183;
const int XEND   = 986;
const int YBEGIN = 942;
const int YEND   = 193;

// ----- Game constants -----
const uint16_t GRID_W = 240;
const uint16_t CELL_W = GRID_W / 3;

const uint16_t GRID_X = (DISPLAY_WIDTH - GRID_W) / 2;
const uint16_t GRID_Y = (DISPLAY_HEIGHT - GRID_W) / 2;

const int16_t NONE = -1;
const int16_t NAUGHT = 1;
const int16_t CROSS = 0;

const uint16_t CROSS_COLOR = RED;
const uint16_t NAUGHT_COLOR = BLUE;
const uint16_t TIE_COLOR = GRAY;

const uint16_t MARK_W = CELL_W - 40;
const uint16_t MARK_THICKNESS = 9;

const uint16_t MENU_W = 240;
const uint16_t MENU_HEIGHT = 320;

const uint16_t MENU_X = (DISPLAY_WIDTH - MENU_W) / 2;
const uint16_t MENU_Y = (DISPLAY_HEIGHT - MENU_HEIGHT) / 2;

const uint16_t ITEM_W = 200;
const uint16_t ITEM_H = 40;
const uint16_t ITEM_X = MENU_X + (MENU_W - ITEM_W) / 2;

const uint16_t WINNER_X = 25;
const uint16_t WINNER_Y = GRID_Y + GRID_W + 40;
const uint16_t TIE_X = GRID_X + CELL_W + 10;
const uint16_t TIE_Y = GRID_Y + GRID_W + 40;

MCUFRIEND_kbv tft;

int16_t player = NONE;
int16_t turn = CROSS;
uint16_t moves = 0;

int16_t grid[3][3] = {
    {NONE, NONE, NONE},
    {NONE, NONE, NONE},
    {NONE, NONE, NONE},
};

// ----- Helper -----
bool in_range(uint16_t value, uint16_t lo, uint16_t hi) {
    return lo <= value && value <= hi;
}

void to_display_mode() {
    pinMode(XM, OUTPUT);
    pinMode(XP, OUTPUT);
    pinMode(YM, OUTPUT);
    pinMode(YP, OUTPUT);
}

void convert_touch_coors(uint16_t tx, uint16_t ty, uint16_t *xptr, uint16_t *yptr) {
    tx = constrain(tx, XBEGIN, XEND);
    ty = constrain(ty, YEND, YBEGIN);

    *xptr = map(tx, XBEGIN, XEND, 0, DISPLAY_WIDTH - 1);
    *yptr = map(ty, YBEGIN, YEND, 0, DISPLAY_HEIGHT - 1);
}

void get_touch_coors(uint16_t *xptr, uint16_t *yptr) {
    TSPoint p;

    for (;;) {
        p = ts.getPoint();
        if (in_range(p.z, PRESSURE_LEFT, PRESSURE_RIGHT)) {
            break;
        }
    }
    convert_touch_coors(p.x, p.y, xptr, yptr);
    to_display_mode();
}

bool get_grid_touch_coors(uint16_t x, uint16_t y, uint16_t *grid_x, uint16_t *grid_y) {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (in_range(x - GRID_X - i * CELL_W, 0, CELL_W)
                && in_range(y - GRID_Y - j * CELL_W, 0, CELL_W)) {
                if (grid[i][j] == NONE) {
                    *grid_x = i, *grid_y = j;
                    return true;
                }
                break;
            }
        }
    }
    return false;
}

// ----- Drawing -----
void draw_thick_hline(uint16_t x, uint16_t y, uint16_t d, uint16_t clr) {
    const int LINE_WIDTH = 7;
    tft.fillRect(x, y - LINE_WIDTH / 2, d, LINE_WIDTH, clr);
    tft.fillCircle(x, y, LINE_WIDTH / 2, clr);
    tft.fillCircle(x + d, y, LINE_WIDTH / 2, clr);
}

void draw_thick_vline(uint16_t x, uint16_t y, uint16_t d, uint16_t clr) {
    const int LINE_WIDTH = 7;
    tft.fillRect(x - LINE_WIDTH / 2, y, LINE_WIDTH, d, clr);
    tft.fillCircle(x, y, LINE_WIDTH / 2, clr);
    tft.fillCircle(x, y + d, LINE_WIDTH / 2, clr);
}

void draw_naught(uint16_t x, uint16_t y, uint16_t clr) {
    tft.fillCircle(x, y, MARK_W / 2, clr);
    tft.fillCircle(x, y, (MARK_W - MARK_THICKNESS) / 2, BLACK);
}

void draw_cross(uint16_t x, uint16_t y, uint16_t clr) {
    const int LINE_WIDTH = MARK_THICKNESS;

    for (int i = -LINE_WIDTH / 2; i <= LINE_WIDTH / 2; i++) {
        // Diagonal from top-left to bottom-right
        tft.drawLine(x - MARK_W / 2 + i, y - MARK_W / 2, 
                     x + MARK_W / 2 + i, y + MARK_W / 2, clr);

        // Diagonal from bottom-left to top-right
        tft.drawLine(x - MARK_W / 2 + i, y + MARK_W / 2, 
                     x + MARK_W / 2 + i, y - MARK_W / 2, clr);
    }
}


void draw_menu_item(uint16_t offset, uint16_t clr, const char msg[]) {
    tft.drawRect(ITEM_X, MENU_Y + offset, ITEM_W, ITEM_H, clr);
    tft.setTextSize(3);
    tft.setTextColor(clr, BLACK);
    tft.setCursor(ITEM_X, MENU_Y + offset);
    tft.println(msg);
}

void draw_menu() {
    tft.drawRect(MENU_X, MENU_Y, MENU_W, MENU_HEIGHT, YELLOW);
    tft.setTextSize(3);
    tft.setTextColor(YELLOW);
    tft.setCursor(60, 20);
    tft.print("TIC-TAC-TOE");
    draw_menu_item(ITEM_H * 1, RED, "PLAY AS X");
    draw_menu_item(ITEM_H * 3, BLUE, "PLAY AS O");
}

void draw_grid() {
    draw_thick_hline(GRID_X, GRID_Y + CELL_W, GRID_W, WHITE);
    draw_thick_hline(GRID_X, GRID_Y + 2 * CELL_W, GRID_W, WHITE);
    draw_thick_vline(GRID_X + CELL_W, GRID_Y, GRID_W, WHITE);
    draw_thick_vline(GRID_X + 2 * CELL_W, GRID_Y, GRID_W, WHITE);
}

void draw_winner(int16_t winner) {
    tft.setTextSize(3);
    tft.setTextColor((winner == CROSS) ? RED : BLUE);
    tft.setCursor(WINNER_X, WINNER_Y);
    tft.print((winner == player) ? "PLAYER " : "ARDUINO ");
    tft.print((winner == CROSS) ? "(X) WINS" : "(O) WINS");
    tft.setCursor(80, 40);
    tft.print("GAME OVER");
}

void draw_tie() {
    tft.setTextSize(3);
    tft.setTextColor(GRAY);
    tft.setCursor(TIE_X, TIE_Y);
    tft.print("TIE");
    tft.setCursor(80, 40);
    tft.print("GAME OVER");
}

// ----- Game logic -----
void choose_piece() {
    uint16_t x, y;
    draw_menu();
    for (;;) {
        get_touch_coors(&x, &y);
        if (!in_range(x, ITEM_X, ITEM_X + ITEM_W)) continue;
        if (in_range(y, MENU_Y + ITEM_H * 1, MENU_Y + ITEM_H * 2)) {
            player = CROSS;
            break;
        }
        if (in_range(y, MENU_Y + ITEM_H * 3, MENU_Y + ITEM_H * 4)) {
            player = NAUGHT;
            break;
        }
    }
}

uint16_t get_winner() {
    for (uint16_t i = 0; i < 3; ++i) {
        if (grid[i][0] != NONE && grid[i][0] == grid[i][1] && grid[i][1] == grid[i][2])
            return grid[i][0];
        if (grid[0][i] != NONE && grid[0][i] == grid[1][i] && grid[1][i] == grid[2][i])
            return grid[0][i];
    }
    if (grid[0][0] != NONE && grid[0][0] == grid[1][1] && grid[1][1] == grid[2][2]) return grid[0][0];
    if (grid[2][0] != NONE && grid[2][0] == grid[1][1] && grid[1][1] == grid[0][2]) return grid[2][0];
    return NONE;
}

bool get_tie() { return ++moves == 9; }

// ----- Setup and loop -----
void setup() {
    Serial.begin(115200);
    tft.begin(0x9488);
    tft.setRotation(0);
    tft.fillScreen(BLACK);
    choose_piece();
    tft.fillScreen(BLACK);
    delay(300);
    draw_grid();
}

void loop() {
    uint16_t grid_x, grid_y;
    uint16_t x, y;
    int16_t winner;

    if (turn != player) {
        delay(1000);
        do {
            grid_x = random(0, 3);
            grid_y = random(0, 3);
        } while (grid[grid_x][grid_y] != NONE);
    } else {
        do {
            get_touch_coors(&x, &y);
        } while (!get_grid_touch_coors(x, y, &grid_x, &grid_y));
    }

    grid[grid_x][grid_y] = turn;
    (turn == CROSS)
        ? draw_cross(GRID_X + CELL_W / 2 + grid_x * CELL_W, GRID_Y + CELL_W / 2 + grid_y * CELL_W, RED)
        : draw_naught(GRID_X + CELL_W / 2 + grid_x * CELL_W, GRID_Y + CELL_W / 2 + grid_y * CELL_W, BLUE);

    if ((winner = get_winner()) != NONE) {
        draw_winner(winner);
        while (true);
    }

    if (get_tie()) {
        draw_tie();
        while (true);
    }

    turn = NAUGHT + CROSS - turn;
}
