#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include "arkanoid_levels.c"

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
// every block can have a unique colour!!!!!! because FUCK!!!!!!!!!!!!!!
uint16_t blocks[160];

int set_level(uint16_t template[], uint16_t *copy_to) {
	int blocks = 0;

	for (int i=0; i<160; i++) {
		copy_to[i] = template[i];
		if (template[i]) {
			blocks++;
		}
	}

	return blocks;
}

void draw_blocks(uint16_t blocks[]) {
	// our perspective is portrait, just like god intended
	for (int y_index=0; y_index<20; y_index++) {
		for (int x_index=0; x_index<8; x_index++) {
			ST7735_FillRectangle(x_index * 10, y_index * 4, 10, 4, blocks[y_index * 8 + x_index]);

			if (blocks[y_index * 8 + x_index]) {
				ST7735_FillRectangle(x_index * 10, y_index * 4, 10, 1, c111);
				ST7735_FillRectangle(x_index * 10, y_index * 4, 1, 4, c111);
				ST7735_FillRectangle(x_index * 10, 3 + y_index * 4, 10, 1, c111);
				ST7735_FillRectangle(9 + x_index * 10, y_index * 4, 1, 4, c111);
			}
		}
	}
}

void draw_block(int x, int y, uint16_t block) {
	ST7735_FillRectangle(x * 10, y * 4, 10, 4, block);

	if (block) {
		ST7735_FillRectangle(x * 10, y * 4, 10, 1, c111);
		ST7735_FillRectangle(x * 10, y * 4, 1, 4, c111);
		ST7735_FillRectangle(x * 10, 3 + y * 4, 10, 1, c111);
		ST7735_FillRectangle(9 + x * 10, y * 4, 1, 4, c111);
	}
}

int main() {
	stdio_init_all();
	setvbuf ( stdout , NULL , _IONBF , 0 );
	sleep_ms(1000);
	ST7735_Init();
	ST7735_DrawImage(0, 0, 80, 160, plaao_logo);

	const uint LED_PIN = 0;
	const uint BUTTON1 = 20;
	const uint BUTTON2 = 21;
	const uint BUTTON3 = 22;

	for (int i=18; i<28; i++) {
		gpio_init(i);

		if (i == 20 || i == 21 || i == 22) {
			gpio_set_dir(i, GPIO_IN);
		} else {
			gpio_set_dir(i, GPIO_OUT);
		}
	}

	/*
	for (int i=0; i<160; i++) {
		blocks[i] = i % 5 == 0 ? ST7735_GREEN : (i % 3 == 0 ? ST7735_RED : ST7735_BLACK);
	}
	blocks[0] = ST7735_CYAN;
	blocks[1] = ST7735_YELLOW;
	blocks[2] = ST7735_BLUE;
	*/

	int blocks_left = set_level(levels[0], blocks);
	int start_blocks = blocks_left;
	int level = 1;

	// TODO will need to store powerups here, probably

	int score = 0;
	int combo = 0;

	sleep_ms(250);
	ST7735_FillScreen(ST7735_BLACK);

	draw_blocks(blocks);

    char bottom_text[40];
    while (true) {
    	{
			Ball playerBall = {
			    {43, 120}, 
			    {
			      {43, 120}, {43, 120}, {43, 120}, {43, 120}, {43, 120}, 
			      {43, 120}, {43, 120}, {43, 120}, {43, 120}, {43, 120}, 
			      {43, 120}, {43, 120}, {43, 120}, {43, 120}, {43, 120}, 
			      {43, 120}, {43, 120}, {43, 120}, {43, 120}, {43, 120}, 
			      {43, 120}, {43, 120}, {43, 120}, {43, 120}, {43, 120}, 
			    },
			    0, 0.9, 1, ST7735_CYAN, ST7735_BLUE
			};

			Paddle playerPaddle = {
				{40, 140},
				24, 0, 0, 0.4, 0.7, ST7735_WHITE, ST7735_CYAN
			};

			while (true) {
			    int block_hit = ballPhysicsStep(&playerBall, &playerPaddle, blocks);
			    finishBallMovement(&playerBall, blocks);
			    if (block_hit >= 0) {
			    	blocks[block_hit] = ST7735_BLACK;
			    	draw_block(block_hit % 8, block_hit / 8, blocks[block_hit]);
			    	blocks_left--;
			    	combo++;
			    	score += 100 * level * combo;
			    }

			    // hit paddle
			    if (block_hit == -2) {
			    	combo = 0;
			    	if (blocks_left <= 10 || gpio_get(BUTTON3) == 0) {
			    		break;
			    	}
			    }

			    if (gpio_get(BUTTON1) == 0) {
			    	playerPaddle.speedX -= playerPaddle.movespeed;
			    }

			    if (gpio_get(BUTTON2) == 0) {
			    	playerPaddle.speedX += playerPaddle.movespeed;
			    }

			    paddlePhysicsStep(&playerPaddle);
			    drawPaddle(&playerPaddle);

			    // TODO we need to put some powerups here, maybe...

			    sprintf(bottom_text, "%d | %dx | LV %d    ", score, combo, level);
				ST7735_WriteString(2, 150, Font4x6, bottom_text, ST7735_CYAN, ST7735_BLACK);

				sleep_us(1000000 / 60);
			}
		}

		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_CYAN, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_RED, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_CYAN, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_RED, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_CYAN, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_RED, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_CYAN, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_RED, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_CYAN, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_RED, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_CYAN, ST7735_BLACK);
		sleep_ms(100);
		ST7735_WriteString(4, 92, Font4x6, "Y O U  W I N !!", ST7735_RED, ST7735_BLACK);
		sleep_ms(100);

		ST7735_FillScreen(ST7735_BLACK);

		blocks_left = set_level(levels[level], blocks);
		start_blocks = blocks_left;
		level++;

		score = 0;
		combo = 0;

		sleep_ms(250);
		ST7735_FillScreen(ST7735_BLACK);

		draw_blocks(blocks);
	}
}