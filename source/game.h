#ifndef _GAME_H
#define _GAME_H
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <tonc_core.h>
#include "common.h"



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

bool is_deck_empty(char* deck);
void clear_deck(char* deck, int len);
void shuffle_deck(char* deck, int len);
game_tile random_game_tile();
void draw_background();
void draw_board();
void attempt_move_tile(int x, int y, int x_n, int y_n);
bool is_valid_move(char t1, char t2);
bool is_game_over();

int pow_int(int base, int exp);
int score_tile(game_tile tile);
int calculate_score();
#define SCORE_LABEL_X 23
#define SCORE_LABEL_Y 4
void draw_score();
void reset_board();
void set_score(int new_score);
int get_score();
void playing_tick(int tick);
void moving_tick(int tick);


#endif
