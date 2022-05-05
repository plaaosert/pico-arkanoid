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

  float friction;

  int col;
  int borderCol;
} Paddle;

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

int findOverlappingBlock(int x, int y) {
  // Bounds from (0, 0) to (80, 80)
  // x / 10, y / 4
  if (y >= 80) {
    return -1;
  }

  return ((y / 4) * 8) + (x / 10);
}

void paddlePhysicsStep(Paddle *paddle) {
  paddle->curPos.x += paddle->speedX;
  paddle->curPos.y += paddle->speedY;

  paddle->speedX *= paddle->friction;
  paddle->speedY *= paddle->friction;

  int l_len = paddle->length / 2;
  int r_len = paddle->length - l_len;
  int left_x = paddle->curPos.x - l_len;
  int right_x = paddle->curPos.x + r_len;

  int left_bounds = l_len;
  int right_bounds = 79 - r_len;

  if (left_x <= 0) {
    paddle->curPos.x = left_bounds + 1;
    paddle->speedX *= -0.5;
  } else if (right_x >= 79) {
    paddle->curPos.x = right_bounds - 1;
    paddle->speedX *= -0.5;
  }
}

// Update ball movement based on speed and do looping/bouncing
// Test if in block position then bounce off the block if needed.
// First, check if transgressing a bound, then see on which side that is happening (reverse both X and Y movement; check which one results in the ball no longer colliding)
// then bounce based on which side is being hit
// and return the index of the block bounced off
int ballPhysicsStep(Ball *ball, Paddle *paddle, uint16_t blocks[]) {
  ball->curPos.x += ball->speedX;
  ball->curPos.y += ball->speedY;

  ball->speedX *= ball->friction;
  ball->speedY *= ball->friction;

  if (ball->curPos.y <= 0) {
    ball->curPos.y = 0;
    ball->speedY *= -1;
  } else if (ball->curPos.y >= 159) {
    ball->curPos.y = 159;
    ball->speedY *= -1;
  }

  if (ball->curPos.x <= 0) {
    ball->curPos.x = 0;
    ball->speedX *= -1;
  } else if (ball->curPos.x >= 79) {
    ball->curPos.x = 79;
    ball->speedX *= -1;
  }

  int blockOverlap = findOverlappingBlock(ball->curPos.x, ball->curPos.y);
  if (blockOverlap != -1) {
    if (blocks[blockOverlap]) {
      // there's something there.
      // Find block of previous coords in the y-position.
      // If, by reversing the y movement only, we find ourselves in a different block, the y-movement was what caused the bounce.
      int reverseYOverlap = findOverlappingBlock(ball->curPos.x, ball->prevCoords[0].y);

      if (reverseYOverlap == blockOverlap) {
        // the backtracking of Y did not cause our block to change, which means the X-movement was what got us in here.
        // so, bounce X
        ball->speedX *= -1;
      } else {
        // the backtracking of Y did cause the block to change, which means the Y-movement was what got us in here.
        ball->speedY *= -1;
      }

      return blockOverlap;
    }
  }

  // Finally, if the ball bounces on the paddle, we will make it bounce back.
  // y >= 150 and y <= 151
  if (ball->curPos.y >= paddle->curPos.y && ball->curPos.y <= (paddle->curPos.y + 2)) {
    int l_len = paddle->length / 2;
    int r_len = paddle->length - l_len;
    int left_x = paddle->curPos.x - l_len;
    int right_x = paddle->curPos.x + r_len;

    if (ball->curPos.x >= left_x && ball->curPos.x <= right_x) {
      // bounce; reverse the movement then apply a change to x and y based on the difference from center
      // get magnitude (total speed of ball)
      float ballMagnitude = sqrt(pow(ball->speedX, 2) + pow(ball->speedY, 2));

      // reverse movement
      ball->speedY *= -1; // always bounce vertically
      // if ball's speedY is positive it is going down, otherwise it is going up
      ball->curPos.y = paddle->curPos.y + (ball->speedY < 0 ? -1 : 1); // just to stop Way Too Much Bouncing, snap to above or below paddle

      // apply change to x based on ball position difference from center
      float diff = ball->curPos.x - paddle->curPos.x;
      ball->speedX += diff / 10.0f;
      // maximum gradient is 1/3
      if (ball->speedX / ball->speedY > 3) {
        ball->speedY = ball->speedX / 3;
      }
      // always tries to get back to center
      ball->speedX *= 0.85f;

      // normalize again
      // find new magnitude and divide by that magnitude, then multiply by original
      float newMagnitude = sqrt(pow(ball->speedX, 2) + pow(ball->speedY, 2));

      ball->speedX = (ball->speedX / newMagnitude) * ballMagnitude;
      ball->speedY = (ball->speedY / newMagnitude) * ballMagnitude;

      return -2;
    }
  }

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
