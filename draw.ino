#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>


MCUFRIEND_kbv tft;


const int XP = 8, XM = A2, YP = A3, YM = 9;  // Touch pins
const int TS_LEFT = 186, TS_RT = 974, TS_TOP = 963, TS_BOT = 205; // Your calibration


TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;


#define MINPRESSURE 200
#define MAXPRESSURE 1000


int16_t BOXSIZE;
int16_t PENRADIUS = 3; // bigger pen size for better visibility
uint16_t ID, oldcolor, currentcolor;


#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF


void setup() {
   tft.reset();
   ID = tft.readID();
   tft.begin(ID);
   tft.setRotation(0); // Portrait
   tft.fillScreen(BLACK);


   // Color palette at the top
   BOXSIZE = tft.width() / 6;
   tft.fillRect(0, 0, BOXSIZE, BOXSIZE, RED);
   tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, YELLOW);
   tft.fillRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, GREEN);
   tft.fillRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, CYAN);
   tft.fillRect(BOXSIZE * 4, 0, BOXSIZE, BOXSIZE, BLUE);
   tft.fillRect(BOXSIZE * 5, 0, BOXSIZE, BOXSIZE, MAGENTA);


   tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE); // highlight default
   currentcolor = RED;
}


void loop() {
   tp = ts.getPoint();
   pinMode(XM, OUTPUT);
   pinMode(YP, OUTPUT);


   if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE) {
       int16_t xpos = map(tp.x, TS_LEFT, TS_RT, 0, tft.width());
       int16_t ypos = map(tp.y, TS_TOP, TS_BOT, 0, tft.height());


       // Color selection
       if (ypos < BOXSIZE) {
           oldcolor = currentcolor;


           if (xpos < BOXSIZE) currentcolor = RED;
           else if (xpos < BOXSIZE * 2) currentcolor = YELLOW;
           else if (xpos < BOXSIZE * 3) currentcolor = GREEN;
           else if (xpos < BOXSIZE * 4) currentcolor = CYAN;
           else if (xpos < BOXSIZE * 5) currentcolor = BLUE;
           else currentcolor = MAGENTA;


           if (oldcolor != currentcolor) {
               // Redraw old box border
               if (oldcolor == RED) tft.drawRect(0, 0, BOXSIZE, BOXSIZE, BLACK);
               else if (oldcolor == YELLOW) tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, BLACK);
               else if (oldcolor == GREEN) tft.drawRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, BLACK);
               else if (oldcolor == CYAN) tft.drawRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, BLACK);
               else if (oldcolor == BLUE) tft.drawRect(BOXSIZE * 4, 0, BOXSIZE, BOXSIZE, BLACK);
               else if (oldcolor == MAGENTA) tft.drawRect(BOXSIZE * 5, 0, BOXSIZE, BOXSIZE, BLACK);


               // Highlight new selection
               if (currentcolor == RED) tft.drawRect(0, 0, BOXSIZE, BOXSIZE, WHITE);
               else if (currentcolor == YELLOW) tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, WHITE);
               else if (currentcolor == GREEN) tft.drawRect(BOXSIZE * 2, 0, BOXSIZE, BOXSIZE, WHITE);
               else if (currentcolor == CYAN) tft.drawRect(BOXSIZE * 3, 0, BOXSIZE, BOXSIZE, WHITE);
               else if (currentcolor == BLUE) tft.drawRect(BOXSIZE * 4, 0, BOXSIZE, BOXSIZE, WHITE);
               else if (currentcolor == MAGENTA) tft.drawRect(BOXSIZE * 5, 0, BOXSIZE, BOXSIZE, WHITE);
           }
       }


       // Drawing area
       if (((ypos - PENRADIUS) > BOXSIZE) && ((ypos + PENRADIUS) < tft.height())) {
           tft.fillCircle(xpos, ypos, PENRADIUS, currentcolor);
       }
   }
}






