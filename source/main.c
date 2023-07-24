#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>
#include <string.h>
#include <stdlib.h>

#include "common.h"
#include "menu.h"
#include "game.h"
#include "high_scores.h"


int tick = 0; // for seeding rng

// menu state


// save data

#define MAGIC_OFFSET

char name_input[10];
int name_input_index = 0;

#define NAME_INPUT_X 10
#define NAME_INPUT_Y 8
void draw_name_input() {
// high score is drawn at x 10, y 5
#define UP_ARROW_TILE (546 - 512)
#define DOWN_ARROW_TILE (563 - 512)
	for(int i = 0; i <= name_input_index; i++) {
		// do or do not draw the arrows
		if (i != name_input_index) {
			SET_TILE(bg2_map, NAME_INPUT_X + i, NAME_INPUT_Y - 1, 0x40, 1);
			SET_TILE(bg2_map, NAME_INPUT_X + i, NAME_INPUT_Y + 2, 0x40, 1);
		} else {
			SET_TILE(bg2_map, NAME_INPUT_X + i, NAME_INPUT_Y - 1, UP_ARROW_TILE, 1);
			SET_TILE(bg2_map, NAME_INPUT_X + i, NAME_INPUT_Y + 2, DOWN_ARROW_TILE, 1);
		}

		if (name_input[i]) {
			draw_char(NAME_INPUT_X + i, NAME_INPUT_Y, name_input[i], 1);
		} else {
			draw_char(NAME_INPUT_X + i, NAME_INPUT_Y, '_', 1);
		}
	}
}

int main() {
	// load save data
	load_save();

	// start the game to the menu
	change_state(menu);

	while(1) {
		vid_vsync();
		key_poll();
		tick++;
		if (state == menu) {
			menu_tick(tick);
		} else if (state == high_score_list) {
			draw_high_scores();
			if (key_hit(KEY_B)) {
				change_state(menu);
				continue;
			}
		} else if (state == playing) {
			playing_tick(tick);
		} else if (state == moving) {
			moving_tick(tick);
		} else if (state == game_over) {
			if (key_hit(KEY_A)) {
				// erase the game over box
				erase_box(10, 4, 10, 6);
				draw_string(13, 5, "    ");
				draw_string(13, 7, "    ");
				reset_board();
				state = playing;
			}
		} else if (state == entering_name) {
			draw_name_input();
			if (key_hit(KEY_UP)) {
				if (name_input[name_input_index]) {
					name_input[name_input_index] += 1;
				} else {
					name_input[name_input_index] = 'A'; // we're on a null char then just at A
				}
			} else if (key_hit(KEY_DOWN)) {
				if (name_input[name_input_index]) {
					name_input[name_input_index] -= 1;
				} else {
					name_input[name_input_index] = '9'; // we're on a null char then just at 9
				}		
			}
			if (key_hit(KEY_A)) {
				// add a character
				if (name_input_index + 1 < sizeof(name_input)) {
					name_input_index += 1;
				}
			} else if (key_hit(KEY_B)) {
				// delete a character
				if (name_input_index > 0) {
					name_input[name_input_index] = 0;
					for(int i = -1; i < 4; i++) {
						SET_TILE(bg2_map, NAME_INPUT_X + name_input_index, NAME_INPUT_Y + i, 0x40, 1);
					}
					name_input_index -= 1;
				}
			}
			if (key_hit(KEY_START)) {
				high_score_entry new_score;
				memcpy(new_score.name, name_input, sizeof(name_input));
				new_score.score = get_score();
				insert_high_score(&new_score);

				// restore the name input state
				memset(name_input, 0, sizeof(name_input));
				name_input_index = 0;

				change_state(high_score_list);
				continue;
			}
		}
	}


	return 0;
}
