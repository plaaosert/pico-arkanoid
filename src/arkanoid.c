#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include <math.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "pico/time.h"

#include "balls.c"

#include "external/fonts.h"
#include "external/st7735.h"

// the size of the screen is 80x160. the size of the board is therefore 80x80 (the top half).
// every block is 10x4 so the block size is 8x20 (160)
//
// just for fun, just for a laugh, dont initialise it so we can have some juicy RAM-based randomness
// every block can have a unique colour!!!!!! because FUCK!!!!!!!!!!!!!!
uint16_t blocks[160];

void draw_blocks(uint16_t blocks[]) {
	// our perspective is portrait, just like god intended
	for (int y_index=0; y_index<20; y_index++) {
		for (int x_index=0; x_index<8; x_index++) {
			ST7735_FillRectangle(x_index * 10, y_index * 4, 10, 4, blocks[y_index * 8 + x_index]);
		}
	}
}

void draw_block(int x, int y, uint16_t block) {
	ST7735_FillRectangle(x * 10, y * 4, 10, 4, block);
}

int main() {
	stdio_init_all();
	setvbuf ( stdout , NULL , _IONBF , 0 );
	sleep_ms(1000);
	ST7735_Init();
	ST7735_DrawImage(0, 0, 80, 160, plaao_logo);
	sleep_ms(1000);
	ST7735_FillScreen(ST7735_BLACK);

	for (int i=0; i<160; i++) {
		blocks[i] = i % 5 == 0 ? ST7735_GREEN : (i % 3 == 0 ? ST7735_RED : ST7735_BLACK);
	}
	blocks[0] = ST7735_CYAN;
	blocks[1] = ST7735_YELLOW;
	blocks[2] = ST7735_BLUE;

	draw_blocks(blocks);

	Ball playerBall = {
    {40, 120}, 
    {
      {40, 120}, {40, 120}, {40, 120}, {40, 120}, {40, 120}, 
      {40, 120}, {40, 120}, {40, 120}, {40, 120}, {40, 120}, 
      {40, 120}, {40, 120}, {40, 120}, {40, 120}, {40, 120}, 
      {40, 120}, {40, 120}, {40, 120}, {40, 120}, {40, 120}, 
      {40, 120}, {40, 120}, {40, 120}, {40, 120}, {40, 120}, 
    },
    1.1, 1, 1, ST7735_CYAN, ST7735_BLUE
  };

	while (true) {
	    int block_hit = ballPhysicsStep(&playerBall, blocks);
	    finishBallMovement(&playerBall);
	    if (block_hit != -1) {
	    	blocks[block_hit] = ST7735_BLACK;
	    	draw_block(block_hit % 8, block_hit / 8, blocks[block_hit]);
	    }

		sleep_ms(10);
	}
}