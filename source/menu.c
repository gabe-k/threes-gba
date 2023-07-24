#include "menu.h"
#include "common.h"
#include "game.h"
#include "title_screen.h"

const menu_item menu_items[] = { {13, 11, "Play"}, {10, 14, "High Scores"} };


int selected_item = 0;

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

void menu_tick(int tick)
{
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
				return;
				break;
			case 1: // high scores
				change_state(high_score_list);
				return;
			default:
				break;
		}
	}
	draw_menu_items();
}

void initialize_menu() {
	vid_vsync();
	// copy the menu tiles in
	dma3_cpy((u8*)MEM_VRAM, title_screenTiles, title_screenTilesLen);

	// copy the menu tile pal
	dma3_cpy((u8*)MEM_PAL, title_screenPal, sizeof(u16) * 256);

	// load the font
	initialize_font();

	// clear the map
	dma3_cpy(bg0_map, title_screenMap, title_screenMapLen);
	draw_menu_items();

	REG_BG0CNT = BG_CBB(0) | BG_SBB(MENU_SCREEN_NUM) | BG_8BPP | BG_PRIO(1);
	REG_BG0HOFS = 0;
	REG_BG0VOFS = 0;

	REG_DISPCNT = DCNT_MODE0 | DCNT_BG2 | DCNT_BG0;
}
