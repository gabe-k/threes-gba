#include "common.h"
#include "menu.h"
#include "game.h"
#include "sprites.h"
#include "bizcat.h"
#include "bg_tiles.h"

OBJ_ATTR test_objs[128];
game_state state;

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

void change_state(game_state new_state) {
	switch(new_state) {
		case menu:
			memset16(bg2_map, 13, 0x1000);
			initialize_menu();
			break;
		case playing:
			set_score(0);
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

void erase_box(int x, int y, int width, int height) {
	for (int cx = 0; cx < width; cx++) {
		for (int cy = 0; cy < height; cy++) {
			bg1_map[((y + cy) * 32) + x + cx] = 5;
		}
	}
}

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


