#include "game.h"
#include "high_scores.h"

char new_board[4][4];
char moving_board[4][4];
char board[4][4];

char start_deck[] = {1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
char current_deck[12];

game_tile next_tile = empty;
int new_tile_x = 0;
int new_tile_y = 0;


int score = 0;
char score_str[6];

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

bool is_valid_move(char t1, char t2) {
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

void draw_score() {
	draw_string(SCORE_LABEL_X, SCORE_LABEL_Y, "Score");
	draw_string(SCORE_LABEL_X, SCORE_LABEL_Y + 2, score_str);
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
	draw_string(SCORE_LABEL_X, SCORE_LABEL_Y + 2, "       ");

	next_tile = random_game_tile();
}

void set_score(int new_score) {
	score = new_score;
	itoa(score, score_str, 10);
}

int get_score() {
	return score;
}

void playing_tick(int tick) {
	if (key_hit(KEY_START)) {
		reset_board();
	} else if (key_hit(KEY_SELECT)) {
		change_state(menu);
		return;
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
	draw_board();
}

void moving_tick(int tick) {
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
			} else {
				draw_box(10, 4, 10, 6);
				draw_string(13, 5, "GAME");
				draw_string(13, 7, "OVER");
				state = game_over;
			}
		}
	}
	draw_board();

}


