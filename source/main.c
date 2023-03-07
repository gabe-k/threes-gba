#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>
#include <string.h>
#include <stdbool.h>

#include "sprites.h"
#include "bg_tiles.h"

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

game_tile next_tile = empty;

game_state state = playing;

int move_state = 0;
int move_x = 0;
int move_y = 0;

#define SCREEN_NUM 4
SCR_ENTRY *bg0_map = se_mem[SCREEN_NUM];

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

	bool bonus = false;
	game_tile highest = empty;
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			if (board[x][y] >= tile_48) {
				bonus = true;
			}
			if (board[x][y] > highest) {
				highest = board[x][y];
			}
		}
	}

	if (bonus && (qran_range(0, 20) == 0)) {
		int size = highest - tile_24;
		return tile_24 + qran_range(1, size+1);
	}

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

			obj_set_attr(&test_objs[(y * 4) + x], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | ((board[x][y] - 1) * 32) | ATTR2_PRIO(0));
			if (moving_board[x][y]) {
				obj_set_pos(&test_objs[(y * 4) + x], 65 + (x * 26) + (move_x * move_state), 16 + (y * 32) + (move_y * move_state));
			} else {
				obj_set_pos(&test_objs[(y * 4) + x], 65 + (x * 26), 16 + (y * 32));
			}
		}
	}

	obj_set_attr(&test_objs[(4 * 4)], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | ((next_tile - 1) * 32) | ATTR2_PRIO(0));
	obj_set_pos(&test_objs[(4 * 4)], 20, 16 + (1 * 32));

	oam_copy(oam_mem, test_objs, (4 * 4) + 1);
}

void attempt_move_tile(int x, int y, int x_n, int y_n) {
	// we're trying to move beyond the edge of the screen
	if (x_n < 0 || x_n > 3 || y_n < 0 || y_n > 3) {
		new_board[x][y] = board[x][y];
		return;
	}

	// if the spot is empty it's easy
	if ((u8)new_board[x_n][y_n] == (u8)empty) {
		new_board[x_n][y_n] = board[x][y];
		moving_board[x][y] = 1;
		state = moving;
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
		state = moving;
		return;
	}

	if ((board[x][y] == tile_1 && board[x_n][y_n] == tile_2) || \
		(board[x][y] == tile_2 && board[x_n][y_n] == tile_1)) {
		new_board[x_n][y_n] = tile_3;
		moving_board[x][y] = 1;
		state = moving;
		return;
	}

	if (board[x][y] != board[x_n][y_n]) {
		new_board[x][y] = board[x][y];
	}
}

inline bool is_valid_move(char t1, char t2) {
	if (t1 == empty || t2 == empty) {
		return true;
	} else if ((t1 == tile_1 && t2 == tile_2) ||
		(t1 == tile_2 && t2 == tile_1)) {
		return true;
	} else if (t1 == t2) {
		return true;
	}
	return false;
}

// check to make sure no legal moves remain
bool is_game_over() {
	char cur_tile = empty;
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			cur_tile = board[x][y];
			// check tiles to the left
			if (x > 0) {
				if (is_valid_move(cur_tile, board[x-1][y])) {
					return false;
				}
			} else if (x < 3) {
				if (is_valid_move(cur_tile, board[x+1][y])) {
					return false;
				}		
			} else if (y > 0) {
				if (is_valid_move(cur_tile, board[x][y-1])) {
					return false;
				}
			} else if (y < 3) {
				if (is_valid_move(cur_tile, board[x][y+1])) {
					return false;
				}		
			}
		}
	}

	return true;
}

int score_tile(game_tile tile) {
	// hardcode the rules for the first few
	if (tile < tile_3) {
		return 0;
	}
	int pow = tile - tile_2;
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

	next_tile = random_game_tile();
}

int main() {
	vu8* vram = MEM_VRAM;

	// copy the tiles in
	dma3_cpy(MEM_VRAM, bg_tilesTiles, bg_tilesTilesLen);

	// copy the tiles pal in
	dma3_cpy(MEM_PAL, bg_tilesPal, 0x10);

	REG_BG0CNT = BG_CBB(0) | BG_SBB(SCREEN_NUM) | BG_4BPP;
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;

	// top border
	bg0_map[(1 * 32) + 8] = SE_PALBANK(0) | 1;
	for (int i = 0; i < 12; i++) {
		bg0_map[(1 * 32) + 9 + i] = SE_PALBANK(0) | 2;
	}
	bg0_map[(1 * 32) + 21] = SE_PALBANK(0) | 3;

	bg0_map[(2 * 32) + 8] = SE_PALBANK(0) | 4;
	for (int i = 0; i < 12; i++) {
		bg0_map[(2 * 32) + 9 + i] = SE_PALBANK(0) | 5;
	}
	bg0_map[(2 * 32) + 21] = SE_PALBANK(0) | 6;


	for (int y = 0; y < 14; y++) {
		bg0_map[((3 + y) * 32) + 8] = SE_PALBANK(0) | 12;
		bg0_map[((3 + y) * 32) + 21] = SE_PALBANK(0) | 13;
		for (int x = 0; x < 12; x++) {
			bg0_map[((3 + y) * 32) + 9 + x] = SE_PALBANK(0) | 5;
		}
	}

	bg0_map[(17 * 32) + 8] = SE_PALBANK(0) | 7;
	bg0_map[(17 * 32) + 21] = SE_PALBANK(0) | 8;
	for (int i = 0; i < 12; i++) {
		bg0_map[(17 * 32) + 9 + i] = SE_PALBANK(0) | 5;
	}

	// bottom border
	bg0_map[(18 * 32) + 8] = SE_PALBANK(0) | 9;
	for (int i = 0; i < 12; i++) {
		bg0_map[(18 * 32) + 9 + i] = SE_PALBANK(0) | 10;
	}
	bg0_map[(18 * 32) + 21] = SE_PALBANK(0) | 11;

	// load the sprites
	dma3_cpy(&tile_mem[4][0], spritesTiles, spritesTilesLen);

	vu8* pal = MEM_PAL;
	dma3_cpy(pal_obj_mem, spritesPal, spritesPalLen);

	oam_init(test_objs, 128);
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0 | DCNT_OBJ | DCNT_OBJ_1D;

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
			} else if (key_hit(KEY_DOWN)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 0; x < 4; x++) {
					for (int y = 3; y >= 0; y--) {
						attempt_move_tile(x, y, x, y+1);
						move_x = 0;
						move_y = 1;
					}
				}
			} else if (key_hit(KEY_LEFT)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 0; x < 4; x++) {
					for (int y = 0; y < 4; y++) {
						attempt_move_tile(x, y, x-1, y);
						move_x = -1;
						move_y = 0;
					}
				}
			} else if (key_hit(KEY_RIGHT)) {
				memset(new_board, 0, sizeof(new_board));
				for(int x = 3; x >= 0; x--) {
					for (int y = 0; y < 4; y++) {
						attempt_move_tile(x, y, x+1, y);
						move_x = 1;
						move_y = 0;
					}
				}
			}
		} else if (state == moving) {
			move_state += 3;
			if (move_state == 21) {
				state = playing;
				memcpy(board, new_board, sizeof(new_board));
				memset(moving_board, 0, sizeof(moving_board));
				int new_cood = 0;
				// new tile goes on bottom row
				if (move_y == -1) {
					do {
						new_cood = qran_range(0, 4);
					} while(board[new_cood][3]);
					board[new_cood][3] = next_tile;
				} else if (move_y == 1) { // new tile goes on the top row
					do {
						new_cood = qran_range(0, 4);
					} while(board[new_cood][0]);
					board[new_cood][0] = next_tile;
				} else if (move_x == -1) { // new tile goes on the right side
					do {
						new_cood = qran_range(0, 4);
					} while(board[3][new_cood]);
					board[3][new_cood] = next_tile;
				} else if (move_x == 1) { // new tile goes on the left side
					do {
						new_cood = qran_range(0, 4);
					} while(board[0][new_cood]);
					board[0][new_cood] = next_tile;
				}
				next_tile = random_game_tile();

				move_state = 0;
			}
		}
		draw_board();
	}


	return 0;
}
