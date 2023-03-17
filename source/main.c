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
	moving,
	game_over,
	high_score_list,
	entering_name
} game_state;

char new_board[4][4];
char moving_board[4][4];
char board[4][4];

char start_deck[] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
char current_deck[12];

game_tile next_tile = empty;
int new_tile_x = 0;
int new_tile_y = 0;

game_state state;

int score = 0;
char score_str[6];

int move_state = 0;
int move_x = 0;
int move_y = 0;

int tick = 0; // for seeding rng

// menu state
int selected_item = 0;

typedef struct _menu_item {
	int x;
	int y;
	char* str;
} menu_item;

const menu_item menu_items[] = { {13, 11, "Play"}, {10, 14, "High Scores"} };

// save data
typedef struct _high_score_entry {
	char name[10];
	int score;
} high_score_entry;

high_score_entry high_scores[5];

char name_input[10];
int name_input_index = 0;

#define MAGIC_OFFSET

#define FONT_CBB 1
#define MENU_SCREEN_NUM 4
#define GAME_SCREEN_NUM 6
#define FONT_SCREEN_NUM 16


SCR_ENTRY *bg0_map = se_mem[MENU_SCREEN_NUM];
SCR_ENTRY *bg1_map = se_mem[GAME_SCREEN_NUM];
SCR_ENTRY *bg2_map = se_mem[FONT_SCREEN_NUM];

#define SET_TILE(m, x, y, t, p) (m[((y) * 32) + x] = SE_PALBANK(p) | t)

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
	SET_TILE(bg1_map, 8, 1, 1, 0);
	for (int i = 0; i < 12; i++) {
		SET_TILE(bg1_map, 9 + i, 1, 2, 0);
	}
	SET_TILE(bg1_map, 21, 1, 3, 0);

	SET_TILE(bg1_map, 8, 2, 4, 0);
	for (int i = 0; i < 12; i++) {
		SET_TILE(bg1_map, 9 + i, 2, 5, 0);
	}
	SET_TILE(bg1_map, 21, 2, 6, 0);

	// middle
	for (int y = 0; y < 14; y++) {
		SET_TILE(bg1_map, 8, 3 + y, 12, 0);
		SET_TILE(bg1_map, 21, 3 + y, 13, 0);
		for (int x = 0; x < 12; x++) {
			SET_TILE(bg1_map, 9 + x, 3 + y, 5, 0);
		}
	}

	SET_TILE(bg1_map, 8, 17, 7, 0);
	SET_TILE(bg1_map, 21, 17, 8, 0);
	for (int i = 0; i < 12; i++) {
		SET_TILE(bg1_map, 9 + i, 17, 5, 0);
	}

	// bottom border
	SET_TILE(bg1_map, 8, 18, 9, 0);
	for (int i = 0; i < 12; i++) {
		SET_TILE(bg1_map, 9 + i, 18, 10, 0);
	}
	SET_TILE(bg1_map, 21, 18, 11, 0);
}

void draw_board() {
	for (int x = 0; x < 4; x++) {
		for (int y = 0; y < 4; y++) {
			if ((u8)(board[x][y]) == (u8)empty) {
				obj_hide(&test_objs[(y * 4) + x]);
				continue;
			}

			obj_set_attr(&test_objs[(y * 4) + x], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | ((board[x][y] - 1) * 32) | ATTR2_PRIO(2));
			if (moving_board[x][y]) {
				obj_set_pos(&test_objs[(y * 4) + x], 65 + (x * 26) + (move_x * move_state), 16 + (y * 32) + (move_y * move_state));
			} else {
				obj_set_pos(&test_objs[(y * 4) + x], 65 + (x * 26), 16 + (y * 32));
			}
		}
	}

	if (move_state > 0) {
		obj_set_attr(&test_objs[(4 * 4) + 1], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | ((next_tile - 1) * 32) | ATTR2_PRIO(2));
		if (move_x) {
			obj_set_pos(&test_objs[(4 * 4) + 1], (65 + (26 * move_x * -1)) + (new_tile_x * 26) + (move_x * move_state), 16 + (new_tile_y * 32) + (move_y * move_state));
		} else 
		if (move_y) {
			obj_set_pos(&test_objs[(4 * 4) + 1], 65 + (new_tile_x * 26) + (move_x * move_state), 16 + (32 * move_y * -1) + (new_tile_y * 32) + (move_y * move_state));
		}

	} else {
		obj_hide(&test_objs[(4 * 4) + 1]);
	}

	obj_set_attr(&test_objs[(4 * 4)], ATTR0_SQUARE | ATTR0_8BPP | ATTR0_REG, ATTR1_SIZE_32, ATTR2_PALBANK(0) | ((next_tile - 1) * 32) | ATTR2_PRIO(0));
	obj_set_pos(&test_objs[(4 * 4)], 16, 16 + (1 * 32));

	oam_copy(oam_mem, test_objs, (4 * 4) + 2);
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

int pow_int(int base, int exp)
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

	return pow_int(3, power);
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

void draw_char(int x, int y, char c, int pal) {
	bg2_map[(y * 32) + x] = SE_PALBANK(pal) | (((c & 0xF0) << 1) | (c & 0xF));
	bg2_map[((y+1) * 32) + x] = SE_PALBANK(pal) | ((c & 0xF0) << 1) | ((c & 0xF) + 0x10);
}

void draw_string_ex(int x, int y, char* s, int pal) {
	while(*s) {
		draw_char(x, y, *s, pal);
		x++;
		s++;
	}
}

void draw_string(int x, int y, char* s) {
	draw_string_ex(x, y, s, 1);
}

void draw_score() {
	draw_string(23, 4, "Score");
	draw_string(23, 6, score_str);
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
	draw_string(23, 6, "       ");

	next_tile = random_game_tile();
}

void load_save() {
	// check for the magic
	if (sram_mem[0] != 'G' || sram_mem[1] != 'a' || sram_mem[2] != 'b' || sram_mem[3] != 'e') {
		sram_mem[0] = 'G';
		sram_mem[1] = 'a';
		sram_mem[2] = 'b';
		sram_mem[3] = 'e';

		memset(high_scores, 0, sizeof(high_scores));
		memcpy(sram_mem + sizeof(u32), high_scores, sizeof(high_scores));
		return;
	}

	//memcpy(high_scores, sram_mem + 4, sizeof(high_scores));
	vu8* v_sram = (vu8*)sram_mem;
	u8* high_scores_ptr = (u8*)high_scores;
	for (int i = 0; i < sizeof(high_scores); i++) {
		high_scores_ptr[i] = v_sram[i+4];
	}
}

#define DIALOG_TOP_LEFT 14
#define DIALOG_TOP_BORDER 15
#define DIALOG_TOP_RIGHT 16
#define DIALOG_LEFT_BORDER 17
#define DIALOG_RIGHT_BORDER 18
#define DIALOG_CENTER 0
#define DIALOG_BOTTOM_LEFT 19
#define DIALOG_BOTTOM_BORDER 20
#define DIALOG_BOTTOM_RIGHT 21

void draw_box(int x, int y, int width, int height) {
	u8 cur_tile = 0;
	for (int cx = 0; cx < width; cx++) {
		for (int cy = 0; cy < height; cy++) {
			if (cx == 0) {
				if (cy == 0) {
					cur_tile = DIALOG_TOP_LEFT;
				} else if (cy == height - 1) {
					cur_tile = DIALOG_BOTTOM_LEFT;
				} else {
					cur_tile = DIALOG_LEFT_BORDER;
				}
			} else if (cx == width - 1) {
				if (cy == 0) {
					cur_tile = DIALOG_TOP_RIGHT;
				} else if (cy == height - 1) {
					cur_tile = DIALOG_BOTTOM_RIGHT;
				} else {
					cur_tile = DIALOG_RIGHT_BORDER;
				}
			} else if (cy == 0) {
				cur_tile = DIALOG_TOP_BORDER;
			} else if (cy == height - 1) {
				cur_tile = DIALOG_BOTTOM_BORDER;
			} else {
				cur_tile = DIALOG_CENTER;
			}
			bg1_map[((y + cy) * 32) + x + cx] = cur_tile;
		}
	}
}

void erase_box(int x, int y, int width, int height) {
	for (int cx = 0; cx < width; cx++) {
		for (int cy = 0; cy < height; cy++) {
			bg1_map[((y + cy) * 32) + x + cx] = 5;
		}
	}
}

void initialize_font()
{
	memset16(bg2_map, 13, 0x1000);

	// cope the font tiles in
	dma3_cpy(&tile_mem[FONT_CBB], bizcatTiles, bizcatTilesLen);

	// copy da pal
	dma3_cpy((u8*)MEM_PAL + (sizeof(u16) * 16), bizcatPal, sizeof(u16) * 16);

	REG_BG2CNT = BG_CBB(FONT_CBB) | BG_SBB(FONT_SCREEN_NUM) | BG_4BPP | BG_PRIO(0);
	REG_BG2HOFS = 0;
	REG_BG2VOFS = 0;
}

void draw_menu_items() {
	for (int i = 0; i < sizeof(menu_items) / sizeof(menu_item); i++) {
		if (i == selected_item) {
			draw_char(menu_items[i].x - 2, menu_items[i].y, 0x11, 1);
		} else {
			draw_char(menu_items[i].x - 2, menu_items[i].y, ' ', 1);
		}
		draw_string(menu_items[i].x, menu_items[i].y, menu_items[i].str);
	}
}

void initialize_menu() {
	vid_vsync();
	// copy the menu tiles in
	dma3_cpy((u8*)MEM_VRAM, menu_tilesTiles, menu_tilesTilesLen);
	dma3_cpy((u8*)MEM_VRAM, title_screenTiles, title_screenTilesLen);

	// copy the menu tile pal
	//dma3_cpy(MEM_PAL, menu_tilesPal, sizeof(u16) * 16);
	dma3_cpy((u8*)MEM_PAL, title_screenPal, sizeof(u16) * 256);

	// load the font
	initialize_font();

	// clear the map
	//memset16(bg0_map, 4, 240 * 160);
	dma3_cpy(bg0_map, title_screenMap, title_screenMapLen);
	draw_menu_items();

	REG_BG0CNT = BG_CBB(0) | BG_SBB(MENU_SCREEN_NUM) | BG_8BPP | BG_PRIO(1);
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;

	REG_DISPCNT = DCNT_MODE0 | DCNT_BG2 | DCNT_BG0;
}

void initialize_display() {
	vid_vsync();
	// clear the palatte memory before loading
	memset16((u16*)MEM_PAL, 0, 0x100);

	memset16(bg1_map, 0, 0x400);

	// copy the tiles in
	dma3_cpy((u8*)MEM_VRAM, bg_tilesTiles, bg_tilesTilesLen);

	// copy the tiles pal in
	dma3_cpy((u8*)MEM_PAL, bg_tilesPal, 0x10);

	// load the sprites
	dma3_cpy(&tile_mem[4][0], spritesTiles, spritesTilesLen);

	// load the sprite palettes
	dma3_cpy(pal_obj_mem, spritesPal, spritesPalLen);

	memset16(bg2_map, 13, 0x1000);

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
			memset16(bg2_map, 13, 0x1000);
			initialize_menu();
			break;
		case playing:
			score = 0;
			score_str[0] = '0';
			score_str[1] = 0;
			initialize_display();
			break;
		case high_score_list:
			memset16(bg2_map, 13, 0x1000);
			REG_DISPCNT = DCNT_MODE0 | DCNT_BG2;
			break;
		default:
			break;
	}

	state = new_state;
}

// checks if our score goes in the leaderboard
bool is_high_score(int score) {
	for (int i = 0; i < sizeof(high_scores) / sizeof(high_score_entry); i++) {
		if (score > high_scores[i].score) {
			return true;
		}
	}
	return false;
}

void insert_high_score(high_score_entry* new_entry) {
	for (int i = 0; i < sizeof(high_scores) / sizeof(high_score_entry); i++) {
		if (new_entry->score > high_scores[i].score) {
			memmove(&high_scores[i+1], &high_scores[i], ((sizeof(high_scores) / sizeof(high_score_entry) - 1 - i) * sizeof(high_score_entry)));
			memcpy(&high_scores[i], new_entry, sizeof(high_score_entry));
			break;
		}
	}

	//memcpy(sram_mem + 4, high_scores, sizeof(high_scores));
	vu8* v_sram = (vu8*)sram_mem;
	u8* high_scores_ptr = (u8*)high_scores;
	for (int i = 0; i < sizeof(high_scores); i++) {
		v_sram[i+4] = high_scores_ptr[i];
	}
}

void draw_high_scores() {
	char cur_score[6];
	for (int i = 0; i < sizeof(high_scores) / sizeof(high_score_entry); i++) {
		if (high_scores[i].name[0] == 0) {
			draw_string(3, 4 + (i * 3), "_________");
		} else {
			draw_string(3, 4 + (i * 3), high_scores[i].name);
		}
		itoa(high_scores[i].score, cur_score, 10);
		draw_string(23, 4 + (i * 3), cur_score);
	}
}

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
			// scroll down
			if (key_hit(KEY_DOWN)) {
				if (selected_item + 1 < sizeof(menu_items) / sizeof(menu_item)) {
					selected_item++;
				}
			} else if (key_hit(KEY_UP)) {
				if (selected_item > 0) {
					selected_item--;
				}
			} else if (key_hit(KEY_A)) {
				// do the action for the selected menu option
				switch(selected_item) {
					case 0: // play game
						sqran(tick); // seed the rng with the number of frames spent at the title screen
						reset_board();
						change_state(playing);
						continue;
						break;
					case 1: // high scores
						change_state(high_score_list);
						continue;
					default:
						break;
				}
			}
			draw_menu_items();
		} else if (state == high_score_list) {
			draw_high_scores();
			if (key_hit(KEY_B)) {
				change_state(menu);
				continue;
			}
		} else if (state == playing) {
			if (key_hit(KEY_START)) {
				reset_board();
			} else if (key_hit(KEY_SELECT)) {
				change_state(menu);
				continue;
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
			// pic the coods for the new tile on the first frame of movement
			if (move_state == 0) {
				int new_cood = 0;
				// new tile goes on bottom row
				if (move_y == -1) {
					do {
						new_cood = qran_range(0, 4);
					} while(new_board[new_cood][3]);
					new_tile_x = new_cood;
					new_tile_y = 3;
				} else if (move_y == 1) { // new tile goes on the top row
					do {
						new_cood = qran_range(0, 4);
					} while(new_board[new_cood][0]);
					new_tile_x = new_cood;
					new_tile_y = 0;
				} else if (move_x == -1) { // new tile goes on the right side
					do {
						new_cood = qran_range(0, 4);
					} while(new_board[3][new_cood]);
					new_tile_x = 3;
					new_tile_y = new_cood;
				} else if (move_x == 1) { // new tile goes on the left side
					do {
						new_cood = qran_range(0, 4);
					} while(new_board[0][new_cood]);
					new_tile_x = 0;
					new_tile_y = new_cood;
				}
			}

			move_state += 3;

			// done moving
			if (move_state == 21) {
				// update the actual board
				memcpy(board, new_board, sizeof(new_board));
				memset(moving_board, 0, sizeof(moving_board));
				board[new_tile_x][new_tile_y] = next_tile;

				// select the new next tile
				next_tile = random_game_tile();
				
				// calculate and draw the score
				score = calculate_score();
				itoa(score, score_str, 10);
				draw_score();

				// change the state back to playing
				move_state = 0;
				state = playing;

				// check if the game is over
				if (is_game_over()) {
					if (is_high_score(score)) {
						draw_box(9, 4, 12, 8);
						draw_string(10, 5, "High score");
						state = entering_name;
						memset(name_input, 0, sizeof(name_input));
						name_input_index = 0;
						continue;
					} else {
						draw_box(10, 4, 10, 6);
						draw_string(13, 5, "GAME");
						draw_string(13, 7, "OVER");
					}
					state = game_over;
				}
			}
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
				new_score.score = score;
				insert_high_score(&new_score);
				change_state(high_score_list);
				continue;
			}
		}
		draw_board();
	}


	return 0;
}
