#include "pico/stdlib.h"
#include "pico/util/datetime.h"

#include <math.h>
#include <stdio.h>

#include "hardware/gpio.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "pico/time.h"

#include "functions.c"

#include "external/fonts.h"
#include "external/st7735.h"

#define FRAME 1000/60

enum BattleMenuType {BMT_MAIN, BMT_MOVE, BMT_INVENTORY};

const uint BUTTON1 = 20;
const uint BUTTON2 = 21;
const uint BUTTON3 = 22;

const uint16_t note_cols[8] = {
	ST7735_BLACK,
	ST7735_RED,
	ST7735_YELLOW,
	ST7735_GREEN,
	ST7735_CYAN,
	ST7735_BLUE,
	ST7735_MAGENTA,
	ST7735_WHITE
};

const float note_freqs[8] = {
	// 440, 493.88, (A4, B4)
	523.25, 587.33, 659.25, 698.46, 783.99, 880,
	987.77, 1046.50
};

// https://forums.raspberrypi.com/viewtopic.php?t=302960
uint32_t rnd() {
  int k, random=0;
  volatile uint32_t *rnd_reg=(uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);
  
  for(k=0; k<32; k++){
  
  random = random << 1;
  random = random + (0x00000001 & (*rnd_reg));

  }
  return random;
}

int secs_to_us(float secs) {
	return round(secs * 1000000);
}

void rest(int time_us) {
	sleep_us(time_us);
}

void play_note(int pitch, int time_us) {
	if (pitch != -1) {
		for (int i=0; i<(time_us / ((float)1000000 / note_freqs[pitch])); i++) {
			gpio_put(18, 1);
			sleep_us((float)1000000 / note_freqs[pitch]);
			gpio_put(18, 0);
			sleep_us((float)1000000 / note_freqs[pitch]);
		}
	} else {
		rest(time_us);
	}
}

void play_note_sequence(int *notes, int *times_us, int num_notes) {
	for (int i=0; i<num_notes; i++) {
		play_note(notes[i], times_us[i]);
	}
}

void play_note_sequence_with_fills(int *notes, int *times_us, int num_notes) {
	for (int i=0; i<num_notes; i++) {
		if (notes[i] != -1) {
			ST7735_FillScreen(note_cols[notes[i] % 8]);
		}
		play_note(notes[i], times_us[i]);
	}
}

void draw_battle_char_stats(Character player, Character enemy) {
	ST7735_WriteString(8, 42, Font4x6, player.name, ST7735_YELLOW, ST7735_BLACK);
	render_health_bar(player, 4, 51, 72, 4, ST7735_BLACK);
	render_mana_bar(player, 4, 67, 72, 4, ST7735_BLACK);

	ST7735_WriteString(88, 8, Font4x6, enemy.name, ST7735_YELLOW, ST7735_BLACK);
	render_enemy_health_bar(enemy, 84, 16, 54, 4, ST7735_BLACK);
}

void wait_all_buttons_up() {
	while (gpio_get(BUTTON1) == 0 || gpio_get(BUTTON2) == 0 || gpio_get(BUTTON3) == 0) {
		sleep_ms(FRAME);
	}
}


void wait_any_button_down() {
	while (gpio_get(BUTTON1) == 1 && gpio_get(BUTTON2) == 0 && gpio_get(BUTTON3) == 0) {
		sleep_ms(FRAME);
	}
}


void wait_full_button_press() {
	wait_all_buttons_up();
	sleep_ms(FRAME);
	wait_any_button_down();
}


void draw_option_list(int x, int y, int option_count, char* options[], char* option_subtexts[], uint16_t option_cols[], uint16_t selected_col, int current_option) {
	char str_write[20];

	ST7735_WriteString(x + 40, y + (8 * 4), Font4x6, option_subtexts[current_option], ST7735_CYAN, ST7735_BLACK);
	
	// option_count = 7
	// 0 => [0, 4] -> [0, 4]
	// 1 => [0, 4] -> [0, 4]
	// 2 => [0, 4] -> [0, 4]
	// 3 => [0, 4] -> [0, 4]
	// 4 => [1, 5] -> [1, 5]
	// 5 => [2, 6] -> [2, 6]
	// 6 => [3, 7] -> [3, 7]
	// 7 => [4, 8] -> [4, 8]

	int lowest_index = max(0, current_option - 3);
	int highest_index = min(lowest_index + 4, (option_count + 1));

	for (int i=lowest_index; i<highest_index; i++) {
		sprintf(str_write, "%s%s ", (i == current_option ? ">" : ""), options[i]);
		ST7735_WriteString(
			x, y + (8 * (i - lowest_index)), Font4x6, str_write,
			(i == current_option ? selected_col : option_cols[i]),
			ST7735_BLACK
		);
	}

	/*
	if (current_option != (highest_index - 1)) {
		ST7735_WriteString(
			x, y + (8 * 4), Font4x6, "  v  ", ST7735_COLOR565(128, 128, 128), ST7735_BLACK
		);
	} else {
		ST7735_WriteString(
			x, y + (8 * 4), Font4x6, "     ", ST7735_COLOR565(128, 128, 128), ST7735_BLACK
		);
	}
	*/
}


void show_info_screen(char* title, char* subtitle, char* info) {
	int notes[] = {0, 1, 2, 3, 4, 5, 6, 7};
	int times[] = {2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500};

	play_note_sequence(notes, times, 8);

	ST7735_FillScreen(ST7735_BLACK);

	ST7735_WriteString(8, 6, Font4x6, title, ST7735_YELLOW, ST7735_BLACK);
	write_text_linewrap(8, 14, subtitle, 80, ST7735_CYAN);
	write_text_linewrap(8, 30, info, 80, ST7735_CYAN);

	wait_full_button_press();

	ST7735_FillScreen(ST7735_BLACK);
}


// Returns "1XXX" for a BUTTON3 option (option 0 => 1000)
// Returns "0XXX" for a BUTTON2 option (option 0 => 0)
int make_menu(int x, int y, int option_count, char* options[], char* option_subtexts[], uint16_t option_cols[], uint16_t selected_col, int start_option) {
	bool button1_down = false;
	bool changed_option = true;
	int current_option = start_option;

	wait_all_buttons_up();

	while (true) {
		if (gpio_get(BUTTON1) == 0 && !button1_down) {
			current_option = (current_option + 1) % option_count;
			changed_option = true;

			int notes[] = {0, 7};
			int times[] = {2000, 2000};
			play_note_sequence(notes, times, 2);

			button1_down = true;
		} else if (gpio_get(BUTTON1) == 1) {
			button1_down = false;
		}

		if (gpio_get(BUTTON2) == 0) {
			return current_option;
		}

		if (gpio_get(BUTTON3) == 0) {
			return 1000 + current_option;
		}

		if (changed_option) {
			changed_option = false;
			draw_option_list(x, y, option_count, options, option_subtexts, option_cols, selected_col, current_option);

			/*
			char str_write[20];
			changed_option = false;

			ST7735_WriteString(x + 40, y + (8 * 4), Font4x6, option_subtexts[current_option], ST7735_CYAN, ST7735_BLACK);
			
			// option_count = 7
			// 0 => [0, 4] -> [0, 4]
			// 1 => [0, 4] -> [0, 4]
			// 2 => [0, 4] -> [0, 4]
			// 3 => [0, 4] -> [0, 4]
			// 4 => [1, 5] -> [1, 5]
			// 5 => [2, 6] -> [2, 6]
			// 6 => [3, 7] -> [3, 7]
			// 7 => [4, 8] -> [4, 8]

			int lowest_index = max(0, current_option - 3);
			int highest_index = min(lowest_index + 4, (option_count + 1));

			for (int i=lowest_index; i<highest_index; i++) {
				sprintf(str_write, "%s%s ", (i == current_option ? ">" : ""), options[i]);
				ST7735_WriteString(
					x, y + (8 * (i - lowest_index)), Font4x6, str_write,
					(i == current_option ? ST7735_YELLOW : ST7735_CYAN),
					ST7735_BLACK
				);
			}

			if (highest_index != (option_count + 1)) {
				ST7735_WriteString(
					x, y + (8 * 4), Font4x6, "  v  ", ST7735_COLOR565(128, 128, 128), ST7735_BLACK
				);
			} else {
				ST7735_WriteString(
					x, y + (8 * 4), Font4x6, "     ", ST7735_COLOR565(128, 128, 128), ST7735_BLACK
				);
			}
			*/
		}
	}
}

int remake_battle_menu(int option_count, char* options[], char* option_subtexts[], int start_option) {
	uint16_t cols[option_count];
	for (int i=0; i<option_count; i++) {
		cols[i] = ST7735_CYAN;
	}

	return make_menu(8, 6, option_count, options, option_subtexts, cols, ST7735_YELLOW, start_option);
}

int make_battle_menu(int option_count, char* options[], char* option_subtexts[]) {
	uint16_t cols[option_count];
	for (int i=0; i<option_count; i++) {
		cols[i] = ST7735_CYAN;
	}

	return make_menu(8, 6, option_count, options, option_subtexts, cols, ST7735_YELLOW, 0);
}

int make_battle_move_menu(Character *player, Character *enemy) {
	int player_move_count = 0;
	for (int i=0; i<40; i++) {
		if (player->all_skills[i] == -1) {
			break;
		}

		player_move_count++;
	}

	char* move_names[player_move_count + 1];
	char* move_subs[player_move_count + 1];

	move_names[0] = "- Back        ";
	move_subs[0] = "              ";

	// set the strings to the skill names and costs
	for (int i=0; i<player_move_count; i++) {
		Skill skill_selected = skill_list[player->all_skills[i]];

		move_names[i+1] = malloc(25);
		move_subs[i+1] = malloc(25);

		sprintf(move_names[i+1], "%-14s", skill_selected.name);
		sprintf(move_subs[i+1], "%d MP      ", skill_selected.mp_cost);
	}

	int selected_battle_option = make_battle_menu(player_move_count + 1, move_names, move_subs);

	if (selected_battle_option < 1000) {
		if (selected_battle_option > 0) {
			char dmg_string[25];
			Skill skill_chosen = skill_list[player->all_skills[selected_battle_option - 1]];

			play_note(selected_battle_option, 150000);

			int dmg = skill_chosen.power + ((0.5 - ((float)rnd() / (float)0xffffffff)) * ((float)skill_chosen.power / 10.0f));
			sprintf(dmg_string, "%d damage!    ", dmg);
			enemy->hp -= dmg;

			ST7735_WriteString(96, 72, Font4x6, dmg_string, ST7735_RED, ST7735_BLACK);
		}
	} else {
		Skill skill_chosen = skill_list[player->all_skills[selected_battle_option - 1001]];
		char mana_cost[25];
		sprintf(mana_cost, "%d MP     ", skill_chosen.mp_cost);

		show_info_screen(skill_chosen.name, mana_cost, skill_chosen.desc);
	}
}

void battle(Character *player, Character *enemy) {
	ST7735_FillScreen(ST7735_BLACK);

	int last_option = 0;
	int menu_type = BMT_MAIN;

	bool stats_changed = true;
	bool option_changed = true;

	while (true) {
		if (stats_changed) {
			stats_changed = false;
			draw_battle_char_stats(*player, *enemy);
		}

		char* main_options[] = {
			"Fight         ", "Item          ", "Status        ", "Run!          ",
			"Blompis       ", "Scrumbo       ", "Wololo        "
		};

		char* main_option_subtexts[] = {"          ", "          ", "          ", "          ", "          ", "          ", "          "};
		int selected_option = remake_battle_menu(
			7, main_options, main_option_subtexts, last_option
		);

		switch (selected_option) {
			case 0:
				last_option = 0;
				make_battle_move_menu(player, enemy);
				stats_changed = true;
				break;

			case 1:
				last_option = 1;
				ST7735_FillScreen(ST7735_RED);
				sleep_ms(1000);
				ST7735_FillScreen(ST7735_BLACK);
				stats_changed = true;
				break;

			case 2:
				last_option = 2;
				ST7735_FillScreen(ST7735_YELLOW);
				sleep_ms(1000);
				ST7735_FillScreen(ST7735_BLACK);
				stats_changed = true;
				break;

			case 3:
				last_option = 3;
				ST7735_FillScreen(ST7735_BLACK);
				sleep_ms(1000);
				ST7735_FillScreen(ST7735_BLACK);
				stats_changed = true;
				break;

			default:
				ST7735_FillScreen(ST7735_MAGENTA);
				sleep_ms(1000);
				ST7735_FillScreen(ST7735_BLACK);
				stats_changed = true;
				break;
		}
	}
}

void setup() {
	stdio_init_all();
	setvbuf ( stdout , NULL , _IONBF , 0 );
	sleep_ms(1000);
	ST7735_Init();
	ST7735_DrawImage(0, 0, 80, 160, plaao_logo);
	sleep_ms(1000);

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

	int notes[] = {0, 2, 4, -1, 0, 1, 2, 3, 4, 5, 6, 7};
	int times[] = {100000, 100000, 100000, 250000, 20000, 20000, 20000, 20000, 20000, 20000, 20000, 50000};

	// play_note_sequence_with_fills(notes, times, 12);

	ST7735_FillScreen(ST7735_BLACK);
	/*
	ST7735_WriteString(12, 6, Font4x6, "( )*+,-./0123456789:;<=>?@", ST7735_CYAN, ST7735_BLACK);
	ST7735_WriteString(12, 14, Font4x6, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", ST7735_CYAN, ST7735_BLACK);
	ST7735_WriteString(12, 22, Font4x6, "abcdefghijklmnopqrstuvwxyz", ST7735_CYAN, ST7735_BLACK);
	ST7735_WriteString(12, 30, Font4x6, "!\"#$%&'[\\]^_`{|}~", ST7735_CYAN, ST7735_BLACK);
	*/

	sleep_ms(400);

	init_enemies(enemy_list);

	/*
	Character player = make_character("plaao");
	player.level = 100;
	player.equipped[4] = 105;
	get_character_moves(&player);

	char str_write[80];
	sprintf(str_write, "%s", equip_list[player.equipped[4]].name);
	ST7735_WriteString(8, 6, Font4x6, str_write, ST7735_YELLOW, ST7735_BLACK);

	sprintf(str_write, "%s", equip_list[player.equipped[4]].desc);
	write_text_linewrap(8, 14, str_write, 80, ST7735_RED);

	for (int i=0; i<8; i++) {
		if (player.all_skills[i] == -1) {
			break;
		}

		sprintf(str_write, "%d | %d -> %s", i, player.all_skills[i], skill_list[player.all_skills[i]]);
		ST7735_WriteString(8, 38 + (8 * i), Font4x6, str_write, ST7735_CYAN, ST7735_BLACK);
	}

	while (gpio_get(BUTTON1) == 1) {
		sleep_ms(50);
	}

	ST7735_FillScreen(ST7735_BLACK);

	player = make_character("plaao");
	player.level = 100;
	player.equipped[4] = 4;
	get_character_moves(&player);

	sprintf(str_write, "%s", equip_list[player.equipped[4]].name);
	ST7735_WriteString(8, 6, Font4x6, str_write, ST7735_YELLOW, ST7735_BLACK);

	sprintf(str_write, "%s", equip_list[player.equipped[4]].desc);
	write_text_linewrap(8, 14, str_write, 80, ST7735_RED);

	for (int i=0; i<8; i++) {
		if (player.all_skills[i] == -1) {
			break;
		}

		sprintf(str_write, "%d | %d -> %s", i, player.all_skills[i], skill_list[player.all_skills[i]]);
		ST7735_WriteString(8, 38 + (8 * i), Font4x6, str_write, ST7735_CYAN, ST7735_BLACK);
	}

	sleep_ms(500);

	while (gpio_get(BUTTON1) == 1) {
		sleep_ms(50);
	}
	*/

	ST7735_FillScreen(ST7735_BLACK);

	Character player = make_character("plaao");
	player.level = 100;
	player.equipped[4] = 105;
	get_character_moves(&player);

	Character enemy = enemy_list[2];
	//get_character_moves(enemy);

	battle(&player, &enemy);

	int selected_option = 0;
	bool changed_option = true;
	bool something_changed = true;
	bool button1_down = false;
	bool button2_down = false;

	int option_locations_x[] = {
		8, 8, 8, 8
	};

	int option_locations_y[] = {
		6, 14, 22, 30
	};

	while (true) {
		if (gpio_get(BUTTON3) == 0) {
			int notes[] = {0, 1, 2, 3, 4, 5, 6, 7};
			int times[] = {2500, 2500, 2500, 2500, 2500, 2500, 2500, 2500};

			play_note_sequence(notes, times, 8);

			ST7735_FillScreen(ST7735_BLACK);

			char str_write[80];
			Skill skill_selected = skill_list[selected_option + 80];

			sprintf(str_write, "%s       ", skill_selected.name);
			ST7735_WriteString(8, 6, Font4x6, str_write, ST7735_YELLOW, ST7735_BLACK);

			sprintf(str_write, "%d MP      ", skill_selected.mp_cost);
			ST7735_WriteString(8, 14, Font4x6, str_write, ST7735_CYAN, ST7735_BLACK);

			sprintf(str_write, "%s", skill_selected.desc);
			write_text_linewrap(8, 30, str_write, 80, ST7735_CYAN);

			while (true) {
				if (gpio_get(BUTTON1) == 0) {
					button1_down = true;
					break;
				} else if (gpio_get(BUTTON2) == 0) {
					button2_down = true;
					break;
				}

				sleep_ms(25);
			}

			ST7735_FillScreen(ST7735_BLACK);
			changed_option = true;
		}

		// change selection
		if (gpio_get(BUTTON1) == 0 && !button1_down) {
			selected_option = (selected_option + 1) % 4;
			changed_option = true;

			int notes[] = {0, 7};
			int times[] = {2000, 2000};
			play_note_sequence(notes, times, 2);

			button1_down = true;
		} else if (gpio_get(BUTTON1) == 1) {
			button1_down = false;
		}

		// select
		if (gpio_get(BUTTON2) == 0 && !button2_down) {
			char dmg_string[20];
			int mp_cost = 10 * selected_option;

			if (player.mp < mp_cost) {
				int notes[] = {0, -1, 0};
				int times[] = {100000, 100000, 100000};
				play_note_sequence(notes, times, 3);
			} else {
				play_note(selected_option, 150000);
				if (selected_option != 2) {
					int dmg = ((float)rnd() / (float)0xffffffff) * (50 * (selected_option + 1));
					sprintf(dmg_string, "%d damage!    ", dmg);
					enemy.hp -= dmg;
					player.mp -= (10 * selected_option);

					ST7735_WriteString(96, 72, Font4x6, dmg_string, ST7735_RED, ST7735_BLACK);
				} else {
					int dmg = 250 + ((float)rnd() / (float)0xffffffff) * 750;
					sprintf(dmg_string, "%d heal!     ", dmg);
					player.hp += dmg;
					enemy.hp = enemy.max_hp;
					player.mp -= (20 * selected_option);

					ST7735_WriteString(96, 72, Font4x6, dmg_string, ST7735_GREEN, ST7735_BLACK);
				}

				if (enemy.hp < 0) {
					enemy.hp = 0;
				} else if (player.hp > player.max_hp) {
					player.hp = player.max_hp;
				}

				if (player.mp < 0) {
					player.mp = 0;
				}
			}
		} else if (gpio_get(BUTTON2) == 1) {
			button2_down = false;
		}

		if (changed_option) {
			char str_write[20];
			changed_option = false;

			Skill skill_selected = skill_list[selected_option + 80];

			sprintf(str_write, "%d MP     ", skill_selected.mp_cost);
			ST7735_WriteString(48, 38, Font4x6, str_write, ST7735_CYAN, ST7735_BLACK);
			
			for (int i=0; i<4; i++) {
				sprintf(str_write, "%s%s ", (i == selected_option ? ">" : ""), skill_list[i + 80].name);
				ST7735_WriteString(option_locations_x[i], option_locations_y[i], Font4x6, str_write, (i == selected_option ? ST7735_YELLOW : ST7735_CYAN), ST7735_BLACK);
			}
		}

		//char str_write[40];
		//sprintf(str_write, "%d", get_absolute_time());
		//ST7735_WriteString(96, 32, Font4x6, str_write, ST7735_WHITE, ST7735_BLACK);

		if (something_changed) {
			draw_battle_char_stats(player, enemy);
		}
	}
}

int main() {
	setup();
}