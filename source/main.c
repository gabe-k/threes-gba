#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#include "sprites.h"
#include "bg_tiles.h"
#include "menu_tiles.h"
#include "title_screen.h"
#include "bizcat.h"

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
	menu,
	playing,
	moving
} game_state;

char new_board[4][4];
char moving_board[4][4];
char board[4][4];

char start_deck[] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
char current_deck[12];

game_tile next_tile = empty;

game_state state;

int score = 0;
char score_str[6];

int move_state = 0;
int move_x = 0;
int move_y = 0;


// menu state
int selected_item = 0;

#define FONT_CBB 1
#define MENU_SCREEN_NUM 4
#define GAME_SCREEN_NUM 6
#define FONT_SCREEN_NUM 16


SCR_ENTRY *bg0_map = se_mem[MENU_SCREEN_NUM];
SCR_ENTRY *bg1_map = se_mem[GAME_SCREEN_NUM];
SCR_ENTRY *bg2_map = se_mem[FONT_SCREEN_NUM];

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

void draw_background() {
	REG_BG1CNT = BG_CBB(0) | BG_SBB(GAME_SCREEN_NUM) | BG_4BPP | BG_PRIO(1);
	REG_BG1HOFS = 0;
	REG_BG1VOFS = 0;

	// top border
	bg1_map[(1 * 32) + 8] = SE_PALBANK(0) | 1;
	for (int i = 0; i < 12; i++) {
		bg1_map[(1 * 32) + 9 + i] = SE_PALBANK(0) | 2;
	}
	bg1_map[(1 * 32) + 21] = SE_PALBANK(0) | 3;

	bg1_map[(2 * 32) + 8] = SE_PALBANK(0) | 4;
	for (int i = 0; i < 12; i++) {
		bg1_map[(2 * 32) + 9 + i] = SE_PALBANK(0) | 5;
	}
	bg1_map[(2 * 32) + 21] = SE_PALBANK(0) | 6;


	for (int y = 0; y < 14; y++) {
		bg1_map[((3 + y) * 32) + 8] = SE_PALBANK(0) | 12;
		bg1_map[((3 + y) * 32) + 21] = SE_PALBANK(0) | 13;
		for (int x = 0; x < 12; x++) {
			bg1_map[((3 + y) * 32) + 9 + x] = SE_PALBANK(0) | 5;
		}
	}

	bg1_map[(17 * 32) + 8] = SE_PALBANK(0) | 7;
	bg1_map[(17 * 32) + 21] = SE_PALBANK(0) | 8;
	for (int i = 0; i < 12; i++) {
		bg1_map[(17 * 32) + 9 + i] = SE_PALBANK(0) | 5;
	}

	// bottom border
	bg1_map[(18 * 32) + 8] = SE_PALBANK(0) | 9;
	for (int i = 0; i < 12; i++) {
		bg1_map[(18 * 32) + 9 + i] = SE_PALBANK(0) | 10;
	}
	bg1_map[(18 * 32) + 21] = SE_PALBANK(0) | 11;
}

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
	obj_set_pos(&test_objs[(4 * 4)], 16, 16 + (1 * 32));

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
	} else if (t1 == t2 && t1 != tile_1 && t1 != tile_2) {
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
			}
			if (y > 0) {
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

int pow(int base, int exp)
{
	if(exp < 0)
		return -1;

	int result = 1;
	while (exp)
	{
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}

int score_tile(game_tile tile) {
	// hardcode the rules for the first few
	if (tile < tile_3) {
		return 0;
	}
	int power = tile - tile_2;

	return pow(3, power);
}

int calculate_score() {
	int res = 0;
	for(int x = 0; x < 4; x++) {
		for(int y = 0; y < 4; y++) {
			res += score_tile(board[x][y]);
		}
	}

	return res;
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

	// clear the "Game over!" text
	draw_string(10, 0, "          ");
	// clear the score text
	draw_string(22, 9, "       ");

	next_tile = random_game_tile();
}

void draw_char(int x, int y, char c) {
	bg2_map[(y * 32) + x] = SE_PALBANK(1) | (((c & 0xF0) << 1) | (c & 0xF));
	bg2_map[((y+1) * 32) + x] = SE_PALBANK(1) | (((c & 0xF0) << 1) | (c & 0xF)) + 0x10;
}

void draw_string(int x, int y, char* s) {
	while(*s) {
		draw_char(x, y, *s);
		x++;
		s++;
	}
}

void draw_score() {
	draw_string(22, 7, "Score");
	draw_string(22, 9, score_str);
}

void initialize_font()
{
	memset16(bg2_map, 13, 0x1000);

	// cope the font tiles in
	dma3_cpy(&tile_mem[FONT_CBB], bizcatTiles, bizcatTilesLen);

	// copy da pal
	dma3_cpy(MEM_PAL + (sizeof(u16) * 16), bizcatPal, sizeof(u16) * 16);

	REG_BG2CNT = BG_CBB(FONT_CBB) | BG_SBB(FONT_SCREEN_NUM) | BG_4BPP | BG_PRIO(0);
	REG_BG2HOFS = 0;
	REG_BG2VOFS = 0;
}

void initialize_menu() {
	vid_vsync();
	// copy the menu tiles in
	dma3_cpy(MEM_VRAM, menu_tilesTiles, menu_tilesTilesLen);
	dma3_cpy(MEM_VRAM, title_screenTiles, title_screenTilesLen);

	// copy the menu tile pal
	//dma3_cpy(MEM_PAL, menu_tilesPal, sizeof(u16) * 16);
	dma3_cpy(MEM_PAL, title_screenPal, sizeof(u16) * 16);

	// clear the map
	//memset16(bg0_map, 4, 240 * 160);
	dma3_cpy(bg0_map, title_screenMap, sizeof(title_screenMap));

	REG_BG0CNT = BG_CBB(0) | BG_SBB(MENU_SCREEN_NUM) | BG_4BPP;
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;

	REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
}

void initialize_display() {
	vid_vsync();
	// clear the palatte memory before loading
	memset16(MEM_PAL, 0, 0x100);

	memset16(bg1_map, 0, 0x400);

	// copy the tiles in
	dma3_cpy(MEM_VRAM, bg_tilesTiles, bg_tilesTilesLen);

	// copy the tiles pal in
	dma3_cpy(MEM_PAL, bg_tilesPal, 0x10);

	// load the sprites
	dma3_cpy(&tile_mem[4][0], spritesTiles, spritesTilesLen);

	// load the sprite palettes
	dma3_cpy(pal_obj_mem, spritesPal, spritesPalLen);

	// load the font
	initialize_font();

	// Drwa the next title and the score
	draw_string(2, 4, "Next");
	draw_score();

	draw_background();

	oam_init(test_objs, 128);
	REG_DISPCNT = DCNT_MODE0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
}

void change_state(game_state new_state) {
	switch(new_state) {
		case menu:
			initialize_menu();
			break;
		case playing:
			score = 0;
			score_str[0] = '0';
			score_str[1] = 0;
			initialize_display();
			break;
		default:
			break;
	}

	state = new_state;
}

int main() {
	vu8* vram = MEM_VRAM;

	change_state(menu);


	reset_board();
	while(1) {
		vid_vsync();
		key_poll();
		if (state == menu) {
			if (key_hit(KEY_START)) {
				change_state(playing);
			}
		} else if (state == playing) {
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

				score = calculate_score();
				itoa(score, score_str, 10);
				draw_score();

				if (is_game_over()) {
					draw_string(10, 0, "Game over!");
				}

				move_state = 0;
			}
		}
		draw_board();
	}


	return 0;
}
