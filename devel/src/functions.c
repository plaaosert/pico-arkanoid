#include <math.h>
#include <string.h>

#include "external/fonts.h"
#include "external/st7735.h"

#include "data.c"


int min(int a, int b) {
	return a < b ? a : b;
}


int max(int a, int b) {
	return a < b ? b : a;
}


int calc_xp_to_next(int level) {
	return 10 + pow((float)level, 1.5f);
}


Character make_character(char* name) {
	int specials[5] = {};
	int innate_skills[25] = {-1};
	int all_skills[40] = {4};
	int weapon_levels[11] = {
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
	};
	int equips[5] = {0, 1, 2, 3, 4};
	Character c = {
		name, "It's you!",
		BASE_PLAYER_HP, BASE_PLAYER_HP,
		BASE_PLAYER_MP, BASE_PLAYER_MP,
		DEFAULT_MP_COLOUR,
		0, 1, calc_xp_to_next(1),
		BASE_PLAYER_ATK, BASE_PLAYER_DEF, BASE_PLAYER_MAG, BASE_PLAYER_WIL,
		BASE_PLAYER_ATK, BASE_PLAYER_DEF, BASE_PLAYER_MAG, BASE_PLAYER_WIL,
		{0,0,0,0,0}, {0,1,2,3,4},
		{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
		{1,1,1,1,1,1,1,1,1,1,1}
	};

	for (int i=0; i<5; i++) {
		c.equipped[i] = i;
		c.specials[i] = SS_NONE;
	}

	for (int i=0; i<11; i++) {
		c.weapon_type_levels[i] = 1;
	}

	for (int i=0; i<25; i++) {
		c.innate_skills[i] = -1;
	}

	for (int i=0; i<40; i++) {
		c.all_skills[i] = -1;
	}

	return c;
}


void get_character_moves(Character *character) {
	// To get a character's moves, we need to compare their level to their learnset.
	// Then, add on their innate moves.
	int global_i = 0;

	int i = 0;
	// Get character weapon type.
	// If no weapon, barehanded.
	int weapon_type = equip_list[character->equipped[4]].type;

	while (character->level >= weapon_skill_levels[weapon_type][i]) {
		character->all_skills[global_i] = weapon_skills[weapon_type][i];
		i++;
		global_i++;
	}

	i = 0;
	while (character->innate_skills[i] != -1) {
		character->all_skills[global_i] = character->innate_skills[i];
		i++;
		global_i++;
	}

	while (global_i < 40) {
		character->all_skills[global_i] = -1;
		global_i++;
	}
}


void render_bar(float val, float max_val, int x, int y, int length, int height, uint16_t bar_color, uint16_t inner_bg_color) {
	// Bar extends from right to left. Start x position should make it such that 160 - (start_x + length) = x.
	//                    														 160 - start_x - length = x
	//  																		 160 - x - length = start_x
	int start_x = 160 - x - length;

	int bits = round((1 - (val / max_val)) * (length));
	printf("%f / %f : %d bits\n", val, max_val, bits);

	ST7735_FillRectangle(y, start_x + bits, height, length - bits, bar_color);
	ST7735_FillRectangle(y, start_x, height, bits, inner_bg_color);
}


void render_bar_bordered(float val, float max_val, int x, int y, int length, int height, uint16_t border_color, uint16_t bar_color, uint16_t inner_bg_color) {
	int start_x = 160 - x - length;

	int bits = round((1 - (val / max_val)) * (length - 2));
	printf("%f / %f : %d bits\n", val, max_val, bits);

	// draw border in such a way so as to not cause eye destruction
	ST7735_FillRectangle(y,          start_x, 1, length, border_color);
	ST7735_FillRectangle(y+height-1, start_x, 1, length, border_color);
	ST7735_FillRectangle(y, start_x,          height, 1, border_color);
	ST7735_FillRectangle(y, start_x+length-1, height, 1, border_color);


	ST7735_FillRectangle(y+1, start_x+1 + bits, height - 2, length - 2 - bits, bar_color);
	ST7735_FillRectangle(y+1, start_x+1, height - 2, bits, inner_bg_color);
}


void render_bar_number(int val, int max_val, int length, int x, int y, uint16_t col) {
	// The number of chars available - 5 pixels per char.
	int num_chars_available = floor(length / 5);
	int spare_length = length - (num_chars_available * 5);
	int left_spare_pad = spare_length / 2;
	int right_spare_pad = spare_length - left_spare_pad;

	char counter[num_chars_available];
	sprintf(counter, "%d/%d", val, max_val);

	int counter_len = strlen(counter);
	int spare_counter_len = num_chars_available - counter_len;
	int left_counter_pad = spare_counter_len / 2;
	int right_counter_pad = spare_counter_len - left_counter_pad;

	char counter_padded[num_chars_available];
	sprintf(counter_padded, "%*s%s%*s", left_counter_pad, "", counter, right_counter_pad, "");
	ST7735_WriteString(
		x + left_spare_pad, 
		y,
		Font4x6,
		counter_padded, 
		col, 
		ST7735_BLACK
	);
}


void render_health_bar_bordered(Character character, int x, int y, int length, int height, uint16_t border_color, uint16_t inner_bg_color) {
	render_bar_bordered(
		(float)character.hp, (float)character.max_hp, x, y, length, height, border_color, ST7735_GREEN, inner_bg_color
	);

	render_bar_number(character.hp, character.max_hp, length, x, y + height + 2, ST7735_GREEN);
}

void render_health_bar(Character character, int x, int y, int length, int height, uint16_t inner_bg_color) {
	render_bar(
		(float)character.hp, (float)character.max_hp, x, y, length, height, ST7735_GREEN, inner_bg_color
	);

	render_bar_number(character.hp, character.max_hp, length, x, y + height + 2, ST7735_GREEN);
}

void render_enemy_health_bar(Character character, int x, int y, int length, int height, uint16_t inner_bg_color) {
	render_bar(
		(float)character.hp, (float)character.max_hp, x, y, length, height, ST7735_RED, inner_bg_color
	);

	render_bar_number(character.hp, character.max_hp, length, x, y + height + 2, ST7735_RED);
}

void render_mana_bar_bordered(Character character, int x, int y, int length, int height, uint16_t border_color, uint16_t inner_bg_color) {
	render_bar_bordered(
		(float)character.mp, (float)character.max_mp, x, y, length, height, border_color, character.mp_color, inner_bg_color
	);

	render_bar_number(character.mp, character.max_mp, length, x, y + height + 2, character.mp_color);
}

void render_mana_bar(Character character, int x, int y, int length, int height, uint16_t inner_bg_color) {
	render_bar(
		(float)character.mp, (float)character.max_mp, x, y, length, height, character.mp_color, inner_bg_color
	);

	render_bar_number(character.mp, character.max_mp, length, x, y + height + 2, character.mp_color);
}


void write_text_linewrap(int x, int y, const char* str, int target_length, uint16_t col) {
	int cur_x = 0;
	int cur_y = y;

	char write_char[2];

	while(*str) {
		if (*str == ' ' && cur_x >= target_length) {
			cur_y += 8;
			cur_x = 0;
		} else {
			sprintf(write_char, "%c", str[0]);
			ST7735_WriteString(cur_x + x, cur_y, Font4x6, write_char, col, ST7735_BLACK);
    	cur_x += 5;
		}

    str++;
  }
}


void render_battle_screen(Character player, Character enemy) {

}
