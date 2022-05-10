#include <math.h>
#include <stdlib.h>
#include "external/st7735.h"
#include "pico/stdlib.h"

typedef struct {
  float x;
  float y;
} Coordinate;

typedef struct {
  Coordinate curPos;
  Coordinate prevCoords[25];

  float speedX;
  float speedY;

  float friction;

  int headCol;
  int tailCol;
} Ball;

typedef struct {
  Coordinate curPos;

  int length;

  float speedX;
  float speedY;

  float movespeed;

  float friction;

  int col;
  int borderCol;
} Paddle;

// looks helpful
int findOverlappingBlock(int x, int y) {
  // Bounds from (0, 0) to (80, 80)
  // x / 10, y / 4
  if (y >= 80) {
    return -1;
  }

  return ((y / 4) * 8) + (x / 10);
}

// Wrapper for drawing by coordinate
void drawCoordPixel(Coordinate c, int col) {
  ST7735_DrawPixel((int)round(c.x), (int)round(c.y), col);
}

// http://rosettacode.org/wiki/Bitmap/Bresenham%27s_line_algorithm#C
void drawCoordLine(int x0, int y0, int x1, int y1, int col) {
 
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;
 
  for(;;){
    Coordinate coord = {x0, y0};
    drawCoordPixel(coord, col);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

void paddlePhysicsStep(Paddle *paddle) {
  // TODO paddle movement, consider copying from ball physics since similar variables are used
}

void resetBallPositions(int x, int y, Ball *ball) {
  for (int i=0; i<25; i++) {
    ball->prevCoords[i].x = x;
    ball->prevCoords[i].y = y;
  }
}

// Update ball movement based on speed and do looping/bouncing
int ballPhysicsStep(Ball *ball, Paddle *paddle) {
  ball->curPos.x += ball->speedX;
  ball->curPos.y += ball->speedY;

  ball->speedX *= ball->friction;
  ball->speedY *= ball->friction;

  if (ball->curPos.y <= 0) {
    ball->curPos.y = 0;
    ball->speedY *= -1;
  } else if (ball->curPos.y >= 200) {
    ball->curPos.x = 43;
    ball->curPos.y = 120;
    resetBallPositions(43, 120, ball);
    ball->speedY = 0.9f;
    ball->speedX = 0;
  }

  if (ball->curPos.x <= 0) {
    ball->curPos.x = 0;
    ball->speedX *= -1;
  } else if (ball->curPos.x >= 79) {
    ball->curPos.x = 79;
    ball->speedX *= -1;
  }

  // If the ball bounces on the paddle, we will make it bounce back.
  // TODO well, that's your job.

  return -1;
}

void drawPaddle(Paddle *paddle) {
  // Paddle has a center pos and a length. Extend half length in each direction
  int l_len = paddle->length / 2;
  int r_len = paddle->length - l_len;
  int left_x = paddle->curPos.x - l_len;
  int right_x = paddle->curPos.x + r_len;

  ST7735_FillRectangle(left_x - 5, paddle->curPos.y, 5, 2, ST7735_BLACK);
  ST7735_FillRectangle(right_x + 1, paddle->curPos.y, 5, 2, ST7735_BLACK);

  ST7735_FillRectangle(left_x, paddle->curPos.y, paddle->length, 2, paddle->col);

  ST7735_DrawPixel(left_x, paddle->curPos.y, paddle->borderCol);
  ST7735_DrawPixel(left_x, paddle->curPos.y + 1, paddle->borderCol);

  ST7735_DrawPixel(right_x, paddle->curPos.y, paddle->borderCol);
  ST7735_DrawPixel(right_x, paddle->curPos.y + 1, paddle->borderCol);
}

// Render ball position and update coordinate history
void finishBallMovement(Ball *ball) {
  for (int i=1; i<25; i++) {
    //drawCoordPixel(ball->prevCoords[i], i == 24 ? ST7735_BLACK : ball->tailCol);
    Coordinate coordT = ball->prevCoords[i];
    Coordinate coordO = ball->prevCoords[i-1];
    int col = i==1 ? ball->headCol : (i==24 ? ST7735_BLACK : ball->tailCol);

    if (coordT.y >= 150 && coordO.y <= 10) {  // looping to the left (0), appearing on the right (159)
        drawCoordLine(coordO.x, coordO.y, coordT.x, 0, col);
        drawCoordLine(coordO.x, 159, coordT.x, coordT.y, col);
    } else if (coordT.y <= 10 && coordO.y >= 150) {  // lopoing to the right (159), appearing on the left (0)
        drawCoordLine(coordO.x, coordO.y, coordT.x, 159, col);
        drawCoordLine(coordO.x, 0, coordT.x, coordT.y, col);
    } else {
        drawCoordLine(ball->prevCoords[i].x, ball->prevCoords[i].y, ball->prevCoords[i-1].x, ball->prevCoords[i-1].y, col);
    }
  }

  for (int i=24; i>0; i--) {
    ball->prevCoords[i] = ball->prevCoords[i-1];
  }

  // line-based draw does not need this
  // drawCoordPixel(ball->curPos, ball->headCol);

  ball->prevCoords[0] = ball->curPos;
}
