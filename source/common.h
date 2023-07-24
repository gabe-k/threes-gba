#ifndef _COMMON_H
#define _COMMON_H

#include <tonc_core.h>
#include <tonc_memmap.h>
#include <tonc_oam.h>
#include <tonc_input.h>
#include <tonc_video.h>

#define MENU_SCREEN_NUM 4
#define GAME_SCREEN_NUM 6
#define FONT_SCREEN_NUM 16

extern OBJ_ATTR test_objs[128];

#define bg0_map ((SCR_ENTRY *)se_mem[MENU_SCREEN_NUM])
#define bg1_map ((SCR_ENTRY *)se_mem[GAME_SCREEN_NUM])
#define bg2_map ((SCR_ENTRY *)se_mem[FONT_SCREEN_NUM])

#define SET_TILE(m, x, y, t, p) (m[((y) * 32) + x] = SE_PALBANK(p) | t)

void oam_init(OBJ_ATTR *obj, uint count);

void draw_char(int x, int y, char c, int pal);
void draw_string_ex(int x, int y, char* s, int pal);
void draw_string(int x, int y, char* s);

typedef enum _game_state {
	menu,
	playing,
	moving,
	game_over,
	high_score_list,
	entering_name
} game_state;


extern game_state state;
void change_state(game_state new_state);

void initialize_font();
void initialize_display();
void erase_box(int x, int y, int width, int height);

#define DIALOG_TOP_LEFT 14
#define DIALOG_TOP_BORDER 15
#define DIALOG_TOP_RIGHT 16
#define DIALOG_LEFT_BORDER 17
#define DIALOG_RIGHT_BORDER 18
#define DIALOG_CENTER 0
#define DIALOG_BOTTOM_LEFT 19
#define DIALOG_BOTTOM_BORDER 20
#define DIALOG_BOTTOM_RIGHT 21
#define FONT_CBB 1

void draw_box(int x, int y, int width, int height);

#endif
