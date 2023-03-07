#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>
#include <string.h>
#include <stdbool.h>

#include "sprites.h"

typedef enum _game_tile {
	empty,
	tile_1,
	tile_2,
	tile_3,
	tile_6,
	tile_12,
	tile_24,
	tile_48,
	tile_96,
	tile_192,
	tile_384,
	tile_768,
	tile_1536,
	tile_3072,
	tile_6144
} game_tile;

typedef enum _game_state {
	playing,
	moving
} game_state;

char new_board[4][4];
char moving_board[4][4];
char board[4][4];

char start_deck[] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
char current_deck[12];

game_state state = playing;

int move_state = 0;
int move_x = 0;
int move_y = 0;

bool is_deck_empty(char* deck) {
	return deck[0] == 0;
}

void clear_deck(char* deck, int len) {
	int c = 0;
	for(int i = 0; i < len; i++) {
		if (deck[i] == 0) {
			continue;
		}
		deck[c] = deck[i];
		c++;
	}
	memset(deck+c, 0, len - c);
}

void shuffle_deck(char* deck, int len) {
	int flips = qran_range(len / 2, len);
	for (int i = 0; i < flips; i++) {
		int i1 = qran_range(0, len);
		int i2 = qran_range(0, len);
		int v1 = deck[i1];
		int v2 = deck[i2];
		deck[i2] = v1;
		deck[i1] = v2;
	}
}

game_tile random_game_tile() {
	game_tile cur_tile = empty;

	// if the deck is empty then bring in the starter deck and shuffle it
	if (is_deck_empty(current_deck))
	 {
		memcpy(current_deck, start_deck, sizeof(current_deck));
		shuffle_deck(current_deck, sizeof(current_deck));
	}

	// take the first
	cur_tile = current_deck[0];
	current_deck[0] = 0;
	clear_deck(current_deck, sizeof(current_deck));

	return cur_tile;
}

void oam_init(OBJ_ATTR *obj, uint count)
{
    u32 nn= count;
    u32 *dst= (u32*)obj;

    // Hide each object

    while(nn--)
    {
        *dst++= ATTR0_HIDE;
        *dst++= 0;
    }
    // init oam
    oam_copy(oam_mem, obj, count);
}


OBJ_ATTR test_objs[128];

typedef struct _POS2D {
	short x;
	short y;
} POS2D;

void draw_board() {
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			if ((u8)(board[x][y]) == (u8)empty) {
				obj_hide(&test_objs[(y * 4) + x]);
				continue;
			}

			obj_set_attr(&test_objs[(y * 4) + x], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | ((board[x][y] - 1) * 32) | ATTR2_PRIO(1));
			if (moving_board[x][y]) {
				obj_set_pos(&test_objs[(y * 4) + x], 50 + (x * 27) + (move_x * move_state), 20 + (y * 32) + (move_y * move_state));
			} else {
				obj_set_pos(&test_objs[(y * 4) + x], 50 + (x * 27), 20 + (y * 32));
			}
		}
	}

	oam_copy(oam_mem, test_objs, 4 * 4);
}

void attempt_move_tile(int x, int y, int x_n, int y_n) {
	//memset(new_board, 0, sizeof(new_board));

	// we're trying to move beyond the edge of the screen
	if (x_n < 0 || x_n > 3 || y_n < 0 || y_n > 3) {
		new_board[x][y] = board[x][y];
		return;
	}

	// if the spot is empty it's easy
	if ((u8)new_board[x_n][y_n] == (u8)empty) {
		new_board[x_n][y_n] = board[x][y];
		moving_board[x][y] = 1;
		//board[x][y] = empty;
		return;
	}

	// if they match
	if (board[x][y] == board[x_n][y_n]) {
		// 1 and 2 can't match
		if (board[x][y] == tile_1 || board[x][y] == tile_2) {
			new_board[x][y] = board[x][y];
			return;
		}
		new_board[x_n][y_n] = (board[x][y]) + 1;
		moving_board[x][y] = 1;
		return;
	}

	if ((board[x][y] == tile_1 && board[x_n][y_n] == tile_2) || \
		(board[x][y] == tile_2 && board[x_n][y_n] == tile_1)) {
		new_board[x_n][y_n] = tile_3;
		moving_board[x][y] = 1;
		return;
	}

	if (board[x][y] != board[x_n][y_n]) {
		new_board[x][y] = board[x][y];
	}
	//memcpy(board, new_board, sizeof(new_board));
}

void reset_board() {
	memset(current_deck, 0, sizeof(current_deck));
	memset(board, 0, sizeof(board));
	memset(moving_board, 0, sizeof(board));

	for (int i = 0; i < 9; i++) {
		int cx, cy;
		do {
			cx = qran_range(0, 4);
			cy = qran_range(0, 4);
		} while ((u8)(board[cx][cy]) != (u8)empty);

		board[cx][cy] = random_game_tile();
	}
}

int main() {
	vu8* vram = MEM_VRAM;
	dma3_cpy(&tile_mem[4][0], spritesTiles, spritesTilesLen);

	vu8* pal = MEM_PAL;
	dma3_cpy(pal_obj_mem, spritesPal, spritesPalLen);

	oam_init(test_objs, 128);
	REG_DISPCNT = DCNT_OBJ | DCNT_OBJ_1D;

	reset_board();
	while(1) {
		vid_vsync();
		key_poll();

		if (state == playing) {
			if (key_hit(KEY_START)) {
				reset_board();
			}

			if (key_hit(KEY_UP)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 0; x < 4; x++) {
					for (int y = 0; y < 4; y++) {
						attempt_move_tile(x, y, x, y-1);
						move_x = 0;
						move_y = -1;
					}
				}
				state = moving;
			} else if (key_hit(KEY_DOWN)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 0; x < 4; x++) {
					for (int y = 3; y >= 0; y--) {
						attempt_move_tile(x, y, x, y+1);
						move_x = 0;
						move_y = 1;
					}
				}
				state = moving;
			} else if (key_hit(KEY_LEFT)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 0; x < 4; x++) {
					for (int y = 0; y < 4; y++) {
						attempt_move_tile(x, y, x-1, y);
						move_x = -1;
						move_y = 0;
					}
				}
				state = moving;
			} else if (key_hit(KEY_RIGHT)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 3; x >= 0; x--) {
					for (int y = 0; y < 4; y++) {
						attempt_move_tile(x, y, x+1, y);
						move_x = 1;
						move_y = 0;
					}
				}
				state = moving;
			}
		} else if (state == moving) {
			move_state += 3;
			if (move_state == 21) {
				state = playing;
				memcpy(board, new_board, sizeof(new_board));
				memset(moving_board, 0, sizeof(moving_board));

				move_state = 0;
			}
		}
		draw_board();
	}


	return 0;
}
